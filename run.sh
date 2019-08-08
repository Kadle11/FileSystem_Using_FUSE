#!/bin/sh

# Example Shell Script to Create Directories, Make a Directory called 'The_File_System' in the Parent Directory
# ./TFS -f ../The_File_System
# ./run.sh

cd ../The_File_System
mkdir Dir1
cd Dir1
mkdir Nest1
cd ..
mkdir Dir2
cd Dir2
mkdir Nest1
cd ..
echo This is a FileSystem Built using FUSE > HelloFS.txt
