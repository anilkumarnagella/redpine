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
# * List of folders compiled by the top level Makefile are net80211, common_hal, supplicant, utils.
# */
ROOT_DIR:=$(PWD)
IEEE_DIR:=$(ROOT_DIR)/wlan/net80211/linux
WLAN_DIR:=$(ROOT_DIR)/wlan/wlan_hal
WLAN_DRV:=$(ROOT_DIR)/wlan
BT_DRV:=$(ROOT_DIR)/bt
ZIGB_DRV:=$(ROOT_DIR)/zigbee
COEX_DRV:=$(ROOT_DIR)/common_hal
DEST_DIR=$(ROOT_DIR)/release
SUPP_DIR:=$(ROOT_DIR)/wlan/supplicant/linux
HOSTAPD_DIR:=$(ROOT_DIR)/wlan/hostapd-2.3/hostapd

UTILS_DIR:=$(ROOT_DIR)/utils

CC=$(CROSS_COMPILE)gcc

#DEF_KERNEL_DIR := /lib/modules/$(shell uname -r)/build
DEF_KERNEL_DIR := /home/in2em/sourcecode/quadchip/riot/buildroot/output/build/linux-3.16
ifeq ($(KERNELDIR),)
	KERNELDIR := $(DEF_KERNEL_DIR)
endif

include $(ROOT_DIR)/config/.config
include $(ROOT_DIR)/config/make.config

#include .config
#include make.config

all: menuconfig obx_common_hal onebox_wlan onebox_bt onebox_zigb onebox_utils
	$(CROSS_COMPILE)strip --strip-unneeded $(DEST_DIR)/*.ko
	@cp -r release /home/rsi
	@echo -e "\033[32mCompilation done SUCCESSFULLY\033[0m"

wlan:obx_common_hal onebox_wlan onebox_utils 
	@echo -e "\033[32m Only WLAN Compilation done SUCCESSFULLY\033[0m"

bt:obx_common_hal onebox_bt 
	@echo -e "\033[32m Only BT Compilation done SUCCESSFULLY\033[0m"

zigbee:obx_common_hal onebox_zigb
	@echo -e "\033[32m Only ZIGBEE Compilation done SUCCESSFULLY\033[0m"

obx_common_hal: 
	@echo -e "\033[32mCompiling Onebox HAL...\033[0m"
	make -C$(COEX_DRV) ROOT_DIR=$(ROOT_DIR) KERNELDIR=$(KERNELDIR) 
	cp $(COEX_DRV)/*.ko $(DEST_DIR)        

onebox_wlan: 
	@echo -e "\033[32mCompiling Onebox WLAN...\033[0m"
	make -C$(WLAN_DRV) ROOT_DIR=$(ROOT_DIR) CC=$(CC) KERNELDIR=$(KERNELDIR)
	cp $(IEEE_DIR)/*.ko  $(DEST_DIR)
	cp $(WLAN_DIR)/*.ko $(DEST_DIR)        
	cp $(SUPP_DIR)/wpa_supplicant/wpa_supplicant $(DEST_DIR)
	cp $(SUPP_DIR)/wpa_supplicant/wpa_cli $(DEST_DIR)
	cp $(HOSTAPD_DIR)/hostapd $(DEST_DIR)
	cp $(HOSTAPD_DIR)/hostapd_cli $(DEST_DIR)



onebox_bt: 
	@echo -e "\033[32mCompiling Onebox BT...\033[0m"
	make -C$(BT_DRV) ROOT_DIR=$(ROOT_DIR) KERNELDIR=$(KERNELDIR)
	cp $(BT_DRV)/*.ko $(DEST_DIR)        

onebox_zigb: 
	@echo -e "\033[32mCompiling Onebox Zigbee...\033[0m"
	make -C$(ZIGB_DRV) ROOT_DIR=$(ROOT_DIR) KERNELDIR=$(KERNELDIR)
	cp $(ZIGB_DRV)/*.ko $(DEST_DIR)        

onebox_utils:
	@echo -e "\033[32mCompiling onebox utils...\033[0m"
	make CC=$(CC) -C $(UTILS_DIR)/

menuconfig: 
	$(MAKE) -C config/lxdialog lxdialog
	$(CONFIG_SHELL) config/Menuconfig config/config.in

clean:
	@echo "- Cleaning All Object and Intermediate Files"
	@find . -name '*.ko' | xargs rm -rf
	@find . -name '*.o' | xargs rm -f
	@find . -name '.*.ko.cmd' | xargs rm -rf
	@find . -name '.*.ko.unsigned.cmd' | xargs rm -rf
	@find . -name '*.ko.*' | xargs rm -rf
	@find . -name '.*.o.cmd' | xargs rm -rf
	@find . -name '*.mod.c' | xargs rm -rf
	@find . -name '.tmp_versions' | xargs rm -rf
	@find . -name '*.markers' | xargs rm -rf
	@find . -name '*.symvers' | xargs rm -rf
	@find . -name '*.order' | xargs rm -rf
	@find . -name '*.d' | xargs rm -rf
	@find . -name 'wpa_priv' | xargs rm -rf
	@find . -name 'onebox_util' | xargs rm -rf
	@find . -name 'onebox_util' | xargs rm -rf
	@find . -name 'nl80211_util' | xargs rm -rf
	@find . -name 'bbp_util' | xargs rm -rf
	@find . -name 'transmit' | xargs rm -rf
	@find . -name 'transmit_packet' | xargs rm -rf
	@find . -name 'receive' | xargs rm -rf
	@find . -name 'sniffer_app' | xargs rm -rf
	@find . -name 'CVS' | xargs rm -rf
	@find . -name '*.swp' | xargs rm -rf
	@rm -rf $(UTILS_DIR)/transmit_packet
	@rm -rf $(SUPP_DIR)/core *~ *.o eap_*.so $(ALL) $(WINALL) eapol_test preauth_test
	@rm -rf $(SUPP_DIR)/wpa_supplicant/wpa_supplicant 
	@rm -rf $(SUPP_DIR)/wpa_supplicant/wpa_cli 
	@rm -rf $(SUPP_DIR)/wpa_supplicant/wpa_gui 
	@rm -rf $(DEST_DIR)/wpa_supplicant 
	@rm -rf $(SUPP_DIR)/wpa_supplicant/wpa_passphrase
	@rm -rf $(DEST_DIR)/wpa_cli 
	@rm -rf $(DEST_DIR)/wpa_gui
	@rm -rf $(HOSTAPD_DIR)/core *~ *.o eap_*.so $(ALL) $(WINALL) eapol_test preauth_test
	@rm -rf $(HOSTAPD_DIR)/hostapd/hostapd 
	@rm -rf $(HOSTAPD_DIR)/hostapd/hostapd_cli 
	@rm -rf $(HOSTAPD_DIR)/hostapd/hostapd_passphrase
	@rm -rf $(DEST_DIR)/hostapd
	@rm -rf $(DEST_DIR)/hostapd_cli 
	@echo "- Done"
