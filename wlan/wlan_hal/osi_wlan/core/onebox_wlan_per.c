/**
 * @file onebox_wlan_per.c
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
 * This file contians PER mode related functions 
 */

#include "onebox_core.h"
/**
 * This function is used to send a frame to the PPE for requesting Stats.
 *
 * @param Pointer to the adapter structure.
 * return ONEBOX_STATUS_SUCCESS on success else ONEBOX_STATUS_FAILURE.
 */
ONEBOX_STATUS stats_frame(PONEBOX_ADAPTER adapter)
{

	ONEBOX_STATUS status = 0;
	onebox_mac_frame_t *mgmt_frame;
	uint8  pkt_buffer[MAX_MGMT_PKT_SIZE];

	FUNCTION_ENTRY (ONEBOX_ZONE_MGMT_SEND);

	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
	             (TEXT("===> Sending PER STATS REQUEST FRAME <===\n")));

	/* Allocating Memory For Mgmt Pkt From Mgmt Free Pool */
	mgmt_frame = (onebox_mac_frame_t *)pkt_buffer;

	adapter->os_intf_ops->onebox_memset(mgmt_frame, 0, FRAME_DESC_SZ);
	
	/* Do not fill with the Zero length, eventhough there is no framebody  */
	/* If you fill Zero length, DMA will stuck*/
//	mgmt_frame->desc_word[0] = ONEBOX_CPU_TO_LE16(5);
	/* FrameType*/
	mgmt_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(STATS_REQUEST_FRAME);
	/* Indication to PPE to request statistics */
//	mgmt_frame->desc_word[2] = ONEBOX_CPU_TO_LE16(0x1);
	mgmt_frame->desc_word[0] = (ONEBOX_WIFI_MGMT_Q << 12);
	if (adapter->recv_stop)
	{
		//FIXME this is reserved field
		//set this value to Indicate firmware to stop sending stats
		mgmt_frame->desc_word[2] = ONEBOX_CPU_TO_LE16(1 << 8);
	}    
	status = onebox_send_internal_mgmt_frame(adapter, (uint16 *)mgmt_frame, (FRAME_DESC_SZ));
	return status;
}


/**
 * This function starts/stops the PER transmission mode (Burst/Continuous).
 *
 * @param Pointer to the adapter structure.
 * @return status 0 on success else -1 on failure.
 */
int start_per_tx(PONEBOX_ADAPTER adapter)
{
	ONEBOX_STATUS status = ONEBOX_STATUS_SUCCESS ;

	if (adapter->endpoint_params.enable)
	{  
		adapter->per_flag = 0;
		if (adapter->tx_running)
		{
			return ONEBOX_STATUS_FAILURE;
		}	

		if(adapter->fsm_state == FSM_MAC_INIT_DONE )
		{

			if (adapter->endpoint_params.channel == 0xFF) {
				if(adapter->devdep_ops->onebox_mgmt_send_bb_reset_req(adapter) != ONEBOX_STATUS_SUCCESS) {
					return ONEBOX_STATUS_FAILURE;
				}
			} else {
				adapter->devdep_ops->onebox_band_check(adapter);
				adapter->devdep_ops->onebox_set_channel(adapter, adapter->endpoint_params.channel);
				adapter->fsm_state = FSM_SCAN_CFM;		
				adapter->os_intf_ops->onebox_reset_event(&(adapter->bb_rf_event));
				adapter->os_intf_ops->onebox_wait_event(&(adapter->bb_rf_event), EVENT_WAIT_FOREVER);
				adapter->fsm_state = FSM_MAC_INIT_DONE;
			}
		}

		if ( (!adapter->endpoint_params.length))
		{    
			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("ONEBOX_set_per_tx_mode: Continuous TX len not supported%d\n"),
			                                       adapter->endpoint_params.length));
			return ONEBOX_STATUS_FAILURE;
		}    
    
		if (adapter->endpoint_params.mode == PER_CONT_MODE)
		{
			ONEBOX_DEBUG(ONEBOX_ZONE_OID, (TEXT(
			          "onebox_set_per_tx_mode: Enabling the PER continuous mode\n")));
			if (do_continuous_send(adapter))
			{
					ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT(
													"Cant send as do_continuous failed : PER\n")));
					return ONEBOX_STATUS_FAILURE;
			}
			adapter->tx_running = CONTINUOUS_RUNNING; //indicating PER_CONT_MODE
		}


		if (adapter->endpoint_params.mode == PER_BURST_MODE)
		{   
			onebox_send_per_frame(adapter,PER_BURST_MODE);
			if(adapter->endpoint_params.aggr_enable == 1)
			{
				send_per_ampdu_indiaction_frame(adapter);
				adapter->fsm_state = FSM_AMPDU_IND_SENT;
			}
			else
			{
				if (adapter->fsm_state == FSM_MAC_INIT_DONE)
				{	
					ONEBOX_DEBUG(ONEBOX_ZONE_OID, (TEXT("onebox_set_per_tx_mode: Enabling the PER burst mode\n")));
					if (adapter->os_intf_ops->onebox_init_thread(&(adapter->sdio_scheduler_thread_handle_per), 
                                                                   "PER THREAD", 
                                                                   0, 
                                                                   adapter->wlan_osd_ops->onebox_per_thread, 
                                                                   adapter) != 0)
					{
						ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("onebox_set_per_tx_mode: Unable to initialize thread\n")));
						return -EFAULT;
					}
					adapter->os_intf_ops->onebox_start_thread(&(adapter->sdio_scheduler_thread_handle_per));
					adapter->tx_running = BURST_RUNNING;//indicating PER_BURST_MODE
				}
				else
				{
					ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT(
									"Driver is not in MAC_INIT_DONE STATE\n")));
					return -EFAULT;
				}	
			}
			return 0;
		}
	}

	else
	{
		if(adapter->tx_running == BURST_RUNNING)
		{
			onebox_send_per_frame(adapter, PER_BURST_MODE);
			adapter->wlan_osd_ops->onebox_kill_per_thread(adapter);
		}
		else if(adapter->tx_running == CONTINUOUS_RUNNING)
		{
			onebox_send_per_frame(adapter, PER_CONT_MODE);
		}
		else
		{
			return ONEBOX_STATUS_FAILURE;
		}
		adapter->tx_running = 0;
		if(adapter->netbuf_cb_per)
		{
//			adapter->os_intf_ops->onebox_free_pkt(adapter, adapter->netbuf_cb_per, 0);
			adapter->total_per_pkt_sent = 0;
			adapter->netbuf_cb_per = NULL;
		}
		else
		{
			return ONEBOX_STATUS_FAILURE;
		}
	}
	return status;
}

/**
 * This function is used to start the burst mode.
 *
 * @param Pointer to the adapter structure.
 * @return ONEBOX_STATUS_SUCCESS on success else ONEBOX_STATUS_FAILURE.
 */
ONEBOX_STATUS start_per_burst(PONEBOX_ADAPTER adapter)
{
	if (adapter->endpoint_params.enable)
	{
		if (do_continuous_send(adapter))
		{
			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("onebox_do_continuous_send failed\n")));
			return ONEBOX_STATUS_FAILURE;
		}
	}
	return ONEBOX_STATUS_SUCCESS;
}

/**
 * This function transmits the prepared PER packet to the PPE.
 *
 * @param Pointer to the adapter structure.
 * return 0 on success else -1 on failure.
 */
int do_continuous_send(PONEBOX_ADAPTER adapter)
{
	int status;
	uint16 seq_num;
	uint8 device_buf_status = 0;
	uint8 extended_desc = 4;
	uint16 length;
	if(adapter->endpoint_params.aggr_enable == 1)
	{
		if(adapter->no_of_per_fragments != 1)
		{
			length = PER_AGGR_LIMIT_PER_PKT;
		}
		else
		{
			length = (adapter->endpoint_params.length - ((adapter->endpoint_params.aggr_count-1)*PER_AGGR_LIMIT_PER_PKT));
		}
                adapter->no_of_per_fragments--;

	}
	else
		length = (adapter->endpoint_params.length > 1536 ) ? 1536 : adapter->endpoint_params.length;
	if(1)//(!adapter->per_flag)
	{
		adapter->netbuf_cb_per = adapter->os_intf_ops->onebox_alloc_skb(length + FRAME_DESC_SZ + extended_desc - 4 /* CRC */);
		if (adapter->netbuf_cb_per == NULL) {
			printk("unable to allocate buffer for tx\n");
			return ONEBOX_STATUS_FAILURE;
		}
		adapter->os_intf_ops->onebox_memset(adapter->netbuf_cb_per->data, 0, ((length + FRAME_DESC_SZ + extended_desc - 4 /* CRC */)));
		adapter->os_intf_ops->onebox_add_data_to_skb(adapter->netbuf_cb_per, (length + FRAME_DESC_SZ + extended_desc - 4 /* CRC */));
		prepare_per_pkt(adapter, adapter->netbuf_cb_per);
		adapter->per_flag = 1;
		adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, (PUCHAR)adapter->netbuf_cb_per->data, (FRAME_DESC_SZ + extended_desc));
	}


	status = adapter->osd_host_intf_ops->onebox_read_register(adapter,
									adapter->buffer_status_register,
									&device_buf_status); /* Buffer */

	if (status != ONEBOX_STATUS_SUCCESS)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
                     (TEXT("%s: Failed to Read Intr Status Register\n"),__func__));
		return ONEBOX_STATUS_FAILURE;
	}

	if(device_buf_status & (1 << SD_PKT_BUFF_FULL))
	{
		if(!adapter->buffer_full)
		{
			adapter->buf_full_counter++;
		}
		adapter->buffer_full = 1;
		ONEBOX_DEBUG(ONEBOX_ZONE_DATA_DUMP, (TEXT( "PER B\n")));
		return ONEBOX_STATUS_SUCCESS;
	}
	else
	{
		adapter->buffer_full = 0;

	}
	if(adapter->netbuf_cb_per->len > (FRAME_DESC_SZ + MIN_802_11_HDR_LEN + extended_desc)) 
	{
		seq_num = (*(uint16 *)(&adapter->netbuf_cb_per->data[FRAME_DESC_SZ + MIN_802_11_HDR_LEN  + extended_desc - 2])) >> 4;
		seq_num = ((seq_num + 1) % 4096);

		adapter->netbuf_cb_per->data[FRAME_DESC_SZ + MIN_802_11_HDR_LEN + extended_desc - 2] = (seq_num << 4) & 0xff;
		adapter->netbuf_cb_per->data[FRAME_DESC_SZ + MIN_802_11_HDR_LEN + extended_desc - 1] = ((seq_num << 4) >> 8) ;
	}
	if(adapter->per_packet.enable)
		adapter->os_intf_ops->onebox_memcpy(&adapter->netbuf_cb_per->data[FRAME_DESC_SZ + extended_desc], &adapter->per_packet.packet[0], \
                                                   ((adapter->per_packet.length)>adapter->endpoint_params.length)?(adapter->endpoint_params.length):(adapter->per_packet.length));
	adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, (PUCHAR)adapter->netbuf_cb_per->data,(adapter->netbuf_cb_per->len));
	status = onebox_send_internal_mgmt_frame(adapter, 
                                           (uint16 *)adapter->netbuf_cb_per->data, 
                                           (adapter->netbuf_cb_per->len));
	if (status != ONEBOX_STATUS_SUCCESS) 
	{ 
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("%s: Failed To Write The Packet\n"),__func__));
		return ONEBOX_STATUS_FAILURE;
	}
	adapter->total_per_pkt_sent++;
	return ONEBOX_STATUS_SUCCESS;
}

/**
 * This function is used to prepare a packet for transmission.
 *
 * @param Pointer to the adapter structure.
 * @param Pointer to the netbuf control block structure.
 * @return ONEBOX_STATUS_SUCCESS.
 */
ONEBOX_STATUS prepare_per_pkt(PONEBOX_ADAPTER adapter, netbuf_ctrl_block_t *netbuf_cb)
{
	uint32 *frame_desc,temp_word;
	struct ieee80211_frame *tmp_hdr = NULL;
	uint8  size_of_hdr; // frame_desc + MAC_HDR + Extended_desc   16 + 24 + 4
	/* For future use */
	uint8 extended_desc = 4;

	uint8 mac_addr[6] = {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5};
	uint8 mac_hdr[24] = {0x8, 0x1, 0, 0, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5,
                         0x0, 0x23, 0xa7, 0xa1, 0xa2, 0xa3, 0x0, 0x23, 0xa7, 0xa1, 0xa2, 0xa3, 0 ,0};
	uint8 offset = 0, min_len = 0;
	uint32 rate_flags, greenfield, ch_bw, i = 0;  
	size_of_hdr = FRAME_DESC_SZ + extended_desc + MIN_802_11_HDR_LEN;

#if 1 //FIXME Below code is not required as data area of SKB has enough space already to hold FRAME_DESC_SZ and extended_desc.
	if (netbuf_cb->len < size_of_hdr) // frame_desc + MAC_HDR + Extended_desc   16 + 24 + 4
	{
		min_len = (netbuf_cb->len  - FRAME_DESC_SZ - extended_desc);
	}
#endif
	frame_desc = (uint32 *)&netbuf_cb->data[0];
	adapter->os_intf_ops->onebox_memset((uint8 *)frame_desc, 0, (FRAME_DESC_SZ + extended_desc));

	temp_word = (netbuf_cb->len - FRAME_DESC_SZ) & 0xffff;
	frame_desc[0] = ONEBOX_CPU_TO_LE32(temp_word);
	if (netbuf_cb->len >= (MIN_802_11_HDR_LEN + FRAME_DESC_SZ + extended_desc))
	{
		frame_desc[1] = ONEBOX_CPU_TO_LE32( MIN_802_11_HDR_LEN << 8);
	}
	else
	{
		frame_desc[1] = ONEBOX_CPU_TO_LE32( adapter->per_packet.length << 8);
	} 
	if((adapter->per_packet.enable && !adapter->per_packet.insert_seq) || !adapter->per_packet.enable)
		frame_desc[1] |= (BIT(2) << 16);  //insert seq_no
	if(adapter->endpoint_params.aggr_enable) 
	{
		temp_word = (( ENABLE_MAC_INFO) << 16); //In mac_info set bit0 and bit9 for bcast pkt
		frame_desc[3] |= ONEBOX_CPU_TO_LE32(QOS_EN); // Indicating of QOS in mac_flags
	}
	else
	temp_word = ((BROADCAST_IND | ENABLE_MAC_INFO) << 16); //In mac_info set bit0 and bit9 for bcast pkt
	rate_flags = (adapter->endpoint_params.rate_flags << 2);
/*FIXME: Put macros for these mask values */
	ch_bw	= (rate_flags & 0x00F0);
	greenfield  = (rate_flags & 0x0008);
	greenfield  = (greenfield << 17);
	if (rate_flags & 0x0004)//checking short_GI
	{
		adapter->endpoint_params.rate |= BIT(9);
	}  
	if (adapter->endpoint_params.mode)
	{
		rate_flags |= CONTINUOUS_MODE;
	} 

  /* Rates are sent in word 5 and channel bandwidth in word 6
   * */
	frame_desc[1] |= ONEBOX_CPU_TO_LE32(temp_word | extended_desc);
	frame_desc[2] = ONEBOX_CPU_TO_LE32((adapter->endpoint_params.rate & 0x3ff) | (ch_bw  << 12) | greenfield);
	frame_desc[4] = ONEBOX_CPU_TO_LE32(adapter->endpoint_params.power );
	if (adapter->endpoint_params.mode == PER_CONT_MODE)
	{	
		frame_desc[4] |= ONEBOX_CPU_TO_LE32(3 << 8);
		ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT(" CONTINUOUS PER MODE STARTED\n")));
	}
	offset += FRAME_DESC_SZ + extended_desc;
	/* Form the Mac header for the PER Packet to be transmitted */
	tmp_hdr = (struct ieee80211_frame *)&netbuf_cb->data[offset];

	if (netbuf_cb->len >= size_of_hdr)
	{
		tmp_hdr->i_fc[0]=0x8;
		tmp_hdr->i_fc[1]=0x01;
		tmp_hdr->i_dur[0]=0x00;
		tmp_hdr->i_dur[1]=0x00;

		adapter->os_intf_ops->onebox_memcpy(tmp_hdr->i_addr1, mac_addr, MAC_ADDR_LEN);
		adapter->os_intf_ops->onebox_memcpy(tmp_hdr->i_addr2, adapter->mac_addr, MAC_ADDR_LEN);
		adapter->os_intf_ops->onebox_memcpy(tmp_hdr->i_addr3, adapter->mac_addr, MAC_ADDR_LEN);

		offset += MIN_802_11_HDR_LEN;

		/*Fill the PER packet with dummy data */
		//adapter->os_intf_ops->onebox_memset(&netbuf_cb->data[offset], 0xdd, (netbuf_cb->len - offset));
		for (i = offset; i <= (netbuf_cb->len); i += 4)
		{
			netbuf_cb->data[i] = 0xff;
			netbuf_cb->data[i + 1] = 0x00;
			netbuf_cb->data[i + 2] = 0xbb;
			netbuf_cb->data[i + 3] = 0x55;
		}
		if (((netbuf_cb->len - offset)%4))
			adapter->os_intf_ops->onebox_memset(&netbuf_cb->data[i -4], 0xdd, ((netbuf_cb->len - offset)%4));
	}
	else
	{
		adapter->os_intf_ops->onebox_memcpy(tmp_hdr, mac_hdr, min_len);
	}
	netbuf_cb->data[1] |= (ONEBOX_WIFI_DATA_Q << 4); 
	if(adapter->endpoint_params.aggr_enable) /* FIXME why if and else cases are same */
		netbuf_cb->data[14] |= 1;// send pkts in queue no: 3
	else
		netbuf_cb->data[14] |= 1;// send pkts in queue no: 5 
	return ONEBOX_STATUS_SUCCESS;
}


/**
 * This function is used to send AMPDU Indication frame in PER mode.
 *
 * @param Pointer to the adapter structure.
 * @return ONEBOX_STATUS_SUCCESS on success else ONEBOX_STATUS_FAILURE.
 */
ONEBOX_STATUS send_per_ampdu_indiaction_frame(PONEBOX_ADAPTER adapter)
{
	onebox_mac_frame_t *mgmt_frame;
	ONEBOX_STATUS status;
	uint8  pkt_buffer[FRAME_DESC_SZ];

	FUNCTION_ENTRY (ONEBOX_ZONE_MGMT_SEND);

	ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
	             (TEXT("===> Frame to Send AMPDU IndiCATIONframe in PER <===\n")));
	
	mgmt_frame = (onebox_mac_frame_t *)pkt_buffer;

	adapter->os_intf_ops->onebox_memset(mgmt_frame, 0, FRAME_DESC_SZ);

	/* FrameType*/
	mgmt_frame->desc_word[0] = ONEBOX_CPU_TO_LE16((sizeof(mgmt_frame->u.ampdu_ind)) | ONEBOX_WIFI_MGMT_Q << 12);
	mgmt_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(AMPDU_IND); 
	//mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16(ni->hal_priv_node.tid[tidno].seq_start); 
  mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16(0); //tid
	mgmt_frame->desc_word[5] = ONEBOX_CPU_TO_LE16(adapter->endpoint_params.aggr_count); //baw_size 
	mgmt_frame->desc_word[7] = ONEBOX_CPU_TO_LE16(0 | (START_AMPDU_AGGR << 4)| ( 0 << 8)); 

	ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT(
					 " ampdu indication frame sent in %s \n"), __func__));
	adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, (PUCHAR)mgmt_frame, FRAME_DESC_SZ);
	status = onebox_send_internal_mgmt_frame(adapter, (uint16 *)mgmt_frame, (FRAME_DESC_SZ));
	return status;


}


/**
 * This function is used to send PER frame for Burst/Continuous mode.
 *
 * @param Pointer to the adapter structure.
 * @return ONEBOX_STATUS_SUCCESS on success else ONEBOX_STATUS_FAILURE.
 */
ONEBOX_STATUS onebox_send_per_frame(PONEBOX_ADAPTER adapter,uint8 mode)
{
	onebox_mac_frame_t *mgmt_frame;
	ONEBOX_STATUS status;
	uint8  pkt_buffer[MAX_MGMT_PKT_SIZE];

	FUNCTION_ENTRY (ONEBOX_ZONE_MGMT_SEND);

	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
	             (TEXT("===> Frame to Send PER Frame <===\n")));
	
	mgmt_frame = (onebox_mac_frame_t *)pkt_buffer;

	adapter->os_intf_ops->onebox_memset(mgmt_frame, 0, FRAME_DESC_SZ);

	/* FrameType*/
	mgmt_frame->desc_word[0] = (ONEBOX_WIFI_MGMT_Q << 12 | sizeof(struct per_params_s));
	mgmt_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(PER_CMD_PKT);
	mgmt_frame->desc_word[3] = ONEBOX_CPU_TO_LE16(mode);
	memcpy(&pkt_buffer[16],(uint8 *)(&adapter->endpoint_params),sizeof(per_params));

	ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT(
					 " Sending PER frame \n")));
	adapter->core_ops->onebox_dump(ONEBOX_ZONE_MGMT_SEND, (PUCHAR)mgmt_frame, FRAME_DESC_SZ);
	status = onebox_send_internal_mgmt_frame(adapter, (uint16 *)mgmt_frame, (FRAME_DESC_SZ + sizeof(per_params)));
	return status;
}

