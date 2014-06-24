setInterval(function(){}, 100);

// Default values
var DEFAULTS = {
    brightness: 100,
    smooth: 20,
    gamma: 67,
    horizontalDepth: 20,
    verticalDepth: 15,
    port: 6000
};

// Initialize the led map for dragging and updating
Ledmap.init($("#led-position-screen"));

// Init sliders
var sliderData = {
    min: 0,
    max: 100,
    range: "min",
    animate: 150,
    slide: function(e, ui) {
        var v = $(this).hasClass("reverse") ?
            $(this).slider("option", "max") - ui.value : ui.value;
        $(this).find(".tooltip-inner").text(v + "%");
        setAPIValueFromSlider(this.id, v);
    },
    stop: function() {
        var $handle = $(this).find(".ui-slider-handle");
        if (!$handle.hasClass("ui-state-hover")) {
            $handle.find(".tooltip").removeClass("show");
        }
    }
};

$("#page-light-settings div.slider").slider(sliderData);

// Adjust the slider to 0-50 and make it vertical
sliderData.orientation = "vertical";
sliderData.max = 50;
sliderData.value = 50;
$("#page-adjust-position div.slider").slider(sliderData);

// Hack fix to make the "slider click" animations for rects to be correct
$("#page-adjust-position").on("slidestart", ".slider", function() {
    $("#led-position-screen").addClass("sliding");
}).on("slidestop", ".slider", function() {
    $("#led-position-screen").removeClass("sliding");
});

// Init progressbar
$(".progressbar").progressbar();

// Input functions
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
    $("#horizontal-height-slider").slider("value", 50 - v);
}

function setVerticalDepthSlider(v) {
    Ledmap.setVerticalDepth(v);
    $("#vertical-height-slider").slider("value", 50 - v);
}

// Apply Defaults
setBrightnessSlider(DEFAULTS.brightness);
setGammaSlider(DEFAULTS.gamma);
setSmoothSlider(DEFAULTS.smooth);
setPortInput(DEFAULTS.port);
setHorizontalDepthSlider(DEFAULTS.horizontalDepth);
setVerticalDepthSlider(DEFAULTS.verticalDepth);

function setAPIValueFromSlider(id, value) {
    try {
    if (id == "brightness-slider") {
        setLPBrightness(value);
    } else if (id == "gamma-slider") {
        setLPGamma(value);
    } else if (id == "smooth-slider") {
        setLPSmooth(value);
    } else if (id == "horizontal-height-slider") {
        Ledmap.setHorizontalDepth(value);
        setLPHorizontalDepth(value);
    } else if (id == "vertical-height-slider") {
        Ledmap.setVerticalDepth(value);
        setLPVerticalDepth(value);
    }
    }catch(e){}
}

// Init Switch button
$("#turn-off-on").switchButton({
    onText: "On",
    offText: "Off",
    click: function(e, ui) {
        setLPEnableLights(ui.checked);
    }
});

/* Add tooltip to the sliders */
function buildTooltip(text) {
    return $("<div>").addClass("tooltip").append(
        $("<div>").addClass("tooltip-arrow")
    ).append(
        $("<div>").addClass("tooltip-inner").text(text)
    );
}

$(".ui-slider-handle").on("mouseenter", function() {
    var $tooltip = $(this).find(".tooltip");
    if (!$tooltip.length) {
        var $slider = $(this).parents(".slider");
        var v = $slider.hasClass("reverse") ?
            $slider.slider("option", "max") - $slider.slider("value") : $slider.slider("value");
        $tooltip = buildTooltip(v + "%");
        $(this).append($tooltip);
        $tooltip.hide().show();
    }
    $tooltip.addClass("show");
}).on("mouseleave", function() {
    if (!$(this).hasClass("ui-state-active")) {
        $(this).find(".tooltip").removeClass("show");
    }
}).on("mousedown", function() {
    $("#page-adjust-position .led-map-screen").addClass("no-rect-animation");
}).on("mouseup", function() {
    $("#page-adjust-position .led-map-screen").removeClass("no-rect-animation");
});

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

$("nav ul").on("click", "li", function(){
    $("#content div.page").removeClass("open");
    $("nav li.selected").removeClass("selected");
    var name = this.id.substring(this.id.indexOf("-"));
    $("#page" + name).addClass("open");
    $("#nav" + name).addClass("selected");
    if (name == "-adjust-position") {
        Ledmap.updateMetrics();
    }
});

// Restore button
$("#page-adjust-position .restore-button").click(function() {
    Ledmap.arrangeDefault();
    $(document.body).addClass("restoring");         // Start animation
    setHorizontalDepthSlider(DEFAULTS.horizontalDepth);
    setVerticalDepthSlider(DEFAULTS.verticalDepth);
});

$(".restore-button").on("click", function() {
    // Animate the restoration
    $(document.body).addClass("restoring");
    setTimeout(function(){
        $(document.body).removeClass("restoring");
    }, 150);
});

$("#page-light-settings .restore-button").click(function() {
    setBrightnessSlider(DEFAULTS.brightness);
    setSmoothSlider(DEFAULTS.smooth);
    setGammaSlider(DEFAULTS.gamma);
});
