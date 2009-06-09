#!/bin/bash
SUFFIX="$1"
shift
SRC="$1"
shift
while [ -n "$1" ]; do
  PREFIX="$1"
  while read DEST NAME; do
    for bin in debian/$SRC$SUFFIX/$PREFIX/$NAME; do
      if [ -f $bin ]; then
        install -d debian/$DEST$SUFFIX/$PREFIX
        mv $bin debian/$DEST$SUFFIX/$PREFIX
      fi
    done
  done < debian/$SRC.split
  shift
done
# return success
true
