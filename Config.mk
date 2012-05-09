# Env
SUPPORT_ICS := false
ARCH_ARM_HAVE_NEON := true


# TI DVSDK
OMX_XDCPATH := \
	$(XDC_INSTALL_DIR)/packages 	\
	$(LINK_INSTALL_DIR)		\
	$(FC_INSTALL_DIR)/packages	\
	$(CE_INSTALL_DIR)/packages	\
	$(XDAIS_INSTALL_DIR)/packages	\
	$(CODEC_INSTALL_DIR)/packages	\
	$(CMEM_INSTALL_DIR)/packages	\
	$(DMAI_INSTALL_DIR)/packages	\
	$(LPM_INSTALL_DIR)/packages	\
	$(XDC_USER_PATH)		\
	$(EDMA3_LLD_INSTALL_DIR)/packages \
	$(C6ACCEL_INSTALL_DIR)/soc/packages

