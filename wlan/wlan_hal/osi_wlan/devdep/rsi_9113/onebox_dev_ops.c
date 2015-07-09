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

/**
 * This function prepares the netbuf control block
 *
 * @param 
 * adapter pointer to the driver private structure
 * @param 
 * buffer pointer to the packet data
 * @param 
 * len length of the packet 
 * @return .This function returns ONEBOX_STATUS_SUCCESS.
 */

#if 0
static netbuf_ctrl_block_t * prepare_netbuf_cb(PONEBOX_ADAPTER adapter, uint8 *buffer,
                      uint32 pkt_len, uint8 extended_desc)
{
	netbuf_ctrl_block_t *netbuf_cb  = NULL;
	uint8 payload_offset;
	pkt_len -= extended_desc;
#ifdef BYPASS_RX_DATA_PATH
	netbuf_cb = adapter->os_intf_ops->onebox_alloc_skb(pkt_len + FRAME_DESC_SZ);
#else
	netbuf_cb = adapter->os_intf_ops->onebox_alloc_skb(pkt_len);
#endif
	payload_offset = extended_desc + FRAME_DESC_SZ;
	if(netbuf_cb == NULL)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("@@Error while allocating skb in %s:\n"), __func__));
		return NULL;
	}
	/* Preparing The Skb To Indicate To core */
	netbuf_cb->dev = adapter->dev;
	adapter->os_intf_ops->onebox_add_data_to_skb(netbuf_cb, pkt_len);
	//(*(uint16 *)(netbuf_cb->data)) = (*((uint16 *)buffer));
#ifdef BYPASS_RX_DATA_PATH
	netbuf_cb->len = pkt_len + FRAME_DESC_SZ;
	adapter->os_intf_ops->onebox_memcpy((netbuf_cb->data), (buffer), FRAME_DESC_SZ);
	adapter->os_intf_ops->onebox_memcpy((netbuf_cb->data) + FRAME_DESC_SZ, (buffer + payload_offset), netbuf_cb->len - FRAME_DESC_SZ);
#else
	netbuf_cb->len = pkt_len;
	adapter->os_intf_ops->onebox_memcpy((netbuf_cb->data), (buffer + payload_offset), netbuf_cb->len);
#endif
	//printk("Pkt to be indicated to Net80211 layer is len =%d\n", netbuf_cb->len);
	//adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, netbuf_cb->data, netbuf_cb->len);
	return netbuf_cb;
}
#endif

/**
 * This function read frames from the SD card.
 *
 * @param  Pointer to driver adapter structure.  
 * @param  Pointer to received packet.  
 * @param  Pointer to length of the received packet.  
 * @return 0 if success else -1. 
 */
ONEBOX_STATUS wlan_read_pkt(PONEBOX_ADAPTER adapter, 
				netbuf_ctrl_block_t *netbuf_cb)
{
	uint32 queueno;
	uint8 extended_desc;
//	uint32 pkt_len;
	uint8 *frame_desc_addr = netbuf_cb->data;
//	uint8 *frame_desc_addr = msg;
	uint32 length = 0;
	uint16 offset =0;
#ifndef BYPASS_RX_DATA_PATH
	netbuf_ctrl_block_t *netbuf_cb = NULL;
	struct ieee80211com *ic = &adapter->vap_com;
	struct ieee80211vap *vap = NULL;
	uint8 vap_id;
#endif

	FUNCTION_ENTRY(ONEBOX_ZONE_DATA_RCV);
	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,(TEXT("adapter = %p\n"),
	             adapter));
	queueno = (uint32)((*(uint16 *)&frame_desc_addr[offset] & 0x7000) >> 12);
	length   = (*(uint16 *)&frame_desc_addr[offset] & 0x0fff);
	extended_desc   = (*(uint8 *)&frame_desc_addr[offset + 4] & 0x00ff);
	ONEBOX_DEBUG(ONEBOX_ZONE_DATA_RCV, (TEXT("###Received in QNumber:%d Len=%d###!!!\n"), queueno, length));
	
	switch(queueno)
	{
		case ONEBOX_WIFI_DATA_Q:
		{
			/* Check if aggregation enabled */
			if (length > (ONEBOX_RCV_BUFFER_LEN * 4 ))
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
					  (TEXT("%s: Toooo big packet %d\n"), __func__,length));	    
				length = ONEBOX_RCV_BUFFER_LEN * 4 ;
			}
			if ((length < ONEBOX_HEADER_SIZE) || (length < MIN_802_11_HDR_LEN))
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
					(TEXT("%s: Too small packet %d\n"), __func__, length));
			   adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, frame_desc_addr, length);
			}
			
			if (!length)
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s: Dummy pkt has comes in\n"), __func__));
			}
			//printk("***WLAN RECEIVED DATA PKT ***\n");
			ONEBOX_DEBUG(ONEBOX_ZONE_DATA_RCV,(TEXT("@@@ Rcvd Data Pkt b4 netbuf_cb preparation:\n"))); 
			adapter->core_ops->onebox_dump(ONEBOX_ZONE_DATA_RCV, (frame_desc_addr + offset), (length + extended_desc));
#if 0
#ifndef BYPASS_RX_DATA_PATH
			netbuf_cb = prepare_netbuf_cb(adapter, (frame_desc_addr + offset), length, extended_desc);
			if(netbuf_cb == NULL)
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_DATA_RCV,(TEXT("@@@ Error in preparing the rcv packet:\n"))); 
				return ONEBOX_STATUS_FAILURE;
			}
#endif
#endif
#ifdef BYPASS_RX_DATA_PATH
			onebox_reorder_pkt(adapter, netbuf_cb); /* coex */
			//onebox_reorder_pkt(adapter, (frame_desc_addr + offset)); /* coex */
			//onebox_reorder_pkt(adapter, (frame_desc_addr + offset));
#else
			adapter->core_ops->onebox_dump(ONEBOX_ZONE_MGMT_RCV, netbuf_cb->data, netbuf_cb->len);
#ifdef PWR_SAVE_SUPPORT
				TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) 
				{
					/* vap_id and check only if that vap_id matches and vap is of STA mode*/
					vap_id = ((netbuf_cb->data[14] & 0xf0) >> 4);	    
					if(vap && (vap->hal_priv_vap->vap_id == vap_id) 
							&& (vap->iv_opmode == IEEE80211_M_STA) 
							&& (TRAFFIC_PS_EN) 
							&& (ps_params_def.ps_en))
					{
						vap->check_traffic(vap, 0, netbuf_cb->len);
						break;
					}
				}
#endif
			/* As these are data pkts we are not sending rssi and chno vales */
			adapter->core_ops->onebox_indicate_pkt_to_net80211(adapter,
						   (netbuf_ctrl_block_t *)netbuf_cb, 0, 0);
#endif
		}
		break;

		case ONEBOX_WIFI_MGMT_Q:
		{
			onebox_mgmt_pkt_recv(adapter, netbuf_cb);
		}  
		break;

		default:
		{
			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s: pkt from invalid queue\n"), __func__)); 
			adapter->os_intf_ops->onebox_free_pkt(adapter, netbuf_cb, 0);
		}
		break;
	} /* End switch */      

	FUNCTION_EXIT(ONEBOX_ZONE_INFO);

	return ONEBOX_STATUS_SUCCESS;
}
EXPORT_SYMBOL(wlan_read_pkt);

/**
 * This function set the AHB master access MS word in the SDIO slave registers.
 *
 * @param  Pointer to the driver adapter structure. 
 * @param  ms word need to be initialized.
 * @return ONEBOX_STATUS_SUCCESS on success and ONEBOX_STATUS_FAILURE on failure. 
 */
ONEBOX_STATUS onebox_sdio_master_access_msword(PONEBOX_ADAPTER adapter,
                                               uint16 ms_word)
{
	UCHAR byte;
	uint8 reg_dmn;
	ONEBOX_STATUS  status=ONEBOX_STATUS_SUCCESS;

	reg_dmn = 0; //TA domain
	/* Initialize master address MS word register with given value*/
	byte=(UCHAR)(ms_word&0x00FF);
	ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
	             (TEXT("%s: MASTER_ACCESS_MSBYTE:0x%x\n"), __func__,byte));
	status = adapter->osd_host_intf_ops->onebox_write_register(adapter,reg_dmn,
	                                                           SDIO_MASTER_ACCESS_MSBYTE,
	                                                           &byte);             
	if(status != ONEBOX_STATUS_SUCCESS)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		             (TEXT("%s: fail to access MASTER_ACCESS_MSBYTE\n"), __func__));
		return ONEBOX_STATUS_FAILURE;
	}
	byte=(UCHAR)(ms_word >>8);
	ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
	             (TEXT("%s:MASTER_ACCESS_LSBYTE:0x%x\n"), __func__,byte));
	status = adapter->osd_host_intf_ops->onebox_write_register(adapter,reg_dmn,
	                                                           SDIO_MASTER_ACCESS_LSBYTE,
	                                                           &byte);
	if(status != ONEBOX_STATUS_SUCCESS)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		             (TEXT("%s: fail to access MASTER_ACCESS_LSBYTE\n"), __func__));
		return ONEBOX_STATUS_FAILURE;
	}
	return ONEBOX_STATUS_SUCCESS;
} /* End <onebox_sdio_master_access_msword */

/**
 * This function schedules the packet for transmission.
 *
 * @param  pointer to the adapter structure .  
 * @return Returns 8 if the file is of UNIX type else returns 9 if
 *         the file is of DOS  type . 
 */
void schedule_pkt_for_tx(PONEBOX_ADAPTER adapter)
{
		adapter->os_intf_ops->onebox_set_event(&(adapter->sdio_scheduler_event));
}

static void wlan_thread_waiting_for_event(PONEBOX_ADAPTER adapter)
{
				uint32 status= 0;

				if(!adapter->beacon_event)
				{
								if (adapter->buffer_full)   
								{
												/* Wait for 2ms, by the time firmware clears the buffer full interrupt */
												ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT("%s: Event wait for 2ms \n"), __func__));
												status = adapter->os_intf_ops->onebox_wait_event(&adapter->sdio_scheduler_event, 2);
								}
								else
								{
												adapter->os_intf_ops->onebox_wait_event(&adapter->sdio_scheduler_event, EVENT_WAIT_FOREVER);
								}
				}
				adapter->os_intf_ops->onebox_reset_event(&adapter->sdio_scheduler_event);

				adapter->sdio_thread_counter++;
}
/**
 * This is a kernel thread to send the packets to the device
 *
 * @param
 *  data Pointer to driver's private data
 * @return
 *  None
 */
void sdio_scheduler_thread(void *data)
{
	PONEBOX_ADAPTER adapter = (PONEBOX_ADAPTER)data;
	FUNCTION_ENTRY(ONEBOX_ZONE_DEBUG);

	do 
	{
		if (adapter->beacon_event)
		{
			adapter->beacon_event = 0;

			if (adapter->hal_vap[adapter->beacon_event_vap_id].vap != NULL) 
			{
				if (IEEE80211_IS_MODE_BEACON(adapter->hal_vap[adapter->beacon_event_vap_id].vap->iv_opmode)) 
				{
					//printk("Calling vap_load_beacon\n");
					adapter->core_ops->onebox_vap_load_beacon(adapter, adapter->beacon_event_vap_id);
					ONEBOX_DEBUG(ONEBOX_ZONE_INFO,(TEXT ("!!Enabling Beacon path!!!!!!!!!!!\n")));
				}
			}
		}	

		if(adapter->core_init_done)
		{
			adapter->core_ops->onebox_core_qos_processor(adapter);
		}

		wlan_thread_waiting_for_event(adapter);
		/* If control is coming up to end schedule again */

	} 
#if KERNEL_VERSION_BTWN_2_6_(18,26)
	while(!onebox_signal_pending());
#elif KERNEL_VERSION_GREATER_THAN_2_6_(27)
	while(adapter->os_intf_ops->onebox_atomic_read(&adapter->txThreadDone) == 0); 
	ONEBOX_DEBUG(ONEBOX_ZONE_WARN, 
	             (TEXT("sdio_scheduler_thread: Came out of do while loop\n")));
	adapter->os_intf_ops->onebox_completion_event(&adapter->txThreadComplete,0);
	ONEBOX_DEBUG(ONEBOX_ZONE_WARN, 
	             (TEXT("sdio_scheduler_thread: Completed onebox_completion_event \n")));
#endif
  
	FUNCTION_EXIT(ONEBOX_ZONE_DEBUG);
}


uint16* hw_queue_status(PONEBOX_ADAPTER adapter)
{

	return NULL;
}
