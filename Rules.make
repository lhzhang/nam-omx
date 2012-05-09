# Define target platform.
PLATFORM=dm3730

# The installation directory of the SDK.
DVSDK_INSTALL_DIR=/home/michael/lp/bbxm/rowboat/gb/gb-dsp/android/external/ti-dsp/ti-dvsdk_dm3730-evm_4_01_00_09

# For backwards compatibility
DVEVM_INSTALL_DIR=$(DVSDK_INSTALL_DIR)

# Where DSP/BIOS is installed.
BIOS_INSTALL_DIR=$(DVSDK_INSTALL_DIR)/dspbios_5_41_03_17

# Where the DSPBIOS Utils package is installed.
BIOSUTILS_INSTALL_DIR=$(DVSDK_INSTALL_DIR)/biosutils_1_02_02

# Where the Codec Engine package is installed.
CE_INSTALL_DIR=$(DVSDK_INSTALL_DIR)/codec-engine_2_26_01_09

# Where the DSP Link package is installed.
LINK_INSTALL_DIR=$(DVSDK_INSTALL_DIR)/dsplink_1_65_00_02

# Where the codecs are installed.
CODEC_INSTALL_DIR=$(DVSDK_INSTALL_DIR)/codecs-omap3530_4_01_00_00

# Where DMAI package is installed.
DMAI_INSTALL_DIR=$(DVSDK_INSTALL_DIR)/dmai_2_20_00_14

# Where the SDK demos are installed
DEMO_INSTALL_DIR=$(DVSDK_INSTALL_DIR)/dvsdk-demos_4_00_00_21

# Where the DVTB package is installed.
DVTB_INSTALL_DIR=$(DVSDK_INSTALL_DIR)/dvtb_4_20_15

# Where the EDMA3 LLD package is installed.
EDMA3_LLD_INSTALL_DIR=$(DVSDK_INSTALL_DIR)/edma3lld_01_11_00_03
EDMA3LLD_INSTALL_DIR=$(EDMA3_LLD_INSTALL_DIR)

# Where the Framework Components package is installed.
FC_INSTALL_DIR=$(DVSDK_INSTALL_DIR)/framework-components_2_25_03_07

# Where the MFC Linux Utils package is installed.
LINUXUTILS_INSTALL_DIR=$(DVSDK_INSTALL_DIR)/linuxutils_2_25_05_11
CMEM_INSTALL_DIR=$(LINUXUTILS_INSTALL_DIR)

# Where the XDAIS package is installed.
XDAIS_INSTALL_DIR=$(DVSDK_INSTALL_DIR)/xdais_6_26_00_02

# Where the RTSC tools package is installed.
XDC_INSTALL_DIR=$(DVSDK_INSTALL_DIR)/xdctools_3_16_03_36

# Where the Code Gen is installed.
CODEGEN_INSTALL_DIR=$(DVSDK_INSTALL_DIR)/cgt6x_6_1_14

# Where the local power manager is installed.
LPM_INSTALL_DIR=$(DVSDK_INSTALL_DIR)/local-power-manager_1_24_02_09

# Where c6accel package is installed.
C6ACCEL_INSTALL_DIR=$(DVSDK_INSTALL_DIR)/c6accel_1_01_00_02

# Where c6run package is installed.
C6RUN_INSTALL_DIR=$(DVSDK_INSTALL_DIR)/c6run_0_94_05_06


