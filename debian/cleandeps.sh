#!/bin/bash
echo Cleaning extra dependencies...
for dep in debian/*.deps; do
  package="$(basename "$dep" .deps)"
  path="debian/$package"
#  echo $path
  rm -f "$path/extradep32" "$path/extradep64"
done
# return success
true
