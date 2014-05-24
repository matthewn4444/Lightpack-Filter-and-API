#!/usr/bin/env node
var client = require("lightpack/prismatik-client");
var assert = require("assert");

var key = "{cb832c47-0a85-478c-8445-0b20e3c28cdd}";

function assertCallback(flag) {
    assert.ok(flag);
}

client.connect(function(connected){
    assert.ok(connected);
    console.log("Connected");
    
    client.lock(function(locked){
        assert.ok(locked);
        console.log("Locked");
        
        // Test the gets
        client.getCountLeds(function(num){
            assert(num, 10);
        });
        client.getProfiles(function(p){
            assert(p[0], "Lightpack");
        });
        client.getProfile(function(p){
            assert(p, "Lightpack");
        });
        client.getStatus(function(s){
            assert(s, "on");
        });
        client.getAPIStatus(function(s){
            assert(s, "busy");
        });
        
        // Test the sets
        client.setSmooth(100, assertCallback);
        client.setColorToAll(0, 255, 0, assertCallback);
        
        setTimeout(function(){
            client.setBrightness(70, assertCallback);
            client.setGamma(4.2, assertCallback);
            client.setColor(4, 255, 0, 0, assertCallback);
            setTimeout(function(){
                client.turnOff(assertCallback);
                setTimeout(function(){
                    client.turnOn(assertCallback);
                    setTimeout(function(){
                        client.disconnect(assertCallback);
                        console.log("All tests pass");
                    }, 1000);
                }, 1000);
            }, 1000);
        }, 1000);
    });
}, {apikey: key});

console.log("Starting...");