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

DRV_DIR=$(ROOT_DIR)/common_hal

RELEASE_DIR=$(ROOT_DIR)/release

OSI_DIR=osi
HOST_INTF_OSD_SDIO_DIR=intfdep/sdio/osd_sdio/linux
HOST_INTF_OSI_SDIO_DIR=intfdep/sdio/osi_sdio
HOST_INTF_OSD_USB_DIR=intfdep/usb/osd_usb/linux
HOST_INTF_OSI_USB_DIR=intfdep/usb/osi_usb
OSD_DIR=osd/linux

include $(ROOT_DIR)/.config
include $(ROOT_DIR)/config/make.config

EXTRA_CFLAGS += -DLINUX -Wimplicit -Wstrict-prototypes

EXTRA_CFLAGS += -DFLASH_BURNING
#EXTRA_CFLAGS += -DULP_GPIO_LP
#EXTRA_CFLAGS += -DRF_SUPPLY_19_V
#EXTRA_CFLAGS += -DRSI_IMX51
#EXTRA_CFLAGS += -DGPIO_HANDSHAKE
EXTRA_CFLAGS += -DPWR_SAVE_SUPPORT
EXTRA_CFLAGS += -DBGSCAN_SUPPORT
EXTRA_CFLAGS += -DRSI_SDIO_MULTI_BLOCK_SUPPORT
EXTRA_CFLAGS += -DSECURITY_SUPPORT
EXTRA_CFLAGS += -DDYNAMIC_VARIABLES
EXTRA_CFLAGS += -DRF_8111
#EXTRA_CFLAGS += -DDISABLE_TALOAD
EXTRA_CFLAGS += -I$(DRV_DIR)/include/linux
EXTRA_CFLAGS += -I$(DRV_DIR)/include/common
#EXTRA_CFLAGS += -DFPGA_VALIDATION
EXTRA_CFLAGS += -I$(NET80211_DIR)/osd_linux/include
EXTRA_CFLAGS += -g

ifeq ($(USE_DEVICE),"SDIO")
EXTRA_CFLAGS += -DUSE_SDIO_INTF

HOST_INTF_OSI_OBJ := $(HOST_INTF_OSI_SDIO_DIR)/onebox_sdio_main_osi.o      \
				 $(HOST_INTF_OSI_SDIO_DIR)/onebox_host_intf_osi_callbacks.o

HOST_INTF_OSD_OBJ := $(HOST_INTF_OSD_SDIO_DIR)/onebox_sdio_main_osd.o      \
				 $(HOST_INTF_OSD_SDIO_DIR)/onebox_host_intf_osd_callbacks.o
else  
EXTRA_CFLAGS += -DUSE_USB_INTF
#EXTRA_CFLAGS += -DUSB_BUFFER_STATUS_INDI 

HOST_INTF_OSI_OBJ := $(HOST_INTF_OSI_USB_DIR)/onebox_usb_main_osi.o        \
				 $(HOST_INTF_OSI_USB_DIR)/onebox_host_intf_osi_callbacks.o

HOST_INTF_OSD_OBJ := $(HOST_INTF_OSD_USB_DIR)/onebox_usb_main_osd.o        \
				 $(HOST_INTF_OSD_USB_DIR)/onebox_host_intf_osd_callbacks.o
endif

CORE_OBJ := $(OSI_DIR)/onebox_common_tx.o  \
	    $(OSI_DIR)/onebox_common_rx.o        \
	    $(OSI_DIR)/onebox_common_init.o      \
	    $(OSI_DIR)/onebox_flashing.o         \
	    $(OSI_DIR)/checksum_onescompliment.o \
	    $(OSI_DIR)/onebox_common_devops.o    \
	    $(OSI_DIR)/onebox_common_callbacks.o \
	    $(OSI_DIR)/onebox_common_pwr_mngr.o \
	    $(OSI_DIR)/onebox_common_dev_config_params.o

ifeq ($(USE_DEVICE),"SDIO")
CORE_OBJ += $(OSI_DIR)/onebox_sdio_devops.o
else
CORE_OBJ += $(OSI_DIR)/onebox_usb_devops.o
endif

OSD_OBJ := $(OSD_DIR)/onebox_modules_init.o  \
	   $(OSD_DIR)/onebox_proc.o          \
	   $(OSD_DIR)/onebox_file_ops.o      \
	   $(OSD_DIR)/onebox_thread.o        \
	   $(OSD_DIR)/onebox_osd_ops.o       \
	   $(OSD_DIR)/onebox_osd_callbacks.o \
	   $(OSD_DIR)/onebox_gpio.o


NONGPL_OBJS :=  $(CORE_OBJ) $(HOST_INTF_OSI_OBJ)

GPL_OBJS := $(HOST_INTF_OSD_OBJ)  $(OSD_OBJ)

obj-m := onebox_gpl.o
onebox_gpl-objs := $(GPL_OBJS)

obj-m += onebox_nongpl.o
onebox_nongpl-objs := $(NONGPL_OBJS)

all: 
	@echo "Compiling gpl and non gpl modules"
	make -C$(KERNELDIR) SUBDIRS=$(DRV_DIR) modules
#	@echo "Copying gpl and nongpl modules..."
#	@cp onebox_nongpl.ko onebox_gpl.ko $(RELEASE_DIR)/.

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

