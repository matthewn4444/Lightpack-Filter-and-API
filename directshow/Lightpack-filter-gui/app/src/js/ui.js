// Default values
var DEFAULTS = {
    brightness: 100,
    smooth: 8,
    gamma: 67,
    horizontalDepth: 15,
    verticalDepth: 10,
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

// Adjust the slider to 0-30 and make it vertical
sliderData.orientation = "vertical";
sliderData.max = 30;
sliderData.value = sliderData.max;
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
function setPowerButton(isOn) {
    $("#turn-off-on").switchButton("checked", isOn);
}

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
    $("#horizontal-height-slider").slider("value", sliderData.max - v);
}

function setVerticalDepthSlider(v) {
    Ledmap.setVerticalDepth(v);
    $("#vertical-height-slider").slider("value", sliderData.max - v);
}

function setCloseStateButton(isOn) {
    $("#turn-off-close").switchButton("checked", isOn);
}

function setWhenVideoPlaysData(enabled, url, startTime, endTime) {
    setAutomationDataSet("play", enabled, url, startTime, endTime);
}

function setWhenVideoPausesData(enabled, url, startTime, endTime) {
    setAutomationDataSet("pause", enabled, url, startTime, endTime);
}

function setWhenVideoClosesData(enabled, url, startTime, endTime) {
    setAutomationDataSet("close", enabled, url, startTime, endTime);
}

function setAutomationDataSet(event, enabled, url, startTime, endTime) {
    $("#video-" + event + "-input-enable").switchButton("checked", enabled);
    $("#video-" + event + "-input-url").val(url);
    $("#video-" + event + "-input-time-start").val(startTime);
    $("#video-" + event + "-input-time-end").val(endTime);
    $("#video-" + event + "-input-url").parent().attr("disabled", !enabled);
}

// Functions to get states
function saveAutomationStateByElement(el) {
    var $container = $(el).parents("fieldset");
    var enabled = $container.find(".switch-button").switchButton("checked");
    var urlHasError = $container.find(".formError").length;
    var url = $container.find(".url-input").val();
    var startTime = $container.find(".start-time").val();
    var endTime = $container.find(".end-time").val();
    var name = el.id.split("-")[1];
    saveVideoEventData(name, enabled, urlHasError ? null : url, startTime, endTime);
}

// Init sortable
$("#modules-list").sortable().disableSelection();

function setNumberOfLightpackModules(n) {
    document.title = "Lightpack Filter (" + (n>0?n*10:"no") + " leds)";
    $("body").toggleClass("has-multiple", n > 1);
    $("#modules-list").empty();
    for (var i = 0; i < n; i++) {
        $("#modules-list").append($("<li>").attr("value", i)
            .text("Lightpack Module " + (i + 1)));
    }
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
$("#turn-off-close").switchButton({
    onText: "On",
    offText: "Off",
    click: function(e, ui) {
        setCloseState(ui.checked);
    }
});
$("#video-play-input-enable").switchButton({
    onText: "On",
    offText: "Off",
    checked: false,
    click: function(e, ui) {
        $(this).prev().attr("disabled", !ui.checked);
        saveAutomationStateByElement(this);
    }
});
$("#video-pause-input-enable").switchButton({
    onText: "On",
    offText: "Off",
    checked: false,
    click: function(e, ui) {
        $(this).prev().attr("disabled", !ui.checked);
        saveAutomationStateByElement(this);
    }
});
$("#video-close-input-enable").switchButton({
    onText: "On",
    offText: "Off",
    checked: false,
    click: function(e, ui) {
        $(this).prev().attr("disabled", !ui.checked);
        saveAutomationStateByElement(this);
    }
});

// Init time pickers
$("#page-automation .timepicker").timepicker({
    'scrollDefault': 'now',
    'disableTextInput': true,
    'noneOption': [
        {
            'label': 'Anytime',
            'value': 'anytime'
        },
    ],
})
.on("change", function(e) {
    var $self = $(e.currentTarget);
    var newValue = $self.val();
    var $other = $self.hasClass("start-time") ? $self.next() : $self.prev();
    var otherValue = $other.val();

    // Make sure that anytime cannot be mixed with an actual time, only anytime, reinforce it
    if (otherValue != newValue) {
        if (newValue == "anytime") {
            // Setting anytime should match the other one as well
            $other.val("anytime");
        } else if (otherValue == "anytime") {
            // Other is set to anytime, set the value to current time
            $other.val(newValue);
        }
    }

    // Saved the changed settings
    saveAutomationStateByElement(this);
})
.timepicker("setTime", "anytime");        // TODO set only if no savings

// Handle automation url events
$("#page-automation .url-input").on("keyup", function() {
    var timeout = $(this).data("timeout");
    if (timeout) {
        clearTimeout(timeout);
    }
    timeout = setTimeout(function() {
        if ($(this).validationEngine('validate')) {
            saveAutomationStateByElement(this);
        }
    }.bind(this), 800);
    $(this).data("timeout", timeout);
}).attr("data-errormessage", "Please enter a valid URL.")
.addClass("validate[required,custom[url]]")
.validationEngine();

// Show popup
function showPopup(title, message) {
    $(document.body).addClass("overlay");
    $("#common-overlay .title").text(title);
    $("#common-overlay .message").text(message);
    $("#common-overlay").show();
}
function hidePopup() {
    $("#common-overlay").fadeOut(250, function() {
        $(document.body).removeClass("overlay");
    });
}
$("#common-overlay .close").on("click", hidePopup);

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
    setLPBrightness(DEFAULTS.brightness);
    setSmoothSlider(DEFAULTS.smooth);
    setLPSmooth(DEFAULTS.smooth);
    setGammaSlider(DEFAULTS.gamma);
    setLPGamma(DEFAULTS.gamma);
});
