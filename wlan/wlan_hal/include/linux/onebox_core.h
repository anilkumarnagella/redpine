/**
 * @file onebox_core.h
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
 * This file contians the function prototypes used in the core module
 * 
 */
#ifndef __ONEBOX_CORE_H__
#define __ONEBOX_CORE_H__

#include "onebox_common.h"

uint8 core_determine_hal_queue(PONEBOX_ADAPTER adapter);
void core_qos_processor(PONEBOX_ADAPTER adapter);
uint32 wlan_core_pkt_recv(PONEBOX_ADAPTER adapter, netbuf_ctrl_block_t *netbuf_cb, int8 rs_rssi, int8 chno);
uint32 bt_core_pkt_recv(PONEBOX_ADAPTER adapter, netbuf_ctrl_block_t *netbuf_cb);
#ifdef BYPASS_RX_DATA_PATH
uint8 bypass_data_pkt(PONEBOX_ADAPTER adapter, netbuf_ctrl_block_t *netbuf_cb);
#endif
uint32 core_send_beacon(PONEBOX_ADAPTER adapter,netbuf_ctrl_block_t *netbuf_cb, 
                        struct core_vap *core_vp);
netbuf_ctrl_block_t* core_dequeue_pkt(PONEBOX_ADAPTER adapter, uint8 q_num);
void core_queue_pkt(PONEBOX_ADAPTER adapter,netbuf_ctrl_block_t *netbuf_cb, 
                           uint8 q_num);

/*onebox_core_hal_intf.c function declarations*/
uint32 core_init(PONEBOX_ADAPTER adapter);
uint32 core_deinit(PONEBOX_ADAPTER adapter);
uint32 core_update_tx_status(PONEBOX_ADAPTER adapter,struct tx_stat_s *tx_stat);
uint32 core_tx_data_done(PONEBOX_ADAPTER adapter,netbuf_ctrl_block_t *netbuf_cb);

/*onebox_core_os_intf.c function declarations*/
int core_xmit(PONEBOX_ADAPTER adapter, netbuf_ctrl_block_t *netbuf_cb);
int onebox_send_mgmt_pkt(struct ieee80211_node *ni, 
                         netbuf_ctrl_block_m_t *netbuf_cb_m,
                         const struct ieee80211_bpf_params *bpf_params);

uint32 core_rcv_pkt(PONEBOX_ADAPTER adapter, netbuf_ctrl_block_t *netbuf_cb);
void core_radiotap_tx(PONEBOX_ADAPTER adapter, struct ieee80211vap *vap, netbuf_ctrl_block_t *netbuf_cb);
ONEBOX_STATUS stats_frame(PONEBOX_ADAPTER adapter);
int start_per_tx(PONEBOX_ADAPTER adapter);
int do_continuous_send(PONEBOX_ADAPTER adapter);
ONEBOX_STATUS prepare_per_pkt(PONEBOX_ADAPTER adapter, netbuf_ctrl_block_t *netbuf_cb);
ONEBOX_STATUS start_per_burst(PONEBOX_ADAPTER adapter);
ONEBOX_STATUS onebox_send_per_frame(PONEBOX_ADAPTER adapter,uint8 mode);
void onebox_print_dump(int32 dbg_zone_l, PUCHAR msg_to_print_p,int32 len);
void core_michael_failure( PONEBOX_ADAPTER, uint8 *msg);
ONEBOX_STATUS init_phy_layer (PONEBOX_ADAPTER adapter);
void set_region(struct ieee80211com *ic);
ONEBOX_STATUS send_per_ampdu_indiaction_frame(PONEBOX_ADAPTER adapter);
void check_traffic_pwr_save(struct ieee80211vap *vap, uint8 tx_path, uint32 payload_size);
void send_traffic_pwr_save_req(struct ieee80211vap *vap);
#endif
