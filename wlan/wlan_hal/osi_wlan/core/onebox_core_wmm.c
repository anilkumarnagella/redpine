/**
 * @file onebox_core_wmm.c
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
 * This file contians the WMM queue determination logic
 */

#include "onebox_common.h"

/**
 * This function gets the contention values for the backoff procedure.
 *
 * @param Pointer to the channel acces params sturcture
 * @param Pointer to the driver's private structure.
 * @return none.
 */ 
void onebox_set_contention_vals(struct ieee80211com *ic, PONEBOX_ADAPTER adapter)
{
	struct chanAccParams *wme_params = &ic->ic_wme.wme_wmeChanParams;

	adapter->wme_org_q[VO_Q_STA] = (((wme_params->cap_wmeParams[WME_AC_VO].wmep_logcwmin / 2 ) + 
	                            (wme_params->cap_wmeParams[WME_AC_VO].wmep_aifsn)) * WMM_SHORT_SLOT_TIME + SIFS_DURATION);
	adapter->wme_org_q[VI_Q_STA] = (((wme_params->cap_wmeParams[WME_AC_VI].wmep_logcwmin / 2 ) + 
	                            (wme_params->cap_wmeParams[WME_AC_VI].wmep_aifsn)) * WMM_SHORT_SLOT_TIME + SIFS_DURATION);
	adapter->wme_org_q[BE_Q_STA] = (((wme_params->cap_wmeParams[WME_AC_BE].wmep_logcwmin / 2 ) + 
	                            (wme_params->cap_wmeParams[WME_AC_BE].wmep_aifsn-1)) * WMM_SHORT_SLOT_TIME + SIFS_DURATION);
	adapter->wme_org_q[BK_Q_STA] = (((wme_params->cap_wmeParams[WME_AC_BK].wmep_logcwmin / 2 ) + 
	                            (wme_params->cap_wmeParams[WME_AC_BK].wmep_aifsn)) * WMM_SHORT_SLOT_TIME + SIFS_DURATION);

	printk("In %s Line %d QUEUE WT are \n", __func__, __LINE__);
	printk("BE_Q %d \n",adapter->wme_org_q[BE_Q_STA] );
	printk("BK_Q %d \n",adapter->wme_org_q[BK_Q_STA] );
	printk("VI_Q %d \n",adapter->wme_org_q[VI_Q_STA] );
	printk("VO_Q %d \n",adapter->wme_org_q[VO_Q_STA] );

	/* For AP 4 more Queues */
	adapter->wme_org_q[VO_Q_AP] = (((wme_params->cap_wmeParams[WME_AC_VO].wmep_logcwmin / 2 ) + 
	                            (wme_params->cap_wmeParams[WME_AC_VO].wmep_aifsn)) * WMM_SHORT_SLOT_TIME + SIFS_DURATION);
	adapter->wme_org_q[VI_Q_AP] = (((wme_params->cap_wmeParams[WME_AC_VI].wmep_logcwmin / 2 ) + 
	                            (wme_params->cap_wmeParams[WME_AC_VI].wmep_aifsn)) * WMM_SHORT_SLOT_TIME + SIFS_DURATION);
	adapter->wme_org_q[BE_Q_AP] = (((wme_params->cap_wmeParams[WME_AC_BE].wmep_logcwmin / 2 ) + 
	                            (wme_params->cap_wmeParams[WME_AC_BE].wmep_aifsn)) * WMM_SHORT_SLOT_TIME + SIFS_DURATION);
	adapter->wme_org_q[BK_Q_AP] = (((wme_params->cap_wmeParams[WME_AC_BK].wmep_logcwmin / 2 ) + 
	                            (wme_params->cap_wmeParams[WME_AC_BK].wmep_aifsn)) * WMM_SHORT_SLOT_TIME + SIFS_DURATION);
	adapter->os_intf_ops->onebox_memcpy(adapter->per_q_wt, adapter->wme_org_q, sizeof(adapter->per_q_wt));
	adapter->os_intf_ops->onebox_memset(adapter->pkt_contended, 0, sizeof(adapter->pkt_contended));
}

/**
 * This function determines the HAL queue from which packets has to be dequeued while transmission.
 *
 * @param Pointer to the driver's private structure .
 * @return ONEBOX_STATUS_SUCCESS on success else ONEBOX_STATUS_FAILURE.
 */
uint8 core_determine_hal_queue(PONEBOX_ADAPTER adapter)
{
	uint8 q_num = INVALID_QUEUE;
	uint8 ii,min = 0;
	uint8 fresh_contention;
	struct ieee80211com *ic;
	struct chanAccParams *wme_params_sta;
	
	ic= &adapter->vap_com;
	wme_params_sta = &ic->ic_wme.wme_wmeChanParams;
	
	if (adapter->os_intf_ops->onebox_netbuf_queue_len(&adapter->host_tx_queue[MGMT_SOFT_Q]))
	{
		q_num = MGMT_SOFT_Q;
		return q_num;
	}   
	else
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT("per q wt values in %d:  %d %d %d %d \n"), 
		                                __LINE__, adapter->per_q_wt[0], adapter->per_q_wt[1],
		                                adapter->per_q_wt[2], adapter->per_q_wt[3]));
		ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT("selected queue num and pkt cnt are : %d %d %d \n"),
		                                __LINE__, adapter->selected_qnum, adapter->pkt_cnt));
		if (adapter->pkt_cnt != 0)
		{
			adapter->pkt_cnt -= 1;
			return (adapter->selected_qnum);
		}

GET_QUEUE_NUM:
		q_num = 0;
		fresh_contention = 0;

		/* Selecting first valid contention value */
		for(ii = 0; ii < NUM_EDCA_QUEUES ; ii++)
		{
			if(adapter->pkt_contended[ii] && 
			   ((adapter->os_intf_ops->onebox_netbuf_queue_len(&adapter->host_tx_queue[ii])) != 0)) /* Check for contended value*/
			{
				min = adapter->per_q_wt[ii];
				q_num = ii;
				break;
			}
		}

		/* Selecting the queue with least back off */
		for(; ii < NUM_EDCA_QUEUES ; ii++) /* Start finding the least value from first valid value itself
		                                    * Leave the value of ii as is  from previous loop */
		{
			if(adapter->pkt_contended[ii] && (adapter->per_q_wt[ii] < min) 
			   && ((adapter->os_intf_ops->onebox_netbuf_queue_len(&adapter->host_tx_queue[ii]) !=0))) /* Check only if contended */
			{
				min = adapter->per_q_wt[ii];
				q_num = ii;
			}
		}
		ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT("min =%d and qnum=%d\n"), min, q_num));

		/* Adjust the back off values for all queues again */
		adapter->pkt_contended[q_num] = 0; /* Reset the contention for the current queue so that it gets org value again if it has more packets */

		for(ii = 0; ii< NUM_EDCA_QUEUES; ii++)
		{
			if(adapter->os_intf_ops->onebox_netbuf_queue_len(&adapter->host_tx_queue[ii]))/* Check for the need of contention */
			{ 
				ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT("queue %d len %d\n"), 
				                             ii, adapter->os_intf_ops->onebox_netbuf_queue_len(&adapter->host_tx_queue[ii])));
				if(adapter->pkt_contended[ii])
				{
					if(adapter->per_q_wt[ii] > min)
					{ /* Subtracting the backoff value if already contended */
						adapter->per_q_wt[ii] -= min;
					}
					else /* This case occurs only two queues end up in having same back off value and is least */
					{
						adapter->per_q_wt[ii] = 0;
					}
				}
				else /* Fresh contention */
				{
					adapter->pkt_contended[ii] = 1;
					adapter->per_q_wt[ii] = adapter->wme_org_q[ii];
					fresh_contention = 1;
				}
			}
			else
			{ /* No packets so no contention */
				adapter->per_q_wt[ii] = 0;
				adapter->pkt_contended[ii] = 0;
			}
		}
		ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT("per q values in %d:  %d %d %d %d \n"),
		                       __LINE__, adapter->per_q_wt[0], adapter->per_q_wt[1], adapter->per_q_wt[2], adapter->per_q_wt[3]));
		ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT("pkt contended val in %d:  %d %d %d %d \n\n"),
		                            __LINE__, adapter->pkt_contended[0], adapter->pkt_contended[1], 
		                            adapter->pkt_contended[2], adapter->pkt_contended[3]));
		if((adapter->os_intf_ops->onebox_netbuf_queue_len(&adapter->host_tx_queue[q_num])) == 0)
		{
		/* If any queues are freshly contended and the selected queue doesn't have any packets
		 * then get the queue number again with fresh values */
			if(fresh_contention)
			{
				goto GET_QUEUE_NUM;
			}
			q_num = INVALID_QUEUE;
			return q_num;
		}

		adapter->selected_qnum = q_num ;
		ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT("WMM::queue num after algo= %d \n"), q_num));
		ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT("queue %d len %d \n"), q_num, 
				                        adapter->os_intf_ops->onebox_netbuf_queue_len(&adapter->host_tx_queue[q_num])));

		if ((adapter->selected_qnum == VO_Q_STA) || (adapter->selected_qnum == VI_Q_STA))
		{
			if(adapter->selected_qnum == VO_Q_STA) {
				if(wme_params_sta->cap_wmeParams[adapter->selected_qnum].wmep_acm)
					adapter->pkt_cnt = 1;
				else
					adapter->pkt_cnt = 6;
				//adapter->pkt_cnt = ((wme_params_sta->cap_wmeParams[adapter->selected_qnum].wmep_txopLimit << 5) / 150);
			}	else { 
				if(wme_params_sta->cap_wmeParams[adapter->selected_qnum].wmep_acm) {
					adapter->pkt_cnt = 1;
				} else {
					adapter->pkt_cnt = ((wme_params_sta->cap_wmeParams[adapter->selected_qnum].wmep_txopLimit << 5) / 800);
					//adapter->pkt_cnt = 6;
				}
			}

			if((adapter->os_intf_ops->onebox_netbuf_queue_len(&adapter->host_tx_queue[q_num])) <= adapter->pkt_cnt)
			{
				adapter->pkt_cnt = ((adapter->os_intf_ops->onebox_netbuf_queue_len(&adapter->host_tx_queue[q_num])));
			}
			if(adapter->pkt_cnt != 0)
			{
			adapter->pkt_cnt -= 1;
			}
			else
			{
			adapter->pkt_cnt = 0;
			}
		}
		ONEBOX_DEBUG(ONEBOX_ZONE_INFO, (TEXT("pkt_cnt and q_num are: %d %d \n"), adapter->pkt_cnt, q_num));
		return (q_num);
	}
	return ONEBOX_STATUS_FAILURE;
}
