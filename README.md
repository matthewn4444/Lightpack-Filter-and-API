# Lightpack Filter & API

**_Author_**: Matthew Ng (matthewn4444@gmail.com)

Add Lights to the back of your TV! Use the **Lightpack filter** to have videos
render color to the back of your TV in ***realtime***!

Or you can program your own projects with your Lightpack device with the API!

![Lightpack](http://i.imgur.com/Kym2v0c.jpg)

_Image from Lightpack's website_

This includes two projects:

1. [Lightpack Filter](https://github.com/matthewn4444/Lightpack-Filter-and-API/wiki/Usage) for [MPC-HC](https://mpc-hc.org/)
2. [Lightpack C++ API (unofficial)](https://github.com/matthewn4444/Lightpack-Filter-and-API/wiki/Lightpack-API)

## Lightpack Filter Installation

Choose either setup installation or unzip the package anywhere on your computer

###Binaries (Windows x86/x64)

- [Installer (0.6.4)](https://github.com/matthewn4444/Lightpack-Filter-and-API/releases/download/v0.6.4/setup.exe)
- [Zip (0.6.4)](https://github.com/matthewn4444/Lightpack-Filter-and-API/releases/download/v0.6.4/lightpack-filter.zip)

Go [here](https://github.com/matthewn4444/Lightpack-Filter-and-API/wiki/Installation) for more information on setup and usage.

###Source Code
- [Source code (zip)](https://github.com/matthewn4444/Lightpack-Filter-and-API/archive/v0.6.4.zip)

Go [here](https://github.com/matthewn4444/Lightpack-Filter-and-API/wiki/Building-the-Source) for compile and building instructions.

## What is this?

### Lightpack Concept/Device
[Lightpack](http://lightpack.tv/) was a kickstarted project and the APIs were openned
source. Users put lights on the back of their televisions and the computer can send
colors to the lights, extending the picture's/video's experience. I have taken
the open source software and created a C++ and Node.js API.

### Lightpack Filter

The filter allows you to play a video with lightpack shown in realtime without any
lag. You can play a video with [Media player classic (MPC)](http://mpc-hc.org/) and
lightpack will update the colors of what is shown on the screen.

For more information, go [here](https://github.com/matthewn4444/Lightpack-Filter-and-API/tree/master/directshow).

**This filter only works on Windows.**

### More Device Support
I do not have any other devices besides Lightpack. If you want support for another
device, you can email me if you can help me build it (I would mainly give you builds
to test on your device).

### C++ API

I have created a single API for C++ and Node.js to interface to the device and
Prismatik. These APIs are **cross-platform** and should work on Windows, Mac and Linux.

For more information, go [here](https://github.com/matthewn4444/Lightpack-Filter-and-API/tree/master/LightpackAPI).

#### Example Usage

```cpp
#include <Lightpack.h>
#include <iostream>

using namespace std;
using namespace Lightpack;

#define HOST "127.0.0.1"
#define PORT 3636
#define APIKEY "{cb832c47-0a85-478c-8445-0b20e3c28cdd}"


int main()
{
    // Send colors to the device
    LedDevice device;
    if (device.open()) {
        cout << "Connected directly to the device" << endl;
        device.setBrightness(100);
        device.setColorToAll(255, 255, 0); // RGB value
    }
    else {
        cout << "Prismatik is probably running, try that" << endl;

        // Send colors via Prismatik
        PrismatikClient light;
        // Test all the functions in the Lightpack API
        if (light.connect(HOST, PORT, {}, APIKEY) == Lightpack::OK) {
            cout << "Connected to Prismatik"
            light.lock();
            light.setBrightness(100);
            light.setColorToAll(255, 255, 0);
        }
        else {
            cout << "USB not connected" << endl;
        }
    }
    return 0;
}
```

Check the wiki for the entire API
