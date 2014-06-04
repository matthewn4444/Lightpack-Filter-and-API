var lightpack = require("lightpack/lightpack-api"),
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
var numLeds = 0,
    normalSmooth = lightpack.getSmooth(),
    isConnected = false,
    isPlaying = false;
lightpack.a(document);

lightpack.init(function(api){
    lightApi = api;

    // Apply the data from API to the GUI
    setBrightnessSlider(lightpack.getBrightness());
    setSmoothSlider(lightpack.getSmooth());
    setGammaSlider(lightpack.getGamma());
    setPortInput(lightpack.getPort());
    Ledmap.setPositions(lightpack.getSavedPositions());

    lightApi.on("connect", function(){
        log("Lights have connected");
        numLeds = lightApi.getCountLeds();
        log("Got", numLeds, "Leds");
        Ledmap.setGroups(numLeds / 10);
        isConnected = true;

        var color = randomColor();
        lightApi.setColorToAll(color[0], color[1], color[2]);

        // Set the colors if on the adjustment page
        if ($("#page-adjust-position.open").length) {
            displayLedMapColors();
        }
    }).on("disconnect", function(){
        isConnected = false;
        log("Lights have disconnected");
    }).on("play", function(){
        log("Filter is playing");
        isPlaying = true;
    }).on("pause", function(){
        log("Filter was paused");
        isPlaying = false;

        // Paused and showing
        if ($("#page-adjust-position.open").length) {
            displayLedMapColors();
        }
    }).connect();
});

function canDisplayColors() {
    return lightApi && isConnected && !isPlaying;
}

function displayLedMapColors() {
    if (canDisplayColors()) {   // Also detect if window is shown
        var c = [];
        var colors = Ledmap.getColorGroup();
        for (var i = 0; i < numLeds; i += 10) {
            c = c.concat(colors);
        }
        lightApi.setColors(c);
    }
}

function setLPBrightness(value) {
    if (value != lightpack.getBrightness()) {
        lightpack.setBrightness(value);
    }
}

function setLPGamma(value) {
    if (value != lightpack.getGamma()) {
        lightpack.setGamma(value);
    }
}

function setLPSmooth(value) {
    if (value != lightpack.getSmooth()) {
        lightpack.setSmooth(value);
        normalSmooth = value;
    }
}

function setLPPort(port) {
    if (!isNaN(port)) {
        lightpack.setPort(port);
    }
}

//  ============================================
//  Handle Ledmap
//  ============================================
Ledmap.on("end", function() {
    lightpack.sendPositions(Ledmap.getPositions());
}).on("startSelection", function() {
    if (canDisplayColors()) {
        normalSmooth = lightpack.getSmooth();
        var n = parseInt($(this).attr("data-led"), 10);
        lightpack.setSmooth(10);
        lightApi.setColorToAll(180, 180, 180);
        lightApi.setColor(n, 255, 0, 0);
    }
}).on("endSelection", function() {
    displayLedMapColors();
    if (canDisplayColors()) {
        lightpack.setSmooth(normalSmooth);
    }
});

$("#nav-adjust-position").click(function(){
    var id = $(this).attr("id");
    if (id == "nav-adjust-position") {
        displayLedMapColors();
    }
});

// Reset button to send the lights again
$("#page-adjust-position .led-map-screen .reset-default-button").click(function(){
    displayLedMapColors();
});

// Fullscreen button
$("#fullscreen").click(function(){
    win.toggleFullscreen();
    $(document.body).toggleClass("fullscreen");
});
win.on("enter-fullscreen", function() {
    $("#fullscreen").text("Exit Fullscreen");
    Ledmap.updateMetrics();
});
win.on("leave-fullscreen", function() {
    $("#fullscreen").text("Fullscreen");
    Ledmap.updateMetrics();
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

win.on("close", function(){
    win.hide();
    lightpack.close(function(){
        win.close(true);
    });
});

$("#turn-off-on").click(function(){
    if ($(this).text() == "Turn On") {
        lightApi.turnOff();
    } else {
        lightApi.turnOn();
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
    lightpack.setBrightness(parseInt(val, 10));
});

$("#smooth").change(function(){
    var val = $(this).val();
    var percent = parseInt(val, 10);
    lightpack.setSmooth(parseInt(val, 10));
});
