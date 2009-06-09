#!/bin/bash
SUFFIX="$1"
for inst in debian/*.install-common; do
  package="$(basename "$inst" .install-common)"
  rm -f debian/$package$SUFFIX.install
done
# return success
true
