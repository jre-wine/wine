#!/bin/bash
SUFFIX="$1"

sed "s,^\(Source: \|Package: \).*$,&${SUFFIX}," debian/control.in > debian/control

# return success
true
