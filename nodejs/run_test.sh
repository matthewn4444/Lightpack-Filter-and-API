#!/bin/sh

chmod +x test.js
echo "Start====================="
echo
./test.js
echo
echo "End======================="
read -rsp $'Press any key to continue...\n' -n1 key