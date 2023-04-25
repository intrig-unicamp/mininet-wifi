#! /bin/sh
echo ""
echo "============================================================================"
echo "Script that generates the structure needed to create the NetBee installation"
echo "All the files will be placed in the "release\\setup" folder"
echo "The setup pack will be created in "release\\nbsetup.zip" "
echo ""
echo "Script launch order:"
echo "  1. createbin.sh"
echo "  2. createdoc.sh"
echo "  3. createdevpack.sh"
echo "  4. createsrc.sh"
echo "============================================================================"
echo ""
echo ""
echo "===================================================="
echo "Making a clean copy of the binaries for installation"
echo "===================================================="
echo ""

rm -rfd release

mkdir release
mkdir release/setup

# The binary release needs almost only libraries (note: 2> redirects stderr)
cp -rf ./bin/*.dll ./release/setup 2> /dev/null
cp -rf ./bin/*.la  ./release/setup 2> /dev/null
cp -rf ./bin/*.so  ./release/setup 2> /dev/null
cp -rf ./bin/*.xml ./release/setup 2> /dev/null
cp -rf ./bin/*.xsd ./release/setup 2> /dev/null


# The debug version of Xerces is not required
rm -f ./release/setup/xerces*D.dll

# Remove temporary files (these are generated in case you run some examples)
rm -f ./release/setup/P*.xml

cd release/setup
zip ../nbsetup.zip *
cd ../..
