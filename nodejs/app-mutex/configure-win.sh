#!/bin/sh
# Use Git/Cygwin bash on Windows

nw-gyp configure --target=0.9.2 --target_arch=ia32

# Move the files to the vs13 folder
mkdir vs13
mv build/config.gypi vs13/config.gypi
mv build/app-mutex.vcxproj vs13/app-mutex.vcxproj
mv build/app-mutex.vcxproj.filters vs13/app-mutex.vcxproj.filters
mv build/binding.sln vs13/app-mutex.sln

# Remove the db file
rm vs13/app-mutex.sdf

# Insert the Extra Lightpack properties into the project
file='vs13/app-mutex.vcxproj'

#sed -i '29a\
#  <ImportGroup Label="PropertySheets" Condition="'"'"'$(Configuration)|$(Platform)'"'"'=='"'"'Release|Win32'"'"'">\n    <Import Project="Additional_libs.props" />\n  </ImportGroup>' $file

#sed -i 's/MultiThreaded</MultiThreadedDLL</g' $file
sed -i 's/$(Configuration)\\nw.lib</ia32\\nw.lib</g' $file
sed -i 's/$(SolutionDir)/..\\..\\..\\/g' $file

rm -r build
