#!/usr/bin/env python
import sys
sys.path.append("../../directshow/directshow-sdk/")
import subprocess, configure_win, vs

def main():
    content = None

    print 'Going to build app-mutex node module'
    path = vs.get_devenv_path()
    configure_win.main()

    # Upgrade visual studio project
    print "\tUpgrading visual studio project"
    res = vs.upgrade(path, "build/app-mutex.vcproj")
    if res != 0:
        raise Exception('Failed to upgrade visual studio project')

    # Build visual studio project
    print "\tBuilding visual studio project"
    res = vs.build(path, "build/app-mutex.vcxproj", True, True)
    if res != 0:
        raise Exception('Failed to build visual studio project')
    print "\tBuilding successful"

if __name__ == "__main__":
    main()