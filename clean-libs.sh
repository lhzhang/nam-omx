#!/bin/bash

OUT=../../../out/target/product/beagleboard
# clean libs
pushd `dirname $0`
pushd $OUT
find -name "libNAM_OMX*" -o -name "libOMX.NAM*" -o -name "libtidmaiiface*" -o -name "libnam*" | xargs rm -rf
popd;popd



