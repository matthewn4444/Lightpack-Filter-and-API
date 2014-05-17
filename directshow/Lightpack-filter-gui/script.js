var light = require("./lightpack/lightpack-api");

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

// Connect the lights
var numLeds = 0;
light.a(document);
light.connect(function(isConnected){
    if (isConnected) {
        log("got connected");
        light.setSmooth(255, function(){
            light.getCountLeds(function(n){
                numLeds = n;
                log("Got", n, "leds");
            });
        });
    } else {
        log("not connected");
    }
});

// GUI stuff
function rand(max) {
    return Math.floor((Math.random() * max));
}

function randomColor() {
    var color = [];
    for (var i = 0; i < 3; i++) {
        color.push(rand(256));
    }
    return color;
}

$("#toggleOnOff").click(function(){
    if ($(this).text() == "Turn On") {
        $(this).text("Turn Off");
        light.turnOn();
    } else {
        $(this).text("Turn On");
        light.turnOff();
    }
});

$("#randomColor").click(function(){
    if (numLeds) {
        var color = randomColor();
        var randLed = rand(numLeds);
        light.setColor(randLed, color[0], color[1], color[2]);
    }
});

$("#randomColorAll").click(function(){
    var color = randomColor();
    light.setColorToAll(color[0], color[1], color[2]);
});

$("#brightness").change(function(){
    var val = $(this).val();
    light.setBrightness(parseInt(val, 10));
});

$("#smooth").change(function(){
    var val = $(this).val();
    var percent = parseInt(val, 10);
    light.setSmooth(255 * percent / 100.0);
});
