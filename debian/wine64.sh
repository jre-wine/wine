#!/bin/sh

XMESSAGE=/usr/bin/xmessage
ARCH="`dpkg --print-architecture`"
ARCH_I386="`echo $ARCH | sed s,amd64,i386,`"

TITLE="Debian / Wine: Multiarch Instructions"

I386_MSG=\
"This is the wine64-bin helper package, which does not provide wine itself,
but instead exists solely to provide the following information about
enabling multiarch on your system in order to be able to install and run
the 32-bit wine packages.

The following commands should be issued as root or via sudo in order to
enable multiarch (the last command installs 32-bit wine):

  # dpkg --add-architecture ${ARCH_I386}
  # apt-get update
  # apt-get install wine-bin:${ARCH_I386}

Be very careful as spaces matter above.  Note that this package
(wine64-bin) will be removed in the process.  For more information on
the multiarch conversion, see: http://wiki.debian.org/Multiarch/HOWTO"

$XMESSAGE -center \
    -buttons ok:0 -default ok \
    -title "$TITLE" \
    "$I386_MSG" 2>/dev/null
notify=$?
if [ $notify -eq 1 ] ; then
    # xmessage was unable to notify the user, try tty instead
    echo "** $TITLE **"
    echo "" >&2
    echo "$I386_MSG" >&2
    echo "" >&2
    echo -n "(okay) " >&2
    read confirm
fi

exit 1
