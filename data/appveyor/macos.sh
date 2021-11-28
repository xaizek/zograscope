#!/bin/bash

set -xe

# make sure bison installed by brew is used
export PATH="/usr/local/opt/bison/bin:$PATH"

if [ "$SRCML" = v1.0 ]; then
    echo "TESTS := '*'" > config.mk
fi

brew install bison boost ccache

make -j4
make -j4 check
