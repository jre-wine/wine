#!/bin/bash
SUFFIX="$1"
shift
echo Cleaning extra dependencies...
for dep in debian/*.deps; do
  package="$(basename "$dep" .deps)"
  path="debian/$package$SUFFIX"
#  echo $path
  rm -f "$path/extradep32" "$path/extradep64"
done
# return success
true
