#!/usr/bin/env python

import subprocess

def get_nw_version():
    with open("../../config.txt", "r") as f:
        for line in f:
            if line.startswith("nw="):
                return line.split("=")[1].strip()

def main():
    content = None
    version = get_nw_version()
    if not version:
        raise Exception("Failed to find nw.js version")

    # Run bash command
    process = subprocess.call(("nw-gyp configure --target=" + version).split(), shell=True)

    # Modify app-mutex.vcproj
    with open("build/app-mutex.vcproj", "r") as f:
        content = f.read()
        content = content.replace("x64", "Win32")
        content = content.replace('RuntimeLibrary="0" RuntimeTypeInfo="false"', 'RuntimeLibrary="4" RuntimeTypeInfo="false"')
        content = content.replace('RuntimeLibrary="1" StringPooling="true"', 'RuntimeLibrary="3" StringPooling="true"')
        content = content.replace("TargetMachine=\"17\"", "TargetMachine=\"18\"")
        content = content.replace("$(Configuration)\\nw.lib", "ia32\\nw.lib")
        content = content.replace("$(Configuration)\\node.lib", "ia32\\node.lib")
        content = content.replace("nw.lib", "nw.lib ..\\..\\..\\lib\\lightpack.lib")
        content = content.replace("$(SolutionDir)", "$(SolutionDir)/..\\..\\..\\")
        content = content.replace("\\include;", "\include;..\\..\\..\\include;")
    with open("build/app-mutex.vcproj", "w") as w:
        w.write(content)

    # Modify binding.sln
    with open("build/binding.sln", "r") as f:
        content = f.read()
        content = content.replace("x64", "Win32")
    with open("build/binding.sln", "w") as w:
        w.write(content)

    # Modify config.gypi
    with open("build/config.gypi", "r") as f:
        content = f.read()
        content = content.replace("x64", "Win32")
    with open("build/config.gypi", "w") as w:
        w.write(content)

if __name__ == "__main__":
    main()
