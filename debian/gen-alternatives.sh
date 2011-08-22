#!/bin/bash
VSUFFIX="$1"
ARCH="$2"
PRIORITY="$3"
SUFFIX="$ARCH$VSUFFIX"

function generate_postinst
{
  echo "update-alternatives \\"
  echo -n "  --install /usr/bin/wine wine /usr/bin/wine$SUFFIX $PRIORITY"
  for bin in $(cat debian/wine-bin.install-alternatives); do
    dname=$(echo $bin|sed -n 's,debian/tmp/\(.*\),/\1,p')
    sname=$(echo $dname|sed -n "s,\(.*/[^./]*\)\(\.[0-9]+\)\?,\1$SUFFIX\2,p")
    mv debian/tmp/$dname debian/tmp/$sname

    case "$dname" in
      /usr/share/man/*)
        name=$(echo $dname|sed -n "s,/usr/share/man/\([^/]*\)/man[0-9]/\(.*\),\2.\1.gz,p;t;s,/usr/share/man/man[0-9]/\(.*\),\1.gz,p")
        dname="$dname.gz"
        sname="$sname.gz"
        ;;
      *)
        name=$(basename $dname)
        ;;
    esac
    if [ "$name" != "wine" ]; then
      echo " \\"
      echo -n "  --slave $dname $name $sname"
    fi
  done
  echo
  echo
}

function generate_prerm
{
  echo 'if [ "$1" != "upgrade" ]; then'
  echo "  update-alternatives --remove wine /usr/bin/wine$SUFFIX"
  echo 'fi'
  echo
}

generate_postinst >> debian/wine-bin$VSUFFIX.postinst.debhelper
generate_prerm >> debian/wine-bin$VSUFFIX.prerm.debhelper

# return success
true
