rem Compiles and places the nw file into the release and debug folders
del ..\..\Release\app.nw
copy ..\..\Release\lightpack.node lightpack\lightpack.node
zip -r ..\..\Release\app.nw package.json node_modules\* src\*
copy "..\..\Release\app.nw" "..\..\Debug\app.nw"
