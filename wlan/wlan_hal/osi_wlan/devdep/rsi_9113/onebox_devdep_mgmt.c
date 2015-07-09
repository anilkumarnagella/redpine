/**
* @file onebox_devdep_mgmt.c
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
* The file contains the message information exchanged between the 
* driver and underlying device/
*/

#include "onebox_common.h"
#include "onebox_per.h"
#include "onebox_bootup_params.h"
#include "onebox_qspi.h"
#if 0
void send_sta_supported_features_at_timeout(struct ieee80211vap *vap)
{
		struct ieee80211com *ic = NULL;
		PONEBOX_ADAPTER adapter = NULL;

		if(is_vap_valid(vap) < 0) {
			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("ERROR: VAP is Not a valid pointer In %s Line %d, So returning\n "), 
									__func__, __LINE__));
			dump_stack();
			return;
		}
		printk("Initial Timeout expired\n");
	/*clear the timer flag*/
		ic = vap->iv_ic;
		adapter = (PONEBOX_ADAPTER)vap->hal_priv_vap->hal_priv_ptr;
		//adapter->sta_mode.initial_timer_running = 0;
		if(vap->iv_state == IEEE80211_S_RUN) {
				onebox_send_sta_supported_features(vap, adapter);
		}
		else {
			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("ERROR: Vap is not run state, so Sta supported features are not being send\n")));
		}

}

void initialize_sta_support_feature_timeout(struct ieee80211vap *vap, PONEBOX_ADAPTER adapter)
{
		if( (!adapter->sta_support_feature_send_timeout.function))
		{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Initializing feature support timeout \n")));
				adapter->os_intf_ops->onebox_init_sw_timer(&adapter->sta_support_feature_send_timeout, (uint32)vap,
								(void *)&send_sta_supported_features_at_timeout, 100);
		}
		else 
		{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Modifiying Initializing feature support timeout \n")));
				adapter->os_intf_ops->onebox_mod_timer(&adapter->sta_support_feature_send_timeout, msecs_to_jiffies(100));
		}
		adapter->sta_mode.initial_timer_running =1;
		driver_ps.delay_pwr_sve_decision_flag = 1;

}
#endif
/**
 * This function prepares the Baseband Programming request
 * frame and sends to the PPE
 *
 * @param
 *  adapter       Pointer to Driver Private Structure
 * @param
 *  bb_prog_vals  Pointer to bb programming values
 * @param
 *  num_of_vals   Number of BB Programming values
 *
 * @return
 *  Returns ONEBOX_STATUS_SUCCESS on success, or ONEBOX_STATUS_FAILURE
 *  on failure
 */
static ONEBOX_STATUS onebox_mgmt_send_bb_prog_frame(PONEBOX_ADAPTER adapter,
                                             uint16 *bb_prog_vals,
                                             uint16 num_of_vals)
{
	onebox_mac_frame_t *mgmt_frame;
	ONEBOX_STATUS status_l;
	uint16 frame_len;
	uint16 count;
	uint8  pkt_buffer[MAX_MGMT_PKT_SIZE];
	uint8 ii = 0;

	FUNCTION_ENTRY (ONEBOX_ZONE_MGMT_SEND);

	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,
	             (TEXT("===> Sending Baseband Programming Packet <===\n")));

	/* Allocating Memory For Mgmt Pkt From Mgmt Free Pool */
	mgmt_frame = (onebox_mac_frame_t *)pkt_buffer;

	adapter->os_intf_ops->onebox_memset(mgmt_frame, 0, MAX_MGMT_PKT_SIZE);

	adapter->core_ops->onebox_dump(ONEBOX_ZONE_MGMT_DUMP, (PUCHAR)bb_prog_vals, num_of_vals);
	/* Preparing BB Request Frame Body */
	for (count=1; ((count < num_of_vals) && (ii< num_of_vals)); ii++, count+=2) 
	{
		mgmt_frame->u.bb_prog_req[ii].reg_addr = ONEBOX_CPU_TO_LE16(bb_prog_vals[count]);
		mgmt_frame->u.bb_prog_req[ii].bb_prog_vals = ONEBOX_CPU_TO_LE16(bb_prog_vals[count+1]);
	}

	if (num_of_vals % 2)
	{
		mgmt_frame->u.bb_prog_req[ii].reg_addr = ONEBOX_CPU_TO_LE16(bb_prog_vals[count]);
	}
	/* Preparing BB Request Frame Header */
	frame_len = ((num_of_vals) * 2);	//each 2 bytes
	
	/*prepare the frame descriptor */
	mgmt_frame->desc_word[0] = ONEBOX_CPU_TO_LE16((frame_len) | (ONEBOX_WIFI_MGMT_Q << 12));
	mgmt_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(BB_PROG_VALUES_REQUEST);

	if (adapter->soft_reset & BBP_REMOVE_SOFT_RST_BEFORE_PROG)
	{
  		mgmt_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16(BBP_REMOVE_SOFT_RST_BEFORE_PROG);
	}	
	if (adapter->soft_reset & BBP_REMOVE_SOFT_RST_AFTER_PROG)
	{
  		mgmt_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16(BBP_REMOVE_SOFT_RST_AFTER_PROG);	
	}	
	
	if (adapter->bb_rf_rw)
	{		
  		mgmt_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16(BBP_REG_READ);
		adapter->bb_rf_rw = 0;
	}	
  	adapter->soft_reset = 0;
	
	//Flags are not handled FIXME:
	mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16(num_of_vals);
#ifdef RF_8230
	mgmt_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16 (PUT_BBP_RESET | BBP_REG_WRITE | (NONRSI_RF_TYPE << 4));
#else
	mgmt_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16 (PUT_BBP_RESET | BBP_REG_WRITE | (RSI_RF_TYPE << 4));
#endif  
	//FIXME: What is the radio id to fill here
//	mgmt_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16 (RADIO_ID << 8 );


	//adapter->core_ops->onebox_dump(ONEBOX_ZONE_MGMT_DUMP, (PUCHAR)mgmt_frame, (frame_len + FRAME_DESC_SZ ));
	status_l = onebox_send_internal_mgmt_frame(adapter, (uint16 *)mgmt_frame, (frame_len + FRAME_DESC_SZ));

	return status_l;
}

/**
 * This function prepares the RF Programming Request frame and sends to the PPE
 *
 * @param 
 *  adapter  Pointer to Driver Private Structure
 *  adapter  Pointer to RF programming values Structure
 *  adapter  number of programming values to be loaded
 *  adapter  number of values in a row
 *
 * @returns 
 *  ONEBOX_STATUS_SUCCESS on success, or corresponding negative
 *  error code on failure
 */
static ONEBOX_STATUS onebox_mgmt_send_rf_prog_frame(PONEBOX_ADAPTER adapter,
                                             uint16 *rf_prog_vals,
                                             uint16 num_of_sets,
                                             uint16 vals_per_set,
                                             uint8  type) 
{
	//FIXME: How are we taking care of band here ??
	onebox_mac_frame_t *mgmt_frame;
	ONEBOX_STATUS status_l;
	uint16 frame_len;
	uint16 count;
	uint8  pkt_buffer[MAX_MGMT_PKT_SIZE];

	FUNCTION_ENTRY(ONEBOX_ZONE_MGMT_SEND);

	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,(TEXT("===> Sending RF Programming Packet <===\n")));

	/* Allocating Memory For Mgmt Pkt From Mgmt Free Pool */
	mgmt_frame = (onebox_mac_frame_t *)pkt_buffer;

	adapter->os_intf_ops->onebox_memset(mgmt_frame, 0, 256);

	
	/* Preparing RF Request Frame Header */
	frame_len = (vals_per_set * num_of_sets * 2);
	mgmt_frame->desc_word[0] = ONEBOX_CPU_TO_LE16(frame_len | (ONEBOX_WIFI_MGMT_Q << 12));
	mgmt_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(RF_PROG_VALUES_REQUEST);
  
	if (adapter->soft_reset & BBP_REMOVE_SOFT_RST_BEFORE_PROG)
	{
  		mgmt_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16(BBP_REMOVE_SOFT_RST_BEFORE_PROG);
	}	
	if (adapter->soft_reset & BBP_REMOVE_SOFT_RST_AFTER_PROG)
	{
  		mgmt_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16(BBP_REMOVE_SOFT_RST_AFTER_PROG);	
	}	
	
	if (adapter->bb_rf_rw)
	{		
  		mgmt_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16(BBP_REG_READ);
		adapter->bb_rf_rw = 0;
	}	
  	adapter->soft_reset = 0;

	if (adapter->Driver_Mode == RF_EVAL_MODE_ON)
	{
		if (adapter->bb_rf_params.value == 4 || adapter->bb_rf_params.value == 5)
		{
			mgmt_frame->desc_word[5] |= ONEBOX_CPU_TO_LE16(ULP_MODE);  //indicating ULP 
			printk("ULP \n");
		}  
	}
	//Flags are not handled FIXME:
	mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16(vals_per_set | (num_of_sets << 8));
#ifdef RF_8230
	mgmt_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16 (PUT_BBP_RESET | BBP_REG_WRITE | (NONRSI_RF_TYPE << 4));
#else
	mgmt_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16 (PUT_BBP_RESET | BBP_REG_WRITE | (RSI_RF_TYPE << 4));
#endif  
	if(adapter->rf_reset)
	{
		mgmt_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16 (RF_RESET_ENABLE);
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("===> RF RESET REQUEST SENT <===\n")));
		adapter->rf_reset = 0;
	}
  //FIXME: What is the radio id to fill here
//	mgmt_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16 (RADIO_ID << 8 );

	/* Preparing RF Request Frame Body */

	for (count = 0; count < (vals_per_set * num_of_sets); count++)
	{
		mgmt_frame->u.rf_prog_req.rf_prog_vals[count] = ONEBOX_CPU_TO_LE16(rf_prog_vals[count]);
	}


	//adapter->core_ops->onebox_dump(ONEBOX_ZONE_MGMT_DUMP,(PUCHAR)mgmt_frame, (frame_len + FRAME_DESC_SZ ));
	status_l = onebox_send_internal_mgmt_frame(adapter, (uint16 *)mgmt_frame, (frame_len + FRAME_DESC_SZ));
	return status_l;
} /* onebox_mgmt_send_rf_prog_frame */


/**
 * This function prepares the Baseband buffer Programming request
 * frame 
 *
 * @param
 *  adapter       Pointer to Driver Private Structure
 * @param
 *  bb_buf_vals  Pointer to bb_buf programming values
 * @param
 *  num_of_vals   Number of BB Programming values
 *
 * @return
 *  Returns ONEBOX_STATUS_SUCCESS on success, or ONEBOX_STATUS_FAILURE
 *  on failure
 */
static ONEBOX_STATUS onebox_bb_buffer_request(PONEBOX_ADAPTER adapter,
                                             uint16 *bb_buf_vals,
                                             uint16 num_of_vals)
{
	onebox_mac_frame_t *mgmt_frame;
	ONEBOX_STATUS status_l;
	uint16 frame_len;
	uint8  pkt_buffer[MAX_MGMT_PKT_SIZE];

	FUNCTION_ENTRY (ONEBOX_ZONE_MGMT_SEND);

	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,
	             (TEXT("===> Sending Buffer Programming Packet <===\n")));

	/* Allocating Memory For Mgmt Pkt From Mgmt Free Pool */
	mgmt_frame = (onebox_mac_frame_t *)pkt_buffer;

	adapter->os_intf_ops->onebox_memset(mgmt_frame, 0, MAX_MGMT_PKT_SIZE);

	adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, (PUCHAR)bb_buf_vals, ((num_of_vals*2 *3) + 6));
	/* Preparing BB BUFFER Request Frame Body */

	frame_len = ((num_of_vals * 2 *3) + 6);	//for 1 value there are 3 regs and each 2 bytes
	adapter->os_intf_ops->onebox_memcpy(&mgmt_frame->u.bb_buf_req.bb_buf_vals[0], &bb_buf_vals[1], (frame_len));
	adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, (PUCHAR)&mgmt_frame->u.bb_buf_req.bb_buf_vals[0], (frame_len));
	
	/* Preparing BB Request Frame Header */
	
	/*prepare the frame descriptor */
	mgmt_frame->desc_word[0] = ONEBOX_CPU_TO_LE16((frame_len) | (ONEBOX_WIFI_MGMT_Q << 12));
	mgmt_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(BB_BUF_PROG_VALUES_REQ);
  
	if (adapter->soft_reset & BBP_REMOVE_SOFT_RST_BEFORE_PROG)
	{
  		mgmt_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16(BBP_REMOVE_SOFT_RST_BEFORE_PROG);
	}	
	if (adapter->soft_reset & BBP_REMOVE_SOFT_RST_AFTER_PROG)
	{
  		mgmt_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16(BBP_REMOVE_SOFT_RST_AFTER_PROG);	
	}	

	if (adapter->bb_rf_rw)
	{		
  		mgmt_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16(BBP_REG_READ);
		adapter->bb_rf_rw = 0;
	}	
  	adapter->soft_reset = 0;

	mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16(num_of_vals);
#ifdef RF_8230
	mgmt_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16 (PUT_BBP_RESET | BBP_REG_WRITE | (NONRSI_RF_TYPE << 4));
#else
	mgmt_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16 (PUT_BBP_RESET | BBP_REG_WRITE | (RSI_RF_TYPE << 4));
#endif  
	//Flags are not handled FIXME:
	//FIXME: What is the radio id to fill here
//	mgmt_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16 (RADIO_ID << 8 );


	//adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, (PUCHAR)mgmt_frame, (frame_len + FRAME_DESC_SZ ));
  status_l = onebox_send_internal_mgmt_frame(adapter, (uint16 *)mgmt_frame, (frame_len + FRAME_DESC_SZ));
  return status_l;
}


static ONEBOX_STATUS onebox_mgmt_send_rf_reset_req(PONEBOX_ADAPTER adapter,
                                             uint16 *bb_prog_vals)
{
	onebox_mac_frame_t *mgmt_frame;
	ONEBOX_STATUS status_l;
	uint8  pkt_buffer[MAX_MGMT_PKT_SIZE];

	FUNCTION_ENTRY (ONEBOX_ZONE_MGMT_SEND);

	ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,("ONEBOX_IOCTL: RF_RESET REQUEST\n"));

	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
			(TEXT("===> Frame request to reset RF<===\n")));

	mgmt_frame = (onebox_mac_frame_t *)pkt_buffer;

	adapter->os_intf_ops->onebox_memset(mgmt_frame, 0, FRAME_DESC_SZ);
	if (bb_prog_vals == NULL)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,("ONEBOX_IOCTL: RF_RESET REQUEST NULL PTR\n"));
		return ONEBOX_STATUS_FAILURE;

	}
	/* FrameType*/
	mgmt_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(RF_RESET_FRAME);
	mgmt_frame->desc_word[3] = ONEBOX_CPU_TO_LE16(bb_prog_vals[1] & 0x00ff);
	mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16((bb_prog_vals[2]) >> 8);
	mgmt_frame->desc_word[0] = (ONEBOX_WIFI_MGMT_Q << 12);
	ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT( " RF_RESET: value is 0x%x, RF_DELAY: %d\n"),mgmt_frame->desc_word[3],mgmt_frame->desc_word[4]));
	//adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, (PUCHAR)mgmt_frame, FRAME_DESC_SZ);
	status_l = onebox_send_internal_mgmt_frame(adapter, (uint16 *)mgmt_frame, ( FRAME_DESC_SZ));
	return status_l;
}

ONEBOX_STATUS bb_reset_req(PONEBOX_ADAPTER adapter)
{
	uint16 bb_prog_vals[3];

	bb_prog_vals[1] = 0x3;
	bb_prog_vals[2] = 0xABAB;

	if(onebox_mgmt_send_rf_reset_req(adapter, bb_prog_vals) != ONEBOX_STATUS_SUCCESS) {
		return ONEBOX_STATUS_FAILURE;
	}
	return ONEBOX_STATUS_SUCCESS;
}

ONEBOX_STATUS onebox_ant_sel(PONEBOX_ADAPTER adapter,
                                             uint8 value)
{
	onebox_mac_frame_t *mgmt_frame;
	ONEBOX_STATUS status_l;
	uint8  pkt_buffer[MAX_MGMT_PKT_SIZE];

  FUNCTION_ENTRY (ONEBOX_ZONE_MGMT_SEND);

  ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,("ONEBOX_IOCTL: ANT_SEL REQUEST\n"));

  ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
      (TEXT("===> Frame request to  ANT_SEL <===\n")));

  mgmt_frame = (onebox_mac_frame_t *)pkt_buffer;

  adapter->os_intf_ops->onebox_memset(mgmt_frame, 0, FRAME_DESC_SZ);
  /* FrameType*/
  mgmt_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(ANT_SEL_FRAME);
  mgmt_frame->desc_word[3] = ONEBOX_CPU_TO_LE16(value & 0x00ff);
  mgmt_frame->desc_word[0] = (ONEBOX_WIFI_MGMT_Q << 12);
  ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT( " ANT_SEL: value is 0x%x, \n"),mgmt_frame->desc_word[3]));
  //adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, (PUCHAR)mgmt_frame, FRAME_DESC_SZ);
  status_l = onebox_send_internal_mgmt_frame(adapter, (uint16 *)mgmt_frame, ( FRAME_DESC_SZ));
	return status_l;
}


static ONEBOX_STATUS onebox_mgmt_lmac_reg_ops_req(PONEBOX_ADAPTER adapter,
                                             uint16 *prog_vals,
                                             uint8  type)
{
	onebox_mac_frame_t *mgmt_frame;
	ONEBOX_STATUS status_l;
	uint8  pkt_buffer[MAX_MGMT_PKT_SIZE];

  FUNCTION_ENTRY (ONEBOX_ZONE_MGMT_SEND);

  ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,("ONEBOX_IOCTL: LMAC_REG_OPS REQUEST\n"));

  ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
      (TEXT("===> Frame request  FOR LMAC_REG_OPS<===\n")));

  mgmt_frame = (onebox_mac_frame_t *)pkt_buffer;

  adapter->os_intf_ops->onebox_memset(mgmt_frame, 0, FRAME_DESC_SZ);
	if (prog_vals == NULL)
	{
					ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,("ONEBOX_IOCTL: LMAC_REG_OPS REQUEST NULL PTR\n"));
					return ONEBOX_STATUS_FAILURE;

	}
  /* FrameType*/
  mgmt_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(LMAC_REG_OPS);
  mgmt_frame->desc_word[3] = ONEBOX_CPU_TO_LE16(prog_vals[1]);
  mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16(prog_vals[2]);
  if (type == LMAC_REG_WRITE)
  {  
    mgmt_frame->desc_word[5] = ONEBOX_CPU_TO_LE16(prog_vals[3]);
    mgmt_frame->desc_word[6] = ONEBOX_CPU_TO_LE16(prog_vals[4]);
    mgmt_frame->desc_word[7] = ONEBOX_CPU_TO_LE16(0);// write Indication
    ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT( " LMAC_REG_OPS: type %d : addr  is 0x%x,  addr1  is 0x%x,\n \t \t Data: 0x%x\nData1: 0x%x\n"), 
          mgmt_frame->desc_word[7], mgmt_frame->desc_word[3],mgmt_frame->desc_word[4], mgmt_frame->desc_word[5],mgmt_frame->desc_word[6]));
  }
  else
  {
    mgmt_frame->desc_word[7] = ONEBOX_CPU_TO_LE16(1); //Read Indication
    ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT( " LMAC_REG_OPS: type %d : addr  is 0x%x,  addr1  is 0x%x"), 
                                    mgmt_frame->desc_word[7], mgmt_frame->desc_word[3],mgmt_frame->desc_word[4]));
  }  
  mgmt_frame->desc_word[0] = (ONEBOX_WIFI_MGMT_Q << 12);
  //adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, (PUCHAR)mgmt_frame, FRAME_DESC_SZ);
  status_l = onebox_send_internal_mgmt_frame(adapter, (uint16 *)mgmt_frame, ( FRAME_DESC_SZ));
	return status_l;
}

#if 0
void load_calc_timer_values(PONEBOX_ADAPTER adapter)
{
	uint16 short_slot_time=0;
	uint16 long_slot_time=0 ;
	uint16 ack_time=0;
	uint16 in_sta_dist = 0;        

	ONEBOX_DEBUG(ONEBOX_ZONE_INFO ,(TEXT("Loading Calculated values\n")));
	ack_time = ( (2 * in_sta_dist) / 300) + 192;/*In micro sec*/
	short_slot_time = ( in_sta_dist / 300) + 9;/*In micro sec*/
	long_slot_time = ( in_sta_dist / 300) + 20;
	adapter->short_slot_time = short_slot_time * 40;
	ONEBOX_DEBUG(ONEBOX_ZONE_INFO ,
	             (TEXT("short_slot_time:%d\n"),adapter->short_slot_time)); 
	adapter->long_slot_time = long_slot_time * 40;
	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
	             (TEXT("long_slot_time:%d\n"),adapter->long_slot_time)); 
	//adapter->short_difs_rx = 0x2F8;
	adapter->short_difs_rx = (SIFS_DURATION +(1*short_slot_time)-6-11)*40;
	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
	             (TEXT("short_difs_rx:%d\n"), adapter->short_difs_rx));
	/* DIFS = (2 * Slot_time + SIFS ) 1 slot will be added in the PPE */ 
	//adapter->short_difs_tx = 0x3E8;
	adapter->short_difs_tx = ((SIFS_DURATION +(1*short_slot_time)-6-2)*40);
	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
	             (TEXT("short_difs_tx:%d\n"),adapter->short_difs_tx)); 
	adapter->long_difs_rx = (SIFS_DURATION +(2*long_slot_time)-6-11)*40;
	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,(TEXT("long_difs_rx:%d\n"),adapter->long_difs_rx)); 
	adapter->long_difs_tx = (SIFS_DURATION +(2*long_slot_time)-6-2)*40;
	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,(TEXT("long_difs_tx:%d\n"),adapter->long_difs_tx));
	adapter->difs_g_delay = 240;
	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
	             (TEXT("difs_g_delay:%d\n"),adapter->difs_g_delay)); 
	adapter->ack_timeout = (ack_time * 40 );
	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
	             (TEXT("ACK timeout:%d\n"),adapter->ack_timeout)); 
	return;
}

#endif

/**
 * This function sends the start_autorate frame to the TA firmware.
 * @param 
 *  adapter  Pointer to the driver private structure
 *
 * @returns 
 *  ONEBOX_STATUS_SUCCESS on success, or corresponding negative
 *  error code on failure
 */
ONEBOX_STATUS start_autorate_stats(PONEBOX_ADAPTER adapter)
{
	onebox_mac_frame_t *mgmt_frame;
	uint8  pkt_buffer[FRAME_DESC_SZ];

	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
	             (TEXT("%s: Starting autorate algo\n"), __func__));


	/* Allocating Memory For Mgmt Pkt From Mgmt Free Pool */
	mgmt_frame = (onebox_mac_frame_t *)pkt_buffer;

	adapter->os_intf_ops->onebox_memset(mgmt_frame, 0, MAX_MGMT_PKT_SIZE);

	mgmt_frame->desc_word[0] = ONEBOX_CPU_TO_LE16(FRAME_DESC_SZ);  
	//mgmt.desc_word[0] |= ONEBOX_TA_MGMT_Q  << 12; 
	mgmt_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(MGMT_DESC_START_AUTORATE);
	return onebox_send_internal_mgmt_frame(adapter, (uint16 *)mgmt_frame, FRAME_DESC_SZ);
}


/**
* This function sends the radio capabilities to firmware
* @param 
*   adapter  Pointer to the driver private structure
*
* @returns 
*   ONEBOX_STATUS_SUCCESS on success, or corresponding negative
* error code on failure
*/
ONEBOX_STATUS onebox_load_radio_caps(PONEBOX_ADAPTER adapter)
{
	struct ieee80211com *ic = NULL;
	struct chanAccParams wme;
	struct chanAccParams wme_sta;
	onebox_mac_frame_t *mgmt_frame;
	uint16 inx = 0;
	int32 status = 0;
	uint16 pkt[256];
	uint8 ii;
	uint8 radio_id;
	uint16 gc[20] = {0xf0, 0xf0, 0xf0, 0xf0,
					 0xf0, 0xf0, 0xf0, 0xf0,
					 0xf0, 0xf0, 0xf0, 0xf0,
					 0xf0, 0xf0, 0xf0, 0xf0,
					 0xf0, 0xf0, 0xf0, 0xf0};	

	FUNCTION_ENTRY(ONEBOX_ZONE_INIT);
	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
	             (TEXT("%s: Sending rate symbol req frame\n"), __func__));
	printk("Sending rate symbol req frame in %s:%d \n", __func__, __LINE__);
	ic = &adapter->vap_com;
	wme = ic->ic_wme.wme_wmeChanParams;
	wme_sta = ic->ic_wme.wme_wmeChanParams;
	/* Allocating Memory For Mgmt Pkt From Mgmt Free Pool */
	mgmt_frame = (onebox_mac_frame_t *)pkt;
	adapter->os_intf_ops->onebox_memset(mgmt_frame, 0, 256);

	mgmt_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(RADIO_CAPABILITIES);
	mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16(ic->ic_curchan->ic_ieee);
	mgmt_frame->desc_word[4] |= ONEBOX_CPU_TO_LE16(RSI_RF_TYPE<<8);

  mgmt_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16(ONEBOX_LMAC_CLOCK_FREQ_80MHZ);	//FIXME Radio config info	
	if(adapter->operating_chwidth == BW_40Mhz)
	{
		printk("===> chwidth is 40Mhz <===\n");
		mgmt_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16(ONEBOX_ENABLE_40MHZ);			
		if (adapter->endpoint_params.per_ch_bw)
		{
			mgmt_frame->desc_word[5] |= ONEBOX_CPU_TO_LE16(adapter->endpoint_params.per_ch_bw << 12);	//FIXME PPE ACK rate info 2-upper is primary
			mgmt_frame->desc_word[5] |= ONEBOX_CPU_TO_LE16(FULL_40M_ENABLE);	//FIXME PPE ACK rate info 2-upper is primary
		}
		if(ic->band_flags & IEEE80211_CHAN_HT40)
		{
      //FIXME PPE ACK rate info 2-upper is primary
			mgmt_frame->desc_word[5] = ONEBOX_CPU_TO_LE16(0);	
			printk("40Mhz: Programming the radio caps for 40Mhz mode\n");
		if(ic->band_flags & IEEE80211_CHAN_HT40U)
			{
				printk("Lower 20 Enable\n");
				mgmt_frame->desc_word[5] |= ONEBOX_CPU_TO_LE16(LOWER_20_ENABLE);	//PPE ACK MASK FOR 11B
				mgmt_frame->desc_word[5] |= ONEBOX_CPU_TO_LE16(LOWER_20_ENABLE >> 12);	//FIXME PPE ACK rate info 2-upper is primary
			}
			else
			{
				printk("Upper 20 Enable\n");
				mgmt_frame->desc_word[5] |= ONEBOX_CPU_TO_LE16(UPPER_20_ENABLE);	//Indicates The primary channel is above the secondary channel
				mgmt_frame->desc_word[5] |= ONEBOX_CPU_TO_LE16(UPPER_20_ENABLE >> 12);	//FIXME PPE ACK rate info 2-upper is primary
			}
   	 }
	}
	radio_id = 0;
	mgmt_frame->u.radio_caps.sifs_tx_11n = adapter->sifs_tx_11n;
	mgmt_frame->u.radio_caps.sifs_tx_11b = adapter->sifs_tx_11b;
	mgmt_frame->u.radio_caps.slot_rx_11n = adapter->slot_rx_11n;
 	mgmt_frame->u.radio_caps.ofdm_ack_tout = adapter->ofdm_ack_tout;
  	mgmt_frame->u.radio_caps.cck_ack_tout = adapter->cck_ack_tout;
 	mgmt_frame->u.radio_caps.preamble_type = adapter->preamble_type;

	mgmt_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16(radio_id << 8);			//radio_id	

	mgmt_frame->u.radio_caps.qos_params[0].cont_win_min_q = (1 << (wme_sta.cap_wmeParams[1].wmep_logcwmin)) - 1; 
	mgmt_frame->u.radio_caps.qos_params[0].cont_win_max_q = (1 << wme_sta.cap_wmeParams[1].wmep_logcwmax) - 1; 
	mgmt_frame->u.radio_caps.qos_params[0].aifsn_val_q = (wme_sta.cap_wmeParams[1].wmep_aifsn);
	mgmt_frame->u.radio_caps.qos_params[0].txop_q = (wme_sta.cap_wmeParams[1].wmep_txopLimit << 5);
	
	mgmt_frame->u.radio_caps.qos_params[1].cont_win_min_q = (1 << wme_sta.cap_wmeParams[0].wmep_logcwmin) - 1; 
	mgmt_frame->u.radio_caps.qos_params[1].cont_win_max_q = (1 << wme_sta.cap_wmeParams[0].wmep_logcwmax) - 1; 
	mgmt_frame->u.radio_caps.qos_params[1].aifsn_val_q = (wme_sta.cap_wmeParams[0].wmep_aifsn-2);
	//mgmt_frame->u.radio_caps.qos_params[1].aifsn_val_q = (wme_sta.cap_wmeParams[0].wmep_aifsn);
	mgmt_frame->u.radio_caps.qos_params[1].txop_q = (wme_sta.cap_wmeParams[0].wmep_txopLimit << 5);

	mgmt_frame->u.radio_caps.qos_params[4].cont_win_min_q = (1 << wme.cap_wmeParams[1].wmep_logcwmin) - 1; 
	mgmt_frame->u.radio_caps.qos_params[4].cont_win_max_q = (1 << wme.cap_wmeParams[1].wmep_logcwmax) - 1; 
	mgmt_frame->u.radio_caps.qos_params[4].aifsn_val_q = (wme.cap_wmeParams[1].wmep_aifsn );
	mgmt_frame->u.radio_caps.qos_params[4].txop_q = (wme.cap_wmeParams[1].wmep_txopLimit << 5);
	
	mgmt_frame->u.radio_caps.qos_params[5].cont_win_min_q = (1 << wme.cap_wmeParams[0].wmep_logcwmin) - 1; 
	mgmt_frame->u.radio_caps.qos_params[5].cont_win_max_q = (1 << wme.cap_wmeParams[0].wmep_logcwmax) - 1; 
	mgmt_frame->u.radio_caps.qos_params[5].aifsn_val_q = (wme.cap_wmeParams[0].wmep_aifsn);
	mgmt_frame->u.radio_caps.qos_params[5].txop_q = (wme.cap_wmeParams[0].wmep_txopLimit << 5);


	for(ii = 2; ii < MAX_HW_QUEUES - 8; ii++)
	{
		/* FIXME: Num of AC's are mapped to 4 How to fill in for Q4-Q7 */
		mgmt_frame->u.radio_caps.qos_params[ii].cont_win_min_q = (1  << wme_sta.cap_wmeParams[ii].wmep_logcwmin) - 1; 
		mgmt_frame->u.radio_caps.qos_params[ii].cont_win_max_q = (1 << wme_sta.cap_wmeParams[ii].wmep_logcwmax) - 1; 
		if(ii != 5)
			mgmt_frame->u.radio_caps.qos_params[ii].aifsn_val_q = (wme_sta.cap_wmeParams[ii].wmep_aifsn -1);
		else
			mgmt_frame->u.radio_caps.qos_params[ii].aifsn_val_q = (wme_sta.cap_wmeParams[ii].wmep_aifsn );

		mgmt_frame->u.radio_caps.qos_params[ii].txop_q = (wme_sta.cap_wmeParams[ii].wmep_txopLimit << 5);

		mgmt_frame->u.radio_caps.qos_params[ii + 4].cont_win_min_q = (1  << wme.cap_wmeParams[ii].wmep_logcwmin); 
		mgmt_frame->u.radio_caps.qos_params[ii + 4].cont_win_max_q = (1 << wme.cap_wmeParams[ii].wmep_logcwmax); 
		mgmt_frame->u.radio_caps.qos_params[ii + 4].aifsn_val_q = (wme.cap_wmeParams[ii].wmep_aifsn -1);
		mgmt_frame->u.radio_caps.qos_params[ii + 4].txop_q = (wme.cap_wmeParams[ii].wmep_txopLimit << 5);
	}


	for(ii = 0; ii < MAX_HW_QUEUES ; ii++)
	{
		/* FIXME: Num of AC's are mapped to 4 How to fill in for Q4-Q7 */
		if(!(mgmt_frame->u.radio_caps.qos_params[ii].cont_win_min_q)) 
		{
			/* Contention window Min is zero indicates that parameters for this queue are not assigned 
			 * Hence Assigning them to defaults */
			mgmt_frame->u.radio_caps.qos_params[ii].cont_win_min_q = 7; 
			mgmt_frame->u.radio_caps.qos_params[ii].cont_win_max_q = 0x3f; 
			mgmt_frame->u.radio_caps.qos_params[ii].aifsn_val_q = 	2;
			mgmt_frame->u.radio_caps.qos_params[ii].txop_q = 0;
		}
	}
	/*Need to write MACROS for Queue_Nos*/
	mgmt_frame->u.radio_caps.qos_params[BROADCAST_HW_Q].txop_q = 0xffff;
	mgmt_frame->u.radio_caps.qos_params[MGMT_HW_Q].txop_q = 0;
	mgmt_frame->u.radio_caps.qos_params[BEACON_HW_Q].txop_q = 0xffff;

	adapter->os_intf_ops->onebox_memcpy(&adapter->rps_rate_mcs7_pwr, &gc[0], 40);
	mgmt_frame->u.radio_caps.gcpd_per_rate[inx++] = ONEBOX_CPU_TO_LE16((adapter->rps_rate_01_pwr  & 0x00FF));    /*1*/ 
	mgmt_frame->u.radio_caps.gcpd_per_rate[inx++] = ONEBOX_CPU_TO_LE16((adapter->rps_rate_02_pwr  & 0x00FF));    /*2*/
	mgmt_frame->u.radio_caps.gcpd_per_rate[inx++] = ONEBOX_CPU_TO_LE16((adapter->rps_rate_5_5_pwr & 0x00FF));   /*5.5*/
	mgmt_frame->u.radio_caps.gcpd_per_rate[inx++] = ONEBOX_CPU_TO_LE16((adapter->rps_rate_11_pwr  & 0x00FF));   /*11*/
	mgmt_frame->u.radio_caps.gcpd_per_rate[inx++] = ONEBOX_CPU_TO_LE16((adapter->rps_rate_48_pwr & 0x00FF));   /*48*/
	mgmt_frame->u.radio_caps.gcpd_per_rate[inx++] = ONEBOX_CPU_TO_LE16((adapter->rps_rate_24_pwr & 0x00FF));    /*24*/
	mgmt_frame->u.radio_caps.gcpd_per_rate[inx++] = ONEBOX_CPU_TO_LE16((adapter->rps_rate_12_pwr & 0x00FF));    /*12*/
	mgmt_frame->u.radio_caps.gcpd_per_rate[inx++] = ONEBOX_CPU_TO_LE16((adapter->rps_rate_06_pwr & 0x00FF));    /*6*/
	mgmt_frame->u.radio_caps.gcpd_per_rate[inx++] = ONEBOX_CPU_TO_LE16((adapter->rps_rate_54_pwr & 0x00FF));   /*54*/
	mgmt_frame->u.radio_caps.gcpd_per_rate[inx++] = ONEBOX_CPU_TO_LE16((adapter->rps_rate_36_pwr & 0x00FF));   /*36*/
	mgmt_frame->u.radio_caps.gcpd_per_rate[inx++] = ONEBOX_CPU_TO_LE16((adapter->rps_rate_18_pwr & 0x00FF));    /*18*/
	mgmt_frame->u.radio_caps.gcpd_per_rate[inx++] = ONEBOX_CPU_TO_LE16((adapter->rps_rate_09_pwr & 0x00FF));    /*9*/
	mgmt_frame->u.radio_caps.gcpd_per_rate[inx++] = ONEBOX_CPU_TO_LE16((adapter->rps_rate_mcs0_pwr & 0x00FF));  /*MCS0*/
	mgmt_frame->u.radio_caps.gcpd_per_rate[inx++] = ONEBOX_CPU_TO_LE16((adapter->rps_rate_mcs1_pwr & 0x00FF));  /*MCS1*/
	mgmt_frame->u.radio_caps.gcpd_per_rate[inx++] = ONEBOX_CPU_TO_LE16((adapter->rps_rate_mcs2_pwr & 0x00FF));  /*MCS2*/
	mgmt_frame->u.radio_caps.gcpd_per_rate[inx++] = ONEBOX_CPU_TO_LE16((adapter->rps_rate_mcs3_pwr & 0x00FF)); /*MCS3*/
	mgmt_frame->u.radio_caps.gcpd_per_rate[inx++] = ONEBOX_CPU_TO_LE16((adapter->rps_rate_mcs4_pwr & 0x00FF)); /*MCS4*/
	mgmt_frame->u.radio_caps.gcpd_per_rate[inx++] = ONEBOX_CPU_TO_LE16((adapter->rps_rate_mcs5_pwr & 0x00FF)); /*MCS5*/
	mgmt_frame->u.radio_caps.gcpd_per_rate[inx++] = ONEBOX_CPU_TO_LE16((adapter->rps_rate_mcs6_pwr & 0x00FF)); /*MCS6*/
	mgmt_frame->u.radio_caps.gcpd_per_rate[inx] = ONEBOX_CPU_TO_LE16((adapter->rps_rate_mcs7_pwr & 0x00FF)); /*MCS7*/
	
	mgmt_frame->desc_word[0] = ONEBOX_CPU_TO_LE16(sizeof(mgmt_frame->u.radio_caps)  | (ONEBOX_WIFI_MGMT_Q << 12));

	adapter->core_ops->onebox_dump(ONEBOX_ZONE_MGMT_DUMP, (PUCHAR)mgmt_frame, sizeof(mgmt_frame->u.radio_caps) + FRAME_DESC_SZ);
	ONEBOX_DEBUG(ONEBOX_ZONE_MGMT_DUMP, (TEXT("===> RADIO CAPS FRAME SENT <===\n")));

	
	status = onebox_send_internal_mgmt_frame(adapter, (uint16 *)mgmt_frame, (sizeof(mgmt_frame->u.radio_caps)+ FRAME_DESC_SZ));

	if (status == ONEBOX_STATUS_FAILURE)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Unable to send radio caps frame\n")));
		return ONEBOX_STATUS_FAILURE;
	}

	FUNCTION_EXIT(ONEBOX_ZONE_INIT);
	return ONEBOX_STATUS_SUCCESS;
}

/**
* This function sends the configuration values  to firmware
* @param 
*   adapter  Pointer to the driver private structure
*
* @returns 
*   ONEBOX_STATUS_SUCCESS on success, or corresponding negative
* error code on failure
*/
static ONEBOX_STATUS onebox_load_config_vals(PONEBOX_ADAPTER  adapter)
{

	uint16 onebox_config_vals[] = 
	{ 19,           /* num params */ 
	                       0x1,          /* internalPmuEnabled */
	                       0x0,          /* ldoControlRequired */
	                       0x9,          /* crystalFrequency values */
	                       /* 9 --> 40MHz, 8 --> 20MHz 7 --> 44MHz   6 --> 52MHz */
	                       /* 5 --> 26MHz, 4 --> 13MHz 2 --> 38.4MHz 1 --> 19.2MHz */
	                       /* 0 --> 9.6MHz */
	                       0x2D,         /* crystalGoodTime */
	                       0x0,          /* TAPLLEnabled  */
#ifndef USE_KTV_INTF
	                       0,            /* TAPLLFrequency */
#else
	                       40,           /* TAPLLFrequency */
#endif
	                       0x2,          /* sleepClockSource */
	                       0x0,          /* voltageControlEnabled */
#ifndef USE_KTV_INTF
	                       5200,         /* wakeupThresholdTime */
#else
	                       6500,         /* wakeupThresholdTime */
#endif
	                       0x0,          /* reserved */        
	                       0x0,          /* reserved */
	                       0x0,          /* reserved */
	                       0x0,          /* host based wkup enable */          
	                       0x0,          /* FR4 enable */
	                       0x0,          /* BT Coexistence */
	                       0x0,          /* host based interrupt enable */ 
	                       0x0,          /* host wakeup_active_high */
	                       0x0,          /* xtal_ip_enable */
	                       0x0           /* external PA*/
	};
	/* Driver Expecting Only 19 paramaeters */
	if (onebox_config_vals[0] == 19)
	{
		adapter->os_intf_ops->onebox_memcpy(&adapter->config_params, &onebox_config_vals[1], 38);
		adapter->core_ops->onebox_dump(ONEBOX_ZONE_INIT, (PUCHAR)&adapter->config_params, 38);
	}
	else
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		    (TEXT("%s: Wrong Number of Config params found\n"), __func__));
		return ONEBOX_STATUS_FAILURE;
	} /* End if <condition> */

	return ONEBOX_STATUS_SUCCESS;
}

static void onebox_eeprom_read_band(PONEBOX_ADAPTER adapter)
{
        printk("Reading EEPROM RF TYPE\n" );
        adapter->eeprom.length = ((WLAN_MAC_MAGIC_WORD_LEN + 3) & (~3)); /* 4 bytes are read to extract RF TYPE , always read dword aligned */
        adapter->eeprom.offset = WLAN_EEPROM_RFTYPE_ADDR; /* Offset val to read RF type is zero */
        if(eeprom_read(adapter) == ONEBOX_STATUS_SUCCESS)
        {
                adapter->fsm_state = FSM_EEPROM_READ_RF_TYPE;
        }
        else
        {
                ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
                                (TEXT("%s: Unable to Read RF type  %d, len %d\n"), __func__,  adapter->eeprom.offset, adapter->eeprom.length));
              adapter->operating_band = BAND_2_4GHZ;
        }
        return;
}
/**
 * This function receives the management packets from the hardware and process them
 *
 * @param 
 *  adapter  Pointer to driver private structure
 * @param 
 *  msg      Received packet
 * @param 
 *  len      Length of the received packet
 *
 * @returns 
 *  ONEBOX_STATUS_SUCCESS on success, or corresponding negative
 *  error code on failure
 */
int32 onebox_mgmt_pkt_recv(PONEBOX_ADAPTER adapter, netbuf_ctrl_block_t *netbuf_cb)
{
	uint16 msg_type;
	int32 msg_len;
	uint8 sub_type;
	uint8 status;
	uint8 associd;
	uint8 *msg;
  uint8 sta_id ;
	uint32 path = 0;

	struct ieee80211com *ic = &adapter->vap_com;
	struct ieee80211vap *vap =NULL;
	const struct ieee80211_node_table *nt = &ic->ic_sta;
	struct ieee80211_node *ni =NULL;

#ifdef PWR_SAVE_SUPPORT
	uint16 confirm_type;
#ifdef ENABLE_DEEP_SLEEP
	//pwr_save_params	ps_params;
#endif
#endif
	//EEPROM_READ read_buf;

	FUNCTION_ENTRY(ONEBOX_ZONE_MGMT_RCV);

	msg = netbuf_cb->data;
	msg_len   = (*(uint16 *)&msg[0] & 0x0fff);
	
	/*Type is upper 5bits of descriptor */
	msg_type = (msg[2] );

	sub_type = (msg[15] & 0xff);
	ONEBOX_DEBUG(ONEBOX_ZONE_MGMT_RCV,(TEXT("Rcvd Mgmt Pkt Len = %d type =%04x subtype= %02x\n"), msg_len, msg_type, sub_type));
	adapter->core_ops->onebox_dump(ONEBOX_ZONE_MGMT_RCV, msg, msg_len);

	ONEBOX_DEBUG(ONEBOX_ZONE_MGMT_RCV,
	             (TEXT("WLAN Msg Len: %d, Msg Type: %4x\n"), msg_len, msg_type));
	switch (adapter->fsm_state)
	{
#if 0 /* coex */
		case FSM_CARD_NOT_READY: 
		{
			if (msg_type == CARD_READY_IND)
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_FSM, (TEXT("CARD READY RECVD\n")));
				/* initializing tx soft queues */
				for (count = 0; count < NUM_SOFT_QUEUES; count++) 
				{ 
					adapter->os_intf_ops->onebox_netbuf_queue_init(&adapter->host_tx_queue[count]);
				}

				if (onebox_umac_init_done(adapter)!= ONEBOX_STATUS_SUCCESS)
				{
					return ONEBOX_STATUS_FAILURE;
				}	
				read_reg_parameters(adapter);
				if(onebox_load_bootup_params(adapter) == ONEBOX_STATUS_SUCCESS)
				{
					ONEBOX_DEBUG(ONEBOX_ZONE_FSM,
					             (TEXT("%s: BOOTUP Parameters loaded successfully\n"),__func__));
					adapter->fsm_state = FSM_LOAD_BOOTUP_PARAMS ;
				}
				else
				{
					ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT ("%s: Failed to load bootup parameters\n"), 
					             __func__));
				}
			}
		}
#ifndef FPGA_VALIDATION    
		break;
#endif
#endif /* 0 coex */
		case FSM_LOAD_BOOTUP_PARAMS:
      {
#ifndef FPGA_VALIDATION    
              if (msg_type == TA_CONFIRM_TYPE && sub_type == BOOTUP_PARAMS_REQUEST)
              {
                      ONEBOX_DEBUG(ONEBOX_ZONE_FSM,
                                      (TEXT("%s: Received bootup parameters confirm sucessfully\n"),__FUNCTION__));
#endif
                      if (adapter->Driver_Mode == QSPI_FLASHING)
                      {
                              adapter->fsm_state = FSM_MAC_INIT_DONE; 
                              break;
                      } 
                      adapter->eeprom.length = (IEEE80211_ADDR_LEN + WLAN_MAC_MAGIC_WORD_LEN
                                      + WLAN_HOST_MODE_LEN);
                      adapter->eeprom.offset = WLAN_MAC_EEPROM_ADDR;
                      ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
                                      (TEXT("%s: Read MAC Addr %d, len %d\n"), __func__,  adapter->eeprom.offset, adapter->eeprom.length));
                      if(eeprom_read(adapter) == ONEBOX_STATUS_SUCCESS)
                      {
                              adapter->fsm_state = FSM_EEPROM_READ_MAC_ADDR;
                      }
                      else
                      {
                              ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
                                              (TEXT("%s %d:Unable to read from EEPROM \n"), __func__, __LINE__));
                              adapter->fsm_state = FSM_CARD_NOT_READY;
                      }
#ifndef FPGA_VALIDATION    
              }
#endif
      }
		break;

		case FSM_EEPROM_READ_MAC_ADDR:
		{
			if (msg_type == TA_CONFIRM_TYPE && sub_type == EEPROM_READ_TYPE)
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_FSM,
						(TEXT("%s: Received MAC Addr read confirmed \n"),__FUNCTION__));
				adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, msg, (16 +msg_len));
				if (msg_len > 0)
				{ 
					if  (msg[16] == MAGIC_WORD)
					{	
						adapter->os_intf_ops->onebox_memcpy((PVOID)(adapter->mac_addr),
								&msg[FRAME_DESC_SZ + WLAN_HOST_MODE_LEN + WLAN_MAC_MAGIC_WORD_LEN], IEEE80211_ADDR_LEN);
						//adapter->os_intf_ops->onebox_fill_mac_address(adapter); 
						adapter->os_intf_ops->onebox_memcpy(adapter->dev->dev_addr, 
										    adapter->mac_addr, ETH_ALEN);
					}
					else
          {
                  if (!adapter->calib_mode)
                  {
                          ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
                                          (TEXT("%s: UNABLE TO READ MAC_ADDR \n"),__FUNCTION__));
#ifndef CHIP_9116
                          adapter->fsm_state = FSM_CARD_NOT_READY;
                          break;
#endif		
                  }						
          }	
				}  
				adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, adapter->mac_addr, (IEEE80211_ADDR_LEN));
        onebox_eeprom_read_band(adapter);
        break;
        case FSM_EEPROM_READ_RF_TYPE:

        printk("CONFIRM FOR RF TYPE READ CAME\n");
        if (msg_type == TA_CONFIRM_TYPE && sub_type == EEPROM_READ_TYPE)
        {
                ONEBOX_DEBUG(ONEBOX_ZONE_FSM,
                                (TEXT("%s: Received EEPROM RF type read confirmed \n"),__func__));
                adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, msg, (16 +msg_len));
#ifdef CHIP_9116
              ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT("!!!! Dual band supported!!!!!\n")));
              printk("!!!! Dual band supported!!!!!\n");
              adapter->operating_band = BAND_5GHZ;
       	      adapter->band_supported = 1;
#else
                if  (msg[16] == MAGIC_WORD)
                {
                        if((msg[17] & 0x3) == 0x3)
                        {
                                ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT("!!!! Dual band supported!!!!!\n")));
                                printk("!!!! Dual band supported!!!!!\n");
                                adapter->operating_band = BAND_5GHZ;
																adapter->band_supported = 1;
                        }
                        else if((msg[17] & 0x3) == 0x1)
                        {
                                ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT("!!!! 2.4Ghz band supported!!!!!\n")));
                                printk("!!!! 2.4Ghz band supported!!!!!\n");
                                adapter->operating_band = BAND_2_4GHZ;
																adapter->band_supported = 0;
                        }
                        else
                        {
                                ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Magic word present, But valid band is not present\n")));
                                adapter->fsm_state=FSM_CARD_NOT_READY;
                                break;
                        }
                }	
                else
                {
                        if (!adapter->calib_mode)
                        {
                                ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("No Magic word present, Defaulting to 2.4Ghz\n")));
                                adapter->fsm_state=FSM_CARD_NOT_READY;
                                break;
                        }
                }
#endif
                adapter->core_ops->onebox_net80211_attach(adapter);
        }

        if(onebox_send_reset_mac(adapter) == 0)
        {

                ONEBOX_DEBUG(ONEBOX_ZONE_FSM,
                                (TEXT("%s: Reset MAC frame sent sucessfully\n"),__FUNCTION__));
                adapter->fsm_state = FSM_RESET_MAC_CFM;
        }
        else
        {
                ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT ("%s: Failed to load reset mac frame \n"), __FUNCTION__));
                //FIXME: return from here ???
        }
      }
    }
    break;

		case FSM_RESET_MAC_CFM:
		{
			if(msg_type == TA_CONFIRM_TYPE && sub_type == RESET_MAC_REQ)	
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_FSM, (TEXT("Reset mac confirm is received\n")));
				//if (onebox_umac_init_done(adapter)!= ONEBOX_STATUS_SUCCESS)
				{
				//	return ONEBOX_STATUS_FAILURE;
				}	
				ONEBOX_DEBUG(ONEBOX_ZONE_FSM, (TEXT("sending radio caps \n")));
				adapter->rx_filter_word = 0x0000;
				adapter->sifs_tx_11n = SIFS_TX_11N_VALUE;
				adapter->sifs_tx_11b = SIFS_TX_11B_VALUE;
				adapter->slot_rx_11n = SHORT_SLOT_VALUE;
				adapter->ofdm_ack_tout = OFDM_ACK_TOUT_VALUE;
				adapter->cck_ack_tout = CCK_ACK_TOUT_VALUE;
				adapter->preamble_type = LONG_PREAMBLE;
				if(!onebox_load_radio_caps(adapter))
				{
					/*FIXME: Freeing the netbuf_cb as returning from here may not handle the netbuf free */
					adapter->os_intf_ops->onebox_free_pkt(adapter, netbuf_cb, 0);
					return ONEBOX_STATUS_FAILURE;
				}	
			 	ONEBOX_DEBUG(ONEBOX_ZONE_FSM, (TEXT("Program BB/RF frames \n")));	
			}

			if(msg_type == TA_CONFIRM_TYPE && sub_type == RADIO_CAPABILITIES)
			{
         
				adapter->rf_reset = 1;
				printk(" RF_RESET AFTER at RESET REQ   = :\n");
				if(!program_bb_rf(adapter))
				{
					ONEBOX_DEBUG(ONEBOX_ZONE_FSM,
				             (TEXT("%s: BB_RF values loaded successfully\n"),__FUNCTION__));
					adapter->fsm_state = FSM_BB_RF_START;
					adapter->fsm_state = FSM_MAC_INIT_DONE;
				}
			}
		}
		break;
		
		case FSM_BB_RF_START:	
		ONEBOX_DEBUG(ONEBOX_ZONE_FSM, (TEXT("In %s Line %d bb_rf_count  %d\n"), __func__, __LINE__,adapter->bb_rf_prog_count));
		if(((msg_type == TA_CONFIRM_TYPE) && ((sub_type == BB_PROG_VALUES_REQUEST) 
						|| (sub_type == RF_PROG_VALUES_REQUEST) || (sub_type == BBP_PROG_IN_TA) )))
		{
			adapter->bb_rf_prog_count--;
			ONEBOX_DEBUG(ONEBOX_ZONE_DEBUG,
			         (TEXT(" FSM_STATE: BB RF count after receiving confirms for previous bb/rf frames %d\n"), adapter->bb_rf_prog_count));
			if(!adapter->bb_rf_prog_count)
				adapter->fsm_state = FSM_MAC_INIT_DONE;  
		}
		break;	
#if 0
		case FSM_DEEP_SLEEP_ENABLE:
		{
			if(msg_type == TA_CONFIRM_TYPE && sub_type == DEEP_SLEEP_ENABLE)
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_FSM,
				        (TEXT("%s: Received confirm for Deep sleep enable frame\n"), __func__));
			}
				
		}
		break;
#endif
		case FSM_AMPDU_IND_SENT:
		{
			if(msg_type == TA_CONFIRM_TYPE && sub_type == AMPDU_IND)
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_FSM,
						(TEXT("%s: Received confirm for ampdu indication frame\n"), __func__));
				ONEBOX_DEBUG(ONEBOX_ZONE_OID, (TEXT("onebox_set_per_tx_mode: Enabling the PER burst mode\n")));
				if (adapter->wlan_osd_ops->onebox_init_wlan_thread(&(adapter->sdio_scheduler_thread_handle_per), 
							"PER THREAD", 
							0, 
							adapter->wlan_osd_ops->onebox_per_thread, 
							adapter) != 0)
				{
					ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("onebox_set_per_tx_mode: Unable to initialize thread\n")));
					adapter->os_intf_ops->onebox_free_pkt(adapter, netbuf_cb, 0);
					return -EFAULT;
				}
				adapter->os_intf_ops->onebox_start_thread(&(adapter->sdio_scheduler_thread_handle_per));
				adapter->tx_running = BURST_RUNNING;//indicating PER_BURST_MODE
			}
			adapter->fsm_state = FSM_MAC_INIT_DONE;
				
		}
		break;	
		
		case FSM_SCAN_CFM:
		{
			if((msg_type == TA_CONFIRM_TYPE) && (sub_type == SCAN_REQUEST))
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT( "FSM_SCAN_CFM Received: \n")));
				adapter->os_intf_ops->onebox_set_event(&(adapter->bb_rf_event));
			}
		}
		break;

		case FSM_MAC_INIT_DONE:
		{
			//printk("Msg type %d\n", msg_type);
			if(msg_type == TA_CONFIRM_TYPE)
			{
				switch (sub_type)
				{
					case STATS_REQUEST_FRAME:
					{
							uint16 *framebody;
							ONEBOX_DEBUG(ONEBOX_ZONE_INFO,(TEXT("\nPER STATS PACKET  :\n")));

							framebody = (uint16 *)&msg[16];

							adapter->os_intf_ops->onebox_memset(&adapter->sta_info, 0, sizeof(per_stats));
							adapter->os_intf_ops->onebox_memcpy((&adapter->sta_info), &msg[16], sizeof(per_stats));
#if 0
							printk("tx_pkts = 0x%04x\n",adapter->sta_info.tx_pkts );
							printk("tx_retries = 0x%04x\n", adapter->sta_info.tx_retries);
							printk("xretries = 0x%04x\n", adapter->sta_info.xretries);
							printk("rssi = 0x%x\n", adapter->sta_info.rssi);
							printk("max_cons_pkts_dropped = 0x%04x\n", adapter->sta_info.max_cons_pkts_dropped);
#endif						
							ONEBOX_DEBUG(ONEBOX_ZONE_FSM,
									(TEXT("%s: Sending Stats response frame\n"), __func__));
							adapter->os_intf_ops->onebox_set_event(&(adapter->stats_event));
					}
					break;   
					case BB_PROG_VALUES_REQUEST:
					{
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT(
											"BBP PROG STATS: Utils BB confirm Received: msg_len: %d \n"),msg_len));

							if ((msg_len) > 0)
							{	
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT(
											"BBP PROG STATS: Utils BB confirm Received: msg_len -16 : %d \n"),msg_len));
							adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, &msg[16], (msg_len));
							adapter->os_intf_ops->onebox_memcpy((PVOID)(&adapter->bb_rf_read.Data[0]), &msg[16], (msg_len));
							adapter->bb_rf_read.no_of_values = (msg_len)/2 ;
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT(
											"BBP PROG STATS: msg_len is : %d no of vls is %d  \n"),msg_len,adapter->bb_rf_read.no_of_values));
							adapter->os_intf_ops->onebox_set_event(&(adapter->bb_rf_event));
							}

					}
					break;
					case RF_PROG_VALUES_REQUEST:
					{
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT(
											"RF_PROG_STATS: Utils RF confirm Received: msg_len : %d \n"),msg_len));

							if ((msg_len) > 0)
							{	
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT(
											"RF_PROG_STATS: Utils RF confirm Received: msg_len -16 : %d \n"),msg_len));
							adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, msg, msg_len);
							adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, &msg[16], (msg_len));
							adapter->os_intf_ops->onebox_memcpy((PVOID)(&adapter->bb_rf_read.Data[0]), &msg[16], (msg_len));
							adapter->bb_rf_read.no_of_values = (msg_len)/2 ;
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT(
											"RF_PROG_STATS: msg_len is : %d no of vls is %d  \n"),msg_len,adapter->bb_rf_read.no_of_values));
							adapter->os_intf_ops->onebox_set_event(&(adapter->bb_rf_event));
							}
					}
					break;
					case BB_BUF_PROG_VALUES_REQ:
					{
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT(
											"BBP_USING_BUFFER: Utils BUF confirm Received: msg_len : %d \n"),msg_len));

							if ((msg_len) > 0)
							{	
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT(
											"BBP_USING_BUFFER: Utils BUF confirm Received: msg_len -16 : %d \n"),msg_len));
							adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, msg, msg_len);
							adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, &msg[16], (msg_len));
							adapter->os_intf_ops->onebox_memcpy((PVOID)(&adapter->bb_rf_read.Data[0]), &msg[16], (msg_len));
							adapter->bb_rf_read.no_of_values = (msg_len)/2 ;
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT(
											"BBP_USING_BUFFER: msg_len is : %d no of vls is %d  \n"),msg_len,adapter->bb_rf_read.no_of_values));
							adapter->os_intf_ops->onebox_set_event(&(adapter->bb_rf_event));
							}
              						else if(adapter->bb_rf_params.value == 7) //BUFFER_WRITE
              						{
                						adapter->os_intf_ops->onebox_set_event(&(adapter->bb_rf_event));
             						}
              						break;
          			 		}
					 case LMAC_REG_OPS:
					 {
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT(
											"LMAC_REG_OPS : Utils LMAC_REG_READ confirm Received: msg_len: %d \n"),msg_len));
							adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, &msg[0], (16));

							if ((msg_len) > 0)
							{	
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT(
											"LMAC_REG_OPS : Utils LMAC_REG_OPS confirm Received: msg_len -16 : %d \n"),msg_len));
							adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, &msg[16], (msg_len));
							adapter->os_intf_ops->onebox_memcpy((PVOID)(&adapter->bb_rf_read.Data[0]), &msg[16], (msg_len));
							adapter->bb_rf_read.no_of_values = (msg_len)/2 ;
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT(
											"LMAC_REG_OPS : msg_len is : %d no of vls is %d  \n"),msg_len,adapter->bb_rf_read.no_of_values));
							adapter->os_intf_ops->onebox_set_event(&(adapter->bb_rf_event));
							}

					}
					break;
          			case CW_MODE_REQ:
          			{
            			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT(
                    		"CW_MODE_REQ:: Utils CW_MODE_REQ: confirm Received: msg_len : %d \n"),msg_len));

            			adapter->os_intf_ops->onebox_set_event(&(adapter->bb_rf_event));
            			break;
					}
					case RF_LOOPBACK_REQ:
					{
						ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT(
										"BBP_USING_BUFFER: Utils RF_LOOPBACK_REQ confirm Received: msg_len : %d \n"),msg_len));

						if ((msg_len) > 0)
						{	
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT(
											"BBP_USING_BUFFER: Utils RF_LOOPBACK_REQ confirm Received: msg_len -16 : %d \n"),msg_len));
							adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, msg, msg_len);
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT(
											"BBP_USING_BUFFER: msg_len is : %d no of vls is %d rf_lpbk: %d \n"),msg_len,adapter->bb_rf_read.no_of_values,adapter->rf_lpbk_len));
							{ 
								adapter->os_intf_ops->onebox_memcpy((PVOID)(&adapter->rf_lpbk_data[adapter->rf_lpbk_len]), &msg[16], (msg_len));
								adapter->rf_lpbk_len += msg_len;
								printk("rf_lpbk_len : %d,\n",adapter->rf_lpbk_len);
								if (adapter->rf_lpbk_len >= (4096))
								{  
									printk("SET EVENT rf_lpbk_len : %d,\n",adapter->rf_lpbk_len);
									adapter->os_intf_ops->onebox_set_event(&(adapter->bb_rf_event));
								}
								else
								{  
									printk("BREAK EVENT rf_lpbk_len : %d,\n",adapter->rf_lpbk_len);
									break;
								}
							}
						}
					}
					break;

					case RF_LPBK_M3:
					{
								
						ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT(
										"BBP_USING_BUFFER: Utils RF_LPBK_REQ confirm Received: msg_len : %d \n"),msg_len));
						if ((msg_len) > 0)
						{	
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT(
											"BBP_USING_BUFFER: Utils RF_LPBK_M3_REQ confirm Received: msg_len -16 : %d \n"),msg_len));
							adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, msg, msg_len);
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT(
											"BBP_USING_BUFFER: msg_len is : %d no of vls is %d rf_lpbk_m3: %d \n"),msg_len,adapter->bb_rf_read.no_of_values,adapter->rf_lpbk_len));
								adapter->os_intf_ops->onebox_memcpy((PVOID)(&adapter->rf_lpbk_data[adapter->rf_lpbk_len]), &msg[16], (msg_len));
								adapter->rf_lpbk_len += msg_len;
								printk("rf_lpbk_len : %d,\n",adapter->rf_lpbk_len);
								if (adapter->rf_lpbk_len >= (512))
								{  
									printk("SET EVENT rf_lpbk_len : %d,\n",adapter->rf_lpbk_len);
									adapter->os_intf_ops->onebox_set_event(&(adapter->bb_rf_event));
								}
								else
								{  
									printk("BREAK EVENT rf_lpbk_len : %d,\n",adapter->rf_lpbk_len);
									break;
								}
						}
					}
					break;

					case BG_SCAN_PROBE_REQ:
						{
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("<==== Recv Bg scan complete event ======>\n")));
							TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next)
							{
								if(vap == NULL)
									break;
								else if (vap->iv_opmode == IEEE80211_M_STA)
								{
									adapter->net80211_ops->onebox_send_scan_done(vap);
								}
							}

						}
						break;
#ifdef PWR_SAVE_SUPPORT
					case WAKEUP_SLEEP_REQUEST:
						{
							if(adapter->Driver_Mode != RF_EVAL_MODE_ON)
							{
									TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next)
									{
											if(vap == NULL)
													break;
											else if (vap->iv_opmode == IEEE80211_M_STA)
											{
															ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("pwr save state PS_STATE %d \n"), PS_STATE));
															ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("PWR save token is %d \n"), *(uint16 *)&msg[12]));
															confirm_type = *(uint16 *)&msg[12];
															if((confirm_type == SLEEP_REQUEST)) {
																			path = PS_EN_REQ_CNFM_PATH;
															}else if(confirm_type == WAKEUP_REQUEST){
																			path = PS_DIS_REQ_CNFM_PATH;
															}
															else {
																			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("In %s Line %d Invalid confirm type %d\n"), __func__, __LINE__, confirm_type));
																			return ONEBOX_STATUS_FAILURE;
															}
															update_pwr_save_status(vap, PS_DISABLE, path);
															break;
															//adapter->net80211_ops->onebox_send_scan_done(vap);
											}
									}
							}
														
						}
						break;

					case DEV_SLEEP_REQUEST:
						{
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("<==== Recv device sleep request======>\n")));
						}
						break;
					case DEV_WAKEUP_CNF:
						{
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("<==== Recv device wakeup confirm======>\n")));
#if 0
							adapter->sleep_request_received = 0;
							if(adapter->queues_have_pkts == 1){
								schedule_pkt_for_tx(adapter);
							}
#endif
						}
						break;
#endif
					case SCAN_REQUEST:
					{	
					//	ONEBOX_DEBUG(ONEBOX_ZONE_DEBUG, (TEXT("SCAN confirm received\n")));
						TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next)
						{
							if(vap == NULL)
								break;
							else if (vap->iv_opmode == IEEE80211_M_STA)
							{
								if(vap->hal_priv_vap->passive)
								{
										printk("SCAN REQUEST CONFIRM RECIVED\n");
										adapter->net80211_ops->onebox_send_probereq_confirm(vap);
										vap->hal_priv_vap->passive = 0;
								}
							}
						}
					}
					break;
					case EEPROM_READ_TYPE:
					{
						ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT(
										"EEPROM_READ: confirm Received:  \n")));

						adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, msg, 16);
						if (msg_len > 0)
						{  
							adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, &msg[16], msg_len);
							adapter->os_intf_ops->onebox_memcpy((PVOID)(&adapter->eeprom.data[0]), &msg[16], (msg_len));
							printk("  eeprom length: %d, eeprom msg_len %d\n",adapter->eeprom.length, msg_len);
							adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, &adapter->eeprom.data[0], adapter->eeprom.length);
						}  
						adapter->os_intf_ops->onebox_set_event(&(adapter->bb_rf_event));
					}
					break;
					case EEPROM_WRITE:
					{
						ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT(
										"EEPROM_WRITE: confirm Received:  \n")));

						adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, msg, 16);
						if (msg_len > 0)
							adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, &msg[16], msg_len);
						adapter->os_intf_ops->onebox_set_event(&(adapter->bb_rf_event));
					}
						break;
					default:
						ONEBOX_DEBUG(ONEBOX_ZONE_MGMT_SEND,(TEXT("\nInvalid type in FSM_MAC_INIT_DONE  :\n")));
						break;
				}
			}
			else if (msg_type == TX_STATUS_IND)
			{
						ONEBOX_DEBUG(ONEBOX_ZONE_INFO,(TEXT("\n STATUS IND   subtype %02x:\n"), sub_type));
						switch(sub_type)
						{
							case NULLDATA_CONFIRM:
								{
									status = msg[12];
									associd = msg[13];
									TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next)
									{
										if (vap->iv_opmode == IEEE80211_M_HOSTAP)
										{ 
											TAILQ_FOREACH(ni, &nt->nt_node, ni_list)
											{
												if((ni->ni_associd & 0xff) == associd)
												{
													if(status)
													{
														ni->ni_inact = ni->ni_inact_reload;
														printk("In %s line %d:inact timer reloaded %d\n", __func__, __LINE__, ni->ni_inact);
													} else {
														printk("In %s line %d:keepalive confirm failed\n", __func__, __LINE__);
														break;
													}
												}
											}
										}
									}
								}
								break;
							case EAPOL4_CONFIRM: 
								{
									if(msg[12])
									{
										ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("<==== Received EAPOL4 CONFIRM ======>\n")));
										TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next)
										{
												if (vap->iv_opmode == IEEE80211_M_STA)
												{ 
														vap->hal_priv_vap->conn_in_prog = 0;
														adapter->sta_mode.eapol4_cnfrm = 1;
														printk("In %s Line %d  eapol %d ptk %d sta_block %d ap_block %d\n", __func__, __LINE__,
																		adapter->sta_mode.eapol4_cnfrm, adapter->sta_mode.ptk_key, adapter->sta_data_block, adapter->block_ap_queues); 
														if((adapter->sta_mode.ptk_key) && 
																						(adapter->sta_mode.eapol4_cnfrm) && 
																						(adapter->sta_data_block)) {
																onebox_send_block_unblock(vap, STA_CONNECTED, 0);
														}
														if(adapter->sta_mode.ptk_key && adapter->sta_mode.gtk_key && adapter->sta_mode.eapol4_cnfrm) {
																printk("Resting ptk ket variable in %s Line %d \n", __func__, __LINE__);
																adapter->sta_mode.ptk_key = 0; 
																printk("calling update_pwr_save_status In %s Line %d\n", __func__, __LINE__);
																update_pwr_save_status(vap, PS_ENABLE, CONNECTED_PATH);

														}
														break;
												}
										}

								}
								else
								{
										printk("EAPOL4 failed Doesn't had sta_id vap_id\n");
										TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next)
										{
										  if (vap->iv_opmode == IEEE80211_M_STA)
										  { 
										    ni = vap->iv_bss; 
										    if(ni)
										      adapter->net80211_ops->onebox_ieee80211_sta_leave(ni);
										  }
										}
										// FIXME:vap_id should be handled here 
										//adapter->sta_mode.ptk_key = 0;
										//hal_load_key(adapter, NULL, 0, adapter->sta_mode.sta_id, ONEBOX_PAIRWISE_KEY, 0,
										//				adapter->sec_mode[0], adapter->hal_vap[adapter->sta_mode.vap_id].vap);
										//hal_load_key(adapter, NULL, 0, 0, ONEBOX_PAIRWISE_KEY, 0, adapter->sec_mode[vap_id]);
										adapter->os_intf_ops->onebox_free_pkt(adapter, netbuf_cb, 0);
										return 0;
								}
						}
						break;
					case PROBEREQ_CONFIRM :
						{
							if((uint16)msg[12])
							{
								ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("<==== Recv PROBEREQ CONFIRM ======>\n")));
							}
							else
							{
								printk("Probe Request Failed \n");
							}

							TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next)
							{
									if (vap->iv_opmode == IEEE80211_M_STA)
									{
										adapter->net80211_ops->onebox_send_probereq_confirm(vap);
									}
							}
						}
						break;
					default:
						ONEBOX_DEBUG(ONEBOX_ZONE_MGMT_DUMP,(TEXT("\nInvalid type in FSM_MAC_INIT_DONE %02x :\n"), sub_type));
						break;
				}
				adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, msg, msg_len);
			}
			else if(msg_type == DEBUG_IND)
			{
					printk("Received debug frame of len %d\n", msg_len);
					dump_debug_frame(msg, msg_len);
			}
			else if (msg_type == HW_BMISS_EVENT)
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Beacon miss occured..! Indicate the same to Net80211 state machine\n")));
				adapter->net80211_ops->ieee80211_beacon_miss(&adapter->vap_com);
			}
			else if (msg_type == TSF_SYNC_CONFIRM)
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Recvd TSF SYNC CONFIRM\n")));
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR ,(TEXT("\n STA_ID %02x TSF_LSB is %02x \n"), msg[13], *(uint32 *)&msg[16]) );
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR ,(TEXT("\n STA_ID %02x TSF_MSB is %02x \n"), msg[13], *(uint32 *)&msg[20]) );
        sta_id = msg[13];
				if(sta_id > MAX_STATIONS_SUPPORTED) {
					printk("Invalid Station ID %s line %d\n", __func__, __LINE__);
					adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, msg, msg_len);
					return -1;
				}
				ni = adapter->sta[sta_id].ni;
				if(ni != NULL) {
					if(ni->ni_vap->iv_opmode != IEEE80211_M_HOSTAP) {
						printk("Invalid ni \n");
					printk("Invalid Station ID %s line %d sta_id %d vap_opmode %d\n", __func__, __LINE__, sta_id, vap->iv_opmode);
					adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, msg, msg_len);
						return ONEBOX_STATUS_FAILURE;
					}
				}else{
					printk(" %s %d: Ni is NULL \n", __func__, __LINE__);
					adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, msg, msg_len);
					return ONEBOX_STATUS_FAILURE;
				}
				ni->uapsd.eosp_tsf = *(uint32 *)&msg[16];
				ni->uapsd.eosp_triggered = 0;
				adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, msg, msg_len);
				//adapter->net80211_ops->ieee80211_beacon_miss(&adapter->vap_com);
			}
			else if (msg_type == RATE_GC_TABLE_UPDATE) {
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR ,(TEXT(" <===== RATE GC TABLE UPDATE RECVD =====>\n")) );
				adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, msg, msg_len);
      }
			else
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
						(TEXT("Reached FSM state %d :Not an internal management frame\n"), adapter->fsm_state));
				//	adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, msg, msg_len);
				onebox_mgmt_pkt_to_core(adapter, netbuf_cb, msg_len, msg_type);
				/* Freeing of the netbuf will be taken care in the above func itself. So, just return from here */
				return ONEBOX_STATUS_SUCCESS;
			}

		}
		break;
		default:
		{
			ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
			         		(TEXT("Reached FSM state %d :Not an internal management frame\n"), adapter->fsm_state));
			break;
		} 
	} /* End switch (adapter->fsm_state) */

	adapter->os_intf_ops->onebox_free_pkt(adapter, netbuf_cb, 0);
	FUNCTION_EXIT(ONEBOX_ZONE_MGMT_RCV);
	return ONEBOX_STATUS_SUCCESS;
}

/**
 * Entry point of Management module
 *
 * @param
 *  adapter    Pointer to driver private data
 * @param
 *  msg    Buffer recieved from the device
 * @param
 *  msg_len    Length of the buffer
 * @return
 *  Status
 */
int onebox_mgmt_pkt_to_core(PONEBOX_ADAPTER adapter,
			   netbuf_ctrl_block_t *netbuf_cb,
                            int32 msg_len,
                            uint8 type)
{
	int8 rssi;
	struct ieee80211com *ic = &adapter->vap_com;
	struct ieee80211vap *vap =NULL;
	uint16  sta_flags;
	uint8 conn_ni_mac[IEEE80211_ADDR_LEN];
	struct ieee80211_node *ni;
	uint32 vap_id;
 	uint8 *msg;
	uint8 pad_bytes;
	int8 chno;
	uint32 sta_index=0;

	FUNCTION_ENTRY(ONEBOX_ZONE_MGMT_RCV);
	vap	= TAILQ_FIRST(&ic->ic_vaps);
	msg = netbuf_cb->data;
	pad_bytes = msg[4];
	rssi = *(uint16 *)(msg + 16);
	chno = msg[15];

	if (type == RX_DOT11_MGMT)
	{
		msg_len -= pad_bytes;
		if ((msg_len <= 0) || (!msg))
		{
			ONEBOX_DEBUG(ONEBOX_ZONE_MGMT_SEND, 
			             (TEXT("Invalid incoming message of message length = %d\n"), msg_len));
			
			/*FIXME: Freeing the netbuf_cb as returning from here may not handle the netbuf free */
			adapter->os_intf_ops->onebox_free_pkt(adapter, netbuf_cb, 0);
			return ONEBOX_STATUS_FAILURE;
		}
		
		if(adapter->Driver_Mode == SNIFFER_MODE)
		{
			vap->hal_priv_vap->extended_desc_size = pad_bytes;
			adapter->os_intf_ops->onebox_netbuf_adj(netbuf_cb, (FRAME_DESC_SZ));
		}
		else
		{
			adapter->os_intf_ops->onebox_netbuf_adj(netbuf_cb, (FRAME_DESC_SZ + pad_bytes));
		}

#if 0

		if( msg[FRAME_DESC_SZ + pad_bytes] == 0x80)
		//if(buffer[0] == 0x50 || buffer[0] == 0x80)
		{
				if(vap->iv_state == IEEE80211_S_RUN)
				{
					ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("===> Beacon rcvd <===\n")));
					adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, buffer, 30);
					ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("===> vap->iv_myaddr  = %02x:%02x:%02x <===\n"), vap->iv_myaddr[0], vap->iv_myaddr[1], vap->iv_myaddr[2]));

				}
		}
#endif

    if(!(adapter->Driver_Mode == SNIFFER_MODE))
    {
		  recv_onair_dump(adapter, netbuf_cb->data, netbuf_cb->len);
    }

#ifdef PWR_SAVE_SUPPORT
		//support_mimic_uapsd(adapter, buffer);
#endif
		adapter->core_ops->onebox_indicate_pkt_to_net80211(adapter, netbuf_cb, rssi, chno);
		return 0;
	}
	else if(type == PS_NOTIFY_IND)
	{
		//adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, msg,  16);

		adapter->os_intf_ops->onebox_memcpy(conn_ni_mac, (msg + 6), IEEE80211_ADDR_LEN);

	//	printk(" PS NOTIFY: connected station %02x:%02x:%02x:%02x:%02x:%02x\n", conn_ni_mac[0], conn_ni_mac[1],
		//		conn_ni_mac[2], conn_ni_mac[3], conn_ni_mac[4], conn_ni_mac[5]);

		sta_flags = *(uint16 *) &msg[12]; /* station flags are defined in descriptor word6 */	
		vap_id = ((sta_flags & 0xC) >> 2);
		
		for (sta_index = 0; sta_index < adapter->max_stations_supported; sta_index++)
		{
			if(!adapter->os_intf_ops->onebox_memcmp(adapter->sta[sta_index].mac_addr, &conn_ni_mac, ETH_ALEN))
			{
					ni = adapter->sta[sta_index].ni;
					if(sta_index != ni->hal_priv_node.sta_id) {

							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
											(TEXT("ERROR: In %s Line %d The sta_id in Hal_station data structure and ni doesn't matches ni_sta_id %d hal_sta id %d\n"), 
											 __func__, __LINE__, ni->hal_priv_node.sta_id, sta_index));
							dump_stack();
							return ONEBOX_STATUS_FAILURE;

					}
				break;
			}
		}
		
		if(sta_index == adapter->max_stations_supported) /* In case if not already connected*/
		{

			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
					(TEXT("ERROR: In %s Line %d The sta_id in Hal_station data structure and ni doesn't matches hal_sta id %d\n"), 
					 __func__, __LINE__, sta_index));
			dump_stack();
			return ONEBOX_STATUS_FAILURE;
		}
		if(vap_id > 3)
		{
			printk("Vap_id is wrong in %s Line %d vap_id %d\n", __func__, __LINE__, vap_id );
			/*Freeing the netbuf coming as input to this function as the upper layers don't take care of the freeing*/
			adapter->os_intf_ops->onebox_free_pkt(adapter, netbuf_cb, 0);
			return ONEBOX_STATUS_FAILURE;
		}

		IEEE80211_LOCK(ic);
		if(!adapter->hal_vap[vap_id].vap_in_use) {
			printk("ERROR: In %s Line %d vap_id %d is not installed\n", __func__, __LINE__, vap_id );
			/*Freeing the netbuf coming as input to this function as the upper layers don't take care of the freeing*/
			adapter->os_intf_ops->onebox_free_pkt(adapter, netbuf_cb, 0);
			IEEE80211_UNLOCK(ic);
			return ONEBOX_STATUS_FAILURE;
			
		}

		TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) 
		{
			if(vap->hal_priv_vap->vap_id == vap_id)
			{
				break;
			}
		}

		/* Find the node of the station */
		if(!vap){
			printk("Unable to find the vap##########vap_id %d  \n", vap_id);
			IEEE80211_UNLOCK(ic);
				dump_stack();
			adapter->os_intf_ops->onebox_free_pkt(adapter, netbuf_cb, 0);
			return ONEBOX_STATUS_FAILURE;
		}

		if((&vap->iv_ic->ic_sta) == NULL)
		{
			printk("ERROR: In %s Line %d\n", __func__, __LINE__);

			return ONEBOX_STATUS_FAILURE;
		}

		ni = adapter->net80211_ops->onebox_find_node(&vap->iv_ic->ic_sta, conn_ni_mac);

		if(ni && (ni->ni_vap == vap))
		{
			//printk("Found the exact node\n");
		}
		else
		{
			printk("Unable to find the node so discarding\n");
			/*Freeing the netbuf coming as input to this function as the upper layers don't take care of the freeing*/
			adapter->os_intf_ops->onebox_free_pkt(adapter, netbuf_cb, 0);
			IEEE80211_UNLOCK(ic);
			return ONEBOX_STATUS_FAILURE;
		}

		if(sta_flags & STA_ENTERED_PS)
		{
			adapter->net80211_ops->onebox_node_pwrsave(ni, 1);
			//printk("powersave: STA Entered into pwrsave\n");
			/* Set the power management bit to indicate station entered into pwr save */
			//ni->ni_flags |= IEEE80211_NODE_PWR_MGT;
		}
		else if(sta_flags & STA_EXITED_PS)
		{
			/* Reset the power management bit to indicate station came out of pwr save */
		//	printk("powersave: STA Exited from pwrsave\n");
			//ni->ni_flags &= ~IEEE80211_NODE_PWR_MGT;
			adapter->net80211_ops->onebox_node_pwrsave(ni, 0);
		}
		IEEE80211_UNLOCK(ic);
	}
	else if(type == BEACON_EVENT_IND) {
		vap_id = msg[15];
		IEEE80211_LOCK(ic);
		if(!adapter->hal_vap[vap_id].vap_in_use) {
			printk("ERROR: In %s Line %d vap_id %d is not installed\n", __func__, __LINE__, vap_id );
			/*Freeing the netbuf coming as input to this function as the upper layers don't take care of the freeing*/
			adapter->os_intf_ops->onebox_free_pkt(adapter, netbuf_cb, 0);
			IEEE80211_UNLOCK(ic);
			return ONEBOX_STATUS_FAILURE;
			
		}
		TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) 
		{
			if(vap->hal_priv_vap->vap_id == vap_id)
			{
				/* We set beacon interrupt only when VAP reaches RUN state, as firmware
				 * gives interrupt after sending vap_caps frame, so due to this there is
				 * a probabilty of sending beacon before programming channel as we give highest
				 * priority to beacon event in core_qos_processor.
				 */
				if((vap->iv_state == IEEE80211_S_RUN) && (!adapter->block_ap_queues))
				{
					adapter->beacon_event = 1;
					adapter->beacon_event_vap_id = vap_id;
					adapter->beacon_interrupt++;
					//ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("Beacon Interrupt Received for vap_id %d \n"), vap_id));
					adapter->os_intf_ops->onebox_set_event(&(adapter->sdio_scheduler_event));
				}
			}
		}
			IEEE80211_UNLOCK(ic);
	}
	else if(type == DECRYPT_ERROR_IND)
	{
		adapter->core_ops->onebox_mic_failure(adapter, msg);

	}
	else
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_MGMT_SEND,(TEXT ("Internal Packet\n")));
	}

	/*Freeing the netbuf coming as input to this function as the upper layers don't take care of the freeing*/
	adapter->os_intf_ops->onebox_free_pkt(adapter, netbuf_cb, 0);
	FUNCTION_EXIT(ONEBOX_ZONE_MGMT_RCV);
	return 0;
}

/**
* Assigns the MAC address to the adapter
*
*/
ONEBOX_STATUS onebox_read_cardinfo(PONEBOX_ADAPTER adapter)
{
	ONEBOX_STATUS status = 0;
	EEPROM_READ mac_info_read;

	mac_info_read.off_set = EEPROM_READ_MAC_ADDR;
	mac_info_read.length  = 6;     /* Length in words i.e 3*2 = 6 bytes */

	ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT("%s: Reading MAC address\n"), __func__));
//	status = onebox_eeprom_read(adapter,(PEEPROM_READ)&mac_info_read);
	if (status != ONEBOX_STATUS_SUCCESS)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		             (TEXT("%s: EEPROM read Failed\n"), __func__));
	}
	return status;
}


int hal_set_sec_wpa_key(PONEBOX_ADAPTER adapter, const struct ieee80211_node *ni_sta, uint8 key_type)
{
	return 0;
}


uint32 hal_send_sta_notify_frame(PONEBOX_ADAPTER adapter, struct ieee80211_node *ni, uint8 notify_event)
{
#define P2P_GRP_GROUP_OWNER  0x0
	struct ieee80211vap *vap = NULL;
	onebox_mac_frame_t *mgmt_frame;
	ONEBOX_STATUS status;
	uint8  pkt_buffer[MAX_MGMT_PKT_SIZE];
	uint16 vap_id = 0;  

	FUNCTION_ENTRY (ONEBOX_ZONE_MGMT_SEND);

	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,
	             (TEXT("===> Sending Peer Notify Packet <===\n")));
	
	vap = ni->ni_vap;
	/* Allocating Memory For Mgmt Pkt From Mgmt Free Pool */
	mgmt_frame = (onebox_mac_frame_t *)pkt_buffer;

	adapter->os_intf_ops->onebox_memset(mgmt_frame, 0, MAX_MGMT_PKT_SIZE);
	
	/* Fill frame body */
	switch(vap->iv_opmode)
	{
		case IEEE80211_M_STA: 
		{	
			/* connected peer is AP */
			mgmt_frame->u.peer_notify.command =  ONEBOX_CPU_TO_LE16((IEEE80211_OP_AP) << 1); //ap
			break;
		}
		case IEEE80211_M_HOSTAP: 
		{	
			mgmt_frame->u.peer_notify.command =  ONEBOX_CPU_TO_LE16((IEEE80211_OP_STA) << 1); //sta
			break;
		}
		case IEEE80211_M_MBSS: 
		{	
			mgmt_frame->u.peer_notify.command =  ONEBOX_CPU_TO_LE16((IEEE80211_OP_IBSS) << 1); //IBSS
			break;
		}
		default:
			printk("Invalid Mode\n");
			return ONEBOX_STATUS_FAILURE;
	}

	vap_id = vap->hal_priv_vap->vap_id;
	printk(" In %s and %d notify_event = %d vap_id %d sta_id %d \n", __func__, __LINE__, notify_event, vap_id, ni->hal_priv_node.sta_id);

	switch(notify_event)
	{
		case STA_CONNECTED:
				if(vap->iv_opmode == IEEE80211_M_STA) {
						printk("Starting ifp queue \n");
						vap->hal_priv_vap->stop_tx_q = 0;
						adapter->os_intf_ops->onebox_start_ifp_txq(vap->iv_ifp);
				}
			mgmt_frame->u.peer_notify.command |= ONEBOX_ADD_PEER;
			break;
		case STA_DISCONNECTED:
			mgmt_frame->u.peer_notify.command |= ONEBOX_DELETE_PEER;
			if(!(vap->hal_priv_vap->roam_ind) && (vap->iv_opmode == IEEE80211_M_STA))
			{
				//adapter->sec_mode[vap_id] = IEEE80211_CIPHER_NONE;
			}
			ni->ni_flags &= ~IEEE80211_NODE_ENCRYPT_ENBL;
#ifdef BYPASS_RX_DATA_PATH
			/** This should be changed based on STA ID in AP mode **/
			if(vap->iv_opmode == IEEE80211_M_STA && (!adapter->sta_data_block))
			{
				onebox_send_block_unblock(vap, STA_DISCONNECTED, 0);
			}

			if(vap->iv_opmode == IEEE80211_M_STA) {	
							adapter->os_intf_ops->onebox_memset(&adapter->sta_mode.ptk_key, 0, sizeof(struct sta_conn_flags));

							printk("In %s Line %d  eapol %d ptk %d sta_block %d ap_block %d\n", __func__, __LINE__,
															adapter->sta_mode.eapol4_cnfrm, adapter->sta_mode.ptk_key, adapter->sta_data_block, adapter->block_ap_queues); 

							if(adapter->traffic_timer.function)
											adapter->os_intf_ops->onebox_remove_timer(&adapter->traffic_timer);

							update_pwr_save_status(vap, PS_ENABLE, DISCONNECTED_PATH);
			}

#endif
		break;
		case STA_ADDBA_DONE:
		case STA_DELBA:
		case STA_RX_ADDBA_DONE:
		case STA_RX_DELBA:
		/*	FIXME: handle here */
			status = onebox_send_ampdu_indication_frame(adapter, ni, notify_event);
			return status;
		break;
		default:
		break;
	}
	/* Fill the association id */
	printk(" association id =%x and command =%02x\n", ni->ni_associd, mgmt_frame->u.peer_notify.command);
	mgmt_frame->u.peer_notify.command |= ONEBOX_CPU_TO_LE16((ni->ni_associd & 0xfff) << 4);
	adapter->os_intf_ops->onebox_memcpy(mgmt_frame->u.peer_notify.mac_addr, ni->ni_macaddr, ETH_ALEN);
	/*  FIXME: Fill ampdu/amsdu size, short gi, GF if supported here */
	mgmt_frame->u.peer_notify.sta_flags |= ONEBOX_CPU_TO_LE32((ni->ni_flags & IEEE80211_NODE_QOS) ? 1 : 0); //connected ap is qos supported or not

	/* Bit{0:11} indicates length of the Packet
 	 * Bit{12:16} indicates host queue number
 	 */ 
	mgmt_frame->desc_word[0] = ONEBOX_CPU_TO_LE16((sizeof(mgmt_frame->u.peer_notify)) | ONEBOX_WIFI_MGMT_Q << 12);
	mgmt_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(PEER_NOTIFY); 
	// FIXME: IN AP Mode fill like this 
	mgmt_frame->desc_word[7] = ONEBOX_CPU_TO_LE16(ni->hal_priv_node.sta_id | (vap_id << 8));
	//mgmt_frame->desc_word[7] = ONEBOX_CPU_TO_LE16((vap_id << 8)); /* Peer ID is zero in sta mode */

	//adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, (PUCHAR)mgmt_frame, sizeof(mgmt_frame->u.peer_notify) + FRAME_DESC_SZ);


	//ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Sending peer notify frame\n")));
	printk("IN %s Line %d Vap_id %d Sta_id %d pkt_queued to head\n", __func__, __LINE__, vap_id, ni->hal_priv_node.sta_id);
	status = onebox_send_internal_mgmt_frame(adapter, (uint16 *)mgmt_frame, (sizeof(mgmt_frame->u.peer_notify)+ FRAME_DESC_SZ));

	if(status)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Unable to send peer notify frame\n")));
		return ONEBOX_STATUS_FAILURE;
	}

	printk("Opmode %d secmode %d \n", vap->iv_opmode, adapter->sec_mode[vap->hal_priv_vap->vap_id]);
	if(!((vap->iv_flags & IEEE80211_F_WPA2) || (vap->iv_flags & IEEE80211_F_WPA1)) 
			&& (vap->iv_flags & IEEE80211_F_PRIVACY))
  {
    /** WEP Mode */
    printk("Key idx %d key_len %d \n ", adapter->wep_key_idx[vap_id], adapter->wep_keylen[vap_id][adapter->wep_key_idx[vap_id]]);
    status = hal_load_key( adapter, adapter->wep_key[vap_id][0][0], 
                           adapter->wep_keylen[vap_id][adapter->wep_key_idx[vap_id]], 
                           ni->hal_priv_node.sta_id,
                           ONEBOX_PAIRWISE_KEY, 
                           adapter->wep_key_idx[vap_id], 
                           IEEE80211_CIPHER_WEP, vap);

    if(vap->iv_opmode == IEEE80211_M_STA)
    {
      status = hal_load_key(adapter, adapter->wep_key[vap_id][0][0], 
                             adapter->wep_keylen[vap_id][adapter->wep_key_idx[vap_id]], 
                             ni->hal_priv_node.sta_id, 
                             ONEBOX_GROUP_KEY, 
                             adapter->wep_key_idx[vap_id], 
                             IEEE80211_CIPHER_WEP, vap);
      ni->ni_vap->hal_priv_vap->conn_in_prog = 0;
    }
    ni->ni_flags |= IEEE80211_NODE_ENCRYPT_ENBL;
  }
	else if((vap->iv_opmode == IEEE80211_M_STA) 
					&& (vap->iv_state == IEEE80211_S_RUN)
					&& !(vap->iv_flags & IEEE80211_F_PRIVACY)) {
		/** OPEN Mode **/
			printk("In %s Line %d resetting conn_in_prog iv_state %d \n", __func__, __LINE__, vap->iv_state);
			ni->ni_vap->hal_priv_vap->conn_in_prog = 0;
	}

	return ONEBOX_STATUS_SUCCESS;
}

#ifdef BYPASS_TX_DATA_PATH
ONEBOX_STATUS onebox_send_block_unblock(struct ieee80211vap *vap, uint8 notify_event, uint8 quiet_enable)
{
	onebox_mac_frame_t *mgmt_frame;
	ONEBOX_STATUS status;
	uint8  pkt_buffer[MAX_MGMT_PKT_SIZE];
	struct net_device *parent_dev = vap->iv_ic->ic_ifp;
	PONEBOX_ADAPTER adapter = netdev_priv(parent_dev);
	struct ieee80211com *ic = vap->iv_ic;
	struct ieee80211vap *vap_temp;

	mgmt_frame = (onebox_mac_frame_t *)pkt_buffer;
	adapter->os_intf_ops->onebox_memset(mgmt_frame, 0, MAX_MGMT_PKT_SIZE);

	mgmt_frame->desc_word[0] = ONEBOX_CPU_TO_LE16(ONEBOX_WIFI_MGMT_Q << 12);
	mgmt_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(BLOCK_UNBLOCK);
	mgmt_frame->desc_word[3] = ONEBOX_CPU_TO_LE16(0x1);

	if(notify_event == STA_DISCONNECTED)
	{
		printk("In %s Line %d \n", __func__, __LINE__);
		vap->hal_priv_vap->sta_data_block = 1;
		adapter->sta_data_block = 1;
		if (quiet_enable)
			mgmt_frame->desc_word[3] |= ONEBOX_CPU_TO_LE16(0x2); /* Enable QUIET */
		mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16(0xf);
		/* We may receive disconnect event when we have programmed the timer 
 		 * so stop timer first
 		 */ 	
		printk("Stopping the timer in %s Line %d\n", __func__, __LINE__);
		ic->ic_stop_initial_timer(ic, vap);
		
		if(!adapter->os_intf_ops->onebox_is_ifp_txq_stopped(vap->iv_ifp))
		{
				printk("Stopping ifp queue \n");
				vap->hal_priv_vap->stop_tx_q = 1;
#ifndef WIFI_ALLIANCE
				adapter->os_intf_ops->onebox_stop_ifp_txq(vap->iv_ifp);
#endif
		}
	}
	else if(notify_event == STA_CONNECTED)
	{
		printk("In %s Line %d \n", __func__, __LINE__);
		vap->hal_priv_vap->sta_data_block = 0;
		adapter->sta_data_block = 0;
		adapter->block_ap_queues = 0;
		mgmt_frame->desc_word[5] = ONEBOX_CPU_TO_LE16(0xf);
	}

	if(adapter->block_ap_queues) {
		mgmt_frame->desc_word[4] |= ONEBOX_CPU_TO_LE16(0xf << 4);
		
	} else {
		TAILQ_FOREACH(vap_temp, &ic->ic_vaps, iv_next) 
		{ 
				if(vap_temp->iv_opmode == IEEE80211_M_HOSTAP) {
					printk("Starting ifp queue at %s Line %d\n", __func__, __LINE__);
					vap_temp->hal_priv_vap->stop_tx_q = 0;
					adapter->os_intf_ops->onebox_start_ifp_txq(vap_temp->iv_ifp);
				}
		}
		mgmt_frame->desc_word[5] |= ONEBOX_CPU_TO_LE16(0xf << 4);
	
	}

	//printk("<<<<<< BLOCK/UNBLOCK %d >>>>>>\n", notify_event);
	//adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, (PUCHAR)mgmt_frame, (FRAME_DESC_SZ));

	printk("In %s Line %d sta_block %d ap_data %d\n", __func__, __LINE__, adapter->sta_data_block, adapter->block_ap_queues);
	status = onebox_send_internal_mgmt_frame(adapter, (uint16 *)mgmt_frame,  FRAME_DESC_SZ);

	if(status)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Unable to send BLOCK/UNBLOCK indication frame\n")));
		return ONEBOX_STATUS_FAILURE;
	}

	return ONEBOX_STATUS_SUCCESS;
}
#endif

ONEBOX_STATUS onebox_send_ampdu_indication_frame(PONEBOX_ADAPTER adapter, struct ieee80211_node *ni, uint8 event)
{
	onebox_mac_frame_t *mgmt_frame;
	ONEBOX_STATUS status;
	uint8  pkt_buffer[MAX_MGMT_PKT_SIZE];
	uint16 tidno;
	/*FIXME: For ap mode get the peerid */
	/* Fill peer_id in case of AP mode. For sta mode it is zero */
	//uint8 peer_id = 0;
	uint8 peer_id = ni->hal_priv_node.sta_id;

	FUNCTION_ENTRY (ONEBOX_ZONE_MGMT_SEND);

	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,
	             (TEXT("===> Sending AMPDU Indication Packet <===\n")));

	/* Allocating Memory For Mgmt Pkt From Mgmt Free Pool */
	mgmt_frame = (onebox_mac_frame_t *)pkt_buffer;

	adapter->os_intf_ops->onebox_memset(mgmt_frame, 0, 256);

	tidno = ni->hal_priv_node.tidnum;
	/* Bit{0:11} indicates length of the Packet
	 * Bit{12:16} indicates host queue number
 	*/ 
	mgmt_frame->desc_word[0] = ONEBOX_CPU_TO_LE16((sizeof(mgmt_frame->u.ampdu_ind)) | ONEBOX_WIFI_MGMT_Q << 12);
	mgmt_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(AMPDU_IND); 

	printk("In %s and %d event %d \n", __func__, __LINE__, event);
	if(event == STA_ADDBA_DONE)
	{
		mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16(ni->hal_priv_node.tid[tidno].seq_start); 
		mgmt_frame->desc_word[5] = ONEBOX_CPU_TO_LE16(ni->hal_priv_node.tid[tidno].baw_size); 
		mgmt_frame->desc_word[7] = ONEBOX_CPU_TO_LE16(tidno | (START_AMPDU_AGGR << 4)| ( peer_id << 8)); 
	}
	else if(event == STA_RX_ADDBA_DONE)
	{
		mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16(ni->hal_priv_node.tid[tidno].seq_start); 
		mgmt_frame->desc_word[7] = ONEBOX_CPU_TO_LE16(tidno | (START_AMPDU_AGGR << 4)| (RX_BA_INDICATION << 5) | ( peer_id << 8)); 
	}
	else if(event == STA_DELBA)
	{
		mgmt_frame->desc_word[7] = ONEBOX_CPU_TO_LE16(tidno | (STOP_AMPDU_AGGR << 4)| (peer_id << 8)); 
	}
	else if(event == STA_RX_DELBA)
	{
		if(ni->hal_priv_node.delba_ind)
		{
			mgmt_frame->desc_word[7] = ONEBOX_CPU_TO_LE16(tidno | (STOP_AMPDU_AGGR << 4)| (RX_BA_INDICATION << 5) | (peer_id << 8)); 
		}
		else
		{
			mgmt_frame->desc_word[7] = ONEBOX_CPU_TO_LE16(tidno | (STOP_AMPDU_AGGR << 4)| (peer_id << 8)); 
		}
	}
		
	//ONEBOX_DEBUG(ONEBOX_ZONE_MGMT_SEND, (TEXT("Sending ampdu indication frame\n")));
	//adapter->core_ops->onebox_dump(ONEBOX_ZONE_MGMT_SEND, (PUCHAR)mgmt_frame, sizeof(mgmt_frame->u.ampdu_ind) + FRAME_DESC_SZ);

	status = onebox_send_internal_mgmt_frame(adapter, (uint16 *)mgmt_frame, (sizeof(mgmt_frame->u.ampdu_ind)+ FRAME_DESC_SZ));
	if(status)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT("Unable to send ampdu indication frame\n")));
		return ONEBOX_STATUS_FAILURE;
	}
	return ONEBOX_STATUS_SUCCESS;
}

ONEBOX_STATUS onebox_send_internal_mgmt_frame(PONEBOX_ADAPTER adapter, uint16 *addr, uint16 len)
{
	netbuf_ctrl_block_t *netbuf_cb = NULL;
	ONEBOX_STATUS status = ONEBOX_STATUS_SUCCESS;

	FUNCTION_ENTRY(ONEBOX_ZONE_MGMT_SEND);

	netbuf_cb = adapter->os_intf_ops->onebox_alloc_skb(len);
	if(netbuf_cb == NULL)
	{	
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s: Unable to allocate skb\n"), __func__));
		status = ONEBOX_STATUS_FAILURE;
		return status;

	}
	adapter->os_intf_ops->onebox_add_data_to_skb(netbuf_cb, len);
	/*copy the internal mgmt frame to netbuf and queue the pkt */
	adapter->os_intf_ops->onebox_memcpy((uint8 *)netbuf_cb->data, (uint8 *)addr, len);
	netbuf_cb->data[1] |= BIT(7);/* Immediate Wakeup bit*/
	netbuf_cb->flags |= INTERNAL_MGMT_PKT;	
	netbuf_cb->tx_pkt_type = WLAN_TX_M_Q;
	//adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, (uint8 *)netbuf_cb->data, netbuf_cb->len);
	//adapter->core_ops->onebox_dump(ONEBOX_ZONE_MGMT_SEND, (uint8 *)netbuf_cb->data, netbuf_cb->len);
	adapter->os_intf_ops->onebox_netbuf_queue_tail(&adapter->host_tx_queue[MGMT_SOFT_Q], netbuf_cb->pkt_addr);
	adapter->devdep_ops->onebox_schedule_pkt_for_tx(adapter);
	return status;
}


ONEBOX_STATUS update_device_op_params(PONEBOX_ADAPTER adapter)
{

	return ONEBOX_STATUS_SUCCESS;
}

/**
 * This function is called after initial configuration is done. 
 * It starts the base band and RF programming
 *
 * @param 
 *      adapter     Pointer to hal private info structure
 *
 * @return 
 *      0 on success, -1 on failure
 */
ONEBOX_STATUS program_bb_rf(PONEBOX_ADAPTER adapter)
{

	onebox_mac_frame_t *mgmt_frame;
	ONEBOX_STATUS status;
	uint8  pkt_buffer[FRAME_DESC_SZ];

	FUNCTION_ENTRY (ONEBOX_ZONE_MGMT_SEND);

	ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
	             (TEXT("===> Send BBP_RF_INIT frame in TA<===\n")));

	mgmt_frame = (onebox_mac_frame_t *)pkt_buffer;

	adapter->os_intf_ops->onebox_memset(mgmt_frame, 0, FRAME_DESC_SZ);

	mgmt_frame->desc_word[0] = ONEBOX_CPU_TO_LE16( ONEBOX_WIFI_MGMT_Q << 12);
	
	mgmt_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(BBP_PROG_IN_TA);
	mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16(adapter->endpoint);

	mgmt_frame->desc_word[3] = ONEBOX_CPU_TO_LE16(adapter->rf_pwr_mode);

	printk("#### in %s rf pwr mode is %d\n", __func__, adapter->rf_pwr_mode);

	if(adapter->rf_reset)
	{
		mgmt_frame->desc_word[7] = ONEBOX_CPU_TO_LE16 (RF_RESET_ENABLE);
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("===> RF RESET REQUEST SENT <===\n")));
		adapter->rf_reset = 0;
	}
	adapter->bb_rf_prog_count = 1;
#if 0
	if(adapter->chw_flag)
	{
		if(adapter->operating_chwidth == BW_40Mhz)
		{
			mgmt_frame->desc_word[4] |= ONEBOX_CPU_TO_LE16(0x1 << 8); /*20 to 40 Bandwidth swicth*/
		}
		else
		{
			mgmt_frame->desc_word[4] |= ONEBOX_CPU_TO_LE16(0x2 << 8);/*40 to 20 Bandwidth switch */
		}
		adapter->chw_flag = 0;
	}
#endif
	mgmt_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16 (PUT_BBP_RESET | BBP_REG_WRITE | (RSI_RF_TYPE << 4));
	//adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, (PUCHAR)mgmt_frame, FRAME_DESC_SZ );
	status = onebox_send_internal_mgmt_frame(adapter, (uint16 *)mgmt_frame,  FRAME_DESC_SZ);
	return status;
}

void onebox_init_chan_pwr_table(PONEBOX_ADAPTER adapter,
                                uint16 *bg_power_set,
                                uint16 *an_power_set)
{
	uint8  ii = 0, cnt = 0, band = 0;
	uint16 *low_indx, *med_indx, *high_indx;
	uint16 *alow_indx, *amed_indx, *ahigh_indx;

	if (adapter->eeprom_type >= EEPROM_VER2)
	{
		if (an_power_set == NULL)
		{
			/* In case of 2.4 GHz band, GC/PD values are given for only 3 channels */
			cnt = 3;
			band = BAND_2_4GHZ;
		}
		else
		{
			/* In case of 5 GHz band, GC/PD values are given for 8 channels */
			cnt = 8;
			band = BAND_5GHZ;
		} /* End if <condition> */  
		
		for (ii = 0; ii < cnt; ii++)
		{
			if (band == BAND_2_4GHZ)
			{
				adapter->os_intf_ops->onebox_memset(&adapter->TxPower11BG[ii], 0, sizeof(CHAN_PWR_TABLE));
				low_indx  = (uint16 *) ((&bg_power_set[0] + (BG_PWR_VAL_LEN * ii)));
				med_indx  = (uint16 *) ((&bg_power_set[0] + (BG_PWR_VAL_LEN * ii) + 1));
				high_indx = (uint16 *) ((&bg_power_set[0] + (BG_PWR_VAL_LEN * ii) + 2));

				adapter->TxPower11BG[ii].mid_pwr = *med_indx;
				adapter->TxPower11BG[ii].low_pwr = *low_indx;
				adapter->os_intf_ops->onebox_memcpy(&adapter->TxPower11BG[ii].high_pwr[0], 
				                                      high_indx, 
				                                      (BG_PWR_VAL_LEN - 2) * 2);
			}
			else
			{
				adapter->os_intf_ops->onebox_memset(&adapter->TxPower11A[ii],  0, sizeof(CHAN_PWR_TABLE));
				if (ii < 5)
				{
					/* Mapping between indices and channels is as follows
					 * 0 - 36 channel, 1 - 60 channel, 2 - 157 channel
					 * 3 - 120 channel, 4 - 11j channel
					 */
					alow_indx  = ((&an_power_set[0] + (AN_PWR_VAL_LEN * ii)));
					amed_indx  = ((&an_power_set[0] + (AN_PWR_VAL_LEN * ii) + 1));
					ahigh_indx = ((&an_power_set[0] + (AN_PWR_VAL_LEN * ii) + 2));
					adapter->TxPower11A[ii].mid_pwr  = *amed_indx;
					adapter->TxPower11A[ii].low_pwr  = *alow_indx;
					adapter->os_intf_ops->onebox_memcpy(&adapter->TxPower11A[ii].high_pwr[0],
					                                    ahigh_indx, 
					                                    (AN_PWR_VAL_LEN - 2) * 2);
				}
				else
				{
					/* For index 5 onwards, a single word of info is given, which needs
					 * to be applied to all rates and all pwr profiles. Mapping between
					 * channels and indices is as follows:
					 * 5 - 64 channel; 6 - 100 channel; 7 - 140 channel 
					 */
					ahigh_indx = (&an_power_set[0] + (AN_PWR_VAL_LEN * 5) + (ii - 5));
					/* FIXME - Anyways PD values are ignored as of now */
					adapter->os_intf_ops->onebox_memset(&adapter->TxPower11A[ii], 
					                                    *ahigh_indx, 
					                                    (AN_PWR_VAL_LEN * 2));
				} /* End if <condition> */
			} /* End if <condition> */
		} /* End for loop */
	}
	else 
	{
		for (ii = 0; ii < 3; ii++)
		{
			adapter->os_intf_ops->onebox_memset(&adapter->TxPower11BG[ii], 0, sizeof(CHAN_PWR_TABLE));
			adapter->os_intf_ops->onebox_memset(&adapter->TxPower11A[ii],  0, sizeof(CHAN_PWR_TABLE));
			if (adapter->eeprom_type == EEPROM_VER1)
			{
				/* New EEPROM Map */
				low_indx  = (uint16 *) ((&bg_power_set[0] + (BG_PWR_VAL_LEN * ii)));
				med_indx  = (uint16 *) ((&bg_power_set[0] + (BG_PWR_VAL_LEN * ii) + 1));
				high_indx = (uint16 *) ((&bg_power_set[0] + (BG_PWR_VAL_LEN * ii) + 2));
				    
				adapter->TxPower11BG[ii].mid_pwr = *med_indx;
				adapter->TxPower11BG[ii].low_pwr = *low_indx;
				adapter->os_intf_ops->onebox_memcpy(&adapter->TxPower11BG[ii].high_pwr[0], 
				                                    high_indx, 
				                                    (BG_PWR_VAL_LEN - 2) * 2);
				
				if (adapter->RFType == ONEBOX_RF_8230)
				{
					alow_indx  = ((&an_power_set[0] + (AN_PWR_VAL_LEN * ii)));
					amed_indx  = ((&an_power_set[0] + (AN_PWR_VAL_LEN * ii) + 1));
					ahigh_indx = ((&an_power_set[0] + (AN_PWR_VAL_LEN * ii) + 2));
					adapter->TxPower11A[ii].mid_pwr  = *amed_indx;
					adapter->TxPower11A[ii].low_pwr  = *alow_indx;
					adapter->os_intf_ops->onebox_memcpy(&adapter->TxPower11A[ii].high_pwr[0],
					                                    ahigh_indx, 
					                                    (AN_PWR_VAL_LEN - 2) * 2);
				}
			}
			else
			{
				if (adapter->RFType == ONEBOX_RF_8230)
				{
					if (adapter->operating_band == BAND_5GHZ)
					{
						adapter->power_mode = ONEBOX_PWR_HIGH;
					}
					ahigh_indx   = ((&an_power_set[0] + (LEGACY_AN_PWR_VAL_LEN * ii)));
					adapter->os_intf_ops->onebox_memcpy(&adapter->TxPower11A[ii].high_pwr[0], 
					                                    ahigh_indx, 
					                                    LEGACY_AN_PWR_VAL_LEN * 2);
				}
				
				low_indx  = (uint16 *) ((&bg_power_set[0] + (LEGACY_BG_PWR_VAL_LEN * ii)));
				med_indx  = (uint16 *) ((&bg_power_set[0] + (LEGACY_BG_PWR_VAL_LEN * ii) + 1));
				high_indx = (uint16 *) ((&bg_power_set[0] + (LEGACY_BG_PWR_VAL_LEN * ii) + 2));
				
				adapter->TxPower11BG[ii].low_pwr = *low_indx;
				adapter->TxPower11BG[ii].mid_pwr = *med_indx;
				adapter->os_intf_ops->onebox_memcpy(&adapter->TxPower11BG[ii].high_pwr[0], 
				                                    high_indx, 
				                                    (LEGACY_BG_PWR_VAL_LEN - 2) * 2);
			} /* End if <condition> */
		} /* End for loop */
	} /* End if <condition> */
	return;
} 

ONEBOX_STATUS set_vap_capabilities(struct ieee80211vap *vap, uint8 vap_status)
{
	onebox_mac_frame_t *mgmt_frame;
	ONEBOX_STATUS status;
	uint8  pkt_buffer[MAX_MGMT_PKT_SIZE];
	PONEBOX_ADAPTER adapter = NULL;
	uint8 opmode ;
	struct ieee80211com *ic = NULL;

	FUNCTION_ENTRY (ONEBOX_ZONE_MGMT_SEND);

	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,
	             (TEXT("===> Sending Vap Capabilities Packet <===\n")));

	if(is_vap_valid(vap) < 0) {
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("ERROR: VAP is Not a valid pointer In %s Line %d, So returning Recvd vap_status %d\n "), 
								__func__, __LINE__, vap_status));
		dump_stack();
		return ONEBOX_STATUS_FAILURE;

	}

	adapter = (PONEBOX_ADAPTER)vap->hal_priv_vap->hal_priv_ptr;
	ic = &adapter->vap_com;
	opmode = vap->iv_opmode;

	/* Allocating Memory For Mgmt Pkt From Mgmt Free Pool */
	mgmt_frame = (onebox_mac_frame_t *)pkt_buffer;
	adapter->os_intf_ops->onebox_memset(mgmt_frame, 0, 256);
	switch(opmode)
	{
		case IEEE80211_M_STA:
			opmode = IEEE80211_OP_STA;
		break;
		case IEEE80211_M_HOSTAP:
			opmode = IEEE80211_OP_AP;
			//adapter->sta_data_block = 0; //FIXME: Handle during Multiple Vaps; Kept this for P2P mode
		break;
		case IEEE80211_M_IBSS:
			opmode = IEEE80211_OP_IBSS;
		break;
		default:
	/* FIXME: In case of P2P after connection if the device becomes GO indicate opmode in some way to firmware */
			opmode = IEEE80211_OP_P2P_GO;
		break;
	}

	/* Fill the length & host queue num */	
	mgmt_frame->desc_word[0] = ONEBOX_CPU_TO_LE16(sizeof(mgmt_frame->u.vap_caps) | (ONEBOX_WIFI_MGMT_Q << 12));
	/* Fill the frame type */
	mgmt_frame->desc_word[1] = (ONEBOX_CPU_TO_LE16(VAP_CAPABILITIES));
	mgmt_frame->desc_word[2] |= (ONEBOX_CPU_TO_LE16(vap_status) << 8);
	if (vap->p2p_enable) {
		if (vap->p2p_mode == IEEE80211_P2P_GO) {
			mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16(IEEE80211_OP_P2P_GO | adapter->ch_bandwidth << 8);
		} else {
			mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16(IEEE80211_OP_P2P_CLIENT | adapter->ch_bandwidth << 8);
		}
	} else {
		mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16(opmode | adapter->ch_bandwidth << 8);
	}
	/* FIXME: Fill antenna Info here */
	mgmt_frame->desc_word[5] = 0;  
	printk("In func %s Line %d Vap_id %d \n", __func__, __LINE__, vap->hal_priv_vap->vap_id);
	mgmt_frame->desc_word[7] = ONEBOX_CPU_TO_LE16((vap->hal_priv_vap->vap_id << 8) | (adapter->mac_id << 4) | adapter->radio_id);
	//mgmt_frame->desc_word[7] = ONEBOX_CPU_TO_LE16((vap_id << 8) | (adapter->mac_id << 4) | adapter->radio_id);

	/*Frame Body*/	
	//adapter->os_intf_ops->onebox_memcpy(mgmt_frame->u.vap_caps.mac_addr, adapter->mac_addr, IEEE80211_ADDR_LEN);
	adapter->os_intf_ops->onebox_memcpy(mgmt_frame->u.vap_caps.mac_addr, vap->iv_myaddr, IEEE80211_ADDR_LEN);
	//Default value for keep alive period is 90secs.
	mgmt_frame->u.vap_caps.keep_alive_period = 90; 
	//adapter->os_intf_ops->onebox_memcpy(mgmt_frame->u.vap_caps.bssid, bssid, IEEE80211_ADDR_LEN);
	if(ic->ic_flags & IEEE80211_F_USEPROT)
	{
		/* Enable this bit if a non erp station is present or if the sizeof the pkt is greater than RTS threshold*/
		if(ic->ic_nonerpsta)
		{
			printk("Enabling self cts bit\n");
			mgmt_frame->u.vap_caps.flags |= ONEBOX_CPU_TO_LE32(ONEBOX_SELF_CTS_ENABLE);
		}
	}
	mgmt_frame->u.vap_caps.frag_threshold = ONEBOX_CPU_TO_LE16(2346); 
	mgmt_frame->u.vap_caps.rts_threshold = ONEBOX_CPU_TO_LE16(vap->iv_rtsthreshold);
	mgmt_frame->u.vap_caps.default_mgmt_rate_bbpinfo = ONEBOX_CPU_TO_LE32(RSI_RATE_6);
	mgmt_frame->u.vap_caps.beacon_miss_threshold = ONEBOX_CPU_TO_LE16(vap->iv_bmissthreshold);
	if(adapter->operating_band == BAND_5GHZ)
	{
			mgmt_frame->u.vap_caps.default_ctrl_rate_bbpinfo = ONEBOX_CPU_TO_LE32(RSI_RATE_6);
			printk("vap->iv_flags_ht = %x IEEE80211_FHT_USEHT40 = %x\n",vap->iv_flags_ht, IEEE80211_FHT_USEHT40);
			if(vap->iv_flags_ht & IEEE80211_FHT_USEHT40)
			{
					if(ic->band_flags & IEEE80211_CHAN_HT40U)
					{
							/* primary channel is below secondary channel */
							mgmt_frame->u.vap_caps.default_ctrl_rate_bbpinfo |= ONEBOX_CPU_TO_LE32(LOWER_20_ENABLE << 16);
					}
					else
					{
							/* primary channel is above secondary channel */
							mgmt_frame->u.vap_caps.default_ctrl_rate_bbpinfo |= ONEBOX_CPU_TO_LE32((UPPER_20_ENABLE << 16));
					}
					printk("full 40 rate in vap caps\n");	
			}
	}
	else
	{
			mgmt_frame->u.vap_caps.default_ctrl_rate_bbpinfo = ONEBOX_CPU_TO_LE32(RSI_RATE_1);
			/* 2.4 Ghz band */
			if(ic->band_flags & IEEE80211_CHAN_HT40U)
			{
				/* primary channel is below secondary channel */
				mgmt_frame->u.vap_caps.default_ctrl_rate_bbpinfo |= ONEBOX_CPU_TO_LE32(LOWER_20_ENABLE << 16);
			}
			else
			{
				/* primary channel is above secondary channel */
				mgmt_frame->u.vap_caps.default_ctrl_rate_bbpinfo |= ONEBOX_CPU_TO_LE32((UPPER_20_ENABLE << 16));
			}
	}
	mgmt_frame->u.vap_caps.default_data_rate_bbpinfo = ONEBOX_CPU_TO_LE32(0);
	mgmt_frame->u.vap_caps.beacon_interval = ONEBOX_CPU_TO_LE16(ic->ic_bintval);
	mgmt_frame->u.vap_caps.dtim_period = ONEBOX_CPU_TO_LE16(vap->iv_dtim_period);
	//printk("VAP CAPABILITIES\n");	
	//adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, (PUCHAR)mgmt_frame, sizeof(mgmt_frame->u.vap_caps) + FRAME_DESC_SZ);
	status = onebox_send_internal_mgmt_frame(adapter, (uint16 *)mgmt_frame, (sizeof(mgmt_frame->u.vap_caps) + FRAME_DESC_SZ));
	if(status)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Unable to send vap capabilities  frame\n")));
		return ONEBOX_STATUS_FAILURE;
	}
	return ONEBOX_STATUS_SUCCESS;
}



ONEBOX_STATUS onebox_send_vap_dynamic_update_indication_frame(struct ieee80211vap *vap)
{

 	struct dynamic_s *dynamic_frame=NULL;
	struct ieee80211com * ic=NULL;
	PONEBOX_ADAPTER adapter =NULL;
	ONEBOX_STATUS status;
	uint8  pkt_buffer[MAX_MGMT_PKT_SIZE];

	FUNCTION_ENTRY (ONEBOX_ZONE_MGMT_SEND);
	
	adapter = (PONEBOX_ADAPTER)vap->hal_priv_vap->hal_priv_ptr;
	ic = &adapter->vap_com;

	dynamic_frame = (struct dynamic_s *)pkt_buffer;

	adapter->os_intf_ops->onebox_memset(dynamic_frame, 0, 256);

	dynamic_frame->desc_word[0] = ONEBOX_CPU_TO_LE16((sizeof(dynamic_frame->frame_body)) | (ONEBOX_WIFI_MGMT_Q << 12));
	dynamic_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(VAP_DYNAMIC_UPDATE); 
	dynamic_frame->desc_word[4] = ONEBOX_CPU_TO_LE16(vap->iv_rtsthreshold);
	//dynamic_frame->desc_word[5] = ONEBOX_CPU_TO_LE16(vap->iv_fragthreshold );
	dynamic_frame->desc_word[6] = ONEBOX_CPU_TO_LE16(vap->iv_bmissthreshold);
	dynamic_frame->frame_body.keep_alive_period = ONEBOX_CPU_TO_LE16(vap->iv_keep_alive_period); 

	if(ic->ic_flags & IEEE80211_F_USEPROT) {

			if((ic->ic_nonerpsta && (vap->iv_opmode == IEEE80211_M_HOSTAP)) 
			   || (vap->iv_opmode == IEEE80211_M_STA)) {
				printk("Enabling self cts bit\n");
				dynamic_frame->desc_word[2] |= ONEBOX_CPU_TO_LE32(ONEBOX_SELF_CTS_ENABLE);
			}
	}

	if(vap->hal_priv_vap->fixed_rate_enable) {
		dynamic_frame->desc_word[3] |= ONEBOX_CPU_TO_LE16(ONEBOX_FIXED_RATE_EN); //Fixed rate is enabled
		dynamic_frame->frame_body.data_rate = ONEBOX_CPU_TO_LE16(vap->hal_priv_vap->rate_hix);
	}

	dynamic_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16((vap->hal_priv_vap->vap_id << 8));
	printk("IN %s Line %d VAP_ID %d\n", __func__, __LINE__, vap->hal_priv_vap->vap_id);

	status = onebox_send_internal_mgmt_frame(adapter, (uint16 *)dynamic_frame, (sizeof(struct dynamic_s)));

	if(status) {
		ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT("Unable to send vap dynamic update indication frame\n")));
		return ONEBOX_STATUS_FAILURE;
	}
	
	return ONEBOX_STATUS_SUCCESS;
}

/**
 * This function load the station update frame to PPE
 *
 * @param
 *  hal_info Pointer to the hal information structure
 * @param
 *  sta_offset sta_id of the station
 * @param
 *  data Station update information buffer
 * @param
 *  len Length of the station update frame
 *
 * @return
 *  This function returns ONEBOX_STATUS_SUCCESS if template loading
 *  is successful otherwise ONEBOX_STATUS_FAILURE.
 */
ONEBOX_STATUS hal_load_key(PONEBOX_ADAPTER adapter,
                           uint8 *data, 
                           uint16 key_len, 
                           uint16 sta_id,
                           uint8 key_type,
                           uint8 key_id,
                           uint32 cipher,
                           struct ieee80211vap *vap)
{
	onebox_mac_frame_t *mgmt_frame;
	uint8  pkt_buffer[MAX_MGMT_PKT_SIZE]; 
	ONEBOX_STATUS status;
	uint8 key_t1 = 0;
	uint16 key_descriptor = 0;
	struct ieee80211com *ic = NULL;
	uint32 vap_id = vap->hal_priv_vap->vap_id;
	FUNCTION_ENTRY(ONEBOX_ZONE_INFO);
	
	ic = &adapter->vap_com;
	mgmt_frame = (onebox_mac_frame_t *)pkt_buffer;

	adapter->os_intf_ops->onebox_memset(mgmt_frame, 0, 256);
	switch (key_type) 
	{
		case ONEBOX_GROUP_KEY:
			/* Load the key into PPE*/
			key_t1 = 1 << 1;
			if(vap->iv_opmode == IEEE80211_M_HOSTAP)
			{
				key_descriptor = ONEBOX_BIT(7);
			}
			else {
				printk("<==== Recvd Group Key ====>\n");
				if((sta_id >= adapter->max_stations_supported) || !(adapter->sta_connected_bitmap[sta_id/32] & (BIT(sta_id%32)))) {
						ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT( " Invalid Sta_id %d in %s Line %d \n"),sta_id, __func__, __LINE__));
						return -1;
				}
			}
			break;
		case ONEBOX_PAIRWISE_KEY:
			/* Load the key into PPE */
				printk("<==== Recvd Pairwise Key ====>\n");
				if((sta_id >= adapter->max_stations_supported) || !(adapter->sta_connected_bitmap[sta_id/32] & (BIT(sta_id%32)))) {
						ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT( "ERROR: Invalid Sta_id %d in %s Line %d \n"),sta_id, __func__, __LINE__));
						return -1;
				
				}
			key_t1 = 0 << 1;
			if(cipher != IEEE80211_CIPHER_WEP)
			{
				key_id = 0;
			}
			break;
	}
	key_descriptor |= key_t1 | ONEBOX_BIT(13) | (key_id << 14);
	if(cipher == IEEE80211_CIPHER_WEP)
	{
		key_descriptor |= ONEBOX_BIT(2);
		if(key_len >= 13)
		{
			key_descriptor |= ONEBOX_BIT(3);
		}
	}
	else if(cipher != IEEE80211_CIPHER_NONE)
	{
		key_descriptor |= ONEBOX_BIT(4);
		if(cipher == IEEE80211_CIPHER_TKIP)
		{
			key_descriptor |= ONEBOX_BIT(5);
		}
	}
	mgmt_frame->desc_word[0] = ONEBOX_CPU_TO_LE16(sizeof(mgmt_frame->u.set_key) | (ONEBOX_WIFI_MGMT_Q << 12));
	mgmt_frame->desc_word[1] = (ONEBOX_CPU_TO_LE16(SET_KEY));
//	mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16((key_t1) | (1<<13)| (key_id <<14) | (1<<4) | (1<<5));
//	mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16((key_descriptor) | (1<<5) | (1<<4));
	mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16((key_descriptor));

	mgmt_frame->desc_word[7] = ONEBOX_CPU_TO_LE16((sta_id));
	mgmt_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16(vap_id << 8);
	printk("In %s Line %d sta_id %d vap_id %d key_type %d\n", __func__, __LINE__, sta_id, vap_id, key_type );
	
	//ONEBOX_DEBUG(ONEBOX_ZONE_DEBUG, (TEXT("In %s %d \n"), __func__, __LINE__));
	//adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, (PUCHAR)data, FRAME_DESC_SZ);

	//adapter->os_intf_ops->onebox_memcpy(&mgmt_frame->u.set_key.key, data, 16);
	//adapter->os_intf_ops->onebox_memcpy(&mgmt_frame->u.set_key.tx_mic_key, &data[16], 16);
	//adapter->os_intf_ops->onebox_memcpy(&mgmt_frame->u.set_key.rx_mic_key, &data[24], 8);
	//adapter->os_intf_ops->onebox_memcpy(&mgmt_frame->u.set_key.key, data, sizeof(mgmt_frame->u.set_key));
	if(data) {
	adapter->os_intf_ops->onebox_memcpy(&mgmt_frame->u.set_key.key, data, 4*32);
	adapter->os_intf_ops->onebox_memcpy(&mgmt_frame->u.set_key.tx_mic_key, &data[16], 8);
	adapter->os_intf_ops->onebox_memcpy(&mgmt_frame->u.set_key.rx_mic_key, &data[24], 8);
	} else {
		adapter->os_intf_ops->onebox_memset(&mgmt_frame->u.set_key, 0, sizeof(mgmt_frame->u.set_key));
	}

	//mgmt_frame->u.set_key.key = data;
	//adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, (PUCHAR)mgmt_frame, sizeof(mgmt_frame->u.set_key) + FRAME_DESC_SZ);

	status = onebox_send_internal_mgmt_frame(adapter, (uint16 *)mgmt_frame, (sizeof(mgmt_frame->u.set_key)+ FRAME_DESC_SZ));
	if(status)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT("Unable to load the keys frame\n")));
		return ONEBOX_STATUS_FAILURE;
	}


	if(vap->iv_opmode == IEEE80211_M_STA) {
		if((key_type == ONEBOX_PAIRWISE_KEY)) 
		{
			adapter->sta_mode.ptk_key = 1;
		}	
		else if((key_type == ONEBOX_GROUP_KEY)) 
		{
			adapter->sta_mode.gtk_key = 1;

		}

		if((adapter->sta_mode.ptk_key) && 
		   (adapter->sta_mode.eapol4_cnfrm) && 
		   (adapter->sta_data_block)) {
				printk("In %s Line %d  eapol %d ptk %d sta_block %d ap_block %d\n", __func__, __LINE__,
								adapter->sta_mode.eapol4_cnfrm, adapter->sta_mode.ptk_key, adapter->sta_data_block, adapter->block_ap_queues); 
			onebox_send_block_unblock(vap, STA_CONNECTED, 0);
		}
		if(adapter->sta_mode.ptk_key && adapter->sta_mode.gtk_key && adapter->sta_mode.eapol4_cnfrm) {
			//onebox_send_sta_supported_features(vap, adapter);
			printk("calling timeout initialziation In %s Line %d\n", __func__, __LINE__);
			printk("Resting ptk ket variable in %s Line %d \n", __func__, __LINE__);
			adapter->sta_mode.ptk_key = 0; 
			//initialize_sta_support_feature_timeout(vap, adapter);
			update_pwr_save_status(vap, PS_ENABLE, CONNECTED_PATH);

		}
	}

	return ONEBOX_STATUS_SUCCESS;
}


/* This function sends bootup parameters frame to TA.
 * @param pointer to driver private structure
 * @return 0 if success else -1. 
 */   
uint8 onebox_load_bootup_params(PONEBOX_ADAPTER adapter)
{         
	onebox_mac_frame_t *mgmt_frame;
	ONEBOX_STATUS status;
	uint8  pkt_buffer[MAX_MGMT_PKT_SIZE];

	FUNCTION_ENTRY (ONEBOX_ZONE_MGMT_SEND);

	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,
	             (TEXT("===> Sending Bootup parameters Packet <===\n")));

	/* Allocating Memory For Mgmt Pkt From Mgmt Free Pool */
	mgmt_frame = (onebox_mac_frame_t *)pkt_buffer;

	adapter->os_intf_ops->onebox_memset(mgmt_frame, 0, 256);
	if (adapter->operating_chwidth == BW_40Mhz)
	{  
		adapter->os_intf_ops->onebox_memcpy(&mgmt_frame->u.bootup_params,
				                            &boot_params_40,
				                            sizeof(BOOTUP_PARAMETERS));
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
				(TEXT("===> Sending Bootup parameters Packet 40MHZ <=== %d\n"),UMAC_CLK_40BW));
		mgmt_frame->desc_word[7] = ONEBOX_CPU_TO_LE16(UMAC_CLK_40BW);
	}
	else
	{
		adapter->os_intf_ops->onebox_memcpy(&mgmt_frame->u.bootup_params,
				                            &boot_params_20,
				                            sizeof(BOOTUP_PARAMETERS));
		if (boot_params_20.valid == VALID_20)
		{
			mgmt_frame->desc_word[7] = ONEBOX_CPU_TO_LE16(UMAC_CLK_20BW);
			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
					(TEXT("===> Sending Bootup parameters Packet 20MHZ <=== %d \n"),UMAC_CLK_20BW));
		}
		else
		{
			//FIXME: This should not occur need to remove
			mgmt_frame->desc_word[7] = ONEBOX_CPU_TO_LE16(UMAC_CLK_40MHZ);
			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
					(TEXT("===>ERROR: Sending Bootup parameters Packet for  40MHZ  In %s Line %d <=== %d \n"),__func__, __LINE__, UMAC_CLK_40MHZ));
		}	
	}  
	/* Bit{0:11} indicates length of the Packet
 	 * Bit{12:15} indicates host queue number
 	 */ 
	mgmt_frame->desc_word[0] = ONEBOX_CPU_TO_LE16(sizeof(BOOTUP_PARAMETERS) | (ONEBOX_WIFI_MGMT_Q << 12));
	mgmt_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(BOOTUP_PARAMS_REQUEST);

	//adapter->core_ops->onebox_dump(ONEBOX_ZONE_DEBUG, (PUCHAR)mgmt_frame, (FRAME_DESC_SZ + sizeof(BOOTUP_PARAMETERS)));
	status = onebox_send_internal_mgmt_frame(adapter, (uint16 *)mgmt_frame, (sizeof(BOOTUP_PARAMETERS)+ FRAME_DESC_SZ));


	return status;
} /*end of onebox_load_bootup_params */
EXPORT_SYMBOL(onebox_load_bootup_params);

/**
 * This function prepares reset MAC request frame and send it to LMAC.
 *
 * @param  Pointer to Adapter structure.  
 * @return 0 if success else -1. 
 */
ONEBOX_STATUS onebox_send_reset_mac(PONEBOX_ADAPTER adapter)
{
	struct driver_assets *d_assets = onebox_get_driver_asset();
	onebox_mac_frame_t *mgmt_frame;
	ONEBOX_STATUS status;
	uint8  pkt_buffer[FRAME_DESC_SZ];

	FUNCTION_ENTRY (ONEBOX_ZONE_MGMT_SEND);

	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,
	             (TEXT("===> Send Reset Mac frame <===\n")));

	/* Allocating Memory For Mgmt Pkt From Mgmt Free Pool */
	mgmt_frame = (onebox_mac_frame_t *)pkt_buffer;

	adapter->os_intf_ops->onebox_memset(mgmt_frame, 0, FRAME_DESC_SZ);

	/* Bit{0:11} indicates length of the Packet
 	 * Bit{12:16} indicates host queue number
 	 */ 

	mgmt_frame->desc_word[0] = ONEBOX_CPU_TO_LE16(ONEBOX_WIFI_MGMT_Q << 12);
	/* Fill frame type for reset mac request */
	mgmt_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(RESET_MAC_REQ);
	if (adapter->Driver_Mode == RF_EVAL_MODE_ON)
	{
		mgmt_frame->desc_word[5] = ONEBOX_CPU_TO_LE16(1); //Value is (2 - 1)
	}
	else if (adapter->Driver_Mode == SNIFFER_MODE)
	{
		mgmt_frame->desc_word[3] = ONEBOX_CPU_TO_LE16(1);
	}
	if (adapter->calib_mode)
	{
		mgmt_frame->desc_word[5] |= ONEBOX_CPU_TO_LE16((1) << 8); 
	}
	if (adapter->per_lpbk_mode)
	{
		mgmt_frame->desc_word[5] |= ONEBOX_CPU_TO_LE16((2) << 8);
	}
#ifdef BYPASS_RX_DATA_PATH
	mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16(0x0001);
#endif
	mgmt_frame->desc_word[4] |= ONEBOX_CPU_TO_LE16(RETRY_COUNT << 8);
		
	/*TA level aggregation of pkts to host */
  if(adapter->Driver_Mode == SNIFFER_MODE)
  {
	  mgmt_frame->desc_word[3] |=  (1 << 8);
  }
  else
  {
	  mgmt_frame->desc_word[3] |=  (d_assets->ta_aggr << 8);
  }
	//adapter->core_ops->onebox_dump(ONEBOX_ZONE_MGMT_SEND, (PUCHAR)mgmt_frame, (FRAME_DESC_SZ));
	status = onebox_send_internal_mgmt_frame(adapter, (uint16 *)mgmt_frame, FRAME_DESC_SZ);

	return status;
} /*end of onebox_send_reset_mac */

ONEBOX_STATUS set_channel(PONEBOX_ADAPTER adapter, uint16 chno)
{
	/* Prepare the scan request using the chno information */
	struct ieee80211com *ic = NULL;
	onebox_mac_frame_t *mgmt_frame;
	int32 status = 0;
	uint16 frame[256];
	uint16 ch_num;
	ch_num = chno;
#ifndef PROGRAMMING_SCAN_TA
	uint16 *rf_prog_vals;
	uint16 vals_per_set;
	uint8 count;
#endif
	ic = &adapter->vap_com;
	
	FUNCTION_ENTRY(ONEBOX_ZONE_INIT);
	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
	             (TEXT("%s: Sending scan req frame\n"), __func__));
	/* Allocating Memory For Mgmt Pkt From Mgmt Free Pool */
	mgmt_frame = (onebox_mac_frame_t *)frame;
	adapter->os_intf_ops->onebox_memset(mgmt_frame, 0, 256);

	
	mgmt_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(SCAN_REQUEST);
	mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16(chno);/*channel num is required */
	//mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE32() & 0xFFFF;   //FIXME SCAN_DURATION in usec
	//mgmt_frame->desc_word[5] = ONEBOX_CPU_TO_LE32() & 0xFFFF0000;   //FIXME SCAN_DURATION in usec
	
#ifdef RF_8111
	mgmt_frame->desc_word[7] = ONEBOX_CPU_TO_LE16 (PUT_BBP_RESET | BBP_REG_WRITE | (RSI_RF_TYPE << 4));
#else
	/* RF type here is 8230 */
	mgmt_frame->desc_word[7] = ONEBOX_CPU_TO_LE16 (PUT_BBP_RESET | BBP_REG_WRITE | (NONRSI_RF_TYPE << 4));
#endif
	//mgmt_frame->desc_word[7] = ONEBOX_CPU_TO_LE16(radio_id << 8);				 
	
	/* Do the channel no translation in case of 5Ghz band */
	if (adapter->operating_band == BAND_5GHZ)
	{
		if(adapter->operating_chwidth == BW_20Mhz)
		{
			if ((chno >= 36) && (chno <= 64))
			{        
				chno = ((chno - 32) / 4);
			}
			else if ((chno > 64) && (chno <= 140))
			{
				chno = ((chno - 100) / 4) + 9;
			} 
			else if(chno >= 149)
			{
				chno = ((chno - 149) / 4) + 20;
			} 
			else
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Invalid chno %d, operating_band = %d\n"), chno, adapter->operating_band));
				return ONEBOX_STATUS_FAILURE;
			} /* End if <condition> */
		}
		else /* For 40Mhz bandwidth */
		{
			if ((chno >= 36) && (chno <= 64))
			{        
				chno = ((chno - 34) / 4);
			}
			else if ((chno > 64) && (chno <= 140))
			{
				chno = ((chno - 102) / 4) + 8;
			} 
			else if(chno >= 149)
			{
				chno = ((chno - 151) / 4) + 18;
			} 
			else
			{
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Invalid chno %d, operating_band = %d\n"), chno, adapter->operating_band));
				return ONEBOX_STATUS_FAILURE;
			} /* End if <condition> */
		}
	}
	else if(chno > 14)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Invalid chno %d, operating_band = %d\n"), chno, adapter->operating_band));
		return ONEBOX_STATUS_FAILURE;
	}

#ifndef PROGRAMMING_SCAN_TA
	if(adapter->operating_band == BAND_5GHZ && ((adapter->RFType == ONEBOX_RF_8230) || (adapter->RFType == ONEBOX_RF_8111)))
	{
		if(adapter->operating_chwidth == BW_40Mhz)
		{
			printk("programming The 40Mhz center freq values in 5Ghz chno =%d\n", chno);
			rf_prog_vals = &adapter->onebox_channel_vals_amode_40[chno].val[0];
			vals_per_set = adapter->onebox_channel_vals_amode_40[chno].vals_row;
		}
		else
		{
			rf_prog_vals = &adapter->onebox_channel_vals_amode[chno].val[0];
			vals_per_set = adapter->onebox_channel_vals_amode[chno].vals_row;       
		}
	}
	else 
	{
		printk("In %s Line %d openrating chwidth = %d\n", __func__, __LINE__,adapter->operating_chwidth);
		if(adapter->operating_chwidth == BW_40Mhz)
		{
			printk("programming The 40Mhz center freq values in 2Ghz chno =%d\n", chno);
			rf_prog_vals = &adapter->onebox_channel_vals_bgmode_40[chno].val[0];
			vals_per_set = adapter->onebox_channel_vals_bgmode_40[chno].vals_row;
		}
		else
		{
			rf_prog_vals = &adapter->onebox_channel_vals_bgmode[chno].val[0];
			vals_per_set = adapter->onebox_channel_vals_bgmode[chno].vals_row;
		}
	}

	mgmt_frame->u.rf_prog_req.rf_prog_vals[0] = ONEBOX_CPU_TO_LE16(vals_per_set + 1);	//indicating no.of vals to fw
	for (count = 1; count <= vals_per_set; count++)
	{
		mgmt_frame->u.rf_prog_req.rf_prog_vals[count] = ONEBOX_CPU_TO_LE16(rf_prog_vals[count - 1]);
	}
	
	if((adapter->RFType == ONEBOX_RF_8111) || (adapter->RFType == ONEBOX_RF_8230))
	{
		mgmt_frame->u.rf_prog_req.rf_prog_vals[count] = ONEBOX_CPU_TO_LE16(250);	//padding delay
	}
	else
	{
		printk("%s: unknown rf type\n", __func__);
	}
#else
	printk("scan values in device ch_bandwidth = %d ch_num %d \n", adapter->operating_chwidth, ch_num);
	//mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16(ic->ic_curchan->ic_ieee);
	if (adapter->Driver_Mode == RF_EVAL_MODE_ON) 
	{
		adapter->ch_power = adapter->endpoint_params.power;
		mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16(ch_num);
	} 
	else if (adapter->Driver_Mode == SNIFFER_MODE)
	{
		adapter->ch_power = adapter->endpoint_params.power;
		mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16(ch_num);
    ic->ic_curchan->ic_ieee = ch_num;
		if(ch_num == 14)
			ic->ic_curchan->ic_freq = 2484;
		else if (ch_num < 14)
			ic->ic_curchan->ic_freq = 2407 + ch_num * 5;
		else if (ch_num >= 36)
			ic->ic_curchan->ic_freq = 5000 + ch_num * 5;
	}
	else
	{  
		//adapter->ch_power = ic->ic_curchan->ic_maxregpower;
    if(ic->ic_txpowlimit > ic->ic_curchan->ic_maxregpower)
    {
		  adapter->ch_power = ic->ic_curchan->ic_maxregpower;
    }else{
		  adapter->ch_power = ic->ic_txpowlimit;
    }
		mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16(ic->ic_curchan->ic_ieee);
	}
	mgmt_frame->desc_word[4] |= ONEBOX_CPU_TO_LE16(chno << 8);/*channel index */
	mgmt_frame->desc_word[5] = ONEBOX_CPU_TO_LE16(0x1);/*scan values in TA */
	mgmt_frame->desc_word[6] = ONEBOX_CPU_TO_LE16(adapter->ch_power);/*POWER VALUE*/
	printk("IN %s Line %d TX_PWER is %d\n", __func__, __LINE__, adapter->ch_power);
	if(adapter->operating_chwidth == BW_40Mhz)
	{
			printk("40Mhz is enabled\n");
		mgmt_frame->desc_word[5] |= ONEBOX_CPU_TO_LE16(0x1 << 8);/*scan values in TA */
	}

#endif

	//ONEBOX_DEBUG(ONEBOX_ZONE_ERROR , (TEXT("Sending Scan Request frame \n")));
	//adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, (PUCHAR)mgmt_frame,sizeof(mgmt_frame->u.rf_prog_req) + FRAME_DESC_SZ );
#ifndef PROGRAMMING_SCAN_TA
	status= onebox_send_internal_mgmt_frame(adapter, (uint16 *)mgmt_frame, (sizeof(mgmt_frame->u.rf_prog_req) + FRAME_DESC_SZ));
	mgmt_frame->desc_word[0] = ONEBOX_CPU_TO_LE16((sizeof(mgmt_frame->u.rf_prog_req)) | (ONEBOX_WIFI_MGMT_Q << 12));
#else
	mgmt_frame->desc_word[0] = ONEBOX_CPU_TO_LE16(ONEBOX_WIFI_MGMT_Q << 12);
	status= onebox_send_internal_mgmt_frame(adapter, (uint16 *)mgmt_frame,  FRAME_DESC_SZ);
#endif

	FUNCTION_EXIT(ONEBOX_ZONE_INIT);
	return ONEBOX_STATUS_SUCCESS;
}

ONEBOX_STATUS onebox_umac_init_done (PONEBOX_ADAPTER adapter)
{
#ifdef USE_USB_INTF
//FIXME 
//it is here as usb will disconnect and connect again. so adapter will be reset
	adapter->mac_addr[0] = 0;
	adapter->mac_addr[1] = 0x23;
	adapter->mac_addr[2] = 0xa7;
	adapter->mac_addr[3] = 0x04;
	adapter->mac_addr[4] = 0x02;
	adapter->mac_addr[5] = 0x48;
	adapter->operating_band = BAND_2_4GHZ;
	adapter->def_chwidth = BW_20Mhz;
	adapter->operating_chwidth = BW_20Mhz;
#ifdef RF_8111
	adapter->RFType = ONEBOX_RF_8111;
#else
	adapter->RFType = ONEBOX_RF_8230;
#endif    
	if (onebox_load_config_vals(adapter) != 0)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR ,
		             (TEXT("%s: Initializing the Configuration vals Failed\n"), __func__));
		return ONEBOX_STATUS_FAILURE;
	}
#endif
	adapter->core_ops->onebox_core_init(adapter);
	return ONEBOX_STATUS_SUCCESS;
}
EXPORT_SYMBOL(onebox_umac_init_done); /* coex */

/**
 * This function is to set channel, Band and Channel Bandwidth
 * specified by user in PER mode.
 *
 * @param  Pointer to adapter structure.  
 * @param  Power Save Cmd.  
 * @return 0 if success else -1. 
 */
ONEBOX_STATUS band_check (PONEBOX_ADAPTER adapter)
{
	uint8 set_band = 0;
	uint8 previous_chwidth = 0;
#ifdef PROGRAMMING_BBP_TA	
	uint8 previous_endpoint = 0;
	previous_endpoint = adapter->endpoint;
#endif
	if(adapter->endpoint_params.channel <= 14)
	{
		set_band = BAND_2_4GHZ;
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Band is 2GHz\n")));	
	}
	else if((adapter->endpoint_params.channel >= 36) && (adapter->endpoint_params.channel <= 165))
	{
		set_band = BAND_5GHZ;
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Band is 5GHz\n")));	
	}
	else
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Invalid channel issued by user\n")));	
		return ONEBOX_STATUS_FAILURE;
	}
	// per_ch_bw:  0 - 20MHz,  4 - Lower 40MHz, 2 - Upper 40MHz, 6 - Full 40MHz
	previous_chwidth = adapter->operating_chwidth;
	if (adapter->endpoint_params.per_ch_bw)
	{
		adapter->operating_chwidth = BW_40Mhz;
	}
	else
	{
		adapter->operating_chwidth = BW_20Mhz;
	}
#ifndef PROGRAMMING_BBP_TA	
	if (adapter->operating_band != set_band)
	{
		adapter->operating_band = set_band;
		adapter->rf_reset = 1;
		printk(" RF_RESET AFTER BAND CHANGE  = :\n");
		adapter->devdep_ops->onebox_program_bb_rf(adapter);
		if (adapter->operating_chwidth == BW_40Mhz)
		{  
			adapter->chw_flag = 1;
			adapter->devdep_ops->onebox_program_bb_rf(adapter);
		}
	}
	else
	{
		if (adapter->operating_chwidth != previous_chwidth)
		{
			adapter->chw_flag = 1;
			adapter->devdep_ops->onebox_program_bb_rf(adapter);
		}
	}
#else	
	if (adapter->operating_band != set_band)
	{
		adapter->rf_reset = 1;
		adapter->operating_band = set_band;
	}
	if (!set_band)
	{
		if (adapter->operating_chwidth == BW_40Mhz)
		{  
			adapter->endpoint = 1;
		}
		else
		{
			adapter->endpoint = 0;
		}
	}
	else
	{
		if (adapter->operating_chwidth == BW_40Mhz)
		{  
			adapter->endpoint = 3;
		}
		else
		{
			adapter->endpoint = 2;
		}
	}
	if ((adapter->endpoint != previous_endpoint) || (adapter->rf_power_mode_change))
	{
		adapter->rf_power_mode_change = 0;
		adapter->devdep_ops->onebox_program_bb_rf(adapter);
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT(" ENDPOINT : %d \n"),adapter->endpoint));
	}
#endif
	if (adapter->operating_chwidth != previous_chwidth)
	{
		adapter->chw_flag = 1;
		if(onebox_load_bootup_params(adapter) == ONEBOX_STATUS_SUCCESS)
		{
			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s: BOOTUP Parameters loaded successfully\n"),
						__FUNCTION__));
		}
		else
		{
			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT ("%s: Failed to load bootup parameters\n"), 
						__FUNCTION__));
		}
		if(onebox_load_radio_caps(adapter))
		{
			return ONEBOX_STATUS_FAILURE;
		}
	}
  	else if ((adapter->operating_chwidth == BW_40Mhz) && 
          (adapter->primary_channel != adapter->endpoint_params.per_ch_bw))
  	{
	  	adapter->primary_channel = adapter->endpoint_params.per_ch_bw;
	  	if(onebox_load_radio_caps(adapter))
	  	{
			return ONEBOX_STATUS_FAILURE;
	  	}
  	}
	return ONEBOX_STATUS_SUCCESS;
}

/**
 * This function programs BB and RF values provided 
 * using MATLAB.
 *
 * @param  Pointer to adapter structure.  
 * @param  Power Save Cmd.  
 * @return 0 if success else -1. 
 */
ONEBOX_STATUS set_bb_rf_values (PONEBOX_ADAPTER adapter, struct iwreq *wrq )
{
	uint8 i = 0;
	uint8 type = 0; 
	uint16 len = 0;
	uint8 rf_len = 0;
	ONEBOX_STATUS status = ONEBOX_STATUS_SUCCESS;

	ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("ONEBOX_read_bb_rf_values: Address  = %x  value = %d "),
				adapter->bb_rf_params.Data[0],
				adapter->bb_rf_params.value));
	printk(" No of values = %d No of fields = %d:\n",adapter->bb_rf_params.no_of_values,adapter->bb_rf_params.no_of_fields);
	adapter->bb_rf_params.Data[0] =  adapter->bb_rf_params.no_of_values;
	type =  adapter->bb_rf_params.value;
	for(i = 0; i <= adapter->bb_rf_params.no_of_values; i++)
		printk(" bb_rf_params.Data[] = %x :\n",adapter->bb_rf_params.Data[i]);

	if(type % 2 == 0)
	{
		adapter->bb_rf_rw = 1; //set_read
		ONEBOX_DEBUG(ONEBOX_ZONE_INFO,  (TEXT(" *** read  \n")));
	}
	adapter->soft_reset = adapter->bb_rf_params.soft_reset;

	adapter->os_intf_ops->onebox_reset_event(&(adapter->bb_rf_event));

	if(type == BB_WRITE_REQ || type == BB_READ_REQ )
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_INFO,  (TEXT("ONEBOX_read_buf_values : BB_REQ\n")));
		if(onebox_mgmt_send_bb_prog_frame(adapter, adapter->bb_rf_params.Data, adapter->bb_rf_params.no_of_values) != ONEBOX_STATUS_SUCCESS)
		{
			return ONEBOX_STATUS_FAILURE;
		}
	}
	else if((type == RF_WRITE_REQ) || (type == RF_READ_REQ)
			|| (type == ULP_READ_REQ) || (type == ULP_WRITE_REQ))
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_INFO,  (TEXT("ONEBOX_read_buf_values : RF_REQ\n")));
    if (type == RF_WRITE_REQ)
    {  
      adapter->bb_rf_params.no_of_fields = 3;
      if(onebox_mgmt_send_rf_prog_frame(adapter,&adapter->bb_rf_params.Data[2], adapter->bb_rf_params.no_of_values, adapter->bb_rf_params.no_of_fields, RSI_RF_TYPE ) != ONEBOX_STATUS_SUCCESS)
      {
        return ONEBOX_STATUS_FAILURE;
      }
    }
    else 
    {
      if(onebox_mgmt_send_rf_prog_frame(adapter,&adapter->bb_rf_params.Data[1], adapter->bb_rf_params.no_of_values, adapter->bb_rf_params.no_of_fields, RSI_RF_TYPE ) != ONEBOX_STATUS_SUCCESS)
      {
        return ONEBOX_STATUS_FAILURE;
      }
    }
  }
	else if(type == BUF_READ_REQ || type == BUF_WRITE_REQ )
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_INFO,  (TEXT("ONEBOX_read_buf_values : BB_BUFFER\n")));
		if(onebox_bb_buffer_request(adapter, adapter->bb_rf_params.Data, adapter->bb_rf_params.no_of_values) != ONEBOX_STATUS_SUCCESS)
		{
			return ONEBOX_STATUS_FAILURE;
		}
	}
	else if(type == RF_LOOPBACK_M2 || type == RF_LOOPBACK_M3 )
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,  (TEXT("ONEBOX_read_buf_values : RF_LOOPBACK_REQ\n")));
//    len =  wrq->u.data.length;
      if ( type == RF_LOOPBACK_M3 )
	    len =  256*2;
	else
		len = 2048 * 2;
    adapter->rf_lpbk_len = 0;
	   adapter->os_intf_ops->onebox_memset(&adapter->rf_lpbk_data[0], 0, len );
		if(onebox_rf_loopback(adapter, adapter->bb_rf_params.Data, adapter->bb_rf_params.no_of_values, type) != ONEBOX_STATUS_SUCCESS)
		{
			return ONEBOX_STATUS_FAILURE;
		}
	}
  else if(type == LMAC_REG_WRITE || type == LMAC_REG_READ )
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_INFO,  (TEXT("ONEBOX_read_buf_values : LMAC_REG_READ/WRITE\n")));
		if(onebox_mgmt_lmac_reg_ops_req(adapter, adapter->bb_rf_params.Data, type) != ONEBOX_STATUS_SUCCESS)
		{
			return ONEBOX_STATUS_FAILURE;
		}
	}
	else if(type == RF_RESET_REQ)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,  (TEXT("ONEBOX_read_buf_values : RF_RESET_REQ\n")));
		if(onebox_mgmt_send_rf_reset_req(adapter, adapter->bb_rf_params.Data) != ONEBOX_STATUS_SUCCESS)
		{
			return ONEBOX_STATUS_FAILURE;
		}
	}
	else if(type == EEPROM_RF_PROG_WRITE || type == EEPROM_RF_PROG_READ )
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,  (TEXT("ONEBOX_read_buf_values : EEPROM_RF_PROG_READ\n")));
//		if(eeprom_read(adapter, adapter->bb_rf_params.Data) != ONEBOX_STATUS_SUCCESS)
//		if(eeprom_read(adapter, cw_mode_buf_write_array) != ONEBOX_STATUS_SUCCESS)
		{
			return ONEBOX_STATUS_FAILURE;
		}
	}
	else
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,  (TEXT(
						"%s: Failed to perform operation type =%d\n"), __func__, type));
		return -EFAULT;
	}
	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,  (TEXT(" Written success and trying to read  \n")));
	if(type % 2 == 0)
	{
    adapter->os_intf_ops->onebox_reset_event(&(adapter->bb_rf_event));
    adapter->os_intf_ops->onebox_wait_event(&(adapter->bb_rf_event), 10000); 
    //adapter->os_intf_ops->onebox_acquire_spinlock(&adapter->lock_bb_rf, 0);
    if (type == RF_LOOPBACK_M3 || type == RF_LOOPBACK_M2)
    {
      printk("Initial rf_lpbk_len : %d",adapter->rf_lpbk_len);
   //   while (rf_len < adapter->rf_lpbk_len)
      { 
      printk("Initial else rf_lpbk_len : %d",adapter->rf_lpbk_len);
	    adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, (PUCHAR)&adapter->rf_lpbk_data[0], adapter->rf_lpbk_len );
        if(copy_to_user((wrq->u.data.pointer), &adapter->rf_lpbk_data[0], adapter->rf_lpbk_len))
        {
          ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,  (TEXT(
                  "onebox_ioctl: Failed to perform operation\n")));
 //         adapter->os_intf_ops->onebox_release_spinlock(&adapter->lock_bb_rf, 0);
          return -EFAULT;
        }
        printk("rf_lpbk_len : %d, rf_len: %d, len: %d",adapter->rf_lpbk_len,rf_len,len);
        rf_len += adapter->rf_lpbk_len/2;
        //adapter->os_intf_ops->onebox_wait_event(&(adapter->bb_rf_event), 10000); 
      }
	      adapter->os_intf_ops->onebox_memset(&adapter->rf_lpbk_data[0], 0, adapter->rf_lpbk_len );
        adapter->rf_lpbk_len = 0;
    } 
    else 
    {  
		wrq->u.data.length = sizeof(bb_rf_params_t);

		if(copy_to_user(wrq->u.data.pointer, &adapter->bb_rf_read, sizeof(bb_rf_params_t)))
		{
			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,  (TEXT(
							"onebox_ioctl: Failed to perform operation\n")));
			adapter->os_intf_ops->onebox_release_spinlock(&adapter->lock_bb_rf, 0);
			return -EFAULT;
		}
    }
//		adapter->os_intf_ops->onebox_release_spinlock(&adapter->lock_bb_rf, 0);
		for(i=0;i<adapter->bb_rf_read.no_of_values;i++)
			ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT("After reading bb_rf_read.Data[] = 0x%x \n"),adapter->bb_rf_read.Data[i]));
	}
	ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT(
					"%s: Success in performing operation\n"), __func__));
	return status;
}  

/**
 * This function programs BBP registers for CW transmissions 
 *
 * @param  Pointer to adapter structure.  
 * @param  Power Save Cmd.  
 * @return 0 if success else -1. 
 */

ONEBOX_STATUS set_cw_mode (PONEBOX_ADAPTER adapter, uint8 mode)
{
	onebox_mac_frame_t *mgmt_frame;
	ONEBOX_STATUS status_l;
	uint8  pkt_buffer[MAX_MGMT_PKT_SIZE];
	int i = 0;
	int j = 0;
	uint16 *cw_mode_buf_write_array;

	FUNCTION_ENTRY (ONEBOX_ZONE_MGMT_SEND);

	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,
	             (TEXT("===> Sending CW transmission Programming Packet <===\n")));

	/* Allocating Memory For Mgmt Pkt From Mgmt Free Pool */
	mgmt_frame = (onebox_mac_frame_t *)pkt_buffer;

	adapter->os_intf_ops->onebox_memset(mgmt_frame, 0, MAX_MGMT_PKT_SIZE);

	/*prepare the frame descriptor */
	mgmt_frame->desc_word[0] = ONEBOX_CPU_TO_LE16(ONEBOX_WIFI_MGMT_Q << 12);
	mgmt_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(CW_MODE_REQ);

	adapter->soft_reset = 0;

	mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16((adapter->cw_sub_type << 8) | adapter->cw_type);

	//adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, (PUCHAR)mgmt_frame, FRAME_DESC_SZ );
	status_l = onebox_send_internal_mgmt_frame(adapter, (uint16 *)mgmt_frame, (FRAME_DESC_SZ));
  if (status_l != ONEBOX_STATUS_SUCCESS)
	{
		return status_l;
	}
	else
	{
    printk("CW MODE write waiting \n");
    adapter->os_intf_ops->onebox_reset_event(&(adapter->bb_rf_event));
    adapter->os_intf_ops->onebox_wait_event(&(adapter->bb_rf_event), 10000); 
#if 1
    //mode = 2;
    printk("CW_mode %d\n",mode);
    switch (mode)
    {
      case 0:
      case 1:
        cw_mode_buf_write_array = cw_mode_buf_write_array_1;
        break;
      case 2:
        cw_mode_buf_write_array = cw_mode_buf_write_array_2;
        break;
      case 3:
        cw_mode_buf_write_array = cw_mode_buf_write_array_3;
        break;
      case 4:
        cw_mode_buf_write_array = cw_mode_buf_write_array_4;
        break;
      case 5:
        cw_mode_buf_write_array = cw_mode_buf_write_array_5;
        break;
      case 6:
        cw_mode_buf_write_array = cw_mode_buf_write_array_6;
        break;
      case 7:
        //cw_mode_buf_write_array = cw_mode_buf_write_array_7;
        cw_mode_buf_write_array = cw_mode_buf_write_array_1;
        break;
      default:
        {
          cw_mode_buf_write_array = cw_mode_buf_write_array_1;
          break;
        }
    } /* End switch */
		i = 6;
		while (i< (258*3))
		{
			memset (&adapter->bb_rf_params, 0, sizeof (bb_rf_params_t));
			adapter->bb_rf_params.Data[1] = 0x315;
			adapter->bb_rf_params.Data[2] = 0x316;
			adapter->bb_rf_params.Data[3] = 0x317;
			for (j =4; j< (35*3);j+=3 )
			{
				if (i >= 258*3 )
					break;	
				adapter->bb_rf_params.Data[j] = cw_mode_buf_write_array[i];
				adapter->bb_rf_params.Data[j+1] = cw_mode_buf_write_array[i+1];
				adapter->bb_rf_params.Data[j+2] = cw_mode_buf_write_array[i+2];
				printk("***** Data[%d] = 0x%x ,cw_mode_buf_write_array[%d] = 0x%x\n",j,adapter->bb_rf_params.Data[j],i,cw_mode_buf_write_array[i]);
				i+=3;
			}	
			adapter->bb_rf_params.value = 7; //BUFFER_WRITE
			adapter->bb_rf_params.no_of_values = 34;
      adapter->soft_reset = BBP_REMOVE_SOFT_RST_AFTER_PROG;
			if(onebox_bb_buffer_request(adapter, adapter->bb_rf_params.Data, adapter->bb_rf_params.no_of_values) != ONEBOX_STATUS_SUCCESS)
			{
				return ONEBOX_STATUS_FAILURE;
			}
      printk("CW MODE BUFFER write waiting \n");
      adapter->os_intf_ops->onebox_reset_event(&(adapter->bb_rf_event));
      adapter->os_intf_ops->onebox_wait_event(&(adapter->bb_rf_event), 10000); 
		}	
			memset (&adapter->bb_rf_params, 0, sizeof (bb_rf_params_t));
			adapter->bb_rf_params.Data[1] = 0x318;
			adapter->bb_rf_params.Data[2] = 0x80;
      adapter->bb_rf_params.value = 1; //BB_WRITE
			adapter->bb_rf_params.no_of_values = 2;
			adapter->soft_reset = 0;
		if(onebox_mgmt_send_bb_prog_frame(adapter, adapter->bb_rf_params.Data, adapter->bb_rf_params.no_of_values) != ONEBOX_STATUS_SUCCESS)
		{
			return ONEBOX_STATUS_FAILURE;
		}

#endif
	}	
	return status_l;
}


ONEBOX_STATUS onebox_rf_loopback(PONEBOX_ADAPTER adapter,
                                             uint16 *bb_prog_vals,
                                             uint16 num_of_vals, uint8 type)
{
	onebox_mac_frame_t *mgmt_frame;
	ONEBOX_STATUS status_l;
	uint16 frame_len;
	uint8  pkt_buffer[MAX_MGMT_PKT_SIZE]; //1024* 4(4 bytes data) = 512*4*2(bytes)

	FUNCTION_ENTRY (ONEBOX_ZONE_MGMT_SEND);

	ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
	             (TEXT("===> Sending RF_LOOPBACK Programming Packet <===\n")));

	/* Allocating Memory For Mgmt Pkt From Mgmt Free Pool */
	mgmt_frame = (onebox_mac_frame_t *)pkt_buffer;

	adapter->os_intf_ops->onebox_memset(mgmt_frame, 0, MAX_MGMT_PKT_SIZE);
// FIXME this commented code may be used for LOOPBACK write for bebugging
#if 0
	adapter->core_ops->onebox_dump(ONEBOX_ZONE_MGMT_DUMP, (PUCHAR)bb_prog_vals, num_of_vals);
	if (adapter->bb_rf_rw)
	{		
  	mgmt_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16(BBP_REG_READ);
		adapter->bb_rf_rw = 0;
    frame_len = 0;
	}
  else
  {
    /* Preparing BB Request Frame Body */
    for (count=1; ((count < num_of_vals) && (ii< num_of_vals)); ii++, count+=2) 
    {
      mgmt_frame->u.bb_prog_req[ii].reg_addr = ONEBOX_CPU_TO_LE16(bb_prog_vals[count]);
      mgmt_frame->u.bb_prog_req[ii].bb_prog_vals = ONEBOX_CPU_TO_LE16(bb_prog_vals[count+1]);
    }

    if (num_of_vals % 2)
    {
      mgmt_frame->u.bb_prog_req[ii].reg_addr = ONEBOX_CPU_TO_LE16(bb_prog_vals[count]);
    }
    /* Preparing BB Request Frame Header */
    frame_len = ((num_of_vals) * 2);	//each 2 bytes
  }  
#endif
    frame_len = 0;
	/*prepare the frame descriptor */
	mgmt_frame->desc_word[0] = ONEBOX_CPU_TO_LE16((frame_len) | (ONEBOX_WIFI_MGMT_Q << 12));
	if( type == RF_LOOPBACK_M2 )
	{		
	mgmt_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(RF_LOOPBACK_REQ);
	}
	else
	{
	mgmt_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(RF_LPBK_M3);
	}	

#if 0  
	if (adapter->soft_reset & BBP_REMOVE_SOFT_RST_BEFORE_PROG)
	{
  		mgmt_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16(BBP_REMOVE_SOFT_RST_BEFORE_PROG);
	}	
	if (adapter->soft_reset & BBP_REMOVE_SOFT_RST_AFTER_PROG)
	{
  		mgmt_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16(BBP_REMOVE_SOFT_RST_AFTER_PROG);	
	}	
  	adapter->soft_reset = 0;
	
	//Flags are not handled FIXME:
	mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16(num_of_vals/2);
	//FIXME: What is the radio id to fill here
//	mgmt_frame->desc_word[7] |= ONEBOX_CPU_TO_LE16 (RADIO_ID << 8 );

	mgmt_frame->desc_word[5] = ONEBOX_CPU_TO_LE16(bb_prog_vals[0]);
#endif	

	//adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, (PUCHAR)mgmt_frame, (frame_len + FRAME_DESC_SZ ));
	status_l = onebox_send_internal_mgmt_frame(adapter, (uint16 *)mgmt_frame, (frame_len + FRAME_DESC_SZ));

	return status_l;
}


ONEBOX_STATUS eeprom_read(PONEBOX_ADAPTER adapter)
{
	uint16 pkt_len = 0;
	uint8  *pkt_buffer;

	onebox_mac_frame_t *mgmt_frame;
	ONEBOX_STATUS status;

	pkt_len = FRAME_DESC_SZ;
	pkt_buffer = adapter->os_intf_ops->onebox_mem_zalloc(pkt_len, GFP_ATOMIC);
	mgmt_frame = (onebox_mac_frame_t *)pkt_buffer;

	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
			(TEXT("===> Frame to PERFORM EEPROM READ <===\n")));

	/* FrameType*/
	mgmt_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(EEPROM_READ_TYPE);


	mgmt_frame->desc_word[0] = ONEBOX_CPU_TO_LE16(ONEBOX_WIFI_MGMT_Q << 12 | (pkt_len - FRAME_DESC_SZ));

	/* Number of bytes to read*/
	printk(" offset = 0x%x, length = %d\n",adapter->eeprom.offset,adapter->eeprom.length);
	mgmt_frame->desc_word[3] = ONEBOX_CPU_TO_LE16(adapter->eeprom.length << 4);
	mgmt_frame->desc_word[2] |= ONEBOX_CPU_TO_LE16(3 << 8); //hsize = 3 as 32 bit transfer

	/* Address to read*/
	mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16(adapter->eeprom.offset);
	mgmt_frame->desc_word[5] = ONEBOX_CPU_TO_LE16(adapter->eeprom.offset >> 16);
	mgmt_frame->desc_word[6] = ONEBOX_CPU_TO_LE16(0); //delay = 0

	status = onebox_send_internal_mgmt_frame(adapter, (uint16 *)mgmt_frame, pkt_len);
	adapter->os_intf_ops->onebox_mem_free(pkt_buffer);
	return status;
}

/**
 * This function prepares reset RX filter request frame and send it to LMAC.
 *
 * @param  Pointer to Adapter structure.  
 * @return 0 if success else -1. 
 */
ONEBOX_STATUS onebox_send_rx_filter_frame(PONEBOX_ADAPTER adapter, uint16_t rx_filter_word)
{
	onebox_mac_frame_t *mgmt_frame;
	ONEBOX_STATUS status;
	uint8  pkt_buffer[FRAME_DESC_SZ];

	FUNCTION_ENTRY (ONEBOX_ZONE_MGMT_SEND);

	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,
	             (TEXT("===> Send Rx Filter frame <===\n")));
        printk("Rx filter word is %x\n", rx_filter_word);

	/* Allocating Memory For Mgmt Pkt From Mgmt Free Pool */
	mgmt_frame = (onebox_mac_frame_t *)pkt_buffer;
	adapter->os_intf_ops->onebox_memset(mgmt_frame, 0, FRAME_DESC_SZ);

	/* Bit{0:11} indicates length of the Packet
 	 * Bit{12:16} indicates host queue number
 	 */ 

	mgmt_frame->desc_word[0] = ONEBOX_CPU_TO_LE16(ONEBOX_WIFI_MGMT_Q << 12);
	/* Fill frame type set_rx_filter */
	mgmt_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(SET_RX_FILTER);
	/* Fill data in form of flags*/
	mgmt_frame->desc_word[4] = ONEBOX_CPU_TO_LE16(rx_filter_word);

	adapter->core_ops->onebox_dump(ONEBOX_ZONE_MGMT_SEND, (PUCHAR)mgmt_frame, (FRAME_DESC_SZ));
	status = onebox_send_internal_mgmt_frame(adapter, (uint16 *)mgmt_frame, FRAME_DESC_SZ);
	return status;
} /*end of onebox_send_rx_filter_frame */
EXPORT_SYMBOL(onebox_send_rx_filter_frame);
