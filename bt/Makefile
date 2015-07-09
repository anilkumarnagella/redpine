#
# @file  Makefile
# @author 
# @version 1.0.0
#
# @section LICENSE
#
# This software embodies materials and concepts that are confidential to Redpine
# Signals and is made available solely pursuant to the terms of a written license
# agreement with Redpine Signals
#
# @section DESCRIPTION
#
# Top level Makefile for compiling the hal components.
#

DRV_DIR=$(ROOT_DIR)/bt

RELEASE_DIR=$(DRV_DIR)/../release

OSD_DIR=osd_bt/linux
OSI_DIR=osi_bt
include $(ROOT_DIR)/.config
include $(ROOT_DIR)/config/make.config

EXTRA_CFLAGS += -DLINUX -Wimplicit -Wstrict-prototypes

EXTRA_CFLAGS += -DFLASH_BURNING
EXTRA_CFLAGS += -DPWR_SAVE_SUPPORT
#EXTRA_CFLAGS += -DENABLE_40MHZ_SUPPORT
EXTRA_CFLAGS += -DBGSCAN_SUPPORT
#EXTRA_CFLAGS += -DONEBOX_DEBUG_ENABLE
#EXTRA_CFLAGS += -DAMPDU_AGGR_SUPPORT
#EXTRA_CFLAGS += -DCHIP_ENABLED
#EXTRA_CFLAGS += -DAUTO_RATE_SUPPORT
#EXTRA_CFLAGS += -DEEPROM_READ_EN
EXTRA_CFLAGS += -DRSI_SDIO_MULTI_BLOCK_SUPPORT
EXTRA_CFLAGS += -DSECURITY_SUPPORT
#EXTRA_CFLAGS += -DENABLE_P2P_SUPPORT
#EXTRA_CFLAGS += -DENABLE_PER_MODE
#EXTRA_CFLAGS += -DENABLE_40_MHZ
EXTRA_CFLAGS += -DDYNAMIC_VARIABLES
EXTRA_CFLAGS += -DRF_8111
#EXTRA_CFLAGS += -DEEPROM_NOT_PRESENT
#EXTRA_CFLAGS += -DDISABLE_TALOAD
EXTRA_CFLAGS += -I$(DRV_DIR)/include/linux
EXTRA_CFLAGS += -I$(ROOT_DIR)/common_hal/include/common
#EXTRA_CFLAGS += -DFPGA_VALIDATION
EXTRA_CFLAGS += -g

ifeq ($(USE_DEVICE),"SDIO")
EXTRA_CFLAGS += -DUSE_SDIO_INTF
else  
EXTRA_CFLAGS += -DUSE_USB_INTF
endif

OSI_OBJ := $(OSI_DIR)/onebox_bt_osi_init.o \
	$(OSI_DIR)/onebox_bt_rx.o \
	$(OSI_DIR)/onebox_bt_tx.o \
	$(OSI_DIR)/onebox_bt.o \
	$(OSI_DIR)/onebox_bt_osi_callbacks.o \

ifeq ($(USE_GENL_BT_STACK), y)
BTSTK := $(OSD_DIR)/onebox_bt_nl.o 
else
BTSTK := $(OSD_DIR)/onebox_osd_bluez.o
endif

OSD_OBJ := $(BTSTK) \
	$(OSD_DIR)/onebox_bt_osd_init.o \
	$(OSD_DIR)/onebox_bt_osd_callbacks.o \
	$(OSD_DIR)/onebox_bt_proc.o  


NONGPL_OBJS :=  $(OSI_OBJ)
GPL_OBJS := $(OSD_OBJ)

obj-m := onebox_bt_gpl.o
onebox_bt_gpl-objs := $(GPL_OBJS)

obj-m += onebox_bt_nongpl.o
onebox_bt_nongpl-objs := $(NONGPL_OBJS)

all: 
	@echo "Compiling non gpl module"	
	@cp ../common_hal/Module.symvers .
	make -C$(KERNELDIR) SUBDIRS=$(DRV_DIR) modules

clean:
	@echo "- Cleaning All Object and Intermediate Files"
	@find . -name '*.ko' | xargs rm -rf
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

