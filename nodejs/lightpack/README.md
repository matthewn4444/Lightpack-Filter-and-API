# Lightpack Nodejs Extension

## What is this?

This is a Node.js extension that allows you to control the lights using Lightpack
through a simple API. This extension is just a wrapper for one LedDevice object
in the Lightpack-lib project. So far only compatible with Node-Webkit. It will
**NOT** work for normal Node projects.

## How to Build

This project uses [nw-gyp](https://github.com/rogerwang/nw-gyp). A forked version
to work with Node-Webkit applications. Their website contains the instructions 
for each platform. Generally you need Python, Git/Cywgin bash and Visual Studio for Windows.

1. To install nw-gyp globally:
`$ npm install -g nw-gyp`

**For Windows**

2. Make sure you follow the [prerequisites (step 1-5)](https://github.com/matthewn4444/Lightpack-Filter-and-API/wiki/Building-the-Source#prerequisites). You can also modify the *config.txt* to set the nw.js version to compile.

3. Run **build.bat** to build the project

4. It will compile to the Release folder (../../Release)

**Linix/Mac**

2. You need config.gypi. In this directory, run the configuration (Node webkit 0.13.0 was used as an example)
`$ nw-gyp configure --target=0.13.0`

3. TODO

## Test

Inside _*nw-test/1-run-test.bat*_ will compile a Node-Webkit project to run the test
file. Naturally you will need to compile this Nodejs extension and then you need
to include the Node-Webkit binaries inside the _*../../Release*_ folder before
running the test bat file.

