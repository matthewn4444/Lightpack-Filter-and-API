# Lightpack C++ Library

Author: Matthew Ng (matthewn4444@gmail)

Similar to pyLightpack from the author's site, this API is used for C++ although containing the same functions.
This library uses [dlib](http://dlib.net/) for their sockets API. 
This library communicates to the Lightpack server [Prismatik](http://lightpack.tv/downloads).

So far this library was compiled on Windows using Visual Studio 2013 however I am sure this will compile on Linux/Mac.

## Include in Your project

- Include the header file **Lightpack.h** in the **_include_** folder
- Include the library file **Lightpack.lib** for Windows or **Lightpack.a** for Linux/Mac
- Compile your project (no need to include dlib)

## How to Compile

### Windows

1. Open the project in Visual Studio
2. There will be two projects: Lightpack and a test project. Compile both
3. Test if the test.exe application passes (make sure Prismartik is running and usb cable is plugged in)

### Linux/Mac

**TODO**