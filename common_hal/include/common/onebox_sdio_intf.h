/**
 * @file onebox_sdio_intf.h
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
 * This file contians the function prototypes of related to sdio interface
 * 
 */
#ifndef __ONEBOX_SDIO_INTF__
#define __ONEBOX_SDIO_INTF__

#ifdef ARASAN_SDIO_STACK
#include "sdio.h"
#include "sdcard.h"
#include "sdio_hci.h"
#include "srb.h"
#include "sdioctl.h"
#else
#if((LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,18))&& \
   (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,23)))
#include <linux/sdio/ctsystem.h>
#include <linux/sdio/sdio_busdriver.h>
#include <linux/sdio/_sdio_defs.h>
#include <linux/sdio/sdio_lib.h>
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26))
#include <linux/mmc/card.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/mmc/sdio.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio_ids.h>
#endif
#endif
#ifdef USE_USB_INTF
#include <linux/usb.h>
#endif

#define ULP_RESET_REG 			0x161
#define WATCH_DOG_TIMER_1		0x16c
#define WATCH_DOG_TIMER_2		0x16d
#define WATCH_DOG_DELAY_TIMER_1		0x16e
#define WATCH_DOG_DELAY_TIMER_2		0x16f
#define WATCH_DOG_TIMER_ENABLE		0x170

#define RESTART_WDT			BIT(11)
#define BYPASS_ULP_ON_WDT		BIT(1)

#define RF_SPI_PROG_REG_BASE_ADDR	0x40080000

#define GSPI_CTRL_REG0			(RF_SPI_PROG_REG_BASE_ADDR)
#define GSPI_CTRL_REG1			(RF_SPI_PROG_REG_BASE_ADDR + 0x2)
#define GSPI_DATA_REG0			(RF_SPI_PROG_REG_BASE_ADDR + 0x4)	
#define GSPI_DATA_REG1			(RF_SPI_PROG_REG_BASE_ADDR + 0x6)
#define GSPI_DATA_REG2			(RF_SPI_PROG_REG_BASE_ADDR + 0x8)

#define GSPI_DMA_MODE                   BIT(13)

#define GSPI_2_ULP			BIT(12)
#define GSPI_TRIG			BIT(7)	
#define GSPI_READ			BIT(6)
#define GSPI_RF_SPI_ACTIVE		BIT(8)
/*CONTENT OF THIS REG IS WRITTEN TO ADDRESS IN SD_CSA_PTR*/

#define SD_REQUEST_MASTER                 0x10000

#define ONEBOX_SDIO_BLOCK_SIZE       256

#define onebox_likely(a)            likely(a)

/* Abort related */
//#define SDIO_REG_ABORT_FEEDBACK    0xF1
//#define MAX_ABORT_FEEDBACK_RETRY    0x05
//#define SDIO_ABORT_FEEDBACK_VALUE   0x11


/* FOR SD CARD ONLY */
#define SDIO_RX_NUM_BLOCKS_REG     0x000F1
#define SDIO_FW_STATUS_REG         0x000F2
#define SDIO_NXT_RD_DELAY2         0x000F5     /* Next read delay 2 */
#define SDIO_FUN1_INT_REG          0x000F9     /* Function interrupt register*/
#define SDIO_READ_START_LVL        0x000FC
#define SDIO_READ_FIFO_CTL         0x000FD
#define SDIO_WRITE_FIFO_CTL        0x000FE
#define SDIO_WAKEUP_REG            0x000FF


/* common registers in SDIO function1 */
#define SDIO_FUN1_INTR_CLR_REG     0x0008
#define TA_SOFT_RESET_REG          0x0004
#define TA_TH0_PC_REG              0x0400
#define TA_HOLD_THREAD_REG         0x0844	
#define TA_RELEASE_THREAD_REG      0x0848
#define TA_POLL_BREAK_STATUS_REG   0x085C

/* WLAN registers in SDIO function 1 */
#define SDIO_RF_CNTRL_REG       0x0000000C     /* Lower MAC control register-1 */
#define SDIO_LMAC_CNTRL_REG     0x00000024    /* Lower MAC control register */
#define SDIO_LMAC_LOAD_REG      0x00000020    /* Lower MAC load register */
#define SDIO_TCP_CHK_SUM        0x0000006C     /* TCP check sum enable register */

#define RF_SELECT               0x0000000c  /* RF Select Register */

/* SDIO STANDARD CARD COMMON CNT REG(CCCR) */

/*IN THESE REGISTERS EACH BIT(0-7) REFERS TO A FUNCTION */

#define CCCR_REVISION       0x00
#define SD_SPEC_REVISION    0x01
#define SD_IO_ENABLE        0x02
#define SD_IO_READY         0x03
#define SD_INT_ENABLE       0x04
#define SD_INT_PENDING      0x05
#define SD_IO_ABART         0x06
#define SD_BUS_IF_CNT       0x07
#define SD_CARD_CAPABILITY  0x08

/*PTR to CARD'S COMMON CARD INFO STRUCT(CIS):0x09-0x0B*/
#define SD_CIS_PTR          0x09 
#define SD_BUS_SUSPEND      0x0C
#define SD_FUNCTION_SELEC   0x0D
#define SD_EXEC_FLAGS       0x0E
#define SD_READY_FLAGS      0x0F
#define SD_FN0_BLK_SZ       0x10 /*FUNCTION0 BLK SIZE:0x10-0x11 */
#define SD_RESERVED         0x12 /*0x12-0xFF:reserved for future */
#define SDIO_REG_HIGH_SPEED 0x13


/* SDIO_FUN1_FIRM_LD_CTRL_REG register bits */

#define TA_SOFT_RST_CLR      0
#define TA_SOFT_RST_SET      BIT(0)
#define TA_PC_ZERO           0
#define TA_HOLD_THREAD_VALUE        0xF
#define TA_RELEASE_THREAD_VALUE     0xF
#define TA_DM_LOAD_CLR       BIT(21)
#define TA_DM_LOAD_SET       BIT(20)

/* Function prototypes */
struct onebox_osd_host_intf_operations *onebox_get_osd_host_intf_operations(void);
struct onebox_osi_host_intf_operations *onebox_get_osi_host_intf_operations(void);
struct onebox_os_intf_operations *onebox_get_os_intf_operations(void);
struct onebox_os_intf_operations *onebox_get_os_intf_operations_from_origin(void);

ONEBOX_STATUS read_register(PONEBOX_ADAPTER adapter, uint32 Addr,uint8 *data);  
int write_register(PONEBOX_ADAPTER adapter,uint8 reg_dmn,uint32 Addr,uint8 *data);
ONEBOX_STATUS host_intf_read_pkt(PONEBOX_ADAPTER adapter,uint8 *pkt,uint32 Len);
ONEBOX_STATUS host_intf_write_pkt(PONEBOX_ADAPTER adapter,uint8 *pkt,uint32 Len, uint8 queueno);
ONEBOX_STATUS send_pkt_to_coex(netbuf_ctrl_block_t* pkt, uint8 hal_queue);
ONEBOX_STATUS get_pkt_from_coex(PONEBOX_ADAPTER adapter,
				netbuf_ctrl_block_t *netbuf_cb);
ONEBOX_STATUS read_register_multiple(PONEBOX_ADAPTER adapter, 
                                     uint32 Addr,
                                     uint32 Count,
                                     uint8 *data );
ONEBOX_STATUS write_register_multiple(PONEBOX_ADAPTER adapter,
                                      uint32 Addr,
                                      uint8 *data,
                                      uint32 Count);
ONEBOX_STATUS ack_interrupt(PONEBOX_ADAPTER adapter,uint8 INT_BIT);
#ifdef USE_USB_INTF
void onebox_rx_urb_submit (PONEBOX_ADAPTER adapter, struct urb *urb, uint8 ep_num);
ONEBOX_STATUS write_ta_register_multiple(PONEBOX_ADAPTER adapter,
                                      uint32 Addr,
                                      uint8 *data,
                                      uint32 Count);
ONEBOX_STATUS read_ta_register_multiple(PONEBOX_ADAPTER adapter,
                                      uint32 Addr,
                                      uint8 *data,
                                      uint32 Count);
#endif

int register_driver(void);
void unregister_driver(void);
ONEBOX_STATUS remove(void);
#if KERNEL_VERSION_BTWN_2_6_(18, 22)
struct net_device * onebox_getcontext(PSDFUNCTION pfunction);
#elif KERNEL_VERSION_GREATER_THAN_2_6_(26)
void* onebox_getcontext(struct sdio_func *pfunction);
#endif
#if KERNEL_VERSION_BTWN_2_6_(18, 22)
VOID onebox_sdio_claim_host(PSDFUNCTION pFunction);
#elif KERNEL_VERSION_GREATER_THAN_2_6_(26)
VOID onebox_sdio_claim_host(struct sdio_func *pfunction);
#endif
#if KERNEL_VERSION_BTWN_2_6_(18, 22)
VOID onebox_sdio_release_host(PSDFUNCTION pFunction);
#elif KERNEL_VERSION_GREATER_THAN_2_6_(26)
VOID onebox_sdio_release_host(struct sdio_func *pfunction);
#endif
#if KERNEL_VERSION_BTWN_2_6_(18, 22)
VOID onebox_setcontext(PSDFUNCTION pFunction, void *adapter);
#elif KERNEL_VERSION_GREATER_THAN_2_6_(26)
VOID onebox_setcontext(struct sdio_func *pfunction, void *adapter);
#endif
ONEBOX_STATUS onebox_setupcard(PONEBOX_ADAPTER adapter);
ONEBOX_STATUS init_host_interface(PONEBOX_ADAPTER adapter);
#if KERNEL_VERSION_BTWN_2_6_(18, 22)
int32 onebox_sdio_release_irq(PSDFUNCTION pFunction);
#elif KERNEL_VERSION_GREATER_THAN_2_6_(26)
int32 onebox_sdio_release_irq(struct sdio_func *pfunction);
#endif

int32 deregister_sdio_irq(PONEBOX_ADAPTER adapter);

ONEBOX_STATUS onebox_setclock(PONEBOX_ADAPTER adapter, uint32 Freq);

ONEBOX_STATUS onebox_abort_handler(PONEBOX_ADAPTER Adapter );
ONEBOX_STATUS onebox_init_sdio_slave_regs(PONEBOX_ADAPTER adapter);
ONEBOX_STATUS disable_sdio_interrupt(PONEBOX_ADAPTER adapter);

#ifdef USE_USB_INTF
uint8 process_usb_rcv_pkt(PONEBOX_ADAPTER adapter, netbuf_ctrl_block_t *netbuf_recv_pkt, uint8 pkt_type);
ONEBOX_STATUS onebox_ulp_reg_write (struct usb_device *usbdev, uint32 reg,uint16 value, uint16 len);
ONEBOX_STATUS onebox_ulp_reg_read (struct usb_device *usbdev, uint32 reg, uint16 * value, uint16 len);
void onebox_rx_done_handler (struct urb *urb);
#endif

#endif
