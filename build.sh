#!/bin/sh

case "x$1" in
xinit)
  set -x
  aclocal -I /usr/share/aclocal
  libtoolize --force --copy --automake
  autoconf --force
  automake --foreign --copy --add-missing -Woverride
  ;;
xclean)
echo 'cleaning...'
  make distclean >/dev/null 2>&1
  rm -rf autom4te.cache
  for fn in aclocal.m4 configure config.guess config.sub depcomp install-sh COPYING INSTALL \
    ltmain.sh libtool missing mkinstalldirs config.log config.status Makefile; do
    rm -f $fn
  done

  find . -name Makefile.in -exec rm -f {} \;
  find . -name .deps -exec rm -rf {} \;
  echo 'done'
  ;;
*)
  set -x
  aclocal -I /usr/share/aclocal
  libtoolize --force --copy --automake
  autoconf --force
  automake --foreign --copy --add-missing -Woverride
  ./configure
  ;;
esac
