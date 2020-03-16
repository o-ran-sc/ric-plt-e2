mkdir -p tmpFiles
cat extFileList.txt | xargs mv -t tmpFiles/.
cd ../oranE2
find . -type f -name \*.c -exec ../int/autogen/autogen -i --no-top-level-comment -l codev {} \;
find . -type f -name \*.h -exec ../int/autogen/autogen -i --no-top-level-comment -l codev {} \;
mv ../asnTextFiles/tmpFiles/* .
rmdir ../asnTextFiles/tmpFiles
