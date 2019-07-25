#!/bin/sh

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
