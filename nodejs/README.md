# Lightpack Nodejs Extension

## What is this?

This is a Node.js extension that allows you to control the lights using Lightpack
through a simple API. This extension is just a wrapper for one LedDevice object
in the Lightpack-lib project.

## How to Build

This project uses [node-gyp](https://github.com/TooTallNate/node-gyp). Their website
contains the instructions for each platform. Generally you need Python and 
Visual Studio for Windows.

1. To install gyp globally:
`$ npm install -g node-gyp`

2. You need config.gypi. In this directory, run the configuration
`$ node-gyp configure`

Copy **/build/config.gypi** and move it to */vs13*. Then you can delete the *build* folder.

**For Windows**

Use Visual Studio to compile the project in lightpack.sln in */vs13*.
