#! /bin/sh
echo ""
echo "==========================================================================="
echo "Script that generates the structure needed to create the NetBee Source Pack"
echo "All the files will be placed in the "release\\sources" folder"
echo "The source pack will be created in "release\\nbsources.zip" "
echo ""
echo "Script launch order:"
echo "  1. createbin.sh"
echo "  2. createdoc.sh"
echo "  3. createdevpack.sh"
echo "  4. createsrc.sh"
echo "==========================================================================="
echo ""
echo ""
echo "=================================="
echo "Making a clean copy of the sources"
echo "=================================="
echo ""

rm -rfd release/sources
mkdir release/sources

echo ""
echo "================================"
echo "Copying source files and samples"
echo "================================"
echo ""

# Copy source files
cp -rf bin release/sources/.
cp -rf contrib release/sources/.
cp -rf samples release/sources/.
cp -rf src release/sources/.
cp -rf tools release/sources/.
cp -rf include release/sources/.
mkdir release/sources/lib

cp *.sh release/sources/.
cp license.txt release/sources/.


echo ""
echo "======================================================"
echo "Deleting files which are not required for a clean copy"
echo "======================================================"
echo ""

# Please note that this portion of code does not work on my CygWin, but works on BSD

cd ./release/sources
find . -name .svn | xargs rm -frd
find . -name *.bak | xargs rm -frd
find . -name "*.BAK" | xargs rm -frd
find . -name *.pdb | xargs rm -frd
find . -name *.ncb | xargs rm -frd
find . -name *.suo | xargs rm -frd

find . -name *.user | xargs rm -frd
find . -name *.cmake | xargs rm -frd
find . -name CMakeCache.txt | xargs rm -frd
find . -name ALL_BUILD.* | xargs rm -frd
find . -name ZERO_CHECK.* | xargs rm -frd

find . -name *.dir | xargs rm -frd
find . -name debug | xargs rm -frd
find . -name release | xargs rm -frd
find . -name CMakeFiles | xargs rm -frd

find src/. -name *.sln | xargs rm -frd
find samples/. -name *.sln | xargs rm -frd
find tools/. -name *.sln | xargs rm -frd

# Remove binaries from 'bin' folder
rm bin/*.dll

cd ../..


# Create a ZIP file for the Source pack
cd ./release/sources
zip -r ../nbsources.zip *
cd ../..

