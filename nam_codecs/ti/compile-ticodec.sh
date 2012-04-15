#!/bin/bash

cd /home/michael/lp/bbxm/rowboat/gb/gb-dsp/android
source ../androidrc
source build/envsetup.sh 
echo "ticodec cleanup start ..."
make -C device/ti/nam_omx/nam_codecs/ti combo_target=TARGET_ TARGET_TOOLS_PREFIX=prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi- HOST_PREBUILT_TAG=linux-x86 clean
echo "ticodec cleanup end ..."
echo ""
echo ""
echo "==========================================================="
echo "==========================================================="
echo ""
echo ""
echo "ticodec compile start ..."
echo ""
echo ""
make -C device/ti/nam_omx/nam_codecs/ti combo_target=TARGET_ TARGET_TOOLS_PREFIX=prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi- HOST_PREBUILT_TAG=linux-x86

echo ""
echo ""
echo "ticodec compile end ..."
echo ""
echo ""

