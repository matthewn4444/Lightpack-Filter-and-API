#!/usr/bin/env python

import subprocess

def main():
    content = None

    # Run bash command
    process = subprocess.call("nw-gyp configure --target=0.13.0".split(), shell=True)

    # Modify lightpack.vcproj
    with open("build/lightpack.vcproj", "r") as f:
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
    with open("build/lightpack.vcproj", "w") as w:
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
        
        # building automatically..../C/Windows/Microsoft.NET/Framework/v4.0.30319/MSBuild.exe build/lightpack.vcxproj -p:Configurat ion=Release

if __name__ == "__main__":
    main()
