#!/bin/sh

DIR=$(cd $(dirname $0); pwd)
cd $DIR/..

mkdir -p config

aclocal && \
autoconf && \
autoheader && \
automake --add-missing && \
./configure && \
make
