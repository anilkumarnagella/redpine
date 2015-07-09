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
# * This is the Utils Makefile used for generating the onebox utility. This utility is used for
# * creating the Virtual access point(VAP), deleting the VAP, setting wmm params etc
# */
#CC=$(CROSS_COMPILE)cc
cpy=cp

include $(PWD)/.config
include $(PWD)/config/make.config

#EXTRA_CFLAGS += -DENABLE_PER_MODE
EXTRA_CFLAGS += -MMD -O2 -Wall -g

OBJS =onebox_util.o

all: onebox_util copy
onebox_util: $(OBJS)
	$(CC) -o onebox_util $(OBJS)
copy:
	$(CC) -o receive receive.c
	$(CC) -o transmit transmit.c
	$(CC) -o transmit_packet transmit_packet.c
	#$(CC) -o nl80211_util nl80211_util.c
	$(CC) -o bbp_util matlab_utils.c
	$(CC) -o sniffer_app sniffer.c
	@echo 'copying to release folder'
	$(cpy) onebox_util transmit transmit_packet receive bbp_util sniffer_app ../release/

clean:
	rm -f onebox_util onebox_util.d onebox_util.o
	rm -rf receive transmit transmit_packet


