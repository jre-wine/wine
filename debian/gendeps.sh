#!/bin/bash
echo Forcing extra dependencies...
for dep in debian/*.deps; do
  package="$(basename "$dep" .deps)"
  path="debian/$package"
  deplist=$(sed 's,^,-l,' $dep)
#  echo $path: $deplist
  gcc -o "$path/extradep" debian/extradep.c $deplist
done
