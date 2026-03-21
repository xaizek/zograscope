#!/bin/bash

set -xe

# make sure bison installed by brew is used
export PATH="/usr/local/opt/bison/bin:$PATH"

brew tap srcml/srcml
brew install bison boost ccache srcml

echo 'HAVE_LIBSRCML := yes' > config.mk

make -j4
make -j4 check
