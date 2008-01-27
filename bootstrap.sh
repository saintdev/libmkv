#! /bin/sh

rm -rf config
mkdir -p config

#aclocal \
#&& libtoolize --force --copy \
#&& aclocal \
#&& autoheader \
#&& automake --foreign  --add-missing --copy \
#&& autoconf

autoreconf --force --install
