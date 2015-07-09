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
# * This is the HAL Makefile used for generating the GPL and NON-GPL modules.
# */

DRV_DIR=$(ROOT_DIR)/wlan/wlan_hal

RELEASE_DIR=$(ROOT_DIR)/release
NET80211_DIR=$(ROOT_DIR)/wlan/net80211/linux

DEVDEP_DIR=osi_wlan/devdep/rsi_9113
CORE_DIR=osi_wlan/core
OSD_DIR=osd_wlan/linux

include $(ROOT_DIR)/.config
include $(ROOT_DIR)/config/make.config

EXTRA_CFLAGS += -DLINUX -Wimplicit -Wstrict-prototypes
EXTRA_CFLAGS += -Dnet80211_s

EXTRA_CFLAGS += -DFLASH_BURNING
EXTRA_CFLAGS += -DPWR_SAVE_SUPPORT
EXTRA_CFLAGS += -DBGSCAN_SUPPORT
#EXTRA_CFLAGS += -DTRILITHIC_RELEASE
#EXTRA_CFLAGS += -DCHIP_9116
#EXTRA_CFLAGS += -DONEBOX_DEBUG_ENABLE
#EXTRA_CFLAGS += -DAMPDU_AGGR_SUPPORT
#EXTRA_CFLAGS += -DCHIP_ENABLED
#EXTRA_CFLAGS += -DAUTO_RATE_SUPPORT
#EXTRA_CFLAGS += -DEEPROM_READ_EN
EXTRA_CFLAGS += -DRSI_SDIO_MULTI_BLOCK_SUPPORT
EXTRA_CFLAGS += -DSECURITY_SUPPORT
#EXTRA_CFLAGS += -DENABLE_P2P_SUPPORT
#EXTRA_CFLAGS += -DENABLE_PER_MODE
EXTRA_CFLAGS += -DDYNAMIC_VARIABLES
EXTRA_CFLAGS += -DRF_8111
EXTRA_CFLAGS += -DEEPROM_NOT_PRESENT
#EXTRA_CFLAGS += -DDISABLE_TALOAD
EXTRA_CFLAGS += -I$(DRV_DIR)/include/linux
EXTRA_CFLAGS += -I$(ROOT_DIR)/common_hal/include/common
#EXTRA_CFLAGS += -DFPGA_VALIDATION
EXTRA_CFLAGS += -I$(NET80211_DIR)/osi_net80211/net80211
EXTRA_CFLAGS += -I$(NET80211_DIR)/osi_net80211
EXTRA_CFLAGS += -I$(NET80211_DIR)/osd_linux/include
#EXTRA_CFLAGS += -DUSE_SUBQUEUES
EXTRA_CFLAGS += -DACM_NO_TSPEC_CNFM

EXTRA_CFLAGS += -g

ifeq ($(USE_DEVICE),"SDIO")
EXTRA_CFLAGS += -DUSE_SDIO_INTF
else  
EXTRA_CFLAGS += -DUSE_USB_INTF
endif
DEVDEP_OBJ := $(DEVDEP_DIR)/onebox_wlan_osi_init.o \
		$(DEVDEP_DIR)/onebox_devdep_wlan_callbacks.o \
		$(DEVDEP_DIR)/onebox_dev_ops.o \
		$(DEVDEP_DIR)/onebox_ps.o \
		$(DEVDEP_DIR)/onebox_devdep_mgmt.o \
		$(DEVDEP_DIR)/onebox_pktpro.o    \
		$(DEVDEP_DIR)/onebox_debug.o    \
		$(DEVDEP_DIR)/onebox_reorder.o    


CORE_OBJ := $(CORE_DIR)/onebox_core_wlan.o  \
			$(CORE_DIR)/onebox_wlan_per.o  \
			$(CORE_DIR)/onebox_core_wlan_callbacks.o  \
			$(CORE_DIR)/onebox_core_hal_intf.o   \
			$(CORE_DIR)/onebox_core_os_intf.o \
			$(CORE_DIR)/onebox_core_autorate.o  \
			$(CORE_DIR)/onebox_core_vap.o \
			$(CORE_DIR)/onebox_core_wmm.o \
			$(CORE_DIR)/onebox_net80211_callbacks.o

OSD_OBJ := $(OSD_DIR)/onebox_wlan_ioctl.o  \
	   $(OSD_DIR)/onebox_wlan_osd_init.o \
	   $(OSD_DIR)/onebox_wlan_osd_ops.o \
	   $(OSD_DIR)/onebox_wlan_osd_callbacks.o \
	   $(OSD_DIR)/onebox_wlan_proc.o \

NONGPL_OBJS :=  $(DEVDEP_OBJ) $(CORE_OBJ)

GPL_OBJS := $(OSD_OBJ)

obj-m := onebox_wlan_gpl.o
onebox_wlan_gpl-objs := $(GPL_OBJS)

obj-m += onebox_wlan_nongpl.o
onebox_wlan_nongpl-objs := $(NONGPL_OBJS)

all: 
	@echo "Compiling non gpl module"
	@cp $(ROOT_DIR)/wlan/net80211/linux/Module.symvers .
	make -C$(KERNELDIR) SUBDIRS=$(DRV_DIR) modules
	@echo "Copying wlan module..."
	@cp onebox_wlan_nongpl.ko onebox_wlan_gpl.ko $(RELEASE_DIR)/.

clean:
	@echo "- Cleaning All Object and Intermediate Files"
	@find . -name '*.ko' | xargs rm -rf
	@find . -name '.*.ko.cmd' | xargs rm -rf
	@find . -name '.*.ko.unsigned.cmd' | xargs rm -rf
	@find . -name '*.ko.*' | xargs rm -rf
	@find . -name '*.order' | xargs rm -rf
	@find . -name '*.symvers' | xargs rm -rf
	@find . -name '*.markers' | xargs rm -rf
	@find . -name '*.o' | xargs rm -f
	@find . -name '.*.ko.cmd' | xargs rm -rf
	@find . -name '.*.o.cmd' | xargs rm -rf
	@find $(OSD_DIR) -name '*.o' | xargs rm -f
	@find $(HOST_INTF_DIR) -name '.*.o.cmd' | xargs rm -rf
	@find . -name '*.mod.c' | xargs rm -rf
	@echo "- Done"

