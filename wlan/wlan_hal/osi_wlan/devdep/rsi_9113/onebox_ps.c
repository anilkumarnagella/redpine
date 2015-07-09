
/**
 * @file onebox_ps.c
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
 */

#include "onebox_common.h"
#include "onebox_core.h"

uint32 check_deep_sleep_status(PONEBOX_ADAPTER adapter)
{
		struct ieee80211com *ic = &adapter->vap_com;
		struct ieee80211vap *vap = NULL;

		TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) 
		{ 
				if(vap->iv_opmode == IEEE80211_M_STA){
						break;
				}
		}

		if(driver_ps.delay_pwr_sve_decision_flag && (vap->iv_state != IEEE80211_S_RUN)) {
			/* considering the delay ps_flag has timer has programmed
			 * so stop the timer as MGMT pending has observed
			 */
				printk("In %s Line %d stoppping timer\n", __func__, __LINE__);
			adapter->os_intf_ops->onebox_remove_timer(&adapter->traffic_timer);//NEED TO TEST
			driver_ps.delay_pwr_sve_decision_flag = 0;
			return 0;

		}
		else if((PS_STATE == DEV_IN_PWR_SVE) && (vap->iv_state != IEEE80211_S_RUN)){
			//	pwr_sve_event_handler(vap, MGMT_PENDING_PATH, 0);
				return 1;
		}
		return 0;
}


void update_pwr_save_status(struct ieee80211vap *vap, uint32 ps_status, uint32 path)
{

		PONEBOX_ADAPTER adapter = NULL;
		uint32 queue_pkt_to_head = 0;
		adapter = (PONEBOX_ADAPTER)vap->hal_priv_vap->hal_priv_ptr;
		//unsigned long flags = 0;

		//driver_ps.allow_ps = ps_status;

		//adapter->os_intf_ops->onebox_acquire_spinlock(&adapter->lock_timer, flags);
		switch(path)
		{
				case IOCTL_PATH:
						if((vap->iv_state == IEEE80211_S_INIT) || (vap->iv_state == IEEE80211_S_RUN)){
								if((!vap->hal_priv_vap->ps_params_ioctl.ps_en)) {
										printk("Disabling power save\n");
										ps_status = PS_DISABLE;
								}
								else{
										printk("Enabling power save\n");
										ps_status = PS_ENABLE;

								}
						}
						else {
								/* Vap is in Middle of process like scan/connection may be in progress
								 * so Update pwr save status only after COnnection complete or IDEAL state
								 */
						break;
						}
						pwr_sve_event_handler(vap, ps_status, path, queue_pkt_to_head);
						break;
				case DISCONNECTED_PATH:
						adapter->sta_mode.delay_sta_support_decision_flag = 0;
				case CONNECTED_PATH:
						/*Sends the pwr save and bgscan with the delay after completion of connection */
						if(vap->iv_state == IEEE80211_S_RUN)
								adapter->sta_mode.delay_sta_support_decision_flag = 1;
				case SCAN_END_PATH:
						driver_ps.delay_pwr_sve_decision_flag = 1;
						if(!adapter->traffic_timer.function) {
								adapter->os_intf_ops->onebox_init_sw_timer(&adapter->traffic_timer, (uint32)adapter,
												(void *)&traffic_timer_callback, msecs_to_jiffies(1000));
						}
						if(vap->iv_state == IEEE80211_S_RUN) {
								printk("Programming timer in %s Line %d\n", __func__, __LINE__);
								adapter->os_intf_ops->onebox_mod_timer(&adapter->traffic_timer, msecs_to_jiffies(100));
								// program timer for 100ms
						} else {
								printk("Programming the timer has pwr staus is delayed\n");
								adapter->os_intf_ops->onebox_mod_timer(&adapter->traffic_timer, msecs_to_jiffies(1000));
						}
						break;
				case MGMT_PENDING_PATH:
						queue_pkt_to_head = 1;
						driver_ps.delay_pwr_sve_decision_flag = 0;
						printk("Stopping timer\n");
			adapter->os_intf_ops->onebox_remove_timer(&adapter->traffic_timer);//NEED TO TEST
						//case SCAN_START:
						if(check_deep_sleep_status(adapter)) {
								/** Dev_IN_SLEEP (DEEP_SLEEP state) so bring device out of sleep as we have MGMT Pending*/
								/** This is being called in MGMT pending and scan_start event */
								ps_status = 0;
								pwr_sve_event_handler(vap, ps_status, path, queue_pkt_to_head);
						}

						break;
				case TIMER_PATH:
						/* Assuming that this is being called only if timer_call_back
						 * is decided to enable power save */

						/* Presently we are disabling power save at the path when the current data load 
						 * has met the required threshold irrespective of the timer.
						 */

						//FIXME: Don't we get any scenario to disable PWR save at timer call back*/
						//pwr_sve_event_handler(vap, ps_status=1, PATH=0, q_to_head=0);
						pwr_sve_event_handler(vap, ps_status, path, queue_pkt_to_head);
						break;
				case TX_RX_PATH:
						/* Send intention of RX and TX path */
				case PS_EN_REQ_CNFM_PATH:
				case PS_DIS_REQ_CNFM_PATH:
				//case SCAN_START_PATH:
						//pwr_sve_event_handler(vap, ps_status, path, q_to_head=0);
						pwr_sve_event_handler(vap, ps_status, path, queue_pkt_to_head);
						break;
				case PS_EN_DEQUEUED_PATH:
						pwr_sve_event_handler(vap, ps_status, path, queue_pkt_to_head);
						break;
				default:
						ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("########## Invalid Path %d ###########\n"), path));
						dump_stack();
		}
		//adapter->os_intf_ops->onebox_release_spinlock(&adapter->lock_timer, flags);

}

static int check_data_load(struct ieee80211vap *vap, PONEBOX_ADAPTER adapter) 
{

		if((((traffic_ps_params_def.tx_dataload > (ps_params_def.tx_threshold * BYTES_CONVRTR)) 
										||(traffic_ps_params_def.rx_dataload > ( ps_params_def.rx_threshold * BYTES_CONVRTR))))
						&& (PS_STATE == DEV_IN_PWR_SVE)
			)
		{
						ONEBOX_DEBUG(ONEBOX_ZONE_PWR_SAVE, 
						      (TEXT("Sending disable PS_req to TA in %s line %d as threshold has met before"
																	" \n expiry and threshold has met tx_bytes %d rx_bytes %d\n expected tx %d rx %d\n "), __func__,
														 __LINE__, traffic_ps_params_def.tx_dataload, 
														 traffic_ps_params_def.rx_dataload, ps_params_def.tx_threshold, 
														 ps_params_def.rx_threshold));
				
				traffic_ps_params_def.tx_dataload = 0;
				traffic_ps_params_def.rx_dataload = 0;
				traffic_ps_params_def.jiffies_prev = jiffies;

				update_pwr_save_status(vap,PS_DISABLE, TX_RX_PATH) ;
				//if(adapter->traffic_timer.function)
				//		adapter->os_intf_ops->onebox_mod_timer(&adapter->traffic_timer, msecs_to_jiffies(vap->hal_priv_vap->ps_params.monitor_interval));
				//else
				//		printk("The timer has been not initialized yet\n");

				return 1;
		}
		else if(((traffic_ps_params_def.tx_dataload > (ps_params_def.tx_threshold * BYTES_CONVRTR)) 
								||(traffic_ps_params_def.rx_dataload > ( ps_params_def.rx_threshold * BYTES_CONVRTR)))) {
								//||(traffic_ps_params_def.rx_dataload > ( ps_params_def.rx_threshold * BYTES_CONVRTR))) && (PS_STATE == PS_NONE)) {
										/* checking for PS_STATE because if we have sent DIS_REQ and we are waiting for cnfrm
										 * there may chances for profiles trying to modyifying the timer*/
				//FIXME: Do we require to check for DEV not in pwr_save state???????????
				
				/* As the traffic threshold has matched just reset the parameter and modify timer*/

				traffic_ps_params_def.tx_dataload = 0;
				traffic_ps_params_def.rx_dataload = 0;
				traffic_ps_params_def.jiffies_prev = jiffies;
				if(adapter->traffic_timer.function)
						adapter->os_intf_ops->onebox_mod_timer(&adapter->traffic_timer, msecs_to_jiffies(vap->hal_priv_vap->ps_params.monitor_interval));
				else
						printk("The timer has been not initialized yet\n");

		}
		return 0;

}

void check_traffic_pwr_save(struct ieee80211vap *vap, uint8 tx_path, uint32 payload_size)
{
		unsigned long current_jiffies;
		//unsigned long flags = 0;
		struct ieee80211com *ic = vap->iv_ic;
		struct onebox_os_intf_operations *os_intf_ops  = onebox_get_os_intf_operations();
		PONEBOX_ADAPTER adapter;

		adapter = os_intf_ops->onebox_get_priv(ic->ic_ifp);


		current_jiffies = jiffies;

		if(ps_params_def.ps_en 
						&& (TRAFFIC_PS_EN)
						&& ((vap->iv_state == IEEE80211_S_RUN))
						&& !(driver_ps.delay_pwr_sve_decision_flag)
			)
		{

				if(tx_path)
				{
						traffic_ps_params_def.tx_dataload += payload_size ;
				}
				else
				{
						traffic_ps_params_def.rx_dataload += payload_size; 
				}

				if(!traffic_ps_params_def.jiffies_prev) {
						traffic_ps_params_def.jiffies_prev = current_jiffies;
						return;
				}

				ONEBOX_DEBUG(ONEBOX_ZONE_PWR_SAVE, 
				           (TEXT("In %s line %d tx_bytes %d expected_tx %d rx_bytes %d expected_rx %d \n"),
										 __func__, __LINE__,
										traffic_ps_params_def.tx_dataload,  
										ps_params_def.tx_threshold * BYTES_CONVRTR, 
										traffic_ps_params_def.rx_dataload
										,ps_params_def.rx_threshold * BYTES_CONVRTR));


				//ONEBOX_DEBUG(ONEBOX_ZONE_PWR_SAVE, (TEXT("Driver_ps %d\n"), driver_ps.allow_ps));
				if((jiffies_to_msecs((current_jiffies - traffic_ps_params_def.jiffies_prev))) >= (ps_params_def.monitor_interval))
				{
						/* Reset the traffic parameters has time stamp difference between the last recvd data pkt 
						 * and current pkt is greater than the required monitor interval. If traffic is greater than the
						 * thresholds check_data_load will take the decission 
						 */
						check_data_load(vap, adapter);
						traffic_ps_params_def.tx_dataload = 0;
						traffic_ps_params_def.rx_dataload = 0;
						traffic_ps_params_def.jiffies_prev = jiffies;

				}
				else
				{
						check_data_load(vap, adapter);
				}
		}
		return ;

}

/** This function is called whenever 
 * monitor interval expires 
 */
void traffic_timer_callback(PONEBOX_ADAPTER adapter)
{
		struct ieee80211com *ic = &adapter->vap_com;
		//unsigned long flags = 0;
		struct ieee80211vap *vap =NULL;
		u_int32_t tx_data_threshold = 0;
		u_int32_t rx_data_threshold = 0;

				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("In %s Line %d timer is expired\n"),
										__func__, __LINE__));
		TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) 
		{ 
				if(vap->iv_opmode == IEEE80211_M_HOSTAP) {

						if(adapter->block_ap_queues && adapter->sta_data_block) {
								printk("########Unblocking data queues\n");
								adapter->block_ap_queues = 0;
								/*Need to Unblock only AP queues has station is still 
								 * in disconnected state
								 */
								onebox_send_block_unblock(vap, STA_DISCONNECTED, 0);
								printk("Starting ifp queue at %s Line %d\n", __func__, __LINE__);
								vap->hal_priv_vap->stop_tx_q = 0;
								adapter->os_intf_ops->onebox_start_ifp_txq(vap->iv_ifp);
						} else {
							ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, 
											(TEXT("Timer Expired and data queues states are AP queue %d, \
													station Data queue %d\n"), adapter->block_ap_queues, adapter->sta_data_block));
						}
				}
		}

		if (!adapter->hal_vap[adapter->sta_mode.vap_id].vap_in_use) {
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("In %s Line %d timer is expired sta_mode vap is not in use\n"),
										__func__, __LINE__));

				return;
		}

		TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) 
		{ 
				if(vap->iv_opmode == IEEE80211_M_STA){
						break;
				}
		}

		if (!vap) {
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("In %s Line %d Unable to get vap pointer hence returnning\n"),
										__func__, __LINE__));
				return;
		}

		if (vap->hal_priv_vap->conn_in_prog) {

				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("In %s Line %d Conn_in_prog hence return\n"),
										__func__, __LINE__));
				return;
		}
		
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("In %s Line %d timer is expired\n"),
								__func__, __LINE__));

		if((vap->iv_state == IEEE80211_S_RUN))
		{
						driver_ps.delay_pwr_sve_decision_flag = 0;
				if(adapter->sta_mode.delay_sta_support_decision_flag) {
					onebox_send_sta_supported_features(vap, adapter);	
					adapter->sta_mode.delay_sta_support_decision_flag = 0;
				}
				if(ps_params_def.tx_threshold < ps_params_def.tx_hysterisis)
				{
						tx_data_threshold = 0;
				}
				else
				{
						tx_data_threshold = ((ps_params_def.tx_threshold  - ps_params_def.tx_hysterisis) * BYTES_CONVRTR);
				}

				if(ps_params_def.rx_threshold < ps_params_def.rx_hysterisis)
				{
						rx_data_threshold = 0;
				}
				else
				{
						rx_data_threshold = ((ps_params_def.rx_threshold  - ps_params_def.rx_hysterisis) * BYTES_CONVRTR);
				}

				if((ps_params_def.ps_en) 
								&& (TRAFFIC_PS_EN)) 
				{
						ONEBOX_DEBUG(ONEBOX_ZONE_PWR_SAVE, (TEXT("PWR Save is Enabled\n")));
						if(((traffic_ps_params_def.tx_dataload > (ps_params_def.tx_threshold * BYTES_CONVRTR) )
												|| (traffic_ps_params_def.rx_dataload > ( ps_params_def.rx_threshold * BYTES_CONVRTR)))
										&& (PS_STATE == DEV_IN_PWR_SVE))
						{
								/** Assumed that this condition doesn't occurs */
								//FIXME: Should it arises with the current code??? Check mee(:->)
								ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT(" ######## Sending disable PS_req to TA ###########\n")));
								update_pwr_save_status(vap, PS_DISABLE, TIMER_PATH);
						}
						else
						{
								if((traffic_ps_params_def.tx_dataload <= tx_data_threshold)
												&& (traffic_ps_params_def.rx_dataload <= rx_data_threshold)
												&&(PS_STATE == PS_NONE)
									)
								{
										ONEBOX_DEBUG(ONEBOX_ZONE_PWR_SAVE, 
										     (TEXT("Sending Enable PS_req to TA PS_STATE %d tx_data %d rx_data %d expected tx %d rx %d\n"), 
										            PS_STATE,traffic_ps_params_def.tx_dataload, traffic_ps_params_def.rx_dataload, 
																tx_data_threshold, rx_data_threshold));
										update_pwr_save_status(vap, PS_ENABLE, TIMER_PATH);

								}
								else if(PS_STATE != DEV_IN_PWR_SVE)
								{
										/* Considering that PS_STATE is in middle of transition so no decission will be taken
										 * hence take decission after expiry if timer again
										 */
										//FIXME: There may be a race condition to enable PWR_SAVE request once we have sent DIS_REQ
										/*Due to the above race condition we may not be able to enable pwr_save until the next monitor
										 * interval expiry
										 */
										ONEBOX_DEBUG(ONEBOX_ZONE_PWR_SAVE, (TEXT("Ps STATE %d\n"), PS_STATE));

										ONEBOX_DEBUG(ONEBOX_ZONE_PWR_SAVE, (TEXT("tx_data is %d rx %d expexted %d %d \n"),
																traffic_ps_params_def.tx_dataload, traffic_ps_params_def.rx_dataload, 
																((ps_params_def.tx_threshold  - ps_params_def.tx_hysterisis) * BYTES_CONVRTR),
																((ps_params_def.rx_threshold  - ps_params_def.rx_hysterisis) * BYTES_CONVRTR)));
										/* add timer to expire after monitor interval from now */
										adapter->os_intf_ops->onebox_mod_timer(&adapter->traffic_timer, msecs_to_jiffies(vap->hal_priv_vap->ps_params.monitor_interval));
								}
						}
						ONEBOX_DEBUG(ONEBOX_ZONE_PWR_SAVE, (TEXT("Resetting traffic params in %s "
														"Line %d\n"), __func__, __LINE__));
						traffic_ps_params_def.tx_dataload = 0;
						traffic_ps_params_def.rx_dataload = 0;
				}
		}
		else {
				if(driver_ps.delay_pwr_sve_decision_flag) {
						driver_ps.delay_pwr_sve_decision_flag = 0;
					update_pwr_save_status(vap, PS_ENABLE, TIMER_PATH);
				}
		
		}
		return;
}

void send_ps_params_req(struct ieee80211vap *vap, uint32 ps_en, uint32 queue_pkt_to_head)
{
	onebox_mac_frame_t *mgmt_frame;
	ONEBOX_STATUS status = 0;
	uint8  pkt_buffer[MAX_MGMT_PKT_SIZE];
	struct ieee80211com *ic = vap->iv_ic;
	struct onebox_os_intf_operations *os_intf_ops  = onebox_get_os_intf_operations();
	PONEBOX_ADAPTER adapter;
	netbuf_ctrl_block_t *netbuf_cb;
	uint8 len;

	adapter = os_intf_ops->onebox_get_priv(ic->ic_ifp);

	mgmt_frame = (onebox_mac_frame_t *)pkt_buffer;

	adapter->os_intf_ops->onebox_memset(mgmt_frame, 0, MAX_MGMT_PKT_SIZE);
	mgmt_frame->desc_word[0] = ONEBOX_CPU_TO_LE16(sizeof(mgmt_frame->u.ps_req_params) | (ONEBOX_WIFI_MGMT_Q << 12));
	mgmt_frame->desc_word[1] = ONEBOX_CPU_TO_LE16(WAKEUP_SLEEP_REQUEST);
	mgmt_frame->u.ps_req_params.ps_req.sleep_type = vap->hal_priv_vap->ps_params.sleep_type;

	mgmt_frame->u.ps_req_params.listen_interval = vap->hal_priv_vap->ps_params.listen_interval;
	mgmt_frame->u.ps_req_params.ps_req.num_beacons_per_listen_interval = vap->hal_priv_vap->ps_params.num_beacons_per_listen_interval;
	mgmt_frame->u.ps_req_params.dtim_interval_duration = vap->hal_priv_vap->ps_params.dtim_interval_duration;
	mgmt_frame->u.ps_req_params.num_dtim_intervals = vap->hal_priv_vap->ps_params.num_dtims_per_sleep;
	
	mgmt_frame->u.ps_req_params.ps_req.sleep_duration = vap->hal_priv_vap->ps_params.deep_sleep_wakeup_period;

	if(vap && (vap->iv_state == IEEE80211_S_RUN))
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Sending connected sleep req \n")));
		mgmt_frame->u.ps_req_params.ps_req.connected_sleep = CONNECTED_SLEEP;
		if(uapsd_params_def.uapsd_acs)
		{
			adapter->os_intf_ops->onebox_memcpy(&mgmt_frame->u.ps_req_params.mimic_support, &uapsd_params_def, sizeof(vap->hal_priv_vap->uapsd_params_updated));
			if(uapsd_params_def.mimic_support){
				ONEBOX_DEBUG(ONEBOX_ZONE_PWR_SAVE, (TEXT("Resetting ACS to 0\n")));
				mgmt_frame->u.ps_req_params.uapsd_acs = 0;

			}
		}
	}
	else
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_PWR_SAVE, (TEXT("Sending deep sleep req in %s Line %d \n"), 
				__func__, __LINE__));
		mgmt_frame->u.ps_req_params.ps_req.connected_sleep = DEEP_SLEEP;
	}


	if(ps_en && (adapter->sc_nvaps < 2))
	{
		mgmt_frame->u.ps_req_params.ps_req.ps_en = 1;
		mgmt_frame->desc_word[6] = SLEEP_REQUEST;
	}
	else
	{
		mgmt_frame->u.ps_req_params.ps_req.ps_en = 0;
		mgmt_frame->desc_word[0] |= 1 << 15; //IMMEDIATE WAKE UP

		if(PS_STATE != PS_DIS_REQ_SENT) {
			printk("Wrk around for syncing\n");
			PS_STATE = PS_DIS_REQ_SENT;
		}

		mgmt_frame->desc_word[6] = WAKEUP_REQUEST;
	}

	//adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, (PUCHAR)mgmt_frame, FRAME_DESC_SZ + sizeof(mgmt_frame->u.ps_req_params));
	if(!queue_pkt_to_head) {
		ONEBOX_DEBUG(ONEBOX_ZONE_PWR_SAVE, (TEXT("In %s Line %d Sending PS req PS_STATE %d pkt queued to tail\n"), __func__, __LINE__, PS_STATE));
		status = adapter->devdep_ops->onebox_send_internal_mgmt_frame(adapter, (uint16 *)mgmt_frame,
									      FRAME_DESC_SZ + sizeof(mgmt_frame->u.ps_req_params));
	} else {
		//adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, (PUCHAR)mgmt_frame, (FRAME_DESC_SZ + (sizeof(mgmt_frame->u.ps_req_params))));
		ONEBOX_DEBUG(ONEBOX_ZONE_PWR_SAVE, (TEXT("In %s Line %d Sending PS req PS_STATE %d pkt queued to head\n"), __func__, __LINE__, PS_STATE));
		len = FRAME_DESC_SZ + sizeof(mgmt_frame->u.ps_req_params);
		netbuf_cb = adapter->os_intf_ops->onebox_alloc_skb((FRAME_DESC_SZ + (sizeof(mgmt_frame->u.ps_req_params))));
		if(netbuf_cb == NULL)
		{	
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("%s: Unable to allocate skb\n"), __func__));
				return ;

		}
		adapter->os_intf_ops->onebox_add_data_to_skb(netbuf_cb, len);
		/*copy the internal mgmt frame to netbuf and queue the pkt */
		adapter->os_intf_ops->onebox_memcpy((uint8 *)netbuf_cb->data, (uint8 *)pkt_buffer, len);
		netbuf_cb->flags |= INTERNAL_MGMT_PKT;	
		netbuf_cb->tx_pkt_type = WLAN_TX_M_Q;
		ONEBOX_DEBUG(ONEBOX_ZONE_PWR_SAVE, (TEXT("In %s Line %d Sending PS req PS_STATE %d pkt queued to head \n"), __func__, __LINE__, PS_STATE));
		adapter->os_intf_ops->onebox_netbuf_queue_head(&adapter->host_tx_queue[MGMT_SOFT_Q], netbuf_cb->pkt_addr);
		adapter->devdep_ops->onebox_schedule_pkt_for_tx(adapter);

	}
	return ;

}


void pwr_sve_event_handler(struct ieee80211vap *vap, uint32 ps_status, uint32 path, uint32 queue_pkt_to_head)
{

		PONEBOX_ADAPTER adapter = (PONEBOX_ADAPTER)vap->hal_priv_vap->hal_priv_ptr;
		struct ieee80211com *ic = vap->iv_ic;
		u_int32_t diff_status = 0;
		u_int32_t loop = 0;

		IEEE80211_LOCK(ic);
		if((vap->iv_state != IEEE80211_S_RUN) 
						&& (vap->iv_state != IEEE80211_S_INIT) 
						&& ((vap->iv_state == IEEE80211_S_SCAN) 
								&& (ic->ic_flags & IEEE80211_F_SCAN) 
								&& (PS_STATE == PS_NONE) )
			) {
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("IN %s Line %dVap is in the transition state "
												"vap_state %d ic_flags %02x\n"), __func__, __LINE__, 
										vap->iv_state, ic->ic_flags));
				IEEE80211_UNLOCK(ic);
				return;
		}

		//FIXME:This may cause problem
		if(((vap->iv_state == IEEE80211_S_AUTH) || (vap->iv_state == IEEE80211_S_ASSOC) || 
						((vap->iv_state == IEEE80211_S_RUN) || (vap->iv_state == IEEE80211_S_SCAN))) 
						 && (vap->hal_priv_vap->conn_in_prog || (vap->hal_priv_vap->roam_ind)) 
						 && (ps_status == PS_ENABLE)
						 && ((path != PS_EN_REQ_CNFM_PATH) && (path != PS_DIS_REQ_CNFM_PATH))) {
				ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("IN %s Line %dVap is in the transition state "
												"vap_state %d ic_flags %02x, conn_in_prog %d\n"), __func__, __LINE__, 
										vap->iv_state, ic->ic_flags, vap->hal_priv_vap->conn_in_prog));
				IEEE80211_UNLOCK(ic);
				return;
		}

		//if(adapter->sta_mode.initial_timer_running) {
		//if(driver_ps.delay_pwr_sve_decision_flag) {
		
		/*Some times At disconnect we are sending disable req and programm the delay_ps_flag,
		 * due to which if we get cnfrm for ps_req we are not serving
		 */
		if(driver_ps.delay_pwr_sve_decision_flag && ((path != PS_EN_REQ_CNFM_PATH) && (path != PS_DIS_REQ_CNFM_PATH))) {
				printk("Initial timer need to expire, so wait until timer expires\n");
				IEEE80211_UNLOCK(ic);
				return;
		}

		if(adapter->sc_nvaps < 2)
		{
				do {
								printk("in %s Line %d path %d PS_STATE %d vap->iv_state %d\n", __func__, __LINE__, path, PS_STATE,vap->iv_state);
						switch(PS_STATE)
						{
								case PS_EN_REQ_SENT:
										//if((path == CONFIRM_PATH) && (confirm_type == SLEEP_REQUEST)) {
										if(path == PS_EN_REQ_CNFM_PATH ) {
												PS_STATE = DEV_IN_PWR_SVE;
												ONEBOX_DEBUG(ONEBOX_ZONE_PWR_SAVE, (TEXT("monitor interval %d\n"), vap->hal_priv_vap->ps_params.monitor_interval));
												ONEBOX_DEBUG(ONEBOX_ZONE_PWR_SAVE, (TEXT("MVD pwr save state PS_STATE %d \n"), PS_STATE));

												if(vap->iv_state != IEEE80211_S_RUN)
												{
														/* checking for any pending pkt in mgmt queue*/
														if (adapter->os_intf_ops->onebox_netbuf_queue_len(&adapter->host_tx_queue[MGMT_SOFT_Q]))
														{
																/* schedule pkt as we would have stopped due to PS_EN_REQ_QUEUED state */
																ONEBOX_DEBUG(ONEBOX_ZONE_PWR_SAVE, 
																				(TEXT("In %s Line %d scheduling mgmt pkt as some pkt is in queue\n"),
																				 __func__,__LINE__));
																adapter->devdep_ops->onebox_schedule_pkt_for_tx(adapter);
														}
												}

												if((vap->iv_state == IEEE80211_S_RUN) && (TRAFFIC_PS_EN))
												{
														traffic_ps_params_def.jiffies_prev = jiffies;
												}

												if((driver_ps.delay_pwr_sve_decision_flag || (vap->hal_priv_vap->conn_in_prog)
																		|| (vap->hal_priv_vap->roam_ind)
																		||(ic->ic_flags & IEEE80211_F_SCAN)) ) {
														loop = 0;
														/*Do nothing just handle cnfrm */
														break;
												}
												else{
														loop = 1;
														printk("Repeat loop in %s Line %d\n", __func__, __LINE__);
												}
										}
										else {
												/** Do not do any thing until a confirm is received */
												break;
										}
								case PS_DIS_REQ_SENT:
										//if((path == CONFIRM_PATH) && (confirm_type == WAKEUP_REQUEST)) {
										if(path == PS_DIS_REQ_CNFM_PATH ) {

												PS_STATE = PS_NONE;

												ONEBOX_DEBUG(ONEBOX_ZONE_PWR_SAVE, (TEXT("MVD pwr save state PS_STATE %d \n"),
																		PS_STATE));

												if((vap->iv_state == IEEE80211_S_RUN) && (TRAFFIC_PS_EN))
												{
														traffic_ps_params_def.tx_dataload = 0;
														traffic_ps_params_def.rx_dataload = 0;
														if(adapter->traffic_timer.function)
														{
																adapter->os_intf_ops->onebox_mod_timer(&adapter->traffic_timer, msecs_to_jiffies(vap->hal_priv_vap->ps_params.monitor_interval));
														}

												}
												if((driver_ps.delay_pwr_sve_decision_flag || (vap->hal_priv_vap->conn_in_prog)
																		|| (vap->hal_priv_vap->roam_ind)
																		||(ic->ic_flags & IEEE80211_F_SCAN)) ) {
														loop = 0;
														/*Do nothing just handle cnfrm */
														break;
												}
												else{
														loop = 1;
														printk("Repeat loop in %s Line %d\n", __func__, __LINE__);
												}
										}
#if 1
										else {
												/** Do not do any thing until a confirm is received */
												loop = 0;
												break;
										}
#endif

								case PS_NONE:
								case DEV_IN_PWR_SVE:
										{
												if((path == MGMT_PENDING_PATH) && (PS_STATE == DEV_IN_PWR_SVE) && (vap->iv_state != IEEE80211_S_RUN)) {
														printk("In %s line %d ps_status %d queue_pkt_to_head %d \n",__func__, __LINE__, ps_status, queue_pkt_to_head);
														send_ps_params_req(vap, ps_status, queue_pkt_to_head);
														PS_STATE = PS_DIS_REQ_SENT;
														// What abt driver_ps.allow_ps state;
														loop = 0;
														break;
												}

												if(driver_ps.update_ta) {
														diff_status = adapter->os_intf_ops->onebox_memcmp((const void *)&ps_params_def, (const void *)&ps_params_def_ioctl, sizeof(struct pwr_save_params));
														adapter->os_intf_ops->onebox_memcpy((void *)&ps_params_def, (const void *)&ps_params_def_ioctl, sizeof(struct pwr_save_params));
														driver_ps.update_ta = 0;
														if(!ps_params_def.ps_en && (PS_STATE == PS_NONE))
														{
																printk("In %s Line %d returning DEV out of sleep\n", __func__, __LINE__);
														} else if(!ps_params_def.ps_en && (PS_STATE == DEV_IN_PWR_SVE)) {
																//Send_disable;
																printk("In %s line %d ps_status %d queue_pkt_to_head %d \n", __func__, __LINE__, ps_status, queue_pkt_to_head);
																send_ps_params_req(vap, ps_status, queue_pkt_to_head);
																//send_ps_params_req(vap, 0, 1);
																PS_STATE = PS_DIS_REQ_SENT;
																if(vap->iv_state != IEEE80211_S_RUN) {
																		//FIXME::: We discussed to remove timer ...
																		//remove_timer;
																}
														}else if(ps_params_def.ps_en && (PS_STATE == DEV_IN_PWR_SVE)){
																/* When We reached this state and there is a chance in change of user paramters
																 * hence send disable request first and then send Enable request
																 */
																if(diff_status){
																		if(vap->iv_state != IEEE80211_S_RUN) {
																				queue_pkt_to_head = 1;
																		}
																		printk("In %s Line %d sending disable req	has there is a change in parameters\n", __func__, __LINE__);
																		send_ps_params_req(vap, PS_DISABLE, queue_pkt_to_head);
																		PS_STATE = PS_DIS_REQ_SENT;
																		driver_ps.disable_ps_for_change_in_params = 1;
																}
#if 0
																if(!driver_ps.allow_ps) {
																		printk("Sending diable in %s Line %d\n", __func__, __LINE__);
																		send_ps_params_req(vap, 0, 1);
																		PS_STATE = PS_DIS_REQ_SENT;
																		if(vap->iv_state != IEEE80211_S_RUN) {
																				//FIXME::: We discussed to remove timer ...
																				//remove_timer;
																		}
																}else if(diff_status && (vap->iv_state == IEEE80211_S_RUN) 
																				&& ps_params_def.ps_en ) {
																		send_ps_params_req(vap, 0, 1);
																		PS_STATE = PS_EN_REQ_QUEUED;
																}
																else {
																		ONEBOX_DEBUG(ONEBOX_ZONE_PWR_SAVE, (TEXT("In %s Line %d Nothing to do so returning"), __func__, __LINE__));
																		loop = 0;
																		break;
																}
#endif
														}else if(ps_params_def.ps_en && (PS_STATE == PS_NONE)) {
																printk("Sending Enable req in %s Line %d\n", __func__, __LINE__);
																send_ps_params_req(vap, PS_ENABLE, 0);
																PS_STATE = PS_EN_REQ_QUEUED;
														}
												}
												else if((path != PS_EN_REQ_CNFM_PATH) && (path != PS_DIS_REQ_CNFM_PATH) && ps_params_def.ps_en) {
														ONEBOX_DEBUG(ONEBOX_ZONE_PWR_SAVE, (TEXT("In %s Line %d sending ps_params ps_status %d queue_pkt_to_head %d path %d"),
																				__func__, __LINE__, ps_status, queue_pkt_to_head, path));
														send_ps_params_req(vap, ps_status, queue_pkt_to_head);
														if(ps_status)
																PS_STATE = PS_EN_REQ_QUEUED;
														else
																PS_STATE = PS_DIS_REQ_SENT;

												}
												else if(driver_ps.disable_ps_for_change_in_params) { /*We have received ps_en event DEV_IN_SLEEP state
																														 at that state we are first disabling power save
																														 and enables power save after receiving confirm to disable request
																														 */
														if(ps_params_def.ps_en && (PS_STATE == PS_NONE)) {
														send_ps_params_req(vap, PS_ENABLE, queue_pkt_to_head);
														PS_STATE = PS_EN_REQ_QUEUED;
														}
														driver_ps.disable_ps_for_change_in_params = 0;
												}
										}
										loop = 0;
										break;
								case PS_EN_REQ_QUEUED:
										/* As in Transition state Do nothing wait for Cnfrm and do transition */
										if(path == PS_EN_DEQUEUED_PATH)
												PS_STATE = PS_EN_REQ_SENT;
										break;
								default:
										ONEBOX_DEBUG(ONEBOX_ZONE_PWR_SAVE, (TEXT("Invalid State HOW I came to here??? PS_STATE %d\n"), PS_STATE));
										break;
						}
				}while(loop);
		}
		else if(ps_params_def.ps_en && (PS_STATE == DEV_IN_PWR_SVE))
		{
				ONEBOX_DEBUG(ONEBOX_ZONE_PWR_SAVE, (TEXT("Sending sleep disable as An vap is enabled\n")));
				//driver_ps.allow_ps = 0;
												send_ps_params_req(vap, PS_DISABLE, 0);
																PS_STATE = PS_DIS_REQ_SENT;
		}
		else {
				if((PS_STATE == PS_EN_REQ_SENT) || 
				   (PS_STATE == PS_DIS_REQ_SENT)) 
				{
					if(path == PS_EN_REQ_CNFM_PATH) {
						if(PS_STATE == PS_EN_REQ_SENT) {
							send_ps_params_req(vap, PS_DISABLE, queue_pkt_to_head);
							PS_STATE = PS_DIS_REQ_SENT;
						}
					} else if((PS_STATE == PS_DIS_REQ_SENT) && (path == PS_DIS_REQ_CNFM_PATH)) {
							PS_STATE = PS_NONE;
				}
		}
			printk("In %s Line %d nvaps %d\n", __func__, __LINE__, adapter->sc_nvaps);
		}
		IEEE80211_UNLOCK(ic);

}
EXPORT_SYMBOL(update_pwr_save_status);

