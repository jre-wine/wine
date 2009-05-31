#!/bin/bash
SRC="$1"
shift
while [ -n "$1" ]; do
  PREFIX="$1"
  while read DEST NAME; do
    for bin in debian/$SRC/$PREFIX/$NAME; do
      if [ -f $bin ]; then
        install -d debian/$DEST/$PREFIX
        mv $bin debian/$DEST/$PREFIX
      fi
    done
  done < debian/$SRC.split
  shift
done
# return success
true
