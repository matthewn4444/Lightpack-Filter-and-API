// Led Map Draggable
var $ledscreen = $("#led-position-screen"),
    screenWidth = Math.round($ledscreen.innerWidth()),
    screenHeight = Math.round($ledscreen.innerHeight());

function arrangeLedsToDefault() {
    var $leds = $(".holder");
    if ($leds.length) {
        $leds.eq(0).attr("data-direction", "up");
        var numOfGroups = $leds.length / 10,
            ledWidth = $leds.eq(0).outerWidth(),
            verticalParts = 3 * numOfGroups,
            horizontalParts = 4 * numOfGroups,

            ledWidthVPercent = ledWidth * 1.0 / screenHeight;
            verticalBlockSize = (1 - ledWidthVPercent * verticalParts) / (verticalParts + 1.0);
            ledWidthHPercent = ledWidth * 1.0 / screenWidth;
            horizontalBlockSize = (1 - ledWidthHPercent * horizontalParts) / (horizontalParts + 1.0);
            x = 0, y = 0;

        // Left
        for (var i = 0, y = ledWidthVPercent / 2; i < verticalParts; i++) {
            y += verticalBlockSize;
            arrangeLed($leds.eq(i), 0, y * 100);
            y += ledWidthVPercent;
        }

        // Top
        for (var i = 0, x = ledWidthHPercent / 2; i < horizontalParts; i++) {
            x += horizontalBlockSize;
            arrangeLed($leds.eq(i + verticalParts), 1, x * 100);
            x += ledWidthHPercent;
        }

        // Right
        for (var i = 0, y = ledWidthVPercent / 2; i < verticalParts; i++) {
            y += verticalBlockSize;
            arrangeLed($leds.eq(i + verticalParts + horizontalParts), 2, y * 100);
            y += ledWidthVPercent;
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
    var $leds = $(".holder");
    if ($leds.length != numOfGroups * 10) {
        $leds.remove();
        for (var i = 0; i < numOfGroups * 10; i++) {
            addLed(0, 0).attr("id", "led_" + i);
        }
        arrangeLedsToDefault();
    }
}

function arrangeLed($holder, side, percentValue) {
    /*    side:
     *        0 - right
     *        1 - top
     *        2 - left
     *        3 - bottom
     */
    $holder.attr("data-direction", "up");
    var ledWidth = $holder.outerWidth(),
        ledHeight = $holder.outerHeight(),
        verticalPos = Math.round(screenHeight * percentValue / 100.0) - ledWidth / 2,
        horizontalPos = Math.round(screenWidth * percentValue / 100.0) - ledWidth / 2,
        bottomEdge = screenHeight - ledHeight, rightEdge = screenWidth - ledHeight,
        leftBound = ledWidth / 4.0, rightBound = (screenWidth - ledWidth) - ledWidth / 4.0,
        topBound = leftBound, bottomBound = (screenHeight - ledWidth) - ledWidth / 4.0;

    switch(side) {
        case 0:    // Right
            if (verticalPos < topBound) {
                $holder.attr("data-direction", "right-up").css({ left: rightEdge, top: 0 });
            } else if (verticalPos > bottomBound) {
                $holder.attr("data-direction", "right-down").css({ left: rightEdge, top: bottomEdge });
            } else {
                $holder.attr("data-direction", "right").css({ left: rightEdge, top: verticalPos });
            }
            break;
        case 1:    // Top
            if (horizontalPos < leftBound) {
                $holder.attr("data-direction", "left-up").css({ left: 0, top: 0 });
            } else if (horizontalPos > rightBound) {
                $holder.attr("data-direction", "right-up").css({ left: rightEdge, top: 0 });
            } else {
                $holder.attr("data-direction", "up").css({ left: horizontalPos, top: 0 });
            }
            break;
        case 2:    // Left
            if (verticalPos < topBound) {
                $holder.attr("data-direction", "left-up").css({ left: 0, top: 0 });
            } else if (verticalPos > bottomBound) {
                $holder.attr("data-direction", "left-down").css({ left: 0, top: bottomEdge });
            } else {
                $holder.attr("data-direction", "left").css({ left: 0, top: verticalPos });
            }
            break;
        case 3:    // Bottom
            if (horizontalPos < leftBound) {
                $holder.attr("data-direction", "left-down").css({ left: 0, top: bottomEdge });
            } else if (horizontalPos > rightBound) {
                $holder.attr("data-direction", "right-down").css({ left: rightEdge, top: bottomEdge });
            } else {
                $holder.attr("data-direction", "down").css({ left: horizontalPos, top: bottomEdge });
            }
            break;
        default:
            throw new Error("Position specified is not valid");
    }
}

function addLed(side, percentValue) {
    var $holder = $("<div>").addClass("holder");
    var $led = $("<div>").addClass("led-item");
    var $pointer = $("<div>").addClass("pointer");
    $led.append($pointer);
    $ledscreen.append($holder.append($led));
    var s1 = $holder.outerWidth(),
        s2 = $holder.outerHeight(),
        smallSide = Math.min(s1, s2),
        largeSide = Math.max(s1, s2),
        rightEdge = screenWidth - largeSide,
        bottomEdge = screenHeight - largeSide;

    $holder.draggable({
        containment: "parent",
        axis: "x",      // TEMP
        drag: function(event, ui) {
            var x = ui.position.left, y = ui.position.top,
                left = parseInt($(this).css("left"),10), top = parseInt($(this).css("top"), 10),
                $self = $(this), axis = $self.draggable("option", "axis"),
                leftBound = smallSide / 4.0, rightBound = rightEdge - smallSide / 4.0,
                topBound = leftBound, bottomBound = bottomEdge - smallSide / 4.0;

            if (axis == 'x') {
                if (x < leftBound) {                // Left
                    ui.position.left = 0;
                    $(this).css("left", 0);
                    if (y < topBound) {             // Top
                        console.log("left-up:x")
                        $(this).attr("data-direction", "left-up").css("top", 0)
                            .draggable("option", "axis", x > y ? "x" : "y");
                        ui.position.top = 0;
                    } else if (y < bottomBound) {   // Vertical middle
                        console.log("left");
                        $(this).attr("data-direction", "left").draggable("option", "axis", "y");
                    } else if (y > bottomBound) {   // Below bottom
                        console.log("left-down:x");
                        $(this).attr("data-direction", "left-down").css("top", bottomEdge)
                            .draggable("option", "axis", (bottomEdge - y) > x ? "y" : "x");
                        ui.position.top = bottomEdge;
                    }
                } else if (x > rightBound) {        // Right
                    ui.position.left = rightEdge;
                    $(this).css("left", rightEdge);
                    if (y < topBound) {             // Top
                        console.log("right-up:x");
                        $(this).attr("data-direction", "right-up").css("top", 0)
                            .draggable("option", "axis", (rightEdge - x) > y ? "x" : "y");
                        ui.position.top = 0;
                    } else if (y < bottomBound) {   // Vertical Middle
                        console.log("right:x");
                        $(this).attr("data-direction", "right").draggable("option", "axis", "y");
                    } else if (y > bottomBound) {   // Bottom
                        console.log("right-down:x");
                        $(this).attr("data-direction", "right-down").css("top", bottomEdge)
                            .draggable("option", "axis", y > x ? "y" : "x");
                        ui.position.top = bottomEdge;
                    }
                } else if (y < topBound && top < topBound) {    // Horizontal middle
                    $(this).attr("data-direction", "top");
                } else if (y > bottomBound && top > bottomBound) {  // Vertical middle
                    $(this).attr("data-direction", "down");
                }
            } else {
                if (y < topBound) {                 // Top
                    ui.position.top = 0;
                    $(this).css("top", 0);
                    if (x < leftBound) {            // Left
                        console.log("left-up:y");
                        $(this).attr("data-direction", "left-up").css("left", 0)
                            .draggable("option", "axis", x > y ? "x" : "y");
                        ui.position.left = 0;
                    } else if (x < rightBound) {    // Horizontal middle
                        console.log("up:y");
                        $(this).attr("data-direction", "up").draggable("option", "axis", "x");
                    } else if (x > rightBound) {    // Right
                        console.log("right-up:y");
                        $(this).attr("data-direction", "right-up").css("left", rightEdge)
                            .draggable("option", "axis", (rightEdge - x) > y ? "x" : "y");
                        ui.position.left = rightEdge;
                    }
                } else if (y > bottomBound) {       // Bottom
                    ui.position.top = bottomEdge;
                    $(this).css("top", bottomEdge);
                    if (x < leftBound) {            // Left
                        console.log("left-down:y");
                        $(this).attr("data-direction", "left-down").css("left", 0)
                            .draggable("option", "axis", (bottomEdge - y) > x ? "y" : "x");
                        ui.position.left = 0;
                    } else if (x < rightBound) {   // Horizontal middle
                        console.log("down:y");
                        $(this).attr("data-direction", "down").draggable("option", "axis", "x");
                    } else if (x > rightBound) {    // Right
                        console.log("right-down:y");
                        $(this).attr("data-direction", "right-down").css("left", rightEdge)
                            .draggable("option", "axis", y > x ? "y" : "x");
                        ui.position.left = rightEdge;
                    }
                } else if (x < leftBound && left < leftBound) {
                    $(this).attr("data-direction", "left");
                } else if (x > rightBound && left > rightBound) {
                    $(this).attr("data-direction", "right");
                }
            }
        },
    });
    arrangeLed($holder, side, percentValue);
    return $holder;
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
