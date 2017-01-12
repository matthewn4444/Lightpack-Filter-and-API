#!/usr/bin/env python
import subprocess, os, sys, urllib2, zipfile, shutil
here = os.path.dirname(os.path.realpath(__file__))

def get_nw_version():
    with open("config.txt", "r") as f:
        for line in f:
            if line.startswith("nw="):
                return line.split("=")[1].strip()

def chunk_report(bytes_so_far, chunk_size, total_size):
    percent = float(bytes_so_far) / total_size
    percent = round(percent*100, 2)
    sys.stdout.write("Downloaded %d of %d bytes (%0.2f%%)\r" %
        (bytes_so_far, total_size, percent))
    if bytes_so_far >= total_size:
        sys.stdout.write('\n')

def download_file(url, save_path, chunk_size=8192, report_hook=chunk_report):
    response = urllib2.urlopen(url);
    with open(save_path, "wb") as f:
        total_size = response.info().getheader('Content-Length').strip()
        total_size = int(total_size)
        bytes_so_far = 0
        while 1:
            chunk = response.read(chunk_size)
            bytes_so_far += len(chunk)
            if not chunk:
                break
            f.write(chunk)
            if report_hook:
                report_hook(bytes_so_far, chunk_size, total_size)
        return bytes_so_far

def build(path, arg1=None):
    os.chdir(path)
    if arg1:
        os.system("python build.py " + arg1)
    else:
        os.system("python build.py")
    os.chdir(here)

def main():
    print "Compiling Directshow sdk"
    build("directshow/directshow-sdk", "-static")

    print "Compile the lightpack libraries project"
    build("LightpackAPI")

    print "Compile Directshow filter"
    build("directshow/Lightpack-filter")

    print "Compile Lightpack for node"
    build("nodejs", "lightpack")

    print "Compile App-mutex for node"
    build("nodejs", "app-mutex")

    # Download node modules and extract it
    print "Download nodewebkit binaries"
    if os.path.exists("nodejs/nw-binaries/"):
        shutil.rmtree("nodejs/nw-binaries/")
    version = get_nw_version()
    if not version:
        raise Exception("Failed to find nw.js version")
    file = "nwjs-v" + version + "-win-ia32.zip"
    save_path = "nodejs/" + file
    try:
        download_file("http://dl.nwjs.io/v" + version + "/" + file, save_path)
    except RuntimeError:
        print "\nError downloading file"
        os.remove(save_path)
        return
    except KeyboardInterrupt:
        print "\nCancelled downloading file"
        os.remove(save_path)
        return
    print "Finished downloading, now extracting..."
    with zipfile.ZipFile(save_path, 'r') as z:
        z.extractall("nodejs/")
    os.remove(save_path)
    os.rename("nodejs/nwjs-v" + version + "-win-ia32", "nodejs/nw-binaries/")
    print "Finished extracting node binaries"

    # Finish off building setup files and zip files
    print "Compile project to setup file and zip"
    os.chdir("directshow/Lightpack-filter-gui")
    os.system("build_all.bat")

    print "Build is fully completed"

if __name__ == "__main__":
    main()