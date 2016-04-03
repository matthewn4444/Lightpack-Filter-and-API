#!/usr/bin/env python
import subprocess, sys
sys.path.append("../directshow/directshow-sdk/")
import vs

def main():
    print 'Building Lightpack libraries'
    path = vs.get_devenv_path()
    if path != None:
        # Build all configurations
        print "\tBuilding release x86"
        res = vs.build(path, "Lightpack.vcxproj", True, True)
        if res != 0:
            print "\tFailure to build release x86, open Visual Studio to see the issue"
            return

        print "\tBuilding release x64"
        res = vs.build(path, "Lightpack.vcxproj", True, False)
        if res != 0:
            print "\tFailure to build release x64, open Visual Studio to see the issue"
            return

        print "\tBuilding debug x86"
        res = vs.build(path, "Lightpack.vcxproj", False, True)
        if res != 0:
            print "\tFailure to build release x86, open Visual Studio to see the issue"
            return

        print "\tBuilding debug x64"
        res = vs.build(path, "Lightpack.vcxproj", False, False)
        if res != 0:
            print "\tFailure to build release x86, open Visual Studio to see the issue"
            return

        print "\tSuccessfully built lightpack libraries, output in /libs"
    else:
        print "\tFailed to find Visual Studio path, cannot build"

if __name__ == "__main__":
    main()
