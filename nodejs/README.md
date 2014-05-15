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

2. Run **configure-win.sh** to configure the solution before compiling

3. Open **vs13/lightpack.sln** and run it, it will compile to the Release folder


**Linix/Mac**

2. You need config.gypi. In this directory, run the configuration (Node webkit 0.9.2 was used)
`$ nw-gyp configure --target=0.9.2`

3. TODO

## Test

Inside _*nw-test/1-run-test.bat*_ will compile a Node-Webkit project to run the test
file. Naturally you will need to compile this Nodejs extension and then you need
to include the Node-Webkit binaries inside the _*../../Release*_ folder before
running the test bat file.

