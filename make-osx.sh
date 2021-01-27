mkdir bin > /dev/null 2>&1
cd bin > /dev/null 2>&1
rm -rf osx > /dev/null 2>&1
mkdir osx && cd osx > /dev/null 2>&1
cmake -GXcode ../..
cd ../..  > /dev/null 2>&1

if [ ! -d bin/osx/*.xcodeproj ]; then
    echo "Make OSX Project Failed.";
fi
