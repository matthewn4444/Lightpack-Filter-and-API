#!/usr/bin/env python

import subprocess, configure_win, os

DEVENV_LOC = "Microsoft Visual Studio 14.0\\Common7\\IDE\\devenv.exe"

def main():
    configure_win.main()

    print 'Going to build Lightpack node module'

    # Check the progrom files location
    program_files = None
    if os.path.exists("c:\\Program Files (x86)\\"):
        program_files = "c:\\Program Files (x86)\\"
    elif os.path.exists("c:\\Program Files\\"):
        program_files = "c:\\Program Files\\"
    else:
        raise Exception('Cannot find program files, are you running this on windows?')

    # Upgrade visual studio project
    print "Upgrading visual studio project"
    res = subprocess.call("\"" + program_files + DEVENV_LOC + "\" -upgrade build/lightpack.vcproj", shell=True, stdout=subprocess.PIPE)
    if res != 0:
        raise Exception('Failed to upgrade visual studio project')

    # Build visual studio project
    print "Building visual studio project"
    res = subprocess.call("\"" + program_files + DEVENV_LOC + "\" build/lightpack.vcxproj -build Release", shell=True, stdout=subprocess.PIPE)
    if res != 0:
        raise Exception('Failed to build visual studio project')
    print "Building successful"

if __name__ == "__main__":
    main()
