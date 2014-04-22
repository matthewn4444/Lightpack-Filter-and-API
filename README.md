# Lightpack API C++ Library

**_Author_**: Matthew Ng (matthewn4444@gmail)

Similar to pyLightpack from the author's site, this API is used for C++ although containing the same functions.
This library uses [dlib](http://dlib.net/) for their sockets API. 
This library communicates to the Lightpack server [Prismatik](http://lightpack.tv/downloads).

So far this library was compiled on Windows using Visual Studio 2013 however I am sure this will compile on Linux/Mac.

## Example Usage

```cpp

#include <Lightpack.h>
#include <string.h>

#define HOST "127.0.0.1"
#define PORT 3636
#define APIKEY "{cb832c47-0a85-478c-8445-0b20e3c28cdd}"

int main() {
    Lightpack light(HOST, PORT, { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }, APIKEY);
    try {
        // Test all the functions in the Lightpack APIj
        if (light.connect() == Lightpack::RESULT::OK) {
            light.lock();
            light.setSmooth(100);
            light.setBrightness(40);
            light.setColorToAll(255, 0, 0);     // Red
        } else {
            printf("Failed to connect.\n");
        }
    }
    catch (std::exception& e) {
        printf("%s\n", e.what());
    }
    return 0;
}

```

Check the wiki for the entire API


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