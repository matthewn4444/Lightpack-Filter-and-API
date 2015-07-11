/*
 *  Draggable Led Maps
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
 *      Ledmap.arrangeDefault()
 *
 *  Get all the led's position by side and percentage position
 *      @return returns an array of object data with the format: { side:<0-3>, percent: <0-100> }
 *      Ledmap.getPositions()
 *
 *  Set all the led's position
 *      @params positions   an array of position values of the format:{ side:<0-3>, percent: <0-100> }
 *      Ledmap.setPositions(positions)
 *
 *  Set the colors of the leds (most be 10 colors of format: [ [r,g,b], [r,g,b], ... ]
 *      @params colors      Colors to set each 10 led groups
 *      Ledmap.setColorGroup(colors)
 *
 *  Get the colors of the leds set
 *      @return colors that are used for each group of leds
 *      Ledmap.getColorGroup()
 *
 *  Set dragging event listeners
 *      @param eventName    String event name
 *      @param callback     Function that gets called when event occurs
 *      @return self        Returns instance of Ledmap
 *      Ledmap.on(eventName, callback)
 *    Possible Events: start | drag | end | mouseover | mouseout | startSelection | endSelection
 *
 *  Set the horizontal height depth percentage
 *      @param percent      number from 0-50 that represents the depth (height)
 *      Ledmap.setHorizontalDepth(percent)
 *
 *  Get the horizontal height depth percentage
 *      @return percent     number from 0-50 that represents the depth (height)
 *      Ledmap.getHorizontalDepth()
 *
 *  Set the vertical width depth percentage
 *      @param percent      number from 0-50 that represents the depth (width)
 *      Ledmap.setVerticalDepth(percent)
 *
 *  Get the vertical width depth percentage
 *      @return percent     number from 0-50 that represents the depth (width)
 *      Ledmap.getVerticalDepth()
 *
 *  Side Data:
 *      'r' = Right
 *      't' = Top
 *      'l' = Left
 *      'b' = Bottom
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
    isDragging = false,
    isMouseOver = false,
    horizontalDepthPercent = 0.15,
    verticalDepthPercent = 0.10,
    listeners = {
        start: null,
        drag: null,
        end: null,
        mouseover: null,
        mouseout: null,
        startSelection: null,
        endSelection: null
    },
    colorGroup = [
        [255, 0, 0],        // Red
        [255, 255, 0],      // Yellow
        [120, 163, 185],    // Darker Blue
        [0, 255, 0],        // Green
        [255, 0, 255],      // Dark Purple
        [255, 144, 0],      // Orange
        [0, 255, 255],      // Light blue
        [192, 60, 68],      // Red
        [255, 214, 160],    // Peach
        [170, 102, 204],    // Light purple
    ];

function init($screen, numOfGroups) {
    $screen = $($screen);

    // Move the leds to the new screen if exists
    if ($ledscreen != null && $ledscreen.length && !$ledscreen.is($screen)) {
        $ledscreen.removeClass("led-map-screen");
        $ledscreen.off(".led-map");
    }

    $ledscreen = $screen;
    $ledscreen.addClass("led-map-screen");
    updateMetrics();
    setDefaultGroups(numOfGroups);

    // Mouse over and out events
    $ledscreen.on("mouseover.led-map", ".holder", function(){
        // On mouse over, make all leds to white but the current led to red
        isMouseOver = true;
        if (!isDragging && $ledscreen) {
            $ledscreen.find(".group").addClass("not-selected");
            $(this).parent().addClass("selected").removeClass("not-selected");
            if (listeners.startSelection) {
                listeners.startSelection.apply(this, arguments);
            }
        }
        if (listeners.mouseover) {
            listeners.mouseover.apply(this, arguments);
        }
    }).on("mouseout.led-map", ".holder", function(){
        isMouseOver = false;
        if (!isDragging && $ledscreen) {
            $ledscreen.find(".group").removeClass("not-selected").removeClass("selected");
            if (listeners.endSelection) {
                listeners.endSelection.apply(this, arguments);
            }
        }
        if (listeners.mouseout) {
            listeners.mouseout.apply(this, arguments);
        }
    });
}

function setHorizontalDepth(percent) {
    percent = Math.min(Math.max(percent, 0), 50);
    if (horizontalDepthPercent != percent) {
        horizontalDepthPercent = percent / 100;
        updateRectangles();
    }
}

function setVerticalDepth(percent) {
    percent = Math.min(Math.max(percent, 0), 50);
    if (verticalDepthPercent != percent) {
        verticalDepthPercent = percent / 100;
        updateRectangles();
    }
}

function updateRectangles() {
    if ($ledscreen == null) {
        throw new Error("Did not run Ledmap.init($screen)");
    }
    var $rects = $ledscreen.find(".rect"),
        leftPos = [],
        rightPos = [],
        topPos = [],
        bottomPos = [];
    $rects.each(function(i){
        var item = getPositionAndSideOfLed(i),
            obj = { n: i, percent: item.percent };
        switch(item.side) {
            case 'r':
                // Right up
                if (item.percent == 0) {
                    topPos.push(obj);
                } else if (item.percent == 100) {
                    bottomPos.push(obj);
                }
                rightPos.push(obj);
                break;
            case 'l':
                // Left up
                if (item.percent == 0) {
                    topPos.push(obj);
                }
                leftPos.push(obj);
                break;
            case 't':
                topPos.push(obj);
                break;
            case 'b':
                // Left down
                if (item.percent == 0) {
                    leftPos.push(obj);
                }
                bottomPos.push(obj);
                break;
            default:
                throw new Error("Side is not valid");
        }
    });
    // Sort the values
    var order = [ leftPos, rightPos, topPos, bottomPos ],
        n = 0,
        sum = 0,
        verticalDepth = Math.round(verticalDepthPercent * screenWidth),
        horizontalDepth = Math.round(horizontalDepthPercent * screenHeight);
    order.sort(function(a,b){ return b.length > a.length; });
    for (var i = 0; i < 4; i++) {
        order[i].sort(function(a,b){ return a.percent > b.percent; });

        // Calculate the width/height and rects for each led
        var prev = 0,
            average = n > 0 ? sum * 1.0 / n : (order[i] == leftPos || order[i] == leftPos ? screenWidth : screenHeight);
        for (var j = 0; j < order[i].length; j++) {
            var item = order[i][j],
                p = item.percent,
                $rect = $ledscreen.find(".group[data-led='" + item.n + "'] .rect");
            // Calculate the midpoint between this and next if there is a next otherwise max
            var next = j + 1 < order[i].length ? (p + order[i][j + 1].percent) / 2.0 : 100.0;
            var prev2this = Math.abs(prev - p);
            var this2next = Math.abs(p - next);
            prev2this = prev2this > 0 ? prev2this : 100;
            this2next = this2next > 0 ? this2next : 100;

            // Calculate the width by getting the min distance from current to prev midpoint
            // or next midpoint and the averaged width
            var halfSizePercent = Math.min(prev2this, this2next);
            var size = 0;

            switch(order[i]) {
                case leftPos:
                    size = Math.round(Math.min(halfSizePercent * 2 * screenHeight / 100.0, average));
                    $rect.css({
                        left: 0,
                        height: size
                    });
                    if (p != 0 && p != 100) {
                        $rect.css({
                            width: verticalDepth,
                            top: Math.round(p / 100.0 * screenHeight - size / 2),
                        });
                    }
                    break;
                case rightPos:
                    size = Math.round(Math.min(halfSizePercent * 2 * screenHeight / 100.0, average));
                    $rect.css({
                        left: screenWidth - verticalDepth,
                        height: size
                    });
                    if (p != 0 && p != 100) {
                        $rect.css({
                            width: verticalDepth,
                            top: Math.round(p / 100.0 * screenHeight - size / 2),
                        });
                    }
                    break;
                case topPos:
                    size = Math.round(Math.min(halfSizePercent * 2 * screenWidth / 100.0, average));
                    $rect.css({
                        top: 0,
                        width: size
                    });
                    if (p != 0 && p != 100) {
                        $rect.css({
                            height: horizontalDepth,
                            left: Math.round(p / 100.0 * screenWidth - size / 2),
                        });
                    }
                    break;
                case bottomPos:
                    size = Math.round(Math.min(halfSizePercent * 2 * screenWidth / 100.0, average));
                    $rect.css({
                        top: screenHeight - horizontalDepth,
                        width: size
                    });
                    if (p != 0 && p != 100) {
                        $rect.css({
                            height: horizontalDepth,
                            left: Math.round(p / 100.0 * screenWidth - size / 2),
                        });
                    } else {
                    }
                    break;
                default:
                    throw new Error("Did not work");
            }
            if (p == 0 || p == 100) {
                $rect.css({
                    width: verticalDepth,
                    height: horizontalDepth
                });
            }

            // Update the data for the average
            sum += size;
            n++;
            prev = next;
        }
    }
}

function handleListeners(eventName, fn) {
    if (typeof(fn) == "function" || fn == null) {
        if (listeners.hasOwnProperty(eventName)) {
            listeners[eventName] = fn;
        }
    }
    return this;
}

function setLedPositions(positions) {
    if ($ledscreen == null) {
        throw new Error("Did not run Ledmap.init($screen)");
    }
    if (!Array.isArray(positions)) {
        throw new Error("Did not set the led positions correctly with an array.");
    }

    // Ensure we have the correct number of leds to display from input
    var numOfGroups = positions.length / 10;
    setDefaultGroups(numOfGroups);
    for (var i = 0; i < positions.length; i++) {
        var side = positions[i].side,
            percent = positions[i].percent,
            $holder = $ledscreen.find(".group[data-led='" + i + "'] .holder");
        arrangeLed($holder, side, percent);
    }
    updateRectangles();
}

function getLedPositions() {
    var values = [];
    if ($ledscreen != null) {
        var $leds = $ledscreen.find(".group");
        $leds.each(function(i){
            values.push(getPositionAndSideOfLed(i));
        });
    }
    return values;
}

function setColorGroup(colors) {
    if (!Array.isArray(colors)) {
        throw new Error("Colors specified is not an array");
    }
    if (colors.length < 10) {
        throw new Error("Did not pass enough colors (10 is minimim)");
    }
    colorGroup = colors;
}

function getColorGroup() {
    return colorGroup;
}

function updateMetrics() {
    var positions = getLedPositions();
    screenWidth = Math.round($ledscreen.innerWidth()),
    screenHeight = Math.round($ledscreen.innerHeight()),
    screenRatio = screenHeight / screenWidth;

    var $leds = $ledscreen.find(".holder");
    if ($leds.length) {
        var s1 = $leds.eq(0).outerWidth(),
            s2 = $leds.eq(0).outerHeight();
        smallSide = Math.min(s1, s2);
        largeSide = Math.max(s1, s2);
        rightEdge = screenWidth - largeSide;
        bottomEdge = screenHeight - largeSide;

        // Update the positions
        setLedPositions(positions);
    }
}

function arrangeDefault() {
    var $leds = $ledscreen.find(".holder");
    if ($leds.length) {
        var numOfGroups = $leds.length / 10,
            verticalParts = 3 * numOfGroups,
            horizontalParts = 4 * numOfGroups,
            verticalBlockSize = 1 / (verticalParts * 2),
            horizontalBlockSize = 1 / (horizontalParts * 2),
            x = 0, y = 0;

        // Right
        for (var i = verticalParts - 1, y = 0; i >= 0 ; i--) {
            y += verticalBlockSize;
            arrangeLed($leds.eq(i), 'r', y * 100);
            y += verticalBlockSize;
        }

        // Top
        for (var i = horizontalParts - 1, x = 0; i >= 0; i--) {
            x += horizontalBlockSize;
            arrangeLed($leds.eq(i + verticalParts), 't', x * 100);
            x += horizontalBlockSize;
        }

        // Left
        for (var i = 0, y = 0; i < verticalParts; i++) {
            y += verticalBlockSize;
            arrangeLed($leds.eq(i + verticalParts + horizontalParts), 'l', y * 100);
            y += verticalBlockSize;
        }
    }
    updateRectangles();
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
    var $groups = $ledscreen.find(".group");
    if ($groups.length != numOfGroups * 10) {
        $groups.remove();
        for (var i = 0; i < numOfGroups * 10; i++) {
            var color = colorGroup[i % 10],
                className = "color_",
                colorObj = {"background-color": "rgb(" + color[0] + "," + color[1] + "," + color[2] + ")"};
            for (var j = 0; j < 3; j++) {
                if (color[j] == 0) {
                    className += "0";
                }
                className += color[j].toString(16);
            }
            $led = addLed('t', 0)
            $led.find(".pointer").addClass(className);
            $led.parent().find(".rect").css(colorObj);
        }
        arrangeDefault();
    }
}

function arrangeLed($holder, side, percentValue) {
    if (!$holder.length) return;
    $holder.parent().attr("data-direction", "up");
    var verticalPos = Math.round(screenHeight * percentValue / 100.0) - smallSide / 2,
        horizontalPos = Math.round(screenWidth * percentValue / 100.0) - smallSide / 2,
        leftBound = smallSide / 4.0, rightBound = (screenWidth - smallSide) - smallSide / 4.0,
        topBound = leftBound, bottomBound = (screenHeight - smallSide) - smallSide / 4.0;

    switch(side) {
        case 'r':    // Right
            if (verticalPos < topBound) {
                $holder.css({ left: rightEdge, top: 0 }).parent().attr("data-direction", "right-up");
            } else if (verticalPos > bottomBound) {
                $holder.css({ left: rightEdge, top: bottomEdge }).parent().attr("data-direction", "right-down");
            } else {
                $holder.css({ left: rightEdge, top: verticalPos }).parent().attr("data-direction", "right");
            }
            $holder.draggable("option", "axis", "y");
            break;
        case 't':    // Top
            if (horizontalPos < leftBound) {
                $holder.css({ left: 0, top: 0 }).parent().attr("data-direction", "left-up");
            } else if (horizontalPos > rightBound) {
                $holder.css({ left: rightEdge, top: 0 }).parent().attr("data-direction", "right-up");
            } else {
                $holder.css({ left: horizontalPos, top: 0 }).parent().attr("data-direction", "up");
            }
            $holder.draggable("option", "axis", "x");
            break;
        case 'l':    // Left
            if (verticalPos < topBound) {
                $holder.css({ left: 0, top: 0 }).parent().attr("data-direction", "left-up");
            } else if (verticalPos > bottomBound) {
                $holder.css({ left: 0, top: bottomEdge }).parent().attr("data-direction", "left-down");
            } else {
                $holder.css({ left: 0, top: verticalPos }).parent().attr("data-direction", "left");
            }
            $holder.draggable("option", "axis", "y");
            break;
        case 'b':    // Bottom
            if (horizontalPos < leftBound) {
                $holder.css({ left: 0, top: bottomEdge }).parent().attr("data-direction", "left-down");
            } else if (horizontalPos > rightBound) {
                $holder.css({ left: rightEdge, top: bottomEdge }).parent().attr("data-direction", "right-down");
            } else {
                $holder.css({ left: horizontalPos, top: bottomEdge }).parent().attr("data-direction", "down");
            }
            $holder.draggable("option", "axis", "x");
            break;
        default:
            throw new Error("Position specified is not valid");
    }
}

function getPositionAndSideOfLed(i) {
    if ($ledscreen != null) {
        var $led = $ledscreen.find(".group[data-led='" + i + "'] .holder");
        if ($led.length) {
            var $rect = $ledscreen.find(".group[data-led='" + i + "'] .rect"),
                side = 'r', percentValue = 0,
                verticalPercentage = (parseInt($led.css("top"), 10) + smallSide / 2) / screenHeight * 100,
                horizontalPercentage = (parseInt($led.css("left"), 10) + smallSide / 2) / screenWidth * 100,
                rect = {
                    x: parseInt($rect.css("left"), 10) * 100 / screenWidth,
                    y: parseInt($rect.position().top, 10) * 100 / screenHeight,
                    width: $rect.width() * 100 / screenWidth,
                    height: $rect.height() * 100 / screenHeight
                };
            // Corners
            switch($led.parent().attr("data-direction")) {
                case "left-up":
                    side = 'l';
                    break;
                case "right-up":
                    // Default values are fine
                    break;
                case "left-down":
                    side = 'b';
                    break;
                case "right-down":
                    percentValue = 100;
                    break;
                case "left":
                    side = 'l';
                    percentValue = verticalPercentage;
                    break;
                case "up":
                    side = 't';
                    percentValue = horizontalPercentage;
                    break;
                case "right":
                    percentValue = verticalPercentage;
                    break;
                case "down":
                    side = 'b';
                    percentValue = horizontalPercentage;
                    break;
                default:
                    throw new Error("There is no valid direction on this led");
            }
            return { side: side, percent: percentValue, rect: rect };
        }
    }
    return null;
}

function addLed(side, percentValue) {
    if ($ledscreen == null) {
        throw new Error("Did not run Ledmap.init($screen)");
    }
    var num = $ledscreen.find(".group");
    var $holder = $("<div>").addClass("holder");
    var $pointer = $("<div>").addClass("pointer");
    var $rect = $("<div>").addClass("rect");
    var $group = $("<div>").addClass("group").attr("data-led", num.length);
    $holder.append($pointer);
    $group.append($holder);
    $group.append($rect);
    $ledscreen.append($group);
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
            isDragging = true;
            if (listeners.start) {
                listeners.start.apply(this, arguments);
            }
        },
        stop: function(){
            isDragging = false;
            if (!isMouseOver && $ledscreen) {
                $ledscreen.find(".group").removeClass("not-selected").removeClass("selected");
                if (listeners.endSelection) {
                    listeners.endSelection.apply(this, arguments);
                }
            }
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
                        $(this).css("top", 0).draggable("option", "axis", x > y ? "x" : "y")
                            .parent().attr("data-direction", "left-up");
                        ui.position.top = 0;
                    } else if (y < bottomBound) {   // Vertical middle
                        console.log("left");
                        $(this).draggable("option", "axis", "y").parent().attr("data-direction", "left");
                    } else if (y > bottomBound) {   // Below bottom
                        console.log("left-down:x");
                        $(this).css("top", bottomEdge).draggable("option", "axis", (bottomEdge - y) > x ? "y" : "x")
                            .parent().attr("data-direction", "left-down")
                        ui.position.top = bottomEdge;
                    }
                } else if (x > rightBound) {        // Right
                    ui.position.left = rightEdge;
                    $(this).css("left", rightEdge);
                    if (y < topBound) {             // Top
                        console.log("right-up:x");
                        $(this).css("top", 0).draggable("option", "axis", (rightEdge - x) > y ? "x" : "y")
                            .parent().attr("data-direction", "right-up");
                        ui.position.top = 0;
                    } else if (y < bottomBound) {   // Vertical Middle
                        console.log("right:x");
                        $(this).draggable("option", "axis", "y").parent().attr("data-direction", "right");
                    } else if (y > bottomBound) {   // Bottom
                        console.log("right-down:x");
                        $(this).css("top", bottomEdge).draggable("option", "axis", y > x ? "y" : "x")
                            .parent().attr("data-direction", "right-down");
                        ui.position.top = bottomEdge;
                    }
                } else {                            // Horizontal Center
                    if (y < topBound && top < topBound) {               // Top
                        $(this).parent().attr("data-direction", "up");
                    } else if (y > bottomBound && top > bottomBound) {  // Bottom
                        $(this).parent().attr("data-direction", "down");
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
                                $(this).draggable("option", "axis", "y").css({ left: 0, top: ui.position.left })
                                    .parent().attr("data-direction", "left");
                                }
                        } else {
                            // Moving mouse down
                            if (prevY - screenRatio * (screenWidth - prevX) < 0
                            && y - screenRatio * (screenWidth - x) > 0
                            // Moving mouse up
                            || prevY + screenRatio * (screenWidth - prevX) - screenHeight > 0
                            && y + screenRatio * (screenWidth - x) - screenHeight < 0) {
                                $(this).draggable("option", "axis", "y").css({ left: rightEdge, top: ui.position.left })
                                    .parent().attr("data-direction", "right");
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
                        $(this).css("left", 0).draggable("option", "axis", x > y ? "x" : "y")
                            .parent().attr("data-direction", "left-up");
                        ui.position.left = 0;
                    } else if (x < rightBound) {    // Horizontal middle
                        console.log("up:y");
                        $(this).draggable("option", "axis", "x").parent().attr("data-direction", "up");
                    } else if (x > rightBound) {    // Right
                        console.log("right-up:y");
                        $(this).css("left", rightEdge).draggable("option", "axis", (rightEdge - x) > y ? "x" : "y")
                            .parent().attr("data-direction", "right-up");
                        ui.position.left = rightEdge;
                    }
                } else if (y > bottomBound) {       // Bottom
                    ui.position.top = bottomEdge;
                    $(this).css("top", bottomEdge);
                    if (x < leftBound) {            // Left
                        console.log("left-down:y");
                        $(this).css("left", 0).draggable("option", "axis", (bottomEdge - y) > x ? "y" : "x")
                            .parent().attr("data-direction", "left-down");
                        ui.position.left = 0;
                    } else if (x < rightBound) {   // Horizontal middle
                        console.log("down:y");
                        $(this).draggable("option", "axis", "x").parent().attr("data-direction", "down");
                    } else if (x > rightBound) {    // Right
                        console.log("right-down:y");
                        $(this).css("left", rightEdge).draggable("option", "axis", y > x ? "y" : "x")
                            .parent().attr("data-direction", "right-down");
                        ui.position.left = rightEdge;
                    }
                } else {                            // Vertical middle
                    if (x < leftBound && left < leftBound) {            // Left
                        $(this).parent().attr("data-direction", "left");
                    } else if (x > rightBound && left > rightBound) {   // Right
                        $(this).parent().attr("data-direction", "right");
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
                                $(this).draggable("option", "axis", "x").css({ left: ui.position.top, top: 0 })
                                    .parent().attr("data-direction", "up");
                                }
                        } else {
                            // Moving mouse left
                            if ((screenHeight - prevY) - screenRatio * prevX > 0
                            && (screenHeight - y) - screenRatio * x < 0
                            // Moving mouse right
                            || (screenHeight - prevY) - (screenWidth - prevX) > 0
                            && (screenHeight - y) - (screenWidth - x) < 0) {
                                console.log("yooo")
                                $(this).draggable("option", "axis", "x").css({ left: ui.position.top, top: bottomEdge })
                                    .parent().attr("data-direction", "down");
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
            updateRectangles();
        },
    });
    arrangeLed($holder, side, percentValue);
    return $holder;
}

// Implementation
w.Ledmap = {
    init: init,
    setPositions: setLedPositions,
    getPositions: getLedPositions,
    setColorGroup: setColorGroup,
    getColorGroup: getColorGroup,
    setHorizontalDepth: setHorizontalDepth,
    getHorizontalDepth: function() { return Math.round(horizontalDepthPercent * 100); },
    setVerticalDepth: setVerticalDepth,
    getVerticalDepth: function() { return Math.round(verticalDepthPercent * 100); },
    setGroups: setDefaultGroups,
    updateMetrics: updateMetrics,
    arrangeDefault: arrangeDefault,
    on: handleListeners
};

}(window));