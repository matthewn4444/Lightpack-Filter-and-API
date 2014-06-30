# Lightpack Filter (For Windows)

**_Author_**: Matthew Ng (matthewn4444@gmail)

This is a Directshow filter for Windows that allows you to use Lightpack with 
videos. 

This is similar to [SVPLight](http://www.svp-team.com/wiki/SVPlight) 
but you do not need to run SVP to use this.

## Installation

Choose either installation or unzip the package anywhere on your computer

###Binaries (Windows 32bit)

- [Installer (0.5.0b)](https://github.com/matthewn4444/Lightpack-Filter-and-API/releases/download/v0.5.0b/setup.exe)
- [Zip (0.5.0b)](https://github.com/matthewn4444/Lightpack-Filter-and-API/releases/download/v0.5.0b/lightpack-filter.zip)

###Source Code
- [Source code (zip)](https://github.com/matthewn4444/Lightpack-Filter-and-API/archive/v0.5.0b.zip)

## Setup
This is compatible with [Media Player Classic (MPC)](http://mpc-hc.org/) and 
[PotPlayer](http://potplayer.daum.net/). You will need to add the filter (lightpack.ax)
to the player.

For more instructions, view [here](s).

## Performance and Support
Using MPC, adding the Lightpack filter increases the CPU overhead about **1-3%**.

The filter supports all forms of video (that I can tell) and other filters including

- 8bit and 10bit video
- [madVR](www.madvr.com)
- [SVP](http://www.svp-team.com/)
- [LAV Filters](https://code.google.com/p/lavfilters/)

## Building the source code
View [here](s) for more information to build the source.

## 3rd Party Libraries
- [Lightpack](http://lightpack.tv)
- [Node-Webkit](https://github.com/rogerwang/node-webkit)
- [Directshow](http://msdn.microsoft.com/en-us/library/windows/desktop/dd375454%28v=vs.85%29.aspx)
- [nw-gyp](https://github.com/rogerwang/nw-gyp)
- [Inno Setup](http://www.jrsoftware.org/isinfo.php)
- [node-webkit updater](https://github.com/edjafarov/node-webkit-updater)
- [hidapi](http://www.signal11.us/oss/hidapi/)


