#!/bin/bash
LIBDIRS="$1"

function expand_common
{
  sed "s,/usr/lib,/usr/$1," debian/$package.install-common > debian/$package.install
  shift
  while [ -n "$1" ]; do
    sed -n "s,/usr/lib,/usr/$1,p" debian/$package.install-common >> debian/$package.install
    shift
  done
}

# certain binaries are only compiled on some platforms;
# if they were compiled on the current one, install them
function expand_platform
{
  if [ ! -f debian/$package.install-platform ]; then
    return
  fi
  for bin in $(sed "s,/usr/lib,/usr/$1," debian/$package.install-platform); do
    [ ! -f $bin ] || echo $bin >> debian/$package.install
  done
  shift
  while [ -n "$1" ]; do
    for bin in $(sed -n "s,/usr/lib,/usr/$1,p" debian/$package.install-platform); do
      [ ! -f $bin ] || echo $bin >> debian/$package.install
    done
    shift
  done
}

for inst in debian/*.install-common; do
  package="$(basename "$inst" .install-common)"
  expand_common $LIBDIRS
  expand_platform $LIBDIRS
done

# return success
true
