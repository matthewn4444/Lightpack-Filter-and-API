setInterval(function(){}, 100);

// Initialize the led map for dragging and updating
Ledmap.init($("#led-position-screen"));

// Init sliders
function setAPIValueFromSlider(id, value) {
    if (id == "brightness-slider") {
        setLPBrightness(value);
    } else if (id == "gamma-slider") {
        setLPGamma(value);
    } else if (id == "smooth-slider") {
        setLPSmooth(value);
    } else if (id == "horizontal-height-slider") {
        Ledmap.setHorizontalDepth(value / 2);
        setLPHorizontalDepth(value / 2);
    } else if (id == "vertical-height-slider") {
        Ledmap.setVerticalDepth(value / 2);
        setLPVerticalDepth(value / 2);
    }
}

$("div.slider").slider({
    min: 0,
    max: 100,
    slide: function(e, ui) {
        setAPIValueFromSlider(this.id, ui.value);
    }
});

function setBrightnessSlider(val) {
    $("#brightness-slider").slider("value", val);
}

function setGammaSlider(val) {
    $("#gamma-slider").slider("value", val);
}

function setSmoothSlider(val) {
    $("#smooth-slider").slider("value", val);
}

function setPortInput(port) {
    $("#port-input").val(port);
}

function setHorizontalDepthSlider(v) {
    Ledmap.setHorizontalDepth(v);
    $("#horizontal-height-slider").slider("value", v * 2);
}

function setVerticalDepthSlider(v) {
    Ledmap.setVerticalDepth(v);
    $("#vertical-height-slider").slider("value", v * 2);
}

// Port
var inputDelay = (function(){
    var timer = 0;
    return function(callback, ms){
        clearTimeout (timer);
        timer = setTimeout(callback, ms);
    };
})();

$("#port-input").keyup(function(){
    inputDelay(function(){
        var port = parseInt($(this).val(), 10);
        setLPPort(port);
    }.bind(this), 1000);
});

// Toggle button
$("#turn-off-on").click(function(){
    var nextState = $(this).attr("data-alternate");
    var curText = $(this).text();
    $(this).text(nextState);
    $(this).attr("data-alternate", curText);
});

$("nav ul").on("click", "li", function(){
    $("#content div.page").removeClass("open");
    var name = this.id.substring(this.id.indexOf("-"));
    $("#page" + name).addClass("open");
    if (name == "-adjust-position") {
        Ledmap.updateMetrics();
    }
});
