// This is a common API to communicate between the filter and directly to the lightpack device
var fs = require("fs"),
    path = require("path"),
    filter = require("./lightpack-filter"),
    client = require("./prismatik-client"),
    device = require("./lightpack");

var API_KEY = "{15b3fc7b-5495-43e0-801f-93fe73742962}";
var INI_FILE = path.dirname(process.execPath) + "/settings.ini";

var currentObj = null;
var isConnected = false;
var server = null;
var connectionTimer = null;
var port = 6000;
var api = {};

// Default States, these will change based on the config file
var states = {
    numberOfLeds: 0,
    brightness: 100,
    gamma: 2.2,
    smooth: 30,
    colors: []
};

var listeners = {
    connect: null,
    disconnect: null,
    play: null,
    pause: null,
};

var document = null;

exports.a = function(d){
    document = d;
}

function log(/*...*/) {
    if (document) {
        var div = document.createElement("div");
        var text = "[empty string]";
        if (arguments.length) {
            text = typeof(arguments[0]) == "undefined" ? "undefined" : arguments[0].toString();
            for (var i = 1; i < arguments.length; i++) {
                text += " " + (typeof(arguments[i]) == "undefined" ? "undefined" : arguments[i]);
            }
        }
        div.appendChild(document.createTextNode(text));
        document.getElementById("output").appendChild(div);
    }
}

var console = {
    log: log
}

//  ============================================
//  Private functions
//  ============================================
function startServer(port, host) {
    server = filter.Server();
    server.listen(port, host, function(){
        log("Running socket server", host, ":", port);
    });
    filter.on("socketconnect", filterConnected)
    .on("socketdisconnect", function() {
        // DO NOT SET isConnected = false because this would cause a 2nd connect event
        currentObj = null;
        var max_attempts = 2;
        function rConnect(i) {
            if (i < max_attempts) {
                internalConnect(function(success){
                    if (!success) {
                        setTimeout(function(){
                            rConnect(i + 1);
                        }, 300);
                    }
                }, true);
            } else {
                notifyDisconnect();
            }
        }
        rConnect(0);
    })
    .on("connect", function() {
        log("filter connected to lights");
        currentObj = filter;
        notifyConnect();
    })
    // The filter is gone, now we must reconnect either device or client
    .on("disconnect", function(){
        notifyDisconnect();
        currentObj = filter;        // Set this back because we give priority to the filter
    })

    // Handle when playing and not playing video
    .on("play", function(){
        if (listeners.play) {
            listeners.play.call(api);
        }
        stopConnectionPing();
    }).on("pause", function(){
        if (listeners.pause) {
            listeners.pause.call(api);
        }
    });
}

// Handle Prismatik disconnections
client.on("error", function(){
    // When disconnected we should try to reconnect to the device
    currentObj = null;
    connect();
});

function startConnectionPing() {
    stopConnectionPing();
    connectionTimer = setInterval(function(){
        if (!isConnected && currentObj != filter) {
            internalConnect();
        } else {
            stopConnectionPing();
        }
    }, 1000);
}

function stopConnectionPing() {
    if (connectionTimer) {
        clearInterval(connectionTimer);
    }
    connectionTimer = null;
}

function setPort(p) {
    port = p;
    saveSettings();
}

//  ============================================
//  Settings File functions
//  ============================================
function saveSettings(callback) {
    callback = callback || function(){};
    // Save the file
    var data = "[General]\n" +
                "port=" + port + "\n" +
                "\n[State]\n" +
                "smooth=" + states.smooth + "\n" +
                "brightness="+ states.brightness + "\n" +
                "gamma=" + states.gamma + "\n";
    fs.writeFile(INI_FILE, data, function(err) {
        if (err) {
            throw err;
        }
        callback(true);
    });
}

function loadSettings(callback) {
    // Load the data into this API
    fs.exists(INI_FILE, function(exists) {
        if (exists) {
            fs.readFile(INI_FILE, function(err, contents) {
                if (err) {
                    throw err;
                }
                var data = contents.toString();
                var lines = data.indexOf("\r") != -1 ? data.split("\r\n") : data.split("\n");
                for (var i = 0; i < lines.length; i++) {
                    var line = lines[i].trim();
                    if (line == "") {
                        continue;
                    }
                    var s = line.indexOf("=");
                    if (s == -1) {
                        // Skip titles and comments
                        if (line.charAt(0) == '[' || line.charAt(0) == ';') {
                            continue;
                        }
                        throw new Error("INI file is not formatted correctly.");
                    }
                    var command = line.substring(0, s).trim();
                    var value = line.substring(s + 1).trim();
                    if (command == "port") {
                        port = parseInt(value, 10);
                    } else if (command == "smooth") {
                        states.smooth = parseInt(value, 10);
                    } else if (command == "brightness") {
                        states.brightness = parseInt(value, 10);
                    } else if (command == "gamma") {
                        states.gamma = parseFloat(value, 10);
                    } else {
                        throw new Error("INI file is not formatted correctly of command", command);
                    }
                }
                if (callback) {
                    callback(true);
                }
            });
        } else if (callback) {
            callback(false);
        }
    });
}

//  ============================================
//  Connection functions
//  ============================================
function connectDevice() {
    if (device.open()) {
        console.log("connected to device");
        currentObj = device;
        notifyConnect();
        return true;
    }
    return false;
}

function connectToPrismatik(opts, callback) {
    if (!opts) {
        opts = { apikey: API_KEY }
    } else if (!opts.apikey) {
        opts.apikey = API_KEY;
    }
    client.connect(function(isConnected){
        if (isConnected) {
            // Now make sure we can lock the lightpack
            client.lock(function(gotLocked){
                if (!gotLocked) {
                    // Since we could not lock, that means we will need to disconnect
                    client.disconnect(callback);
                } else {
                    console.log("connected to prismatik");
                    currentObj = client;
                    callback(true);
                    notifyConnect();
                }
            });
        } else {
            callback(false);
        }
    }, opts);
}

function filterConnected() {
    // Since filter is now connected, this has higher priority and disconnect
    // device and Prismatik client
    internalDisconnect(function(){
        currentObj = filter;
        filter.signalReconnect(function(success){
            if (success) {
                console.log("connected to filter");
                notifyConnect();
            }
        });
    });
}

function internalConnect(callback, tryPrismatik, opts) {
    callback = callback || function(){};

    if (currentObj == filter) {
        filter.signalReconnect(function(success){
            log("signalReconnect", success);
            if (success) {
                notifyConnect();
            }
            callback.call(api, success);
        });
    } else {
        if (currentObj != null) {
            callback(true);
            return api;
        }
        log("try to connect to device")
        if (!connectDevice()) {
            if (tryPrismatik) {
                log("try to connect to prismatik")
                connectToPrismatik(opts, function(success) {
                    callback(success || currentObj);
                });
            } else {
                // Failed to connect
                notifyDisconnect();
                callback(false);
            }
        } else {
            callback(true);
        }
    }
    return api;
}

function internalDisconnect(callback) {
    if (currentObj == device) {
        log("Disconnected from device");
        device.closeDevices();
    } else if (currentObj == client) {
        log("Disconnected from Prismatik");
        return client.disconnect(callback);
    }
    currentObj = null;
    callback();
}

//  ============================================
//  Notify events
//  ============================================
function notifyConnect() {
    var wasConnected = isConnected;
    function runConnected() {
        if (listeners.connect && !wasConnected) {
            listeners.connect.call(api);
        }
    }
    isConnected = true;
    stopConnectionPing();

    // Since connected to new device, we should apply the current states
    getCountLeds(function(n) {
        states.numberOfLeds = n;
        setSmooth(states.smooth, function(success) {
            if (success) {
                setGamma(states.gamma, function(success) {
                    if (success) {
                        setBrightness(states.brightness, function(success) {
                            if (success) {
                                if (states.colors.length) {
                                    setColors(states.colors, function(success) {
                                        if (success) {
                                            runConnected();
                                        }
                                    });
                                } else {
                                    runConnected();
                                }
                            }
                        });
                    }
                });
            }
        });
    });
}

function notifyDisconnect() {
    log("notifyDisconnect")
    if (isConnected) {
        if (listeners.disconnect) {
            listeners.disconnect.call(api);
        }
        startConnectionPing();
    }
    isConnected = false;
    currentObj = null;
    states.numberOfLeds = 0;
}

//  ============================================
//  Functions
//  ============================================
function proxyFunc(callback, args) {
    function handleReturnValue(success) {
        if (success === false && currentObj) {
            // For device: once disconnected, we should try to reconnect or fail
            disconnect(callback);
        }
        if (callback) {
            callback.apply(api, arguments);
        }
    }

    if (arguments.callee.caller && arguments.callee.caller.name) {
        var fnName = arguments.callee.caller.name;
        args = args || [];

        function runProxy() {
            if (currentObj == device) {
                // If crash next line, you did not make the function same name to call this
                var ret = device[fnName].apply(api, args);
                handleReturnValue(ret);
            } else if (currentObj) {
                args.push(handleReturnValue);
                // If crash next line, you did not make the function same name to call this
                currentObj[fnName].apply(api, args);
            } else if (callback) {
                callback.call(api, false);
            }
        }

        // If not connected yet try to connect
        if (!isConnected) {
            internalConnect(function(success){
                if (success) {
                    runProxy();
                } else if (callback) {
                    notifyDisconnect();
                    callback.call(api, false);
                }
            });
        } else {
            runProxy();
        }
    } else {
        throw new Error("proxyFunc was called incorrectly!");
    }
    return api;
}

function getCountLeds(callback) {
    return proxyFunc(callback);
}

function setColor(n, r, g, b, callback) {
    return proxyFunc(function(success) {
        // Keep track of the colors
        if (success && n < states.numberOfLeds) {
            states.colors[n] = [r, g, b];
        }
        if (callback) {
            callback.call(api, success);
        }
    }, [n, r, g, b]);
}

function setColors(colorArr, callback) {
    return proxyFunc(function(success) {
        if (success) {
            // Keep track of the colors
            for (var i = 0; i < states.numberOfLeds; i++) {
                if (colorArr[i] == -1) continue;
                states.colors[i] = colorArr[i];
            }
        }
        if (callback) {
            callback.call(api, success);
        }
    }, [colorArr]);
}

function setColorToAll(r, g, b, callback) {
    return proxyFunc(function(success) {
        if (success) {
            // Keep track of the colors
            for (var i = 0; i < states.numberOfLeds; i++) {
                states.colors[i] = [r, g, b];
            }
        }
        if (callback) {
            callback.call(api, success);
        }
    }, [r, g, b]);
}

function setGamma(value, callback) {
    return proxyFunc(function(success){
        if (success) {
            states.gamma = value;
            saveSettings();
        }
        if (callback) {
            callback.call(api, success);
        }
    }, [value]);
}

function setSmooth(value, callback) {
    return proxyFunc(function(success){
        if (success) {
            states.smooth = value;
            saveSettings();
        }
        if (callback) {
            callback.call(api, success);
        }
    }, [Math.floor(value)]);
}

function setExportedSmooth(value, callback) {
    return setSmooth(Math.round(255 * value / 100.0), callback);
}

function setBrightness(value, callback) {
    return proxyFunc(function(success){
        if (success) {
            states.brightness = value;
            saveSettings();
        }
        if (callback) {
            callback.call(api, success);
        }
    }, [value]);
}

function turnOn(callback) {
    return proxyFunc(callback);
}

function turnOff(callback) {
    return proxyFunc(callback);
}

function connect(opts) {
    return internalConnect(null, true, opts);
}

function disconnect() {
    internalDisconnect(notifyDisconnect);
    return api;
}

function on(eventName, fn) {
    if (fn == null || typeof(fn) == "function") {
        if (listeners.hasOwnProperty(eventName)) {
            listeners[eventName] = fn;
        }
    }
    return api;
}

// Implementation
api.connect = connect;
api.disconnect = disconnect;
api.getCountLeds = function(){ return states.numberOfLeds; };
api.setColor = setColor;
api.setColors = setColors;
api.setColorToAll = setColorToAll;
api.setGamma = setGamma;
api.setSmooth = setExportedSmooth;
api.setPort = setPort;
api.setBrightness = setBrightness;
api.turnOn = turnOn;
api.turnOff = turnOff;
api.on = on;
api.getPort = function(){ return port; };
api.getSmooth = function(){ return Math.round((states.smooth / 255.0) * 10) * 10; };
api.getGamma = function(){ return states.gamma; };
api.getBrightness = function(){ return states.brightness; };

exports.init = function(callback){
    // Start by loading the file and then start the server
    loadSettings(function(fileExists){
        if (fileExists) {
            startServer(port, "127.0.0.1");
            if (callback) {
                callback(api);
            }
        } else {
            saveSettings(function() {
                startServer(port, "127.0.0.1");
                if (callback) {
                    callback(api);
                }
            });
        }
    });
}
