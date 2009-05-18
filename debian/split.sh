#!/bin/bash
SRC="$1"
PREFIX="$2"
while read DEST NAME; do
  for bin in debian/$SRC/$PREFIX/$NAME; do
    [ ! -f $bin ] || mv $bin debian/$DEST/$PREFIX
  done
done < debian/$SRC.split
# return success
true
