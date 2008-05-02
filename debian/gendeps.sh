#!/bin/bash
ARCHS="$@"
echo Forcing extra dependencies...
for dep in debian/*.deps; do
  package="$(basename "$dep" .deps)"
  path="debian/$package"
  deplist=$(sed 's,^,-l,' $dep)
#  echo $path: $deplist
  for arch in $ARCHS; do
    gcc -m$arch -Wl,--noinhibit-exec -o "$path/extradep$arch" debian/extradep.c $deplist
  done
done
# return success
true
