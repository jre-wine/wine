#!/bin/bash
for inst in debian/*.install-common; do
  package="$(basename "$inst" .install-common)"
  cp debian/$package.install-common debian/$package.install
  # certain binaries are only compiled on some platforms;
  # if they were compiled on the current one, install them
  for bin in $(cat debian/$package.install-platform); do
    [ ! -f $bin ] || echo $bin >> debian/$package.install
  done
done
# return success
true
