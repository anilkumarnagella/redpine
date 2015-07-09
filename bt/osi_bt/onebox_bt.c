#include "onebox_core.h"

int32 bt_core_pkt_recv(PONEBOX_ADAPTER adapter, netbuf_ctrl_block_t *netbuf_cb)
{
	ONEBOX_STATUS status;
	
	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
	    (TEXT("%s: sending to APP len %d, pkttype %d\n"),
	    __func__, netbuf_cb->len, netbuf_cb->bt_pkt_type));
	status = adapter->osd_bt_ops->onebox_send_pkt_to_btstack(adapter, netbuf_cb);
 	return status;
}

int32 core_bt_init(PONEBOX_ADAPTER adapter)
{
	int8 rc = -1;
	
	adapter->os_intf_ops->onebox_netbuf_queue_init(&adapter->bt_tx_queue);

	rc = adapter->osd_bt_ops->onebox_btstack_init(adapter);
	if (rc) {
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, 
		    (TEXT("%s: btstack_init fail %d\n"),__func__, rc));
		return ONEBOX_STATUS_FAILURE;
	}

	adapter->core_init_done = 1;

	return ONEBOX_STATUS_SUCCESS;
}

int32 core_bt_deinit(PONEBOX_ADAPTER adapter)
{
	adapter->core_init_done = 0;
	adapter->os_intf_ops->onebox_queue_purge(&adapter->bt_tx_queue);
	adapter->osd_bt_ops->onebox_btstack_deinit(adapter);

	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,(TEXT("%s: Uninitialized Procfs\n"), __func__));   

	return ONEBOX_STATUS_SUCCESS;	
}

int32 bt_xmit(PONEBOX_ADAPTER adapter, netbuf_ctrl_block_t *netbuf_cb)
{

  	/* Drop Zero Length Packets */
	if (!netbuf_cb->len) {
    		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		    (TEXT("%s:Zero Length packet\n"),__func__));
      		goto xmit_fail;
  	}

  	/* Drop Packets if FSM state is not open */
  	if (adapter->fsm_state != FSM_DEVICE_READY) {
    		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
			(TEXT("%s: FSM state not open\n"),__func__));
      		goto xmit_fail;
  	}
	
	adapter->osi_bt_ops->onebox_send_pkt(adapter, netbuf_cb);
	return ONEBOX_STATUS_SUCCESS;
xmit_fail:

	adapter->stats.tx_dropped++;
  	ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
	    (TEXT("%s:Failed to xmit packet\n"),__func__));
  	if (netbuf_cb) {
		adapter->os_intf_ops->onebox_free_pkt(adapter,netbuf_cb,0);
  	}
  	return ONEBOX_STATUS_SUCCESS;
}

int core_bt_deque_pkts(PONEBOX_ADAPTER adapter)
{
	netbuf_ctrl_block_t *netbuf_cb;

	while(adapter->os_intf_ops->onebox_netbuf_queue_len(&adapter->bt_tx_queue))
	{
		netbuf_cb = adapter->os_intf_ops->onebox_dequeue_pkt((void *)&adapter->bt_tx_queue);
		if(netbuf_cb == NULL)
		{
			ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("queue is empty but length is not zero in %s"), __func__));
			return ONEBOX_STATUS_FAILURE;
		}
		adapter->osi_bt_ops->onebox_send_pkt(adapter, netbuf_cb);
	}

	return ONEBOX_STATUS_SUCCESS;
}

/**
 * This routine dump the given data through the debugger..
 *
 * @param  Debug zone.  
 * @param  Pointer to data that has to dump.  
 * @param  Length of the data to dump.  
 * @return VOID. 
 */
void onebox_print_dump(int32 zone, UCHAR *vdata, int32 len)
{
	uint16 ii;

	if(!zone || !(zone & onebox_bt_zone_enabled))
		return;

	for (ii=0; ii< len; ii++) {
		if (!(ii % 16) && ii) {
			//ONEBOX_DEBUG(zone, (TEXT("\n%04d: "), ii));
			ONEBOX_DEBUG(zone, (TEXT("\n")));
		}
		ONEBOX_DEBUG(zone,(TEXT("%02x "),(vdata[ii])));
	}
	ONEBOX_DEBUG(zone, (TEXT("\n")));
}
