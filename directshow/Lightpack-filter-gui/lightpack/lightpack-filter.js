var net = require('net');

var DEFAULT_PORT = 6000,
    DEFAULT_HOST = "127.0.0.1";

// Create the server
var clients = [];
var queue = [];
var isRunning = false;
var currentSocketName = null;
var server = null;
var isResettingServer = false;
var serverPort = DEFAULT_PORT;
var serverHost = DEFAULT_HOST;

// Listeners
var listeners = {
    socketconnect: null,
    socketdisconnect: null,
    connect: null,
    disconnect: null,
    play: null,
    pause: null
};

// Events
var EVENT_MSG_COUNT_LEDS =      0,
    EVENT_MSG_SET_COLOR =       1,
    EVENT_MSG_SET_COLORS =      2,
    EVENT_MSG_SET_ALL_COLOR =   3,
    EVENT_MSG_SET_RECTS =       4,
    EVENT_MSG_SET_BRIGHTNESS =  5,
    EVENT_MSG_SET_SMOOTH =      6,
    EVENT_MSG_SET_GAMMA =       7,
    EVENT_MSG_TURN_OFF =        8,
    EVENT_MSG_TURN_ON =         9,
    EVENT_MSG_CONNECT =         10,
    EVENT_MSG_NEW_PORT =        11;

// Receive events
var EVENT_REC_RETURN =          0,
    EVENT_REC_IS_RUNNING =      1,
    EVENT_REC_IS_PAUSED =       2,
    EVENT_REC_CONNECTED =       3,
    EVENT_REC_DISCONNECTED =    4,
    EVENT_REC_INVALID_ARGS =    5;

function clamp(val, min, max) {
    return Math.max(Math.min(val, max), min);
}

function startServer(callback, port, host) {
    callback = callback || function(){};
    if (server) return callback(true);

    port = port || serverPort;
    host = host || serverHost;
    isResettingServer = false;
    isRunning = false;
    server = net.createServer(function(socket){
        socket.name = socket.remoteAddress + ":" + socket.remotePort;
        clients.push(socket);

        if (clients.length == 1) {
            currentSocketName = socket.name;
            // Notify connected listenere.log("\tGot back:",data)
            if (listeners.socketconnect) {
                listeners.socketconnect.call(exports, socket);
            }
        }

        socket.on("data", function(data){
            //log(socket.name + "> " + data);
            data = data.toString();
            if (data == "") {
                return;// log(socket.name + "> Failed [no data]")
            }
            var retEvt = parseInt(data.charAt(0), 10);
            data = data.substring(1);
            console.log("ret->",data);
            switch(retEvt) {
                case EVENT_REC_RETURN:
                    if (queue.length) {
                        var obj = queue.shift();
                        if (obj.event == EVENT_MSG_NEW_PORT) {
                            if (data == '1') {
                                // Disconnect user and start the server over again
                                isResettingServer = true;
                                close(function(){
                                    startServer(obj.callback);
                                });
                            } else {
                                throw new Error("Failed to receive true after setting new port.");
                            }
                        } else {
                            var callback = obj.callback;
                            if (callback) {
                                switch(obj.event) {
                                    case EVENT_MSG_COUNT_LEDS:
                                        callback.call(exports, parseInt(data, 10));
                                        break;
                                    case EVENT_MSG_SET_BRIGHTNESS:
                                    case EVENT_MSG_SET_GAMMA:
                                    case EVENT_MSG_SET_SMOOTH:
                                    case EVENT_MSG_SET_ALL_COLOR:
                                    case EVENT_MSG_SET_COLORS:
                                    case EVENT_MSG_SET_COLOR:
                                    case EVENT_MSG_TURN_ON:
                                    case EVENT_MSG_TURN_OFF:
                                        callback.call(exports, data == '1');
                                        break;
                                    case EVENT_MSG_CONNECT:
                                        if (data != '1') {      // Failed
                                            queue = [];
                                        }
                                        callback.call(exports, data == '1');
                                        break;
                                }
                            }
                            isRunning = false;
                            runQueue();
                        }
                    }
                    break;
                case EVENT_REC_IS_RUNNING:
                    if (listeners.play) {
                        listeners.play.call(exports);
                    }
                    break;
                case EVENT_REC_IS_PAUSED:
                    if (listeners.pause) {
                        listeners.pause.call(exports);
                    }
                    break;
                case EVENT_REC_CONNECTED:
                    if (listeners.connect) {
                        listeners.connect.call(exports);
                    }
                    break;
                case EVENT_REC_DISCONNECTED:
                    if (listeners.disconnect) {
                        listeners.disconnect.call(exports);
                    }
                    queue = [];
                    isRunning = false;
                    break;
                case EVENT_REC_INVALID_ARGS:
                    // Invalid arguments sent
                    var obj = queue.shift();
                    throw new Error("Invalid arguments sent to filter", obj.event);
                    break;
            }
        });

        socket.on("end", function(){
            if (isResettingServer) {
                return;
            }
            clients.splice(clients.indexOf(socket), 1);
            //log(socket.name + " disconnected\n");

            // New socket, when the disconnected socket was the current one
            if (clients.length == 1 && currentSocketName == socket.name) {
                currentSocketName = clients[0].name;
                if (listeners.socketconnect) {
                    listeners.socketconnect.call(exports, clients[0]);
                }
            } else if (clients.length == 0 && listeners.socketdisconnect) {
                listeners.socketdisconnect.call(exports);
            }
        });

        socket.on("error", function(e){
            throw new e;
        });
    });

    server.listen(port, host, function(){
        serverPort = port;
        serverHost = host;
        callback(true);
    });
}

function runQueue() {
    if (queue.length && !isRunning) {
        isRunning = true;
        //log("Queue event: " + queue[0].event + " => " + queue[0].query);
        clients[0].write(queue[0].query);
    }
}

function queueEvent(eventId, data, callback) {
    if (clients.length) {
        var eventCode = String.fromCharCode('a'.charCodeAt(0) + eventId);
        var q = "" + eventCode + (data ? data : "") + "\n";
        var obj = { event: eventId, query: q, callback: callback };
        queue.push(obj);
        runQueue();
    } else if (callback) {
        callback.call(exports, 0);
    }
}

function setColor(n, r, g, b, callback) {
    queueEvent(EVENT_MSG_SET_COLOR, [n, r, g, b].join(","), callback);
}

function setColors(colorArr, callback) {
    if (!colorArr || !colorArr.length) {
        throw new Error("Incorrect arguments or empty color array!");
    }
    var colors = [];
    for (var i = 0; i < colorArr.length; i++) {
        var color = colorArr[i];
        if (color.join) {
            color = (color[0] & 0xFF) | ((color[1] & 0xFF) << 8) | ((color[2] & 0xFF) << 16);
        }
        colors.push(color);
    }
    queueEvent(EVENT_MSG_SET_COLORS, colors.join(","), callback);
}

function setColorToAll(r, g, b, callback) {
    queueEvent(EVENT_MSG_SET_ALL_COLOR, [r, g, b].join(","), callback);
}

function getCountLeds(callback) {
    queueEvent(EVENT_MSG_COUNT_LEDS, null, callback);
}

function setBrightness(value, callback) {
    queueEvent(EVENT_MSG_SET_BRIGHTNESS, clamp(value, 0, 100), callback);
}

function setSmooth(value, callback) {
    queueEvent(EVENT_MSG_SET_SMOOTH, value % 256, callback);
}

function setGamma(value, callback) {
    queueEvent(EVENT_MSG_SET_GAMMA, clamp(value, 0, 10) * 10, callback);
}

function turnOn(callback) {
    queueEvent(EVENT_MSG_TURN_ON, null, callback);
}

function turnOff(callback) {
    queueEvent(EVENT_MSG_TURN_OFF, null, callback);
}

function setPort(port, callback) {
    var p = parseInt(port, 10);
    if (isNaN(p)) {
        throw new Error("Port is invalid", port);
    }
    serverPort = p;
    queueEvent(EVENT_MSG_NEW_PORT, p, callback);
}

function signalReconnect(callback) {
    queueEvent(EVENT_MSG_CONNECT, null, callback);
}

function on(eventName, fn) {
    if (fn == null || typeof(fn) == "function") {
        if (listeners.hasOwnProperty(eventName)) {
            listeners[eventName] = fn;
        }
    }
    return exports;
}

function close(callback) {
    // Disconnect user and start the server over again
    for (var i = 0; i < clients.length; i++) {
        clients[i].end();
    }
    clients = [];
    queue = [];
    server.close(function() {
        server = null;
        if (callback) {
            callback.call(exports);
        }
    });
}

exports.startServer = startServer;
exports.getCountLeds = getCountLeds;
exports.setColor = setColor;
exports.setColors = setColors;
exports.setColorToAll = setColorToAll;
exports.setGamma = setGamma;
exports.setSmooth = setSmooth;
exports.setBrightness = setBrightness;
exports.turnOn = turnOn;
exports.turnOff = turnOff;
exports.signalReconnect = signalReconnect;
exports.on = on;
exports.setPort = setPort;
exports.getPort = function() { return serverPort; };
exports.close = close;
