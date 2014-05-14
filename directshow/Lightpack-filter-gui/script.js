var light = require("./lightpack-server");

var HOST = "127.0.0.1",
    PORT = 6000;
    
var numLeds = 0;

function log(text) {
    var div = document.createElement("div");
    div.appendChild(document.createTextNode(text));
    document.getElementById("output").appendChild(div);
}

var server = light.Server(document);
server.listen(PORT, HOST, function(){
    log("Running socket server 6000 " + server.address().address);
});

light.on("connection", function(){
    log("light connected");
    light.getCountLeds(function(num){
        numLeds = num;
        log("Read " + num + " leds");
    });
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
