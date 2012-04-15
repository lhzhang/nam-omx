# Copyright (C) 2010 Texas Instruments ITC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

OMAPES=5.x

ANDROID_ROOT_DIR := /home/michael/lp/bbxm/rowboat/gb/gb-dsp/android

ifeq ($(OMAPES), 5.x)
DVSDK_INSTALL_DIR := $(ANDROID_ROOT_DIR)/external/ti-dsp/ti-dvsdk_dm3730-evm_4_01_00_09
else
DVSDK_INSTALL_DIR := $(ANDROID_ROOT_DIR)/external/ti-dsp/ti-dvsdk_omap3530-evm_4_01_00_09
endif
DVSDK_TARGET_DIR:=$(ANDROID_ROOT_DIR)/out/target/product/$(TARGET_PRODUCT)/system/ti-dsp

include $(DVSDK_INSTALL_DIR)/Rules.make

#-------------- Bionic tools -----------------

ANDROID_TOOLCHAIN := $(ANDROID_ROOT_DIR)/$($(combo_target)TOOLS_PREFIX)
ANDROID_TOOLCHAIN_PREFIX := $(shell echo $(ANDROID_TOOLCHAIN) | sed -n 's/.*\///;p')
ANDROID_TOOLCHAIN_PATH := $(shell echo $(ANDROID_TOOLCHAIN) | sed 's/[a-zA-Z\-]*$$//' | sed 's/[\/]*$$//' | sed 's/[a-zA-Z\-]*$$//')
ANDROID_TOOLCHAIN_LONGNAME := bin/$(ANDROID_TOOLCHAIN_PREFIX)gcc

#$(warning  ANDROID_TOOLCHAIN: $(ANDROID_TOOLCHAIN))
#$(warning  ANDROID_TOOLCHAIN_PREFIX: $(ANDROID_TOOLCHAIN_PREFIX))
#$(warning  ANDROID_TOOLCHAIN_PATH: $(ANDROID_TOOLCHAIN_PATH))
#$(warning  ANDROID_TOOLCHAIN_LONGNAME:$(ANDROID_TOOLCHAIN_LONGNAME))

BIONIC_PATH := $(ANDROID_ROOT_DIR)/bionic
BIONIC_LIBC_INCS := \
	-I$(BIONIC_PATH)/libc/arch-arm/include	  \
	-I$(BIONIC_PATH)/libc/include		  \
	-I$(BIONIC_PATH)/libstdc++/include	  \
	-I$(BIONIC_PATH)/libc/kernel/common	  \
	-I$(BIONIC_PATH)/libc/kernel/arch-arm	  \
	-I$(BIONIC_PATH)/libm/include		  \
	-I$(BIONIC_PATH)/libm/arch/arm/include	  \
	-I$(BIONIC_PATH)/libthread_db/include

ANDROID_PRODUCT_PATH=$(ANDROID_ROOT_DIR)/out/target/product
ANDROID_OUT_DIR=$(ANDROID_PRODUCT_PATH)/$(TARGET_PRODUCT)

ANDROID_CC_FLAGS :=			      \
	-DHAVE_ARM_TLS_REGISTER		      \
	-DANDROID			      \
	-D_ANDROID_			      \
	-DSK_RELEASE			      \
	-DNDEBUG			      \
	-UDEBUG				      \
	-fpic				      \
	-ffunction-sections		      \
	-funwind-tables			      \
	-fstack-protector		      \
	-fno-short-enums		      \
	-finline-functions		      \
	-fno-inline-functions-called-once     \
	-fgcse-after-reload		      \
	-frerun-cse-after-loop		      \
	-frename-registers		      \
	-fomit-frame-pointer		      \
	-fno-strict-aliasing		      \
	-finline-limit=64		      \
	-fno-exceptions			      \
	-fmessage-length=0		      \
	-march=armv7-a			      \
	-mfloat-abi=softfp		      \
	-msoft-float			      \
	-mfpu=neon			      \
	-mthumb				      \
	-mthumb-interwork		      \
	-W				      \
	-Wall				      \
	-Wno-unused			      \
	-Winit-self			      \
	-Wpointer-arith			      \
	-Werror=return-type		      \
	-Werror=non-virtual-dtor	      \
	-Werror=address			      \
	-Werror=sequence-point		      \
	-Wstrict-aliasing=2		      \
	-Wno-multichar			      \
	-Wno-missing-field-initializers       \
	-O0				      \
	-g

ANDROID_CPP_FLAGS :=			      \
	$(ANDROID_CC_FLAGS)		      \
	-fvisibility-inlines-hidden	      \
	-fno-rtti			      \
	-Wsign-promo

ANDROID_LDFLAGS := \
	-fuse-ld=bfd -lc -lstdc++ -lm -nostdlib -Bdynamic -Wl,-rpath-link=$(ANDROID_OUT_DIR)/obj/lib -L$(ANDROID_OUT_DIR)/obj/lib -Wl,-T,$(ANDROID_ROOT_DIR)/build/core/armelf.x -Wl,-dynamic-linker,/system/bin/linker -Wl,--gc-sections -Wl,-z,nocopyreloc $(ANDROID_ROOT_DIR)/prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/../lib/gcc/arm-eabi/4.4.3/libgcc.a $(ANDROID_OUT_DIR)/obj/lib/crtend_android.o $(ANDROID_OUT_DIR)/obj/lib/crtbegin_dynamic.o -L$(ANDROID_ROOT_DIR)/out/target/product/$(TARGET_PRODUCT)/obj/STATIC_LIBRARIES/libasound_intermediates

ANDROID_CFLAGS=$(BIONIC_LIBC_INCS) $(ANDROID_CC_FLAGS)

export ANDROID_TOOLCHAIN_PATH ANDROID_TOOLCHAIN_PREFIX ANDROID_TOOLCHAIN_LONGNAME ANDROID_TOOLCHAIN BIONIC_LIBC_INCS ANDROID_ROOT_DIR ANDROID_CC_FLAGS ANDROID_LDFLAGS ANDROID_CFLAGS CODEC_SERVER

all:	ticodec

clean:	ticodec_clean

install: ticodec_install


#-------------- OpenMax components -----------------

OMX_INSTALL_DIR := $(ANDROID_ROOT_DIR)/hardware/ti/omx
OMX_VIDEO_INSTALL_DIR := $(OMX_INSTALL_DIR)/video

OMX_CFLAGS = -I./ \
	$(BIONIC_LIBC_INCS)	\
	-I$(ANDROID_ROOT_DIR)/frameworks/base/include \
	-I$(ANDROID_ROOT_DIR)/system/core/include \
	$(ANDROID_CPP_FLAGS) \
	-DENABLE_SHAREDFD_PLAYBACK -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DUSE_CML2_CONFIG -DHAS_OSCL_LIB_SUPPORT -MD

OMX_LDFLAGS = -nostdlib -Wl,-soname,libagl.so \
	-Wl,-T,$(ANDROID_ROOT_DIR)/build/core/armelf.xsc \
	-Wl,--gc-sections -Wl,-shared,-Bsymbolic -Wl,--no-whole-archive \
	-L$(ANDROID_ROOT_DIR)/out/target/product/$(TARGET_PRODUCT)/obj/lib \
	-lstdc++ -lc -lm -fuse-ld=bfd -lcutils

OMX_IFACE_LDFLAGS = $(OMX_LDFLAGS) -Wl,--no-undefined -llog

OMX_XDCPATH = ".;$(XDC_INSTALL_DIR)/packages;$(LINK_INSTALL_DIR);$(FC_INSTALL_DIR)/packages;$(CE_INSTALL_DIR)/packages;$(XDAIS_INSTALL_DIR)/packages;$(CODEC_INSTALL_DIR)/packages;$(CMEM_INSTALL_DIR)/packages;$(DMAI_INSTALL_DIR)/packages;$(LPM_INSTALL_DIR)/packages;$(XDC_USER_PATH);$(EDMA3_LLD_INSTALL_DIR)/packages;$(C6ACCEL_INSTALL_DIR)/soc/packages"

#-------------- TI Codec -----------------

PLATFORM = omap3530
XDC_CONFIG_BASENAME = ticodec_$(PLATFORM)
CONFIGURO = $(XDC_INSTALL_DIR)/xs xdc.tools.configuro
CXX = $(ANDROID_TOOLCHAIN)g++

openmaxticomponent_objs = iface.o

xdc_config:
	XDCPATH=$(OMX_XDCPATH) $(CONFIGURO) -c $(ANDROID_TOOLCHAIN_PATH) -o $(XDC_CONFIG_BASENAME) -t gnu.targets.arm.GCArmv5T -p ti.platforms.evm3530 -b ./config.bld ticodec_$(PLATFORM).cfg

OMX_INCLUDES = $(shell cat $(XDC_CONFIG_BASENAME)/compiler.opt) -I./


.PHONY:	ticodec

ticodec: xdc_config
	$(CXX) -DHAVE_CONFIG_H $(OMX_CFLAGS) $(OMX_INCLUDES) -c -o iface.o iface.cpp
	$(CXX) $(OMX_IFACE_LDFLAGS) $(openmaxticomponent_objs) -T $(XDC_CONFIG_BASENAME)/linker.cmd -Wl,-soname -Wl,libticodec.so -o libticodec.so

ticodec_clean:
	rm -rf *.o *.d *.so ticodec_$(PLATFORM)


ticodec_install:
	cp libticodec.so $(ANDROID_ROOT_DIR)/out/target/product/$(TARGET_PRODUCT)/system/lib
	cp libticodec.so $(ANDROID_ROOT_DIR)/out/target/product/$(TARGET_PRODUCT)/symbols/system/lib
