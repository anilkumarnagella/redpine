/**
 * @file onebox_common.h
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
 * used in the driver .
 */

#ifndef __ONEBOX_COMMON_H__
#define __ONEBOX_COMMON_H__

#include <linux/netdevice.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/init.h>
#include <linux/etherdevice.h>
#include <linux/if.h>
#include <linux/wireless.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/mutex.h>
#ifdef USE_USB_INTF
#include <linux/usb.h>
#endif
/*Bluetooth Specific */
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>
#include <linux/workqueue.h>

#ifdef USE_USB_INTF
#define onebox_likely(a) likely(a)
#define USB_VENDOR_REGISTER_READ 0x15
#define USB_VENDOR_REGISTER_WRITE 0x16
#endif

#if((LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18))&& \
    (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,23)))
#include <linux/sdio/ctsystem.h>
#include <linux/sdio/sdio_busdriver.h>
#include <linux/sdio/_sdio_defs.h>
#include <linux/sdio/sdio_lib.h>
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26))
#include <linux/mmc/card.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>
#endif

#include "onebox_datatypes.h"

/* Kernel version between and including a & b */
#define KERNEL_VERSION_BTWN_2_6_(a,b) \
  ((LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,a)) && \
  (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,b)))

#define KERNEL_VERSION_EQUALS_2_6_(a) \
  (LINUX_VERSION_CODE == KERNEL_VERSION(2,6,a))

/* Kernel version greater than equals */
#define KERNEL_VERSION_GREATER_THAN_2_6_(a)  \
 (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,a)) 

/* Kernel version less than or equal to */
#define KERNEL_VERSION_LESS_THAN_3_6(a)  \
 (LINUX_VERSION_CODE <= KERNEL_VERSION(3,6,a)) 

#define KERNEL_VERSION_LESS_THAN_3_12_(a) \
 (LINUX_VERSION_CODE <= KERNEL_VERSION(3,12,a)) 


//#define USE_INTCONTEXT
#define USE_WORKQUEUES
//#define USE_TASKLETS
#ifdef USE_TASKLETS
#include <linux/interrupt.h>
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#define ONEBOX_BIT(n)                   (1 << (n))
#define ONEBOX_ZONE_ERROR               ONEBOX_BIT(0)  /* For Error Msgs              */
#define ONEBOX_ZONE_WARN                ONEBOX_BIT(1)  /* For Warning Msgs            */
#define ONEBOX_ZONE_INFO                ONEBOX_BIT(2)  /* For General Status Msgs     */
#define ONEBOX_ZONE_INIT                ONEBOX_BIT(3)  /* For Driver Init Seq Msgs    */
#define ONEBOX_ZONE_OID                 ONEBOX_BIT(4)  /* For IOCTL Path Msgs         */
#define ONEBOX_ZONE_MGMT_SEND           ONEBOX_BIT(5)  /* For TX Mgmt Path Msgs       */
#define ONEBOX_ZONE_MGMT_RCV            ONEBOX_BIT(6)  /* For RX Mgmt Path Msgs       */
#define ONEBOX_ZONE_DATA_SEND           ONEBOX_BIT(7)  /* For TX Data Path Msgs       */
#define ONEBOX_ZONE_DATA_RCV            ONEBOX_BIT(8)  /* For RX Data Path Msgs       */
#define ONEBOX_ZONE_FSM                 ONEBOX_BIT(9)  /* For State Machine Msgs      */
#define ONEBOX_ZONE_ISR                 ONEBOX_BIT(10) /* For Interrupt Specific Msgs */
#define ONEBOX_ZONE_MGMT_DUMP           ONEBOX_BIT(11) /* For Dumping Mgmt Pkts       */
#define ONEBOX_ZONE_DATA_DUMP           ONEBOX_BIT(12) /* For Dumping Data Pkts       */
#define ONEBOX_ZONE_CLASSIFIER          ONEBOX_BIT(13) /* For Classification Msgs     */
#define ONEBOX_ZONE_PROBE               ONEBOX_BIT(14) /* For Probe Req & Rsp Msgs    */
#define ONEBOX_ZONE_EXIT                ONEBOX_BIT(15) /* For Driver Unloading Msgs   */
#define ONEBOX_ZONE_DEBUG               ONEBOX_BIT(16) /* For Extra Debug Messages    */
#define ONEBOX_ZONE_PWR_SAVE             ONEBOX_BIT(17) /* For Powersave Blk Msgs     */
#define ONEBOX_ZONE_AGGR                ONEBOX_BIT(18) /* For Aggregation Msgs        */
#define ONEBOX_ZONE_DAGGR               ONEBOX_BIT(19) /* For Deaggregation Msgs      */
#define ONEBOX_ZONE_AUTORATE            ONEBOX_BIT(20) /* For Autorate Msgs           */
#define ONEBOX_ZONE_ACL_DATA            ONEBOX_BIT(21) /* For BT_ACL DATA PACKETS     */
#define ONEBOX_ZONE_SCO_DATA            ONEBOX_BIT(22) /* For BT_SCO DATA PACKETS     */

#define ONEBOX_STATUS_SUCCESS       0
#define ONEBOX_STATUS_FAILURE      -1


#define INVALID_QUEUE              0xff
#define NO_STA_DATA_QUEUES          4

#define ONEBOX_HEADER_SIZE          18 /* Size of PS_POLL */
#define TX_VECTOR_SZ                12

#define ONEBOX_TOTAL_PWR_VALS_LEN   30
#define ONEBOX_BGN_PWR_VALS_LEN     30
#define ONEBOX_AN_PWR_VALS_LEN      18

#define ONEBOX_EXTERN   extern
#define ONEBOX_STATIC   static
#define HEX_FILE       1
#define BIN_FILE       0
#define ONEBOX_SDIO_FRM_TRF_SIZE   (256 - 16)

#define FRAME_DESC_SZ                    16
#define OFFSET_EXT_DESC_SIZE    		 4
#define ONEBOX_DESCRIPTOR_SZ             64
#define ONEBOX_EXTENDED_DESCRIPTOR       12
#define ONEBOX_RCV_BUFFER_LEN      2000

typedef int ONEBOX_STATUS;

/***************************** START MACROS ********************************/
#define ONEBOX_PRINT printk
#define ONEBOX_SPRINTF  sprintf
#ifdef ONEBOX_DEBUG_ENABLE
#define ONEBOX_DEBUG1    ONEBOX_PRINT



#define PRINT_MAC_ADDR(zone, buf) do {\
	if (zone & onebox_bt_zone_enabled) {\
               ONEBOX_PRINT("%02x:%02x:%02x:%02x:%02x:%02x\n",\
                buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);\
	}\
} while (0);

#define ONEBOX_ASSERT(exp) do {\
	if (!(exp)) {\
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,\
		             ("===> ASSERTION FAILED <===\n"));\
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,\
		             ("Expression: %s\n", #exp));\
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,\
		             ("Function        : %s()\n", __FUNCTION__));\
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,\
		             ("File: %s:%d\n", __FILE__, __LINE__));\
	}\
} while (0)

#define FUNCTION_ENTRY(zone) do {\
	if (zone & onebox_bt_zone_enabled) {\
            ONEBOX_PRINT("+%s()\n", __FUNCTION__);\
	}\
} while (0);

#define FUNCTION_EXIT(zone) do {\
	if (zone & onebox_bt_zone_enabled) {\
             ONEBOX_PRINT("-%s()\n", __FUNCTION__);\
	}\
} while (0);
#else
 #define PRINT_MAC_ADDR(zone, buf)
 #define ONEBOX_ASSERT(exp)
 #define FUNCTION_ENTRY(zone)
 #define FUNCTION_EXIT(zone)
#endif

#ifndef DANUBE_ADDRESSING
 #define ONEBOX_HOST_VIR_TO_PHY(virt_addr)  virt_to_phys((void *)virt_addr)
 #define ONEBOX_HOST_PHY_TO_VIR(phy_addr)  phys_to_virt((uint32)phy_addr)
#else
 #define ONEBOX_HOST_VIR_TO_PHY(virt_addr) ((void *)(((uint32)virt_addr) & 0x5FFFFFFF))
 #define ONEBOX_HOST_PHY_TO_VIR(phy_addr) (((uint32)phy_addr) | 0xA0000000)
#endif


#define ONEBOX_CPU_TO_LE16(x)   cpu_to_le16(x)
#define ONEBOX_CPU_TO_LE32(x)   cpu_to_le32(x)
#define ONEBOX_CPU_TO_BE16(x)   cpu_to_be16(x)

#define ETH_PROTOCOL_OFFSET             12
#define ETH_HDR_OFFSET                  0
#define ETH_HDR_LEN                     14


#define DMA_MEMORY                      1
#define NORMAL_MEMORY                   0
#define LMAC_VER_OFFSET                 0x200


/*BT specific */
#define MAX_BER_PKT_SIZE    	1030
#define GANGES_BER_QUEUE_LEN    100
/* BT device states */
#define BT_STAND_BY     		0
#define BT_INQUIRY      		1
#define BT_PAGE         		2
#define BT_INQUIRY_SCAN     	3   
#define BT_PAGE_SCAN        	4
#define BT_INQUIRY_RESP     	5   
#define BT_SLAVE_PAGE_RESP  	6
#define BT_MASTER_PAGE_RESP 	6
#define BT_CONN         		7
#define BT_PARK         		8
#define BT_SLAVE_PAGE_RESP_2    6
#define BT_MASTER_PAGE_RESP_2   6
#define BT_CLASSIC_MODE     	0
#define BT_LE_MODE      		1

#define FREQ_HOP_ENABLE     0
#define FREQ_HOP_DISABLE    1

/* zigbee queues */
//#define BT_TA_MGMT_Q            0x0
//#define ZB_INTERNAL_MGMT_Q      0x0
#define BT_TA_MGMT_Q        	  0x6
#define ZB_DATA_Q                 0x7
#define ZB_INTERNAL_MGMT_Q        0x6
#define E2E_MODE_ON                      1

/* Bluetooth Queues */
#define BT_DATA_Q       	0x7
#define BT_TA_MGMT_Q        0x6
#define BT_INT_MGMT_Q       0x6
/* EPPROM_READ_ADDRESS */

#define WLAN_MAC_EEPROM_ADDR               40 
#define WLAN_MAC_MAGIC_WORD_LEN            01 
#define WLAN_HOST_MODE_LEN                 04 
#define MAGIC_WORD			   0x5A


/* sifs and slot time defines */
#define SIFS_TX_11N_VALUE 580
#define SIFS_TX_11B_VALUE 346
#define SHORT_SLOT_VALUE  360
#define LONG_SLOT_VALUE   640
#define OFDM_ACK_TOUT_VALUE 2720
#define CCK_ACK_TOUT_VALUE 9440
#define LONG_PREAMBLE    0x0000
#define SHORT_PREAMBLE    0x0001
#define REQUIRED_HEADROOM_FOR_BT_HAL	 16
#define ONEBOX_DEBUG_MESG_RECVD 1

/*Master access types*/
#define ONEBOX_MASTER_READ	11
#define ONEBOX_MASTER_WRITE	22
#define ONEBOX_MASTER_ACK	33

/*
 * Bluetooth-HCI
 */
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(3, 1, 0)) && \
     (LINUX_VERSION_CODE < KERNEL_VERSION(3, 4, 0)))
# define onebox_hci_dev_hold(_hdev) hci_dev_hold(_hdev)
# define onebox_hci_dev_put(_hdev)  hci_dev_put(_hdev)
#else
# define onebox_hci_dev_hold(_hdev)
# define onebox_hci_dev_put(_hdev)
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
# define get_hci_drvdata(_hdev)        hci_get_drvdata(_hdev)
# define set_hci_drvdata(_hdev, _data) hci_set_drvdata(_hdev, _data)
#else
# define get_hci_drvdata(_hdev)        (_hdev)->driver_data
# define set_hci_drvdata(_hdev, _data) ((_hdev)->driver_data = _data)
#endif

#ifdef USE_SDIO_INTF
# define HCI_DEV_BUS HCI_SDIO
#else
# define HCI_DEV_BUS HCI_USB
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,12,28)
# define onebox_hci_recv_frame(_hd, _sk) hci_recv_frame(_sk)
#else
# define onebox_hci_recv_frame(_hd, _sk) hci_recv_frame(_hd, _sk)
#endif

typedef struct
{
	uint16  major;
	uint16  minor;
	uint8 release_num;
	uint8 patch_num;
	union 
	{
		struct
		{
			uint8 fw_ver[8];
		}info;
	}ver;
}__attribute__ ((packed))version_st;

typedef struct onebox_priv ONEBOX_ADAPTER, *PONEBOX_ADAPTER;

#include "onebox_netbuf.h"
#include "onebox_coex.h"
#include "onebox_mgmt.h"
#include "onebox_bt_ops.h"
#include "onebox_os_intf_ops.h"
#include "onebox_sdio_intf.h"

/* Adapter structure */
struct onebox_priv 
{

	/* Network Interface Specific Variables */
	struct hci_dev *hdev;
	onebox_netbuf_head_t		bt_tx_queue;
	uint32	fsm_state;
	uint32           core_init_done;
#define NAME_MAX_LENGTH         32
	uint8               name[NAME_MAX_LENGTH];
	struct net_device_stats  stats;

#ifdef USE_USB_INTF
	struct usb_interface *pfunction;
#else
#if((LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18))&& \
    (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,23)))
	PSDDEVICE pDevice;
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26))
	
	struct  sdio_func *pfunction;
#endif
#endif

	struct onebox_os_intf_operations *os_intf_ops;
	struct onebox_osd_host_intf_operations *osd_host_intf_ops;
	struct onebox_osi_host_intf_operations *osi_host_intf_ops;
	struct onebox_osi_bt_ops         *osi_bt_ops;  // core ops + devdep ops
	struct onebox_bt_osd_operations  *osd_bt_ops;
	ONEBOX_STATUS (*onebox_send_pkt_to_coex)(netbuf_ctrl_block_t* netbuf_cb, uint8 hal_queue);

	struct semaphore bt_gpl_lock;
	/*version related variables*/
	version_st        driver_ver;
	version_st        ta_ver;
	version_st        lmac_ver;
	struct genl_cb *genl_cb;
} __attribute__((__aligned__(4)));

extern uint32 onebox_bt_zone_enabled;

struct onebox_bt_osd_operations *onebox_get_bt_osd_operations_from_origin(void);
struct onebox_osi_bt_ops *onebox_get_osi_bt_ops(void);
ONEBOX_STATUS bt_read_pkt(PONEBOX_ADAPTER adapter,
				netbuf_ctrl_block_t *netbuf_cb);
ONEBOX_STATUS setup_bt_procfs(PONEBOX_ADAPTER adapter);
void destroy_bt_procfs(void);
extern uint32 onebox_zone_enabled;

#ifdef USE_BLUEZ_BT_STACK
int bluez_init(PONEBOX_ADAPTER);	
int bluez_deinit(PONEBOX_ADAPTER);	
ONEBOX_STATUS send_pkt_to_bluez(PONEBOX_ADAPTER adapter, netbuf_ctrl_block_t *netbuf_cb);
#endif

#ifdef USE_GENL_BT_STACK
int32 btgenl_init(PONEBOX_ADAPTER adapter);
int32 btgenl_deinit(PONEBOX_ADAPTER adapter);
ONEBOX_STATUS send_pkt_to_btgenl(PONEBOX_ADAPTER adapter, netbuf_ctrl_block_t *netbuf_cb);
#endif

/***************** END DRIVER DATA STRUCTURE TEMPLATES ******************/
#endif
