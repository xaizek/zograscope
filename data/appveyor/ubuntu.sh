#!/bin/bash

set -xe

if [ -n "$SRCML" ]; then
    if [ "$SRCML" != v1.0.0 ]; then
        arch=_amd64
    fi

    wget https://github.com/srcML/srcML/releases/download/$SRCML/srcml_${SRCML:1}-1_ubuntu20.04$arch.deb
    wget https://github.com/srcML/srcML/releases/download/$SRCML/srcml-dev_${SRCML:1}-1_ubuntu20.04$arch.deb

    sudo apt install -y ./srcml_${SRCML:1}-1_ubuntu20.04$arch.deb
    sudo apt install -y ./srcml-dev_${SRCML:1}-1_ubuntu20.04$arch.deb

    srcml --version

    echo 'HAVE_LIBSRCML := yes' > config.mk
fi

sudo apt install -y libboost-filesystem-dev \
                    libboost-iostreams-dev \
                    libboost-program-options-dev \
                    libboost-system-dev \
                    bison flex \
                    ccache

make -j4
make -j4 check
