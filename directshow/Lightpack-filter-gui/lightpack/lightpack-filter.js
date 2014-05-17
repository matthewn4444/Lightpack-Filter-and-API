var net = require('net');

// Create the server
var clients = [];
var queue = [];
var isRunning = false;
var currentSocketName = null;
var server = null;

// Listeners
var connectionListener = null;
var disconnectionListener = null;

// Events
var EVENT_MSG_COUNT_LEDS =      0,
    EVENT_MSG_SET_COLOR =       1,
    EVENT_MSG_SET_ALL_COLOR =   2,
    EVENT_MSG_SET_RECTS =       3,
    EVENT_MSG_SET_BRIGHTNESS =  4,
    EVENT_MSG_SET_SMOOTH =      5,
    EVENT_MSG_SET_GAMMA =       6,
    EVENT_MSG_TURN_OFF =        7,
    EVENT_MSG_TURN_ON =         8;
    EVENT_MSG_CONNECT =         9;

function clamp(val, min, max) {
    return Math.max(Math.min(val, max), min);
}

function startServer() {
    if (server) return server;
    server = net.createServer(function(socket){
        socket.name = socket.remoteAddress + ":" + socket.remotePort;
        clients.push(socket);

        if (clients.length == 1) {
            currentSocketName = socket.name;
            // Notify connected listener
            if (connectionListener) {
                connectionListener.call(exports, socket);
            }
        }

        socket.on("data", function(data){
            //log(socket.name + "> " + data);
            data = data.toString();
            if (data == "") {
                return;// log(socket.name + "> Failed [no data]")
            }
            if (queue.length) {
                var obj = queue.shift();
                var callback = obj.callback;
                if (callback) {
                    switch(obj.event) {
                        case EVENT_MSG_COUNT_LEDS:
                            console.log(data)
                            callback.call(exports, parseInt(data, 10));
                            break;
                        case EVENT_MSG_SET_BRIGHTNESS:
                        case EVENT_MSG_SET_GAMMA:
                        case EVENT_MSG_SET_SMOOTH:
                        case EVENT_MSG_SET_ALL_COLOR:
                        case EVENT_MSG_SET_COLOR:
                        case EVENT_MSG_CONNECT:
                            callback.call(exports, data == '0');
                            break;
                    }
                }
                isRunning = false;
                runQueue();
            }
        });

        socket.on("end", function(){
            clients.splice(clients.indexOf(socket), 1);
            //log(socket.name + " disconnected\n");

            // New socket, when the disconnected socket was the current one
            if (clients.length == 1 && currentSocketName == socket.name) {
                currentSocketName = clients[0].name;
                if (connectionListener) {
                    connectionListener.call(exports, clients[0]);
                }
            } else if (clients.length == 0 && disconnectionListener) {
                disconnectionListener.call(exports);
            }
        });
    });
    return server;
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
        var q = "" + eventId + (data ? data : "") + "\n";
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

function signalReconnect(callback) {
    queueEvent(EVENT_MSG_CONNECT, null, callback);
}

function on(event, fn) {
    if (typeof(fn) == "function") {
        if (event == "connection") {
            connectionListener = fn;
        } else if (event == "disconnected") {
            disconnectionListener = fn;
        }
    }
}

exports.Server = startServer;
exports.getCountLeds = getCountLeds;
exports.setColor = setColor;
exports.setColorToAll = setColorToAll;
exports.setGamma = setGamma;
exports.setSmooth = setSmooth;
exports.setBrightness = setBrightness;
exports.turnOn = turnOn;
exports.turnOff = turnOff;
exports.signalReconnect = signalReconnect;
exports.on = on;
