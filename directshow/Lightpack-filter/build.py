#!/usr/bin/env python
import subprocess, sys
sys.path.append("../directshow-sdk/")
import vs

def main():
    print 'Building Lightpack filter'
    path = vs.get_devenv_path()
    if path != None:
        # Build all configurations
        print "\tBuilding release x86"
        res = vs.build(path, "Lightpack-filter.vcxproj", True, True)
        if res != 0:
            print "\tFailure to build release x86, open Visual Studio to see the issue"
            return

        print "\tBuilding release x64"
        res = vs.build(path, "Lightpack-filter.vcxproj", True, False)
        if res != 0:
            print "\tFailure to build release x64, open Visual Studio to see the issue"
            return

        print "\tBuilding debug x86"
        res = vs.build(path, "Lightpack-filter.vcxproj", False, True)
        if res != 0:
            print "Failure to build release x86, open Visual Studio to see the issue"
            return

        print "\tBuilding debug x64"
        res = vs.build(path, "Lightpack-filter.vcxproj", False, False)
        if res != 0:
            print "Failure to build release x86, open Visual Studio to see the issue"
            return

        print "\tSuccessfully built lightpack filter"
    else:
        print "\tFailed to find Visual Studio path, cannot build"

if __name__ == "__main__":
    main()
