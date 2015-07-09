/**
 *
 * @file   onebox_dev_ops.c
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
 * The file contains the initialization part of the SDBus driver and Loading of the 
 * TA firmware.
 */

/* include files */
#include "onebox_common.h"
#include "onebox_hal.h"
#include "onebox_linux.h"
#include "onebox_pktpro.h"
#include <linux/usb.h>

#if 0
ONEBOX_EXTERN uint8 process_usb_rcv_pkt(PONEBOX_ADAPTER adapter, uint32 pkt_len, uint8 pkt_type);
#endif

ONEBOX_EXTERN uint8 deploy_packet_to_assets(PONEBOX_ADAPTER adapter, netbuf_ctrl_block_t *nb_deploy);
ONEBOX_EXTERN uint8 process_unaggregated_pkt(PONEBOX_ADAPTER adapter, netbuf_ctrl_block_t *nb_deploy, int32 total_len);

ONEBOX_STATUS load_data_master_write(PONEBOX_ADAPTER adapter, uint32 base_address, uint32 instructions_sz,
	uint32 block_size, uint8 *ta_firmware)
{
	uint32 num_blocks;
	uint16 msb_address;
	uint32 cur_indx , ii;
	uint8  temp_buf[256];
	num_blocks = instructions_sz / block_size;
	msb_address = base_address >> 16;
	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,(TEXT("num_blocks: %d\n"),num_blocks));

	for (cur_indx = 0,ii = 0; ii < num_blocks; ii++,cur_indx += block_size)
	{
		adapter->os_intf_ops->onebox_memset(temp_buf, 0, block_size);
		adapter->os_intf_ops->onebox_memcpy(temp_buf, ta_firmware + cur_indx, block_size);
		if (adapter->osd_host_intf_ops->onebox_ta_write_multiple(adapter,
					base_address,
					(uint8 *)(temp_buf),
					block_size)
				!=ONEBOX_STATUS_SUCCESS)
		{
			return ONEBOX_STATUS_FAILURE;
		}      
		ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
				(TEXT("%s: loading block: %d\n"), __func__,ii));
		base_address += block_size;
	}

	if (instructions_sz % block_size)
	{
		adapter->os_intf_ops->onebox_memset(temp_buf, 0, block_size);
		adapter->os_intf_ops->onebox_memcpy(temp_buf,
				ta_firmware + cur_indx,
				instructions_sz % block_size);
		if (adapter->osd_host_intf_ops->onebox_ta_write_multiple(adapter,
					base_address,
					(uint8 *)(temp_buf),
					instructions_sz % block_size
					)!=ONEBOX_STATUS_SUCCESS)
		{
			return ONEBOX_STATUS_FAILURE;
		}
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
				(TEXT("*Written Last Block in Address 0x%x Successfully**\n"),
				 cur_indx | SD_REQUEST_MASTER));
	}
	return ONEBOX_STATUS_SUCCESS;
}

void interrupt_handler(PONEBOX_ADAPTER adapter)
{
	/*Dummy for USB*/
	return;
}

int32 ta_reset_ops(PONEBOX_ADAPTER adapter)
{
	/*Dummy for USB*/
	return ONEBOX_STATUS_SUCCESS;
}

/**
 * This is a kernel thread to receive the packets from the device
 *
 * @param
 *  data Pointer to driver's private data
 * @return
 *  None
 */
void usb_rx_scheduler_thread(void *data)
{
	PONEBOX_ADAPTER adapter = (PONEBOX_ADAPTER)data;
#if 0
	uint16 polling_flag = 0;
	uint32 rcv_len = 0;
	uint8 pkt_type = 0;
	uint8 ep_num = 0;
	uint32 pipe = 0;
	struct urb *urb;
	struct usb_host_endpoint *hep;
#else
	netbuf_ctrl_block_t *netbuf_cb = NULL;
	uint8 s = ONEBOX_STATUS_SUCCESS;
#endif	
	FUNCTION_ENTRY(ONEBOX_ZONE_DEBUG);
	do
	{

		adapter->os_intf_ops->onebox_wait_event(&adapter->usb_rx_scheduler_event, EVENT_WAIT_FOREVER);
		adapter->os_intf_ops->onebox_reset_event(&adapter->usb_rx_scheduler_event);
		if(adapter->usb_rx_thread_exit)
		{
			goto data_fail;
		}
		while (adapter->os_intf_ops->onebox_netbuf_queue_len(&adapter->deferred_rx_queue)) {
			netbuf_cb = adapter->os_intf_ops->onebox_dequeue_pkt(&adapter->deferred_rx_queue);
			if (!netbuf_cb) {
				printk("HAL : Invalid netbuf_cb from deferred_rx_queue \n");
				break;
			}

			s = deploy_packet_to_assets(adapter, netbuf_cb);
			if (s) {
				printk("FAILED TO DEPLOY packet[%p]\n", netbuf_cb);
				continue;
			}
		}
	}while(adapter->os_intf_ops->onebox_atomic_read(&adapter->rxThreadDone) == 0);//FIXME: why not thread_exit 

data_fail:	
	ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s: Terminated thread"
					"Data Pkt\n"), __func__));
	complete_and_exit(&adapter->rxThreadComplete, 0);
	return ;
}

uint8 process_usb_rcv_pkt(PONEBOX_ADAPTER adapter, netbuf_ctrl_block_t *netbuf_recv_pkt, uint8 pkt_type)
{
	if (ZIGB_PKT == pkt_type) {
#ifndef USE_INTCONTEXT
		adapter->os_intf_ops->onebox_netbuf_queue_tail(&adapter->deferred_rx_queue,
				netbuf_recv_pkt->pkt_addr);
		adapter->os_intf_ops->onebox_set_event(&adapter->usb_rx_scheduler_event);
#else
		ONEBOX_STATUS status = ONEBOX_STATUS_SUCCESS;
		status = deploy_packet_to_assets(adapter, netbuf_recv_pkt);
		if (status) {
			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("FAILED TO DEPLOY part of aggr packet[%p]\n"), netbuf_recv_pkt));
			return ONEBOX_STATUS_FAILURE;
		}
		return ONEBOX_STATUS_SUCCESS;
#endif			
	}
	else {
		/* Handling unaggregated packets(As No aggregation in USB), in BT/WLAN protocols */
		process_unaggregated_pkt(adapter, netbuf_recv_pkt, netbuf_recv_pkt->len);
	}

	FUNCTION_EXIT(ONEBOX_ZONE_INFO);

	return ONEBOX_STATUS_SUCCESS;
}
EXPORT_SYMBOL(process_usb_rcv_pkt);
