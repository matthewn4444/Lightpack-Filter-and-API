rem Compiles and places the nw file into the release and debug folders
del ..\..\Release\app.nw
zip ..\..\Release\app.nw index.html package.json *.js
copy "..\..\Release\app.nw" "..\..\Debug\app.nw"
