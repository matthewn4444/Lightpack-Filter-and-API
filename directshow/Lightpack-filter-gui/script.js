var light = require("./lightpack/lightpack-api");
//require('nw.gui').Window.get().showDevTools()

function log(/*...*/) {
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

//  ============================================
//  Handle Lightpack
//  ============================================
var numLeds = 0;
light.a(document);

light.on("connect", function(){
    log("Lights have connected");
    light.getCountLeds(function(n){
        log("Got", n, "leds");
        numLeds = n;
    }).setSmooth(255);
}).on("disconnect", function(){
    log("Lights have disconnected");
}).on("play", function(){
    log("Filter is playing");
}).on("pause", function(){
    log("Filter was paused");
}).connect();

//  ============================================
//  GUI stuff
//  ============================================
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

$("#randomEachColorAll").click(function(){
    if (numLeds) {
        var colors = [];
        for (var i = 0; i < numLeds; i++) {
            colors.push(randomColor());
        }
        light.setColors(colors);
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
