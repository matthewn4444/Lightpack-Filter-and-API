#!/usr/bin/env node
var assert = require('assert');
var lightpack = require('./vs13/Release/lightpack');

// Check the static variables
assert.equal(10, lightpack.LedsPerDevice);
assert.equal(100, lightpack.DefaultBrightness);
assert.equal(2.2, lightpack.DefaultGamma);

var connected = lightpack.open();
if (!connected) {
    console.log("Failed: Please connect the lightpack usb device.");
} else {
    assert.ok(lightpack.setSmooth(0));
    assert.ok(lightpack.setColorToAll(0));
    
    // Check if closing will return true when setting colors
    lightpack.closeDevices();
    assert.ok(!lightpack.getCountLeds() == 0);
    assert.ok(lightpack.tryToReopenDevice());
    
    // If there is only one device
    assert.ok(lightpack.getCountLeds() > 0);
    console.log("There are", lightpack.getCountLeds(), "leds");

    assert.ok(lightpack.setSmooth(200));
    assert.ok(lightpack.setBrightness(75));
    assert.ok(lightpack.setGamma(2.0));
    assert.ok(lightpack.setColorDepth(10));
    assert.ok(lightpack.setRefreshDelay(100));
    assert.ok(lightpack.setColorToAll(255, 0, 0));

    setTimeout(function(){
        assert.ok(lightpack.turnOff());
        setTimeout(function(){
            assert.ok(lightpack.turnOn());
            assert.ok(lightpack.setColor(4, 0, 255, 255));
            
            setTimeout(function(){
                assert.ok(lightpack.setColors([
                    [255, 0, 255],
                    -1,
                    [255, 0, 255],
                    0,
                    -1,
                    [0, 255, 0],
                    [0, 0, 255]
                ]));
            }, 1000);
        }, 500);
    }, 1000);
}

