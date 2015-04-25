#!/bin/sh
# Use Git/Cygwin bash on Windows

nw-gyp configure --target=0.8.6

# Move the files to the vs folder
mkdir vs
mv build/config.gypi vs/config.gypi
mv build/app-mutex.vcxproj vs/app-mutex.vcxproj
mv build/app-mutex.vcxproj.filters vs/app-mutex.vcxproj.filters
mv build/binding.sln vs/app-mutex.sln

# Remove the db file
rm vs/app-mutex.sdf

# Insert the Extra Lightpack properties into the project
file='vs/app-mutex.vcxproj'

#sed -i '29a\
#  <ImportGroup Label="PropertySheets" Condition="'"'"'$(Configuration)|$(Platform)'"'"'=='"'"'Release|Win32'"'"'">\n    <Import Project="Additional_libs.props" />\n  </ImportGroup>' $file

#sed -i 's/MultiThreaded</MultiThreadedDLL</g' $file
sed -i 's/$(Configuration)\\nw.lib</ia32\\nw.lib</g' $file
sed -i 's/$(SolutionDir)/..\\..\\..\\/g' $file

rm -r build
