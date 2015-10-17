#!/bin/sh

DIR=$(cd $(dirname $0); pwd)
cd $DIR/..

aclocal && \
autoconf && \
autoheader && \
automake --add-missing && \
./configure && \
make
