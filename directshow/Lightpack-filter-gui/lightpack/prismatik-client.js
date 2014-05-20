var socket = require("net").Socket();

var LOCALHOST = "127.0.0.1",
    DEFAULT_PORT = 3636;

var ledMap = [],
    numLeds = 0;
    
var listeners = {
    error: null,
};

// Events
var EVENT_CONNECTION = 0,
    EVENT_API_KEY = 1,
    EVENT_COUNT_LEDS = 2,
    EVENT_LOCK = 3,
    EVENT_UNLOCK = 4,
    EVENT_GET_PROFILES = 5,
    EVENT_GET_PROFILE = 6,
    EVENT_GET_STATUS = 7,
    EVENT_GET_API_STATUS = 8,
    EVENT_TURN_ON = 9,
    EVENT_TURN_OFF = 10,
    EVENT_SET_PROFILE = 11,
    EVENT_SET_GAMMA = 12,
    EVENT_SET_SMOOTH = 13,
    EVENT_SET_BRIGHTNESS = 14,
    EVENT_SET_COLOR = 15;

var queue = [];
var isRunning = false;
var isConnected = false;

function runNextEvent() {
    if (queue.length && !isRunning) {
        isRunning = true;
        if (queue[0].query) {
            socket.write(queue[0].query + "\n");
        }
    }
}

function queueEvent(eventId, query, callback) {
    queue.push({ event: eventId, query: query, callback: callback });
    runNextEvent();
}

socket.on("data", function(data){
    if (data) {
        data = data.toString().trim();
        if (isRunning && queue.length) {
            var item = queue.shift();
            var callback = item.callback;
            var parts = data.split(":");
            var value = parts.length >= 2 ? parts[1] : 0;
            var event = item.event;
            
            if (event == EVENT_CONNECTION) {
                isConnected = true;
                if (item.apikey) {
                    queueEvent(EVENT_API_KEY, "apikey:" + item.apikey, function(success){
                        if (!success) {
                            if (callback) {
                                callback.call(exports, false);
                            }
                        } else {
                            getCountLeds(function(num){
                                numLeds = num;
                                
                                // Fill ledMap
                                for (var i = ledMap.length; i < num; i++) {
                                    ledMap[i] = i + 1;
                                }
                                if (callback) {
                                    callback.call(exports, true);
                                }
                            });
                        }
                    });
                } else {
                    getCountLeds(function(num){
                        numLeds = num;
                        if (callback) {
                            callback.call(exports, true);
                        }
                    });
                }
            } else if (callback) {
                switch(event) {
                    case EVENT_API_KEY:
                        callback.call(exports, data === "ok");
                        break;
                    case EVENT_COUNT_LEDS:
                        callback.call(exports, parseInt(value, 10));
                        break;
                    case EVENT_LOCK:
                    case EVENT_UNLOCK:
                        callback.call(exports, value == "success")
                        break;
                    case EVENT_GET_PROFILES:
                        var profiles = value.split(";");
                        if (profiles.length && profiles[profiles.length - 1] == "") {
                            profiles.pop();
                        }
                        callback.call(exports, profiles);
                        break;
                    case EVENT_GET_STATUS:
                    case EVENT_GET_PROFILE:
                    case EVENT_GET_API_STATUS:
                        callback.call(exports, value);
                        break;
                    case EVENT_SET_PROFILE:
                    case EVENT_SET_GAMMA:
                    case EVENT_SET_SMOOTH:
                    case EVENT_SET_BRIGHTNESS:
                    case EVENT_SET_COLOR:
                    case EVENT_TURN_ON:
                    case EVENT_TURN_OFF:
                        callback.call(exports, data === "ok")
                        break;
                    default:
                        return;
                }
            }
        }
        isRunning = false;
        runNextEvent();
    }
});

socket.on("error", function(err){
    socket.end();
    if (isRunning && queue.length) {
        var item = queue.shift();
        var callback = item.callback;
        if (item.event == EVENT_CONNECTION) {
            if (callback) {
                callback.call(exports, false);
            }
            isRunning = false;
            runNextEvent();
            return;
        }
    }
    if (listeners.error) {
        listeners.error.apply(exports, arguments);
    }
    isRunning = false;
});

function connect(/* [opts], [callback] */) {
    // Parse Arguments
    var callback = null, opts = null;
    if (arguments.length) {
        if (typeof(arguments[0]) == "object") {
            opts = arguments[0];
            if (arguments.length >= 2 && typeof(arguments[1]) == "function") {
                callback = arguments[1];
            }
        } else if (typeof(arguments[0]) == "function") {
            callback = arguments[0];
        }
    }

    if (isConnected) {
        if (callback) {
            callback.call(exports, true);
        }
    } else {
        var host = opts && opts.host || LOCALHOST,
            port = opts && opts.port || DEFAULT_PORT,
            ledMap = opts && opts.ledMap || [],
            apikey = opts && opts.apikey || "";

        if (port <= 0 || !host.trim().length) {
            return callback.call(exports, false);
        }
        queueEvent(EVENT_CONNECTION, null, callback);
        queue[0].apikey = apikey;
        socket.connect(port, host);
    }
}

function getCountLeds(callback) {
    queueEvent(EVENT_COUNT_LEDS, "getcountleds", callback);
}

function lock(callback) {
    queueEvent(EVENT_LOCK, "lock", callback);
}

function unlock(callback) {
    queueEvent(EVENT_UNLOCK, "unlock", callback);
}

function disconnect(callback) {
    if (isConnected) {
        unlock(function(success){
            socket.end();
            if (callback) {
                callback.call(exports, success);
            }
            isConnected = false;
        });
    } else if (callback) {
        callback.call(exports, true);
    }
}

function getProfiles(callback) {
    queueEvent(EVENT_GET_PROFILES, "getprofiles", callback);
}

function getProfile(callback) {
    queueEvent(EVENT_GET_PROFILE, "getprofile", callback);
}

function getStatus(callback) {
    queueEvent(EVENT_GET_STATUS, "getstatus", callback);
}

function getAPIStatus(callback) {
    queueEvent(EVENT_GET_API_STATUS, "getstatusapi", callback);
}

function turnOn(callback) {
    queueEvent(EVENT_TURN_ON, "setstatus:on", callback);
}

function turnOff(callback) {
    queueEvent(EVENT_TURN_OFF, "setstatus:off", callback);
}

function setProfile(name, callback) {
    queueEvent(EVENT_SET_PROFILE, "setprofile:" + name, callback);
}

function setGamma(value, callback) {
    queueEvent(EVENT_SET_GAMMA, "setgamma:" + value, callback);
}

function setSmooth(value, callback) {
    queueEvent(EVENT_SET_SMOOTH, "setsmooth:" + value, callback);
}

function setBrightness(value, callback) {
    queueEvent(EVENT_SET_BRIGHTNESS, "setbrightness:" + value, callback);
}

function setColorToAll(r, g, b, callback) {
    if (numLeds) {
        var query = "setcolor:";
        var each = "-" + r + "," + g + "," + b + ";";
        for (var i = 0; i < numLeds; i++) {
            query += ledMap[i] + each;
        }
        queueEvent(EVENT_SET_COLOR, query, callback);
    } else if (callback) {
        callback.call(exports, false);
    }
}

function setColors(colorArr, callback) {
    if (!colorArr || !colorArr.length) {
        return callback.call(exports, false);
    }
    var query = "setcolor:";
    for (var i = 0; i < Math.min(numLeds, colorArr.length); i++) {
        var val = colorArr[i];
        if (val == -1) continue;
        var color;
        if (val instanceof Array) {
            color = val[0] + "," + val[1] + "," + val[2];
        } else {
            color = val & 0xFF + "," + (val & 0xFF00) >> 8 + "," + (val & 0xFF0000) >> 16;
        }
        query += ledMap[i] + "-" + color;
    }
    queueEvent(EVENT_SET_COLOR, query, callback);
}

function setColor(i, r, g, b, callback) {
    if (i < numLeds) {
        queueEvent(EVENT_SET_COLOR, "setcolor:" + ledMap[i] + "-" + r + "," + g + "," + b + ";", callback);
    } else if (callback) {
        callback.call(exports, false);
    }
}

function on(eventName, fn) {
    if (fn == null || typeof(fn) == "function") {
        if (listeners.hasOwnProperty(eventName)) {
            listeners[eventName] = fn;
        }
    }
    return exports;
}

exports.connect = connect;
exports.getCountLeds = getCountLeds;
exports.getProfiles = getProfiles;
exports.getProfile = getProfile;
exports.getStatus = getStatus;
exports.getAPIStatus = getAPIStatus;
exports.setProfile = setProfile;
exports.setGamma = setGamma;
exports.setSmooth = setSmooth;
exports.setBrightness = setBrightness;
exports.setColorToAll = setColorToAll;
exports.setColor = setColor;
exports.lock = lock;
exports.unlock = unlock;
exports.turnOn = turnOn;
exports.turnOff = turnOff;
exports.disconnect = disconnect;
exports.on = on;
