#/**
# * @file Makefile 
# * @author 
# * @version 1.0
# *
# * @section LICENSE
# *
# * This software embodies materials and concepts that are confidential to Redpine
# * Signals and is made available solely pursuant to the terms of a written license
# * agreement with Redpine Signals
# *
# * @section DESCRIPTION
# *
# * This is the Top Level Makefile used for compiling all the folders present in the parent directory.
# * List of folders compiled by the top level Makefile are net80211, hal, supplicant, utils.
# */
IEEE_DIR:=$(ROOT_DIR)/wlan/net80211/linux
ONEBOX_WLAN:=$(ROOT_DIR)/wlan/wlan_hal
DEST_DIR=$(ROOT_DIR)/release
SUPP_DIR:=$(ROOT_DIR)/wlan/supplicant/linux
HOSTAPD_DIR:=$(ROOT_DIR)/wlan/hostapd-2.3/hostapd			

include $(ROOT_DIR)/config/.config
include $(ROOT_DIR)/config/make.config

all:ieee80211 wlan_onebox wpa_supplicant
#all:ieee80211 wlan_onebox wpa_supplicant hostapd 
#all:ieee80211 wpa_supplicant 

wlan_onebox: 
	@cp $(ROOT_DIR)/wlan/net80211/linux/Module.symvers $(ROOT_DIR)/wlan/wlan_hal/
	make -C$(ONEBOX_WLAN) ROOT_DIR=$(ROOT_DIR) KERNELDIR=$(KERNELDIR)

ieee80211:
	@echo -e "\033[32mCompiling net80211...\033[0m"
	@cp $(ROOT_DIR)/common_hal/Module.symvers $(IEEE_DIR)/
	make -C$(KERNELDIR) SUBDIRS=$(IEEE_DIR) ROOT_DIR=$(ROOT_DIR) KERNELDIR=$(KERNELDIR)

wpa_supplicant:
	@echo -e "\033[32mCompiling wpa supplicant...\033[0m"
	make CC=$(CC) -C $(SUPP_DIR)/wpa_supplicant

hostapd:
	@echo -e "\033[32mCompiling hostapd...\033[0m"
	make CC=$(CC) -C $(HOSTAPD_DIR)
	

	
