#!/bin/sh

arch=$(dpkg --print-architecture | sed s/amd64/i386/)

echo "This is the wine64-bin helper package, which does not provide wine itself,"
echo "but instead exists solely to provide the following information about"
echo "enabling multiarch on your system in order to be able to install and run"
echo "the 32-bit wine packages."
echo ""
echo "The following commands should be issued as root or via sudo in order to"
echo "enable multiarch (the last command installs 32-bit wine):"
echo ""
echo "  # dpkg --add-architecture $arch"
echo "  # apt-get update"
echo "  # apt-get install wine-bin:$arch"
echo ""
echo "Depending on which Debian release is present on this system, the development"
echo "version of wine may be available, which if available can be installed with:"
echo ""
echo "  # apt-get install wine-bin-unstable:$arch"
echo ""
echo "Note that this package (wine64-bin) will be removed in the process.  For"
echo "more information on the multiarch conversion, see:"
echo "http://wiki.debian.org/Multiarch/HOWTO"
