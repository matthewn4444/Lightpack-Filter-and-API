// This is a common API to communicate between the filter and directly to the lightpack device
var filter = require("./lightpack-filter"),
    client = require("./prismatik-client"),
    device = require("./lightpack");

var API_KEY = "{15b3fc7b-5495-43e0-801f-93fe73742962}";

var currentObj = null;
var server = null;

var document = null;

exports.a = function(d){
    document = d;
}

function log(/*...*/) {
    var div = document.createElement("div");
    var text = "[empty string]";
    if (arguments.length) {
        text = arguments[0].toString();
        for (var i = 1; i < arguments.length; i++) {
            text += " " + arguments[i];
        }
    }
    div.appendChild(document.createTextNode(text));
    document.getElementById("output").appendChild(div);
}

var console = {
    log: log
}

//
// Functions
//
function proxyFunc(fnName, args, callback) {
    args = args || [];
    if (currentObj == device) {
        var ret = device[fnName].apply(this, args);
        if (callback) {
            callback.call(exports, ret);
        }
    } else {
        args.push(callback);
        currentObj[fnName].apply(this, args);
    }
}

function getCountLeds(callback) {
    proxyFunc("getCountLeds", null, callback);
}

function setColor(n, r, g, b, callback) {
    proxyFunc("setColor", [n, r, g, b], callback);
}

function setColorToAll(r, g, b, callback) {
    proxyFunc("setColorToAll", [r, g, b], callback);
}

function setGamma(value, callback) {
    proxyFunc("setGamma", [value], callback);
}

function setSmooth(value, callback) {
    proxyFunc("setSmooth", [value], callback);
}

function setBrightness(value, callback) {
    proxyFunc("setBrightness", [value], callback);
}

function turnOn(callback) {
    proxyFunc("turnOn", null, callback);
}

function turnOff(callback) {
    proxyFunc("turnOff", null, callback);
}

function disconnect(callback) {
    if (currentObj == device) {
        closeDevice();
        if (callback) {callback.call(exports, true);}
    } else if (currentObj == client) {
        closePrismatik(callback);
    } else if (currentObj == filter && callback) {
        callback.call(exports, true);
    } else if (callback) {
        callback.call(exports, false);
    }
}

// Device specific
function closePrismatik(callback) {
    if (currentObj == client) {
        currentObj = null;
        console.log("disconnected from prismatik");
        client.disconnect(callback);
    } else if (callback) {
        callback.call(exports, false);
    }
}

function closeDevice() {
    if (currentObj == device) {
        device.closeDevices();
        console.log("disconnected from device");
        currentObj = null;
    }
}

function handleFilter(callback) {
    // Since filter is now connected, this has higher priority and disconnect
    // device and Prismatik client
    disconnect(function(){
        currentObj = filter;
        filter.signalReconnect(function(){
            console.log("connected to filter");
            if (callback) {
                callback.call(exports, true);
            }
        });
    });
}

function handlePrismatik(callback) {
    // Ready to speak to Prismatik
    log("attempt to connect to prismatik")
    closeDevice();
    currentObj = client;

    // Try to lock, if already locked, give up
    client.lock(function(gotLocked){
		log("lock", gotLocked)
        if (!gotLocked) {
            // Since we could not lock, that means we will discuss it
            closePrismatik(function(){
                if (callback) {
                    callback.call(exports, gotLocked);
                }
            });
        } else if (callback) {
            console.log("connected to prismatik");
            callback.call(exports, gotLocked);
        }
    });
}

function handleDevice(callback) {
    // Ready to speak directly to the device
    disconnect(function(){
        currentObj = device;
        console.log("connected to device");
        if (callback) {
            callback.call(exports, true);
        }
    });
}

function connect(/* [options], [callback] */) {
    // Parse Arguments
    var callback = null;
    if (arguments.length) {
        if (typeof(arguments[0]) == "object") {
            // TODO options
            if (arguments.length >= 2 && typeof(arguments[1]) == "function") {
				callback = arguments[1];
			}
        } else if (typeof(arguments[0]) == "function") {
            callback = arguments[0];
        }
    }

    // We connect to the device somehow, we try in this order:
    //  1. Start the server and wait
    //  2. Connect directly
    //  3. Connect to Prismatik

    if (!server) {
        server = filter.Server();
        server.listen(6000, "127.0.0.1", function(){
            log("Running socket server 6000 " + server.address().address);
        });
        filter.on("connection", function(){
            handleFilter.call(exports, callback);
        });

        // The filter is gone, now we must reconnect either device or client
        filter.on("disconnected", function(){
            // Try to reconnect now, if fails try 500ms after twice
            connect(function(isConnected){
                if (!isConnected) {
                    setTimeout(function(){
                        connect(function(isConnected){
                            if (!isConnected) {
                                setTimeout(connect, 500);
                            }
                        });
                    }, 500);
                }
            });
        });
    }

    log("Trying to connect to either device or prismatik")
    if (device.open()) {
        // Connected to the device
        handleDevice(callback);
    } else {
        client.connect({ apikey: API_KEY }, function(isConnected){
            if (isConnected) {
                handlePrismatik(callback);
            } else {
                // USB and filter is not connected or on
                console.log("Nothing connected");
                if (callback) {
                    return callback.call(exports, false);
                }
            }
        });
    }
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
