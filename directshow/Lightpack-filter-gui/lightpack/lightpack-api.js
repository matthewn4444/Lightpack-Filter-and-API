// This is a common API to communicate between the filter and directly to the lightpack device
var filter = require("./lightpack-filter"),
    client = require("./prismatik-client"),
    device = require("./lightpack");

var API_KEY = "{15b3fc7b-5495-43e0-801f-93fe73742962}";

var currentObj = null;
var isConnected = false;
var server = null;

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
    } else {
        console.log.apply(null, arguments);
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
    filter.on("connect", filterConnected)

    // The filter is gone, now we must reconnect either device or client
    .on("disconnect", function(){
        currentObj = null;
        connect();
    })

    // Handle when playing and not playing video
    .on("play", function(){
        if (listeners.play) {
            listeners.play.call(exports);
        }
    }).on("pause", function(){
        if (listeners.pause) {
            listeners.pause.call(exports);
        }
    });
}

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
    client.connect(opts, function(isConnected){
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
    });
}

function filterConnected() {
    // Since filter is now connected, this has higher priority and disconnect
    // device and Prismatik client
    internalDisconnect(function(){
        filter.signalReconnect(function(){
            console.log("connected to filter");
            currentObj = filter;
            notifyConnect();
        });
    });
}

function internalDisconnect(callback) {
    if (currentObj == device) {
        log("Disconnected from device");
        device.closeDevices();
    } else if (currentObj == client) {
        log("Disconnected from Prismatik");
        return client.disconnect(callback);
    }
    callback();
}

//  ============================================
//  Notify events
//  ============================================
function notifyConnect() {
    if (listeners.connect && !isConnected) {
        listeners.connect.call(exports);
    }
    isConnected = true;
}

function notifyDisconnect() {
    log("notifyDisconnect")
    if (listeners.disconnect && isConnected) {
        listeners.disconnect.call(exports);
    }
    isConnected = false;
    currentObj = null;
}

//  ============================================
//  Functions
//  ============================================
function proxyFunc(fnName, args, callback) {
    args = args || [];
    if (currentObj == device) {
        var ret = device[fnName].apply(this, args);
        if (callback) {
            callback.call(exports, ret);
        }
    } else if (currentObj) {
        args.push(callback);
        currentObj[fnName].apply(this, args);
    } else if (callback) {
        callback.call(exports, false);
    }
    return exports;
}

function getCountLeds(callback) {
    return proxyFunc("getCountLeds", null, callback);
}

function setColor(n, r, g, b, callback) {
    return proxyFunc("setColor", [n, r, g, b], callback);
}

function setColorToAll(r, g, b, callback) {
    return proxyFunc("setColorToAll", [r, g, b], callback);
}

function setGamma(value, callback) {
    return proxyFunc("setGamma", [value], callback);
}

function setSmooth(value, callback) {
    return proxyFunc("setSmooth", [value], callback);
}

function setBrightness(value, callback) {
    return proxyFunc("setBrightness", [value], callback);
}

function turnOn(callback) {
    return proxyFunc("turnOn", null, callback);
}

function turnOff(callback) {
    return proxyFunc("turnOff", null, callback);
}

function connect(opts) {
    var maxAttempts = 3;
    log("Run connect")
    opts = opts || { apikey: API_KEY };
    function rConnect(attempt) {
        // Return if already connected
        if (currentObj != null) {
            return;
        }
        log("Trying to connect to either device or prismatik")
        if (!connectDevice()) {
            connectToPrismatik(opts, function(isConnected) {
                if (!isConnected) {
                    if (attempt + 1 < maxAttempts) {
                        setTimeout(function(){
                            rConnect(attempt + 1);
                        }, 500);
                    } else {
                        // After all the attempts, we have failed to connect
                        notifyDisconnect();
                    }
                }
            });
        }
    }
    rConnect(0);
    return exports;
}

function disconnect() {
    internalDisconnect(notifyDisconnect);
    return exports;
}

function on(eventName, fn) {
    if (fn == null || typeof(fn) == "function") {
        if (listeners.hasOwnProperty(eventName)) {
            listeners[eventName] = fn;
        }
    }
    return exports;
}

// Implementation
exports.connect = connect;
exports.disconnect = disconnect;
exports.getCountLeds = getCountLeds;
exports.setColor = setColor;
exports.setColorToAll = setColorToAll;
exports.setGamma = setGamma;
exports.setSmooth = setSmooth;
exports.setBrightness = setBrightness;
exports.turnOn = turnOn;
exports.turnOff = turnOff;
exports.on = on;

// Start the server
startServer(6000, "127.0.0.1");