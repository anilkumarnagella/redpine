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
# * This is the Net80211 Makefile used for compiling the osi and osd wrappers in Net80211.
# */

#KERNELDIR=/lib/modules/$(KERNELRELEASE)/build
KERNELDIR=/home/in2em/sourcecode/quadchip/riot/buildroot/output/build/linux-3.16
#KERNELRELEASE=$(shell uname -r)
DRV_DIR=$(ROOT_DIR)/wlan/net80211/linux
RELEASE_DIR=release

SRC_DIR=osi_net80211/src
OSD_DIR=osd_linux/src

include $(ROOT_DIR)/.config
include $(ROOT_DIR)/config/make.config

EXTRA_CFLAGS += -DLINUX -Wimplicit -Wstrict-prototypes
#EXTRA_CFLAGS += -DANDROID_PLATFORM
ifeq ($(USE_DEVICE),"SDIO")
EXTRA_CFLAGS += -DUSE_SDIO_INTF
EXTRA_CFLAGS += -DPWR_SAVE_SUPPORT
endif
#EXTRA_CFLAGS += -DIEEE80211_AMPDU_AGE
#EXTRA_CFLAGS += -DENABLE_P2P_SUPPORT
#EXTRA_CFLAGS += -DINET
#EXTRA_CFLAGS += -Dnet80211_s
#EXTRA_CFLAGS += -DIEEE80211_DEBUG
#EXTRA_CFLAGS += -I./include/linux
#EXTRA_CFLAGS += -I$(DRV_DIR)/inc/
#EXTRA_CFLAGS += -I$(DRV_DIR)/inc/osd/linux/
#EXTRA_CFLAGS += -I$(DRV_DIR)/inc/osi/
EXTRA_CFLAGS += -I$(ROOT_DIR)/common_hal/include/common
EXTRA_CFLAGS += -I$(DRV_DIR)/osd_linux/include/
EXTRA_CFLAGS += -I$(DRV_DIR)/osi_net80211/net80211/
EXTRA_CFLAGS += -I$(DRV_DIR)/osi_net80211/
EXTRA_CFLAGS += -g

#EXTRA_CFLAGS += -DONEBOX_ENABLE_RX_DC_OFFSET
#EXTRA_CFLAGS += -DONEBOX_ENABLE_SSB_CAL

wlan_wep-objs   := $(SRC_DIR)/ieee80211_crypto_wep.o
wlan_tkip-objs   := $(SRC_DIR)/ieee80211_crypto_tkip.o
wlan_scan_sta-objs   := $(SRC_DIR)/ieee80211_scan_sta.o
wlan_ccmp-objs   := $(SRC_DIR)/ieee80211_crypto_ccmp.o
wlan_xauth-objs   := $(SRC_DIR)/ieee80211_xauth.o

wlan_acl-objs   := $(SRC_DIR)/ieee80211_acl.o
wlan-objs :=$(SRC_DIR)/ieee80211.o \
	$(SRC_DIR)/ieee80211_linux.o \
	$(SRC_DIR)/ieee80211_ioctl.o \
	$(SRC_DIR)/ieee80211_node.o \
	$(SRC_DIR)/ieee80211_action.o       \
	$(SRC_DIR)/ieee80211_adhoc.o        \
	$(SRC_DIR)/ieee80211_ageq.o        \
	$(SRC_DIR)/ieee80211_crypto.o \
	$(SRC_DIR)/ieee80211_crypto_none.o \
	$(SRC_DIR)/ieee80211_dfs.o  \
	$(SRC_DIR)/ieee80211_quiet.o  \
	$(SRC_DIR)/ieee80211_ht.o \
	$(SRC_DIR)/ieee80211_hwmp.o \
	$(SRC_DIR)/ieee80211_input.o \
	$(SRC_DIR)/ieee80211_mesh.o \
	$(SRC_DIR)/ieee80211_monitor.o \
	$(SRC_DIR)/ieee80211_phy.o \
	$(SRC_DIR)/ieee80211_proto.o \
	$(SRC_DIR)/ieee80211_ratectl.o \
	$(SRC_DIR)/ieee80211_regdomain.o \
	$(SRC_DIR)/ieee80211_wds.o \
	$(SRC_DIR)/ieee80211_scan.o \
	$(SRC_DIR)/ieee80211_sta.o \
	$(SRC_DIR)/ieee80211_superg.o \
	$(SRC_DIR)/ieee80211_hostap.o \
	$(SRC_DIR)/ieee80211_radiotap.o \
	$(SRC_DIR)/ieee80211_power.o \
	$(SRC_DIR)/ieee80211_output.o\
	$(SRC_DIR)/ieee80211_p2p.o\
	$(OSD_DIR)/mbuf.o \
	$(OSD_DIR)/sysctl.o \
	$(OSD_DIR)/ndiface.o \
	$(OSD_DIR)/ioctl.o \
	$(OSD_DIR)/linux.o \
	$(OSD_DIR)/cfg80211_ioctl.o \
	$(OSD_DIR)/cfg80211_wrapper.o \

#	$(SRC_DIR)/ieee80211_ddb.o  
#	$(SRC_DIR)/ieee80211_tdma.o
MOD_INSTALL  := wlan.o wlan_wep.o wlan_tkip.o wlan_ccmp.o  wlan_xauth.o wlan_scan_sta.o wlan_acl.o

obj-m := $(MOD_INSTALL)

all:
	@echo "Compiling Onebox  code"
	make -C$(KERNELDIR) SUBDIRS=$(DRV_DIR) modules
	#@cp *.ko ../release


clean:
	@echo "- Cleaning All Object and Intermediate Files"
	@find . -name '*.ko' | xargs rm -rf
	@find . -name '*.order' | xargs rm -rf
	@find . -name '*.symvers' | xargs rm -rf
	@find . -name '*.markers' | xargs rm -rf
	@find . -name '*.o' | xargs rm -f
	@find . -name '.*.ko.cmd' | xargs rm -rf
	@find . -name '.*.ko.unsigned.cmd' | xargs rm -rf
	@find . -name '*.ko.*' | xargs rm -rf
	@find . -name '.*.o.cmd' | xargs rm -rf
	@find $(OSD_DIR) -name '*.o' | xargs rm -f
	@find $(HOST_INTF_DIR) -name '.*.o.cmd' | xargs rm -rf
	@find . -name '*.mod.c' | xargs rm -rf
	@echo "- Done"
