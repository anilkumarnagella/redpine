
/**
 * @file onebox_pktpro.h
 * @author 
 * @version 1.0
 *
 * @section LICENSE
 *
 * This software embodies materials and concepts that are confidential to Redpine
 * Signals and is made available solely pursuant to the terms of a written license
 * agreement with Redpine Signals
 *
 * @section DESCRIPTION
 *
 * This file contians the function prototypes used for sending /receiving packets to/from the
 * driver
 * 
 */

#ifndef __ONEBOX_PKTPRO_H__
#define __ONEBOX_PKTPRO_H__

#include "onebox_datatypes.h"
#include "onebox_common.h"

#define IEEE80211_FCTL_STYPE  0xF0
#define ONEBOX_80211_FC_PROBE_RESP 0x50

/* Function Prototypes */
void interrupt_handler(PONEBOX_ADAPTER adapter);
ONEBOX_STATUS onebox_read_pkt(PONEBOX_ADAPTER adapter);
ONEBOX_STATUS send_beacon(PONEBOX_ADAPTER adapter, 
                          netbuf_ctrl_block_t *netbuf_cb,
                          struct core_vap *core_vp,
                          int8 dtim_beacon);
ONEBOX_STATUS send_onair_data_pkt(PONEBOX_ADAPTER adapter,
                                  netbuf_ctrl_block_t *netbuf_cb,
                                  int8 q_num);
ONEBOX_STATUS send_bt_pkt(PONEBOX_ADAPTER adapter, netbuf_ctrl_block_t *netbuf_cb);
ONEBOX_STATUS send_zigb_pkt(PONEBOX_ADAPTER adapter, netbuf_ctrl_block_t *netbuf_cb);
ONEBOX_STATUS onebox_load_config_vals(PONEBOX_ADAPTER  adapter);
int32 load_ta_instructions(PONEBOX_ADAPTER adapter);
void sdio_scheduler_thread(void *data);
ONEBOX_STATUS device_init(PONEBOX_ADAPTER adapter);
ONEBOX_STATUS device_deinit(PONEBOX_ADAPTER adapter);
ONEBOX_STATUS device_queues_status(PONEBOX_ADAPTER adapter, uint8 q_num);
int hal_set_sec_wpa_key(PONEBOX_ADAPTER adapter,const struct ieee80211_node *ni_sta, uint8 key_type);
uint32 hal_send_sta_notify_frame(PONEBOX_ADAPTER adapter, struct ieee80211_node *sta_addr, int32 sta_id);

#endif

