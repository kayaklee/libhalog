#!/bin/sh

case "x$1" in
xinit)
  set -x
  aclocal
	automake --add-missing
	libtoolize --force --automake
	automake --foreign
  autoconf
  ;;
xclean)
echo 'cleaning...'
  make distclean >/dev/null 2>&1
  rm -rf autom4te.cache
  for fn in *.m4 configure config.guess config.sub depcomp install-sh COPYING INSTALL \
    ltmain.sh libtool missing mkinstalldirs config.log config.status Makefile \
		NEWS README AUTHORS ChangeLog; do
    rm -f $fn
  done

  find . -name Makefile.in -exec rm -f {} \;
  find . -name .deps -exec rm -rf {} \;
  echo 'done'
  ;;
*)
  set -x
  aclocal
	automake --add-missing
	libtoolize --force --automake
	automake --foreign
  autoconf
  ./configure
  ;;
esac
