#!/bin/bash
for inst in debian/*.install-common; do
  package="$(basename "$inst" .install-common)"
  rm -f debian/$package.install
done
# return success
true
