#include "onebox_common.h"

#ifdef BYPASS_RX_DATA_PATH

//void onebox_reorder_pkt(PONEBOX_ADAPTER adapter, uint8 *msg)
void onebox_reorder_pkt(PONEBOX_ADAPTER adapter, netbuf_ctrl_block_t *netbuf_cb)
{
	uint8 tid = 0, vap_id = 0; 
	uint8 *msg = netbuf_cb->data;
	uint8 pad_bytes = msg[4];
	uint16 seqno;
	uint8 status;
	int32 msg_len;
	struct ieee80211com *ic = &adapter->vap_com;
	struct ieee80211vap *vap = NULL;
	struct ieee80211_node *ni = NULL;
	netbuf_ctrl_block_m_t *netbuf_cb_m = NULL;
	uint8 qos_pkt = 0, pn_valid = 0;

	msg_len  = (*(uint16 *)&msg[0] & 0x0fff);
	msg_len -= pad_bytes;

	vap_id = ((msg[14] & 0xf0) >> 4);	    

	adapter->os_intf_ops->onebox_acquire_sem(&adapter->ic_lock_vap, 0);

	TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) 
	{
		if(vap->hal_priv_vap->vap_id == vap_id)
		{
			break;
		}
	}

	if(!vap)
	{
		printk("Vap Id %d \n", vap_id);
		printk("Pkt recvd is \n");
		adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, msg, 32);
		adapter->os_intf_ops->onebox_free_pkt(adapter, netbuf_cb, 0);
		adapter->os_intf_ops->onebox_release_sem(&adapter->ic_lock_vap);
		return;

	}
	//printk("NETBUF DUMP IN REORDER FUNCTION\n");
	//adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, netbuf_cb->data, netbuf_cb->len);

	adapter->os_intf_ops->onebox_netbuf_adj(netbuf_cb, (FRAME_DESC_SZ + pad_bytes));
	//printk("NETBUF DUMP AFTER ADJUST\n");
	//adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, netbuf_cb->data, netbuf_cb->len);

	netbuf_cb_m = onebox_translate_netbuf_to_mbuf(netbuf_cb);

	if (netbuf_cb_m == NULL) {
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("Unable to alloc mem %s %d\n"), __func__, __LINE__));
		adapter->os_intf_ops->onebox_free_pkt(adapter, netbuf_cb, 0);
		adapter->os_intf_ops->onebox_release_sem(&adapter->ic_lock_vap);
		return;
	}

	/* 
	 * We need to fill the packet details which we recieved 
	 * from the descriptor in Device decap mode
	 */
	if(netbuf_cb->len < 12){
		printk("@@Alert: Data Packet less than Ethernet HDR size received\n");	
		adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, (PUCHAR)netbuf_cb->data, netbuf_cb->len);
	}

	if(vap->iv_opmode == IEEE80211_M_HOSTAP)
	{
		ni = adapter->net80211_ops->onebox_find_node(&vap->iv_ic->ic_sta, &netbuf_cb_m->m_data[6] );
	}
	else
	{
		ni = adapter->net80211_ops->onebox_find_node(&vap->iv_ic->ic_sta, (uint8 *)&vap->iv_bss->ni_macaddr );
	}

	if(!ni) {
		/* what should I do AS this should not happen for data pkts?????*/
		printk("Giving pkt directly to hostap In %s Line %d \n", __func__, __LINE__);
		vap->iv_deliver_data(vap, vap->iv_bss, netbuf_cb_m);
		adapter->os_intf_ops->onebox_release_sem(&adapter->ic_lock_vap);
		return;
	}

	if((netbuf_cb->data[12] == 0x88) && (netbuf_cb->data[13] == 0x8e)) {
		printk("<==== Recvd EAPOL from %02x:%02x:%02x:%02x:%2x:%02x ====>\n", ni->ni_macaddr[0], ni->ni_macaddr[1],
						ni->ni_macaddr[2], ni->ni_macaddr[3], ni->ni_macaddr[4], ni->ni_macaddr[5]);
		adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, (PUCHAR)netbuf_cb->data, netbuf_cb->len);
	}
	/* Indicates the Packet has been decapsulted*/
	netbuf_cb_m->m_flags |= M_DECAP;

#ifdef PWR_SAVE_SUPPORT
	if((vap->iv_opmode == IEEE80211_M_STA)
			&& (TRAFFIC_PS_EN)
			&& (ps_params_def.ps_en)
			&& !(netbuf_cb_m->m_data[0] & 0x01)
			)
	{
		vap->check_traffic(vap, 0, netbuf_cb->len);
	}
#endif
	qos_pkt = (msg[7] & BIT(0));
	pn_valid = (msg[7] & BIT(1));

	if(!qos_pkt)
	{
		//printk("Recvd non qos pkt seq no %d tid %d \n", (((*(uint16 *)&msg[5]) >> 4) & 0xfff), (msg[14] & 0x0f));
		vap->iv_deliver_data(vap, vap->iv_bss, netbuf_cb_m);
		adapter->os_intf_ops->onebox_release_sem(&adapter->ic_lock_vap);
		return;
	}

	tid = (msg[14] & 0x0f);	    

	if(tid > 8)
	{
		printk("Pkt with unkown tid is \n");
		adapter->core_ops->onebox_dump(ONEBOX_ZONE_ERROR, msg, 32);
		vap->iv_deliver_data(vap, vap->iv_bss, netbuf_cb_m);
		adapter->os_intf_ops->onebox_release_sem(&adapter->ic_lock_vap);
		return;
	}

	seqno = (((*(uint16 *)&msg[5]) >> 4) & 0xfff); 

	netbuf_cb_m->tid = tid;
	netbuf_cb_m->aggr_len = seqno;

	if(ni->ni_flags & IEEE80211_NODE_HT)
	{
		if (tid < WME_NUM_TID)
		{
			if (ni->ni_rx_ampdu[tid].rxa_flags & IEEE80211_AGGR_RUNNING)
			{
				netbuf_cb_m->m_flags |= M_AMPDU;
			}
		}
	}
	status = vap->iv_input(ni, netbuf_cb_m, 0, 0);	
	adapter->os_intf_ops->onebox_release_sem(&adapter->ic_lock_vap);
	return;

}

#endif
