#! /bin/sh
echo ""
echo "================================================================================"
echo "Script that generates the structure needed to create the NetBee Developer's Pack"
echo "All the files will be placed in the "release\\devpack" folder"
echo "The developer's pack will be created in "release\\nbdevpack.zip" "
echo ""
echo "Script launch order:"
echo "  1. createbin.sh"
echo "  2. createdoc.sh"
echo "  3. createdevpack.sh"
echo "  4. createsrc.sh"
echo "================================================================================"
echo ""
echo ""
echo "==================================================="
echo "Making a clean copy of the DevPack for installation"
echo "==================================================="
echo ""


# Remove the folder that will contain the devpack
rm -rfd release/devpack
# Create the folder in case it does not exist
mkdir -p release/devpack


# Copying binaries
mkdir -p release/devpack/bin
cp -rf release/setup/* release/devpack/bin 2> /dev/null

# Copying libraries
mkdir -p release/devpack/lib
cp -rf lib/*.lib release/devpack/lib 2> /dev/null
cp -rf lib/*.exp release/devpack/lib 2> /dev/null

# Copying include files
mkdir -p release/devpack/include
cp -rf include/* release/devpack/include 2> /dev/null


# Copying samples (binaries)
cp -rf bin/*.exe release/devpack/bin 2> /dev/null

# Copying samples (source files)
mkdir -p release/devpack/samples
cp -rf samples/* release/devpack/samples 2> /dev/null


# Copying all files related to the "tools" section (including source files)
# Please note that these files have a different license compared to the main NetBee
mkdir -p ./release/devpack/tools
cp -rf ./tools release/devpack 2> /dev/null



# Copy Helps and License
#mkdir ./release/devpack/help
#cp -f ./release/nbee.chm release/devpack/help/.
cp license.txt release/devpack/.
cp readme-devpack.txt release/devpack/readme.txt


echo ""
echo "==========================================================="
echo "Deleting files that are not needed for a clean installation"
echo "==========================================================="
echo ""


cd ./release/devpack
find . -name .svn | xargs rm -frd
find . -name *.bak | xargs rm -frd
find . -name "*.BAK" | xargs rm -frd
# find . -name *.txt | xargs rm -frd
find . -name *.pdb | xargs rm -frd
find . -name *.ncb | xargs rm -frd
find . -name *.suo | xargs rm -frd

find . -name *.sln | xargs rm -frd
find . -name *.vcproj | xargs rm -frd

# Remove the .devpack extension to files
find . -name *.devpack | sed "s/\(.*\)\.devpack$/mv '&' '\1'/" | sh

find . -name *.user | xargs rm -frd
find . -name *.cmake | xargs rm -frd
find . -name CMakeCache.txt | xargs rm -frd
find . -name ALL_BUILD.* | xargs rm -frd
find . -name ZERO_CHECK.* | xargs rm -frd

find . -name *.dir | xargs rm -frd
find . -name debug | xargs rm -frd
find . -name release | xargs rm -frd
find . -name CMakeFiles | xargs rm -frd

cd ../..

# Create a ZIP file for the Developer's pack
cd ./release/devpack
zip -r ../nbdevpack.zip *
cd ../..

