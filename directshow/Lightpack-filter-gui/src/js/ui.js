// Led Map Draggable
var $ledscreen = $("#led-position-screen"),
    screenWidth = $ledscreen.width(),
    screenHeight = $ledscreen.height();

function arrangeLedsToDefault() {
    var $leds = $(".led-item");
    if ($leds.length) {
        var numOfGroups = $leds.length / 10,
            ledHeight = $leds.eq(0).height(),
            ledWidth = $leds.eq(0).width(),
            verticalParts = 3 * numOfGroups,
            horizontalParts = 4 * numOfGroups,

            ledHeightPercent = ledHeight * 1.0 / (screenHeight - ledHeight);
            verticalBlockSize = 1 / (verticalParts + 1.0) - ledHeightPercent;
            ledWidthPercent = ledWidth * 1.0 / (screenWidth - ledWidth);
            horizontalBlockSize = 1 / (horizontalParts + 1.0) - ledWidthPercent;
            x = 0, y = 0;

        // Right
        for (var i = 0; i < verticalParts; i++) {
            y += verticalBlockSize + ledHeightPercent / 2.0;
            arrangeLed($leds.eq(i), 0, y * 100);
            y += ledHeightPercent;
        }

        // Top
        for (var i = 0; i < horizontalParts; i++) {
            x += horizontalBlockSize + ledWidthPercent / 2.0;
            arrangeLed($leds.eq(i + verticalParts), 1, x * 100);
            x += ledWidthPercent;
        }

        // Left
        for (var i = 0, y = 0; i < verticalParts; i++) {
            y += verticalBlockSize + ledHeightPercent / 2.0;
            arrangeLed($leds.eq(i + verticalParts + horizontalParts), 2, y * 100);
            y += ledHeightPercent;
        }
    }
}

function applyDefaultLedsGroup(numOfGroups) {
    /*
     * We apply the default positions of the LEDs
     *      Default has 3 on left and right
     *      and 4 on top evenly distributed with
     *      nothing on the bottom.
     *      Order of leds usually right, top, left
     */
    var $leds = $(".led-item");
    if ($leds.length != numOfGroups * 10) {
        $leds.remove();
        for (var i = 0; i < numOfGroups * 10; i++) {
            addLed(0, 0).attr("id", "led_" + i);
        }
        arrangeLedsToDefault();
    }
}

function arrangeLed($led, side, percentValue) {
    /*    side:
     *        0 - right
     *        1 - top
     *        2 - left
     *        3 - bottom
     */
    var ledWidth = $led.width(),
        ledHeight = $led.height(),
        verticalPos = Math.round((screenHeight - ledHeight) * percentValue / 100.0),
        horizontalPos = Math.round((screenWidth - ledWidth) * percentValue / 100.0);
    switch(side) {
        case 0:    // Left
            $led.css({ left: 0, top: verticalPos });
            break;
        case 1:    // Top
            $led.css({ left: horizontalPos, top: 0 });
            break;
        case 2:    // Right
            $led.css({ left: (screenWidth - ledWidth), top: verticalPos });
            break;
        case 3:    // Bottom
            $led.css({ left: horizontalPos, top: (screenHeight - ledHeight) });
            break;
        default:
            throw new Error("Position specified is not valid");
    }
}

function addLed(side, percentValue) {
    var $led = $("<div>").addClass("led-item");
    $ledscreen.append($led);
    var ledWidth = $led.width(),
        ledHeight = $led.height(),
        rightEdge = 0, bottomEdge = 0;
    $led.draggable({
        containment: "parent",
        axis: "x",      // TEMP
        start: function() {
            rightEdge = screenWidth - ledWidth;
            bottomEdge = screenHeight - ledHeight;
        },
        drag: function(event, ui) {
            var left = ui.position.left, top = ui.position.top,
                axis = $(this).draggable("option", "axis");
            if (axis == 'x') {
                if (left == 0) {
                    $(this).draggable("option", "axis", "y").css("left", 0);
                } else if (left == rightEdge) {
                    $(this).draggable("option", "axis", "y").css("left", rightEdge);
                }
            } else {
                if (top == 0) {
                    $(this).draggable("option", "axis", "x").css("top", 0);
                } else if (top == bottomEdge) {
                    $(this).draggable("option", "axis", "x").css("top", bottomEdge);
                }
            }
        },
    });
    arrangeLed($led, side, percentValue);
    return $led;
}

setInterval(function(){}, 100);

// Init sliders
function setAPIValueFromSlider(id, value) {
    if (id == "brightness-slider") {
        setLPBrightness(value);
    } else if (id == "gamma-slider") {
        setLPGamma(value);
    } else if (id == "smooth-slider") {
        setLPSmooth(value);
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
});
