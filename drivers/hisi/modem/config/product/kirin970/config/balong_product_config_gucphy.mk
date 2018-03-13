# MD5: 57521c892cc25fac02b8a7e377d80f9e
ifeq ($(chip_type),es)
CFG_ZSP_DSP_CHIP_BB_TYPE		:=13
else
CFG_ZSP_DSP_CHIP_BB_TYPE		:=14
endif
CFG_NV_VERSION :=nv_boston
CFG_ZSP_DSP_PRODUCT_FORM		:=4
CFG_BOARD                               := SFT
CFG_UPHY_BOARD_TYPE                     := 2
CFG_FEATURE_TEMP_MULTI_MODE_LP          := FEATURE_ON
CFG_FEATURE_GSM_SDR			:= FEATURE_ON
CFG_FEATURE_GSM_SDR_DAIC		:= FEATURE_OFF
CFG_FEATURE_GSM_WHOLE_SDR		:= FEATURE_OFF
CFG_FEATURE_XBBE16_NEW_MAIL		:= FEATURE_ON
CFG_FEATURE_SRAM_400K                   := FEATURE_OFF
CFG_FEATURE_EXTERNAL_32K_CLK		:= FEATURE_ON
CFG_FEATURE_SOCP_ON_DEMAND		:= FEATURE_OFF
CFG_FEATURE_TAS				:= FEATURE_ON
CFG_FEATURE_DC_DPA			:= FEATURE_ON
CFG_FEATURE_RFIC_RESET_GPIO_ON		:= FEATURE_OFF
CFG_FEATURE_VIRTUAL_BAND			    := FEATURE_ON
CFG_FEATURE_HI6363                		:= FEATURE_ON
CFG_FEATURE_RTT_TEST                            := FEATURE_ON
CFG_FEATURE_RTT_RANDOM_TEST                     := FEATURE_OFF
CFG_FEATURE_GUTLC_ONE_DSP               := FEATURE_OFF
CFG_FEATURE_NX_CORE_OPEN                := FEATURE_OFF
ifeq ($(cdma),false)
CFG_FEATURE_CSDR                        := FEATURE_OFF
else
CFG_FEATURE_CSDR                        := FEATURE_ON
endif
CFG_CPHY_PUB_DTCM_BASE	    :=0xE3840000
CFG_CPHY_PUB_ITCM_BASE	    :=0xE3940000
CFG_CPHY_PRV_DTCM_BASE	    :=0x49480000
CFG_CPHY_PRV_ITCM_BASE	    :=0x49580000
CFG_CPHY_PUB_DTCM_SIZE		:=0x98000
CFG_CPHY_PUB_ITCM_SIZE		:=0x98000
CFG_CPHY_PRV_DTCM_SIZE		:=0x80000
CFG_CPHY_PRV_ITCM_SIZE		:=0x80000
CFG_CPHY_PUB_DTCM_GLB_MINUS_LOCAL	    :=(0)
ifeq ($(chip_type),es)
CFG_XTENSA_CORE_X_CACHE             := Boston_bbe16_nc_x
CFG_LD_MAP_PATH                     := kirin970
CFG_XTENSA_CORE_X_SYSTEM            := RF-2015.3
else
CFG_XTENSA_CORE_X_CACHE             := Boston_cbbe16_nc
CFG_LD_MAP_PATH                     := kirin970
CFG_XTENSA_CORE_X_SYSTEM            := RF-2015.3
endif
CFG_FEATURE_EASYRF                        := FEATURE_OFF
