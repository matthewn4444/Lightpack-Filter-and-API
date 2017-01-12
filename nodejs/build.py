#!/usr/bin/env python
import sys, glob, os.path, shutil, os
import subprocess

def get_nw_version():
    with open("../../config.txt", "r") as f:
        for line in f:
            if line.startswith("nw="):
                return line.split("=")[1].strip()

def main():
    # Get the path from command line
    path = os.getcwd()
    if len(sys.argv) > 1:
        path = path + "/" + sys.argv[1]
    os.chdir(path)

    # Check if running this script correctly
    if not os.path.isfile("binding.gyp"):
        print "Please run this script from or point to binding.gyp to build the node module"
        print "     Try:   build.py ./path/to/binding"
        return

    version = get_nw_version()
    if not version:
        raise Exception("Failed to find nw.js version")

    print 'Going to build node module'

    # Run bash command
    print '\tConfigure the nw module...'
    process = subprocess.call(("nw-gyp configure  --arch=ia32 --target=" + version).split(), shell=True)

    # Build visual studio project
    print "\tBuilding module...."
    process = subprocess.call(("nw-gyp build  -j 8 --release --arch=ia32 --target=" + version).split(), shell=True)
    print "\tBuilding successful"

    # Copy node file back
    files = glob.glob("build/Release/*.node")
    if len(files) == 0:
        raise Exception("Building failed")
    for file in files:
        if os.path.isfile(file):
            shutil.copy2(file, "../../Release")

if __name__ == "__main__":
    main()
