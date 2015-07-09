
/**
 *
 * @file   onebox_debug.c
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
#ifdef USE_USB_INF
#include <linux/usb.h>
#endif

//void onebox_print_mac_address(PONEBOX_ADAPTER adapter, uint8 mac_addr[ETHER_ADDR_LEN])
void onebox_print_mac_address(PONEBOX_ADAPTER adapter, uint8 *mac_addr)
{

		uint32 ii = 0;

		printk("\nThe MAc addess is ");
		for(; ii < ETHER_ADDR_LEN; ii++) {
			printk("%02x ", mac_addr[ii]);
		}
		printk("\n");

}

ONEBOX_STATUS onebox_send_debug_frame(PONEBOX_ADAPTER adapter, struct test_mode *test)
{
	onebox_mac_frame_t *mgmt_frame;
	ONEBOX_STATUS status;
	uint8  pkt_buffer[FRAME_DESC_SZ];

	mgmt_frame = (onebox_mac_frame_t *)pkt_buffer;
	adapter->os_intf_ops->onebox_memset(mgmt_frame, 0, FRAME_DESC_SZ);

	/* Bit{0:11} indicates length of the Packet
 	 * Bit{12:16} indicates host queue number
 	 */ 

	mgmt_frame->desc_word[0] = ONEBOX_CPU_TO_LE16(ONEBOX_WIFI_MGMT_Q << 12);
	/* Fill frame type for debug frame */
	mgmt_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(DEBUG_FRAME);
	/* Fill data in form of flags*/
	mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16(test->subtype);
	mgmt_frame->desc_word[5] = ONEBOX_CPU_TO_LE16(test->args);

	status = onebox_send_internal_mgmt_frame(adapter, (uint16 *)mgmt_frame, FRAME_DESC_SZ);
	return status;
	
}


ONEBOX_STATUS onebox_do_master_ops(PONEBOX_ADAPTER adapter,struct master_params_s *master ,uint16 type)
{
	int status = 0;
	int i;
	uint8 *data;
#ifdef USE_SDIO_INTF
	uint8  temp_buf[1000];
	uint16 msb_address = (master->address >> 16);
	uint16 lsb_address = (uint16)master->address;

#endif
	struct driver_assets *d_assets = onebox_get_driver_asset();
	//FIXME: Make use of spin locks before doing master reads/write's.

	//unsigned long flags = 0;
	//printk("In %s :Aquireing lock\n", __func__);
	//adapter->os_intf_ops->onebox_acquire_spinlock(&adapter->master_ops_lock, flags);
#ifdef USE_SDIO_INTF

	if (onebox_sdio_master_access_msword(adapter, msb_address) != ONEBOX_STATUS_SUCCESS)      
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,	(TEXT("%s: Unable to set ms word reg\n"), __func__));

		goto end_master_ops;
	}
#endif

	if(type == ONEBOX_MASTER_READ)
	{
		printk("In %s line %d master_read\n", __func__, __LINE__);
		data = kmalloc(master->no_of_bytes, GFP_KERNEL);
#ifdef USE_SDIO_INTF
		status = adapter->osd_host_intf_ops->onebox_read_multiple(d_assets->global_priv, lsb_address | SD_REQUEST_MASTER, master->no_of_bytes, (uint8 *)data);
#else
		status = adapter->osd_host_intf_ops->onebox_ta_read_multiple(d_assets->global_priv, master->address, (uint8 *)data, master->no_of_bytes);
#endif
	/*FIXME: mimic copy to user  functionality here*/
		memcpy(master->data, data, master->no_of_bytes);
		kfree(data);
	}
	else if(type == ONEBOX_MASTER_WRITE)
	{
		printk("In %s line %d master_write\n", __func__, __LINE__);
		for(i = 0; i < master->no_of_bytes; i++)
		{
			printk(" Data to write = 0x%x \n", master->data[i]);
		}
#ifdef USE_SDIO_INTF
		//FIXME:support for multiblock write need to added
		adapter->os_intf_ops->onebox_memset(temp_buf, 0, master->no_of_bytes);
		adapter->os_intf_ops->onebox_memcpy(temp_buf, (uint8 *)master->data, master->no_of_bytes);
		status = adapter->osd_host_intf_ops->onebox_write_multiple(d_assets->global_priv, 
		                                                        lsb_address | SD_REQUEST_MASTER,
									temp_buf, 
									master->no_of_bytes);
				
#else
		status = adapter->osd_host_intf_ops->onebox_ta_write_multiple(d_assets->global_priv, master->address, (uint8 *)master->data, master->no_of_bytes);
#endif
	}

#ifdef USE_SDIO_INTF
end_master_ops:
	if (onebox_sdio_master_access_msword(adapter, 0x4105) != ONEBOX_STATUS_SUCCESS)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,	(TEXT("%s: Unable to set ms word to common reg\n"), __func__));
		status = ONEBOX_STATUS_FAILURE;
		
	}
#endif
	//printk("In %s lIne %d :Releasing lock\n", __func__, __LINE__);
	//adapter->os_intf_ops->onebox_release_spinlock(&adapter->master_ops_lock, flags);
	//printk("In %s lIne %d :Released lock Succesfully\n", __func__, __LINE__);
	return status;
}

void dump_debug_frame(uint8 *msg, int32 msg_len)
{

		uint32 *debug;
		msg = msg +16;

		debug = (uint32 *)msg;
		printk("In %s Line %d Printing the debug frame contents\n", __func__, __LINE__);

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		             (TEXT("bg_state : %02x \n"), debug[0]));
		
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		             (TEXT("bgscan_flags : %02x \n"), (debug[1] >> 16)));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		             (TEXT("bgscan_nextchannel : %02x \n"), (*(uint16 *)&debug[1])));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		             (TEXT("TX buffers : %02x \n"), (debug[2] >> 16)));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		             (TEXT("RX buffers : %02x \n"), (*(uint16 *)&debug[2])));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		             (TEXT("Glbl buffers : %02x \n"), (debug[3])));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		             (TEXT("Tx Hostq_pkt_cnt : %02x \n"), (debug[4])));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("rx Hostq_pkt_cnt : %02x \n"), (debug[5])));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("BBP_CTRL_REG : %02x \n"), (debug[6])));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("ED REG : %02x \n"), (debug[7])));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("PPE PC : %02x \n"), (debug[8])));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("PPE PC : %02x \n"), (debug[9])));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("PPE PC : %02x \n"), (debug[10])));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("PPE PC : %02x \n"), (debug[11])));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("QUEUE_0 lmac_state  : %02x QUEUE_1 lmac_state  : %02x \n"), (debug[12] >> 16), *(uint16 *)&debug[12]));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("QUEUE_2 lmac_state  : %02x QUEUE_3 lmac_state  : %02x \n"), (debug[13] >> 16), *(uint16 *)&debug[13]));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("QUEUE_4 lmac_state  : %02x QUEUE_5 lmac_state  : %02x \n"), (debug[14] >> 16), *(uint16 *)&debug[14]));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("QUEUE_6 lmac_state  : %02x QUEUE_7 lmac_state  : %02x \n"), (debug[15] >> 16), *(uint16 *)&debug[15]));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("QUEUE_8 lmac_state  : %02x QUEUE_9 lmac_state  : %02x \n"), (debug[16] >> 16), *(uint16 *)&debug[16]));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("QUEUE_10 lmac_state  : %02x QUEUE_11 lmac_state  : %02x \n"), (debug[17] >> 16), *(uint16 *)&debug[17]));


		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("QUEUE_0 SW_q.pkt_cnt  : %02x QUEUE_0 dot11_q.pkt_cnt  : %02x \n"), debug[18] , debug[19]));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("QUEUE_1 SW_q.pkt_cnt  : %02x QUEUE_1 dot11_q.pkt_cnt  : %02x \n"), debug[20] , debug[21]));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("QUEUE_2 SW_q.pkt_cnt  : %02x QUEUE_2 dot11_q.pkt_cnt  : %02x \n"), debug[22] , debug[23]));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("QUEUE_3 SW_q.pkt_cnt  : %02x QUEUE_3 dot11_q.pkt_cnt  : %02x \n"), debug[24] , debug[25]));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("QUEUE_4 SW_q.pkt_cnt  : %02x QUEUE_4 dot11_q.pkt_cnt  : %02x \n"), debug[26] , debug[27]));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("QUEUE_5 SW_q.pkt_cnt  : %02x QUEUE_5 dot11_q.pkt_cnt  : %02x \n"), debug[28] , debug[29]));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("QUEUE_6 SW_q.pkt_cnt  : %02x QUEUE_6 dot11_q.pkt_cnt  : %02x \n"), debug[30] , debug[31]));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("QUEUE_7 SW_q.pkt_cnt  : %02x QUEUE_7 dot11_q.pkt_cnt  : %02x \n"), debug[32] , debug[33]));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("QUEUE_8 SW_q.pkt_cnt  : %02x QUEUE_8 dot11_q.pkt_cnt  : %02x \n"), debug[34] , debug[35]));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("QUEUE_9 SW_q.pkt_cnt  : %02x QUEUE_9 dot11_q.pkt_cnt  : %02x \n"), debug[36] , debug[37]));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("QUEUE_10 SW_q.pkt_cnt  : %02x QUEUE_10 dot11_q.pkt_cnt  : %02x \n"), debug[38] , debug[39]));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("QUEUE_11 SW_q.pkt_cnt  : %02x QUEUE_11 dot11_q.pkt_cnt  : %02x \n"), debug[38] , debug[41]));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("Lmac_abort stats delay  : %02x \n"), debug[42]));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("Block_q_bitmap : %02x \n"), (debug[43] >> 16)));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("bgscan Block_q_bitmap : %02x \n"), (*(uint16 *)&debug[43])));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("host Block_q_bitmap : %02x  coex block_q_bitmap %02x \n"), (debug[44] >> 16),(*(uint16 *)&debug[44])));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("PS state: %02x  tx_pkt_ts %02x rx_pkt_ts %02x expected_dtim_tsf %02lx\n"), 
												(debug[45]),(debug[46]), debug[47], *(uint64 *)&debug[48]));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("scaheduler_info_g[0] th_int_event_map : %02x  th_int_mask_map1 : %02x th_int_map2 : %02x th_int_map1_poll : %02x \n")
										 , (debug[50]),(debug[51]), debug[52], debug[53]));
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("scaheduler_info_g[1] th_int_event_map : %02x  th_int_mask_map1 %02x th_int_map2 %02x th_int_map1_poll %02x \n")
										 , (debug[54]),(debug[55]), debug[56], debug[57]));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("scaheduler_info_g[2] th_int_event_map : %02x  th_int_mask_map1 %02x th_int_map2 %02x th_int_map1_poll %02x \n")
										 , (debug[58]),(debug[59]), debug[60], debug[61]));

		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
										(TEXT("scaheduler_info_g[3] th_int_event_map : %02x  th_int_mask_map1 %02x th_int_map2 %02x th_int_map1_poll %02x \n")
										 , (debug[62]),(debug[63]), debug[64], debug[65]));
		return;
}
