var lightpack = require("./lightpack/lightpack-api"),
    gui = require('nw.gui'),
    lightApi = null,
    win = gui.Window.get();
//win.showDevTools()

function log(/*...*/) {
    var div = document.createElement("div");
    div.style.textAlign = "right";
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
lightpack.a(document);

lightpack.init(function(api){
    lightApi = api;
    lightApi.on("connect", function(){
        log("Lights have connected");
        numLeds = lightApi.getCountLeds();
        log("Got", numLeds, "Leds");

        // Apply the data from API to the GUI
        $("#brightness").val(Math.round(lightApi.getBrightness() / 10.0) * 10);
        $("#smooth").val(Math.round(lightApi.getSmooth() / 10.0) * 10);
        $("#port").val(lightApi.getPort());
    }).on("disconnect", function(){
        log("Lights have disconnected");
    }).on("play", function(){
        log("Filter is playing");
    }).on("pause", function(){
        log("Filter was paused");
    }).connect();
});

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

var inputDelay = (function(){
    var timer = 0;
    return function(callback, ms){
        clearTimeout (timer);
        timer = setTimeout(callback, ms);
    };
})();

win.on("close", function(){
    win.hide();
    if (lightApi) {
        lightpack.close(function(){
            win.close(true);
        });
    } else {
        win.close(true);
    }
});

$("#toggleOnOff").click(function(){
    if ($(this).text() == "Turn On") {
        $(this).text("Turn Off");
        lightApi.turnOn();
    } else {
        $(this).text("Turn On");
        lightApi.turnOff();
    }
});

$("#randomColor").click(function(){
    if (numLeds) {
        var color = randomColor();
        var randLed = rand(numLeds);
        lightApi.setColor(randLed, color[0], color[1], color[2]);
    }
});

$("#randomEachColorAll").click(function(){
    if (numLeds) {
        var colors = [];
        for (var i = 0; i < numLeds; i++) {
            colors.push(randomColor());
        }
        lightApi.setColors(colors);
    }
});

$("#randomColorAll").click(function(){
    var color = randomColor();
    lightApi.setColorToAll(color[0], color[1], color[2]);
});

$("#brightness").change(function(){
    var val = $(this).val();
    lightApi.setBrightness(parseInt(val, 10));
});

$("#smooth").change(function(){
    var val = $(this).val();
    var percent = parseInt(val, 10);
    lightApi.setSmooth(parseInt(val, 10));
});

$("#port").keyup(function(){
    inputDelay(function(){
        var port = parseInt($("#port").val(), 10);
        if (!isNaN(port)) {
            lightApi.setPort(port, function(success){
                if (!success) {
                    $("#port").val(lightApi.getPort());
                }
            });
        }
    }, 1000);
});
