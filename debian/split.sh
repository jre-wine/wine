#!/bin/bash
SRC="$1"
PREFIX="$2"
while read DEST NAME; do
  for bin in debian/$SRC/$PREFIX/$NAME; do
    if [ -f $bin ]; then
      install -d debian/$DEST/$PREFIX
      mv $bin debian/$DEST/$PREFIX
    fi
  done
done < debian/$SRC.split
# return success
true
