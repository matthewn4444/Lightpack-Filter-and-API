# Lightpack Nodejs Extension

## What is this?

This is a Node.js extension that **only runs on Windows** because it uses
Win32 C++ ```createMutex("Name of mutex")```. You can see if the Node-webkit
application is running by checking the mutex you create with this small extension.

This is used for [Inno Setup](http://www.jrsoftware.org/isinfo.php) to distinguish 
if a Node-webkit application is openned before running setup.

So far only compatible with  Node-Webkit. It will **NOT** work for normal Node projects.

## How to Build

This project uses [nw-gyp](https://github.com/rogerwang/nw-gyp). A forked version
to work with Node-Webkit applications. Their website contains the instructions 
for each platform. Generally you need Python, Git/Cywgin bash and Visual Studio for Windows.

1. To install nw-gyp globally:
`$ npm install -g nw-gyp`

**For Windows**

2. Run **configure-win.sh** to configure the solution before compiling

3. Open **vs13/app-mutex.sln**, change to **Release build** and run it, 
it will compile to the Release folder

**Linix/Mac**

2. You need config.gypi. In this directory, run the configuration (Node webkit 0.9.2 was used)
`$ nw-gyp configure --target=0.9.2`

3. TODO

## Example usage

```js
var mutex = require("./app-mutex");

// Create a mutex
var m = mutex.create("Node webkit Application");

// Get the name of the mutex
console.log(mutex.getName(m));      // Outputs: Node webkit Application

// Destroy the mutex
mutex.destroy(m);

// Name of the mutex is null because we destroyed it
console.log(mutex.getName(m));      // Outputs: null
```

For more information on the API check the [wiki](https://github.com/matthewn4444/Lightpack-Filter-and-API/wiki/App-Mutex-%28Nodejs-extension%29)

## Test

Inside _*nw-test/1-run-test.bat*_ will compile a Node-Webkit project to run the test
file. Naturally you will need to compile this Nodejs extension and then you need
to include the Node-Webkit binaries inside the _*../../Release*_ folder before
running the test bat file.

