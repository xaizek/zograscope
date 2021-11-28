#!/bin/bash

set -xe

if [ "$SRCML" = v0.9.5 ]; then
    wget http://131.123.42.38/lmcrs/beta/srcML-Ubuntu18.04.deb
    sudo apt install -y ./srcML-Ubuntu18.04.deb libarchive13

    srcml --version

    echo "TESTS := '~[srcml095-broken]'" > config.mk
    echo "CFLAGS += -fPIC" >> config.mk
elif [ "$SRCML" = v1.0 ]; then
    wget http://131.123.42.38/lmcrs/v1.0.0/srcml_1.0.0-1_ubuntu18.04.deb
    sudo apt install -y ./srcml_1.0.0-1_ubuntu18.04.deb

    srcml --version

    echo "TESTS := '*'" > config.mk
fi

sudo apt install -y libboost-filesystem-dev \
                    libboost-iostreams-dev \
                    libboost-program-options-dev \
                    libboost-system-dev \
                    bison flex \
                    ccache

make -j4
make -j4 check
