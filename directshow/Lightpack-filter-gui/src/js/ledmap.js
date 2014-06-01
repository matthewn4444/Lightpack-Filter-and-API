/*  Draggable Led Maps
 *      User sets the position of the leds via interactive map
 *
 *  Dependencies: jQuery and jQuery.ui
 *
 *  Initialize the led map with its container and optionally set number of groups
 *  This function will automatically set the Leds' positions to default
 *      @params $screen     set the jQuery container of where the leds would cling to
 *      @params numOfGroups [option] set the number of led groups (10 leds each group)
 *      Ledmap.init($screen, numOfGroups)
 *
 *  Set the number of Leds (groups) used by the ledmap
 *  This function will automatically set the Leds' positions to default
 *      @param n    number of led groups (each group is 10 leds) | default is 1
 *      Ledmap.setGroups(n)
 *
 *  Arrange all the Leds to their default positions
 *      arrangeDefault()
 *
 *  Set dragging event listeners
 *      @param eventName    String event name
 *      @param callback     Function that gets called when event occurs
 *      on(eventName, callback)
 *    Possible Events: start | drag | end
 */
(function(w){

// Properties that are set on init
var $ledscreen = null,
    screenWidth = 0,
    screenHeight = 0,
    screenRatio = 0,
    smallSide = 0,
    largeSide = 0,
    rightEdge = 0,
    bottomEdge = 0,
    listeners = {
        start: null,
        drag: null,
        end: null
    };

function init($screen, numOfGroups) {
    $screen = $($screen);

    // Move the leds to the new screen if exists
    if ($ledscreen != null && $ledscreen.length && !$ledscreen.is($screen)) {
        $ledscreen.removeClass("led-map-screen");
        var $existingHolders = $ledscreen.find(".holder");
        if ($existingHolders.length) {
            $screen.append($existingHolders);
        }
    }

    $ledscreen = $screen;
    $ledscreen.addClass("led-map-screen");
    screenWidth = Math.round($ledscreen.innerWidth()),
    screenHeight = Math.round($ledscreen.innerHeight()),
    screenRatio = screenHeight / screenWidth;
    setDefaultGroups(numOfGroups);
}

function handleListeners(eventName, fn) {
    if (typeof(fn) == "function" || fn == null) {
        if (listeners.hasOwnProperty(eventName)) {
            listeners[eventName] = fn;
        }
    }
}

function arrangeDefault() {
    var $leds = $ledscreen.find(".holder");
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

function setDefaultGroups(numOfGroups) {
    /*
     * We apply the default positions of the LEDs
     *      Default has 3 on left and right
     *      and 4 on top evenly distributed with
     *      nothing on the bottom.
     *      Order of leds usually right, top, left
     */
     numOfGroups = numOfGroups || 1;
    var $leds = $ledscreen.find(".holder");
    if ($leds.length != numOfGroups * 10) {
        $leds.remove();
        for (var i = 0; i < numOfGroups * 10; i++) {
            addLed(0, 0).attr("id", "led_" + i);
        }
        arrangeDefault();
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
    if ($ledscreen == null) {
        throw new Error("Did not run Ledmap.init($screen)");
    }
    var $holder = $("<div>").addClass("holder");
    var $led = $("<div>").addClass("led-item");
    var $pointer = $("<div>").addClass("pointer");
    $led.append($pointer);
    $ledscreen.append($holder.append($led));
    var s1 = $holder.outerWidth(),
        s2 = $holder.outerHeight(),
        prevX = 0, prevY = 0;

    // Initialize new values
    smallSide = Math.min(s1, s2);
    largeSide = Math.max(s1, s2);
    rightEdge = screenWidth - largeSide;
    bottomEdge = screenHeight - largeSide;

    $holder.draggable({
        containment: "parent",
        axis: "x",      // TEMP
        start: function(){
            if (listeners.start) {
                listeners.start.apply(this, arguments);
            }
        },
        stop: function(){
            if (listeners.end) {
                listeners.end.apply(this, arguments);
            }
        },
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
                } else {                            // Horizontal Center
                    if (y < topBound && top < topBound) {               // Top
                        $(this).attr("data-direction", "top");
                    } else if (y > bottomBound && top > bottomBound) {  // Bottom
                        $(this).attr("data-direction", "down");
                    } else {                                            // Vertical middle
                        // Move it to the left if equals or if user moved mouse above
                        // the diagonal (eq of line: y-(h/w)*x=0) to below
                        // For coming from bottom up, the equation y+(h/w)*x-h=0 is used
                        if (x < screenWidth / 2) {
                            // Moving mouse down
                            if (    prevY - screenRatio * prevX < 0 && y - screenRatio * x > 0
                            // Moving mouse up
                            || prevY + screenRatio * prevX - screenHeight > 0
                            && y + screenRatio * x - screenHeight < 0) {
                                $(this).attr("data-direction", "left").draggable("option", "axis", "y")
                                    .css({ left: 0, top: ui.position.left });
                                }
                        } else {
                            // Moving mouse down
                            if (prevY - screenRatio * (screenWidth - prevX) < 0
                            && y - screenRatio * (screenWidth - x) > 0
                            // Moving mouse up
                            || prevY + screenRatio * (screenWidth - prevX) - screenHeight > 0
                            && y + screenRatio * (screenWidth - x) - screenHeight < 0) {
                                $(this).attr("data-direction", "right").draggable("option", "axis", "y")
                                    .css({ left: rightEdge, top: ui.position.left });
                            }
                        }
                    }
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
                } else {                            // Vertical middle
                    if (x < leftBound && left < leftBound) {            // Left
                        $(this).attr("data-direction", "left");
                    } else if (x > rightBound && left > rightBound) {   // Right
                        $(this).attr("data-direction", "right");
                    } else {                                            // Horizontal middle
                        // Move it to top or bottom
                        //  Top   : same equation as above, '> 0' -> '< 0'
                        //  Bottom: same equation as above, '< 0' -> '> 0'
                        if (y < screenHeight / 2) {
                            // Moving mouse right
                            if (prevY - screenRatio * prevX > 0 && y - screenRatio * x < 0
                            // Moving mouse left
                            || prevY - screenRatio * (screenWidth - prevX) > 0
                            && y - screenRatio * (screenWidth - x) < 0) {
                                $(this).attr("data-direction", "top").draggable("option", "axis", "x")
                                    .css({ left: ui.position.top, top: 0 });
                                }
                        } else {
                            // Moving mouse left
                            if ((screenHeight - prevY) - screenRatio * prevX > 0
                            && (screenHeight - y) - screenRatio * x < 0
                            // Moving mouse right
                            || (screenHeight - prevY) - (screenWidth - prevX) > 0
                            && (screenHeight - y) - (screenWidth - x) < 0) {
                                console.log("yooo")
                                $(this).attr("data-direction", "down").draggable("option", "axis", "x")
                                    .css({ left: ui.position.top, top: bottomEdge });
                            }
                        }
                    }
                }
            }
            prevX = x;
            prevY = y;
            if (listeners.drag) {
                listeners.drag.apply(this, arguments);
            }
        },
    });
    arrangeLed($holder, side, percentValue);
    return $holder;
}

// Implementation
w.Ledmap = {
    init: init,
    setGroups: setDefaultGroups,
    arrangeDefault: arrangeDefault,
    on: handleListeners
};

}(window));