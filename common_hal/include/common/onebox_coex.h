/**
 * @file onebox_coex.h
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
 * This file contians the data structures and variables/ macros commonly
 * used across all protocols .
 */

#ifndef __ONEBOX_COEX_H__
#define __ONEBOX_COEX_H__

#define GS_CARD_DETACH			0 /* card status */
#define GS_CARD_ABOARD			1 

/**
 * MODULE_REMOVED: 
 * modules are not installed or the modules are
 * uninstalled after an installation.
 *
 * MODULE_INSERTED:
 * modules are installled and the layer is not yet initialised.
 * or the layer is deinitialized.
 *
 * MODULE_ACTIVE:
 * modules are installed and the layer is initialised.
 *
 */
#define MODULE_REMOVED		0
#define MODULE_INSERTED		1
#define MODULE_ACTIVE		2

#define FW_INACTIVE		0
#define FW_ACTIVE		1

#define BT_CARD_READY_IND	0x89
#define WLAN_CARD_READY_IND	0x0
#define ZIGB_CARD_READY_IND	0xff

#define COEX_Q      0 
#define BT_Q        1 
#define WLAN_Q      2 
#define VIP_Q       3 
#define ZIGB_Q      4

#define COEX_TX_Q      0 
#define ZIGB_TX_Q      1
#define BT_TX_Q        2 
#define WLAN_TX_M_Q    4 
#define WLAN_TX_D_Q    5 

#define COEX_PKT      0 
#define ZIGB_PKT      1
#define BT_PKT        2 
#define WLAN_PKT      3 

#define MAX_IDS  3
#define WLAN_ID  0
#define BT_ID    1
#define ZB_ID    2

#define ASSET_NAME(id) \
	  (id == WLAN_ID)  ? "wlan_asset" : \
	   ((id == BT_ID)  ? "bluetooth_asset" : \
	    ((id == ZB_ID) ? "zigbee_asset" : "null"))

#define COMMAN_HAL_WAIT_FOR_CARD_READY 1
#define COMMON_HAL_SEND_CONFIG_PARAMS 2
#define COMMON_HAL_TX_ACESS 3
struct driver_assets {
	uint32 card_state;	
	uint16 ta_aggr;
	uint8  asset_role;
	struct wireless_techs {
		uint32 drv_state;
		uint32 fw_state;
#ifdef USE_USB_INTF
		uint32 buffer_status_reg_addr;
#endif
		int32  (*inaugurate)(void);
		int32  (*disconnect)(void);
		ONEBOX_STATUS (*onebox_get_pkt_from_coex)(netbuf_ctrl_block_t *netbuf_cb);
		ONEBOX_STATUS (*onebox_get_buf_status)(uint8 buf_status);
		ONEBOX_STATUS (*onebox_get_ulp_sleep_status)(uint8 sleep_status);
		void (*wlan_dump_mgmt_pending)(void);//remove me after coex optimisations
		bool tx_intention;
		bool tx_access; /* Per protocol tx access */
		uint8 deregister_flags;
		struct __wait_queue_head deregister_event;
		void *priv;
		uint32 default_ps_en;
	} techs[MAX_IDS];
	void *global_priv;
	void *pfunc;
	bool common_hal_tx_access;
	bool sleep_entry_recvd;
	bool ulp_sleep_ack_sent;
	struct semaphore tx_access_lock;
	struct semaphore wlan_init_lock;
	struct semaphore bt_init_lock;
	struct semaphore zigbee_init_lock;
	void (*update_tx_status)(uint8 prot_id);
	uint32 common_hal_fsm;
	uint8 lp_ps_handshake_mode;
	uint8 ulp_ps_handshake_mode;
	uint8 rf_power_val;
	uint8 device_gpio_type;
};

#define WLAN_TECH d_assets->techs[WLAN_ID]
#define BT_TECH d_assets->techs[BT_ID]
#define ZB_TECH d_assets->techs[ZB_ID]
extern struct driver_assets d_assets;
struct driver_assets *onebox_get_driver_asset(void);
/*
 * Generic Netlink Sockets
 */
struct genl_cb {
	uint8 gc_cmd, *gc_name;
	int32 gc_seq, gc_pid;
	int32 gc_assetid, gc_done;
	int32 gc_n_ops;
	void  *gc_drvpriv;
	struct nla_policy  *gc_policy;
	struct genl_family *gc_family;
	struct genl_ops    *gc_ops; 
	struct genl_info   *gc_info;
	struct sk_buff     *gc_skb;
};

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 6, 11)
# define get_portid(_info) (_info)->snd_pid
#else
# define get_portid(_info) (_info)->snd_portid
#endif

/*==========================================================/
 * attributes (variables): the index in this enum is 
 * used as a reference for the type, userspace application 
 * has to indicate the corresponding type the policy is 
 * used for security considerations
 *==========================================================*/

enum {
	RSI_USER_A_UNSPEC,
	RSI_USER_A_MSG,
	__RSI_USER_A_MAX,
};

/*=================================================================/ 
 * commands: enumeration of all commands (functions),
 * used by userspace application to identify command to be executed
 *=================================================================*/

enum {
	RSI_USER_C_UNSPEC,
	RSI_USER_C_CMD,
	__RSI_USER_C_MAX,
};
#define RSI_USER_A_MAX (__RSI_USER_A_MAX - 1)
#define RSI_VERSION_NR 1

#endif
