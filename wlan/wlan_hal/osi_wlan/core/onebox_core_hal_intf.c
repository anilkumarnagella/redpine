#include "onebox_core.h"

/* ***************** Core 2 HAL functions **************** */
/**
 * This function updates the autorate stats.
 *
 * @param Pointer to the driver private structure.
 * @param Pointer to transmit stats structure.
 * @return ONEBOX_STATUS_SUCCESS on success else negative number on failure.
 */
uint32 core_update_tx_status(PONEBOX_ADAPTER adapter,struct tx_stat_s *tx_stat)
{
	int i;

	for(i=0;i<tx_stat->count;i++) 
	{
		adapter->autorate_stats[tx_stat->staid][tx_stat->stat[i].rate_idx].total_success = tx_stat->stat[i].success;
		adapter->autorate_stats[tx_stat->staid][tx_stat->stat[i].rate_idx].total_attempts = tx_stat->stat[i].attempts;
	}
	return ONEBOX_STATUS_SUCCESS;
}

/* Tx data done processing */
/**
 * This function process the data packets to send.
 *
 * @param Pointer to the Adapter structure.
 * @param Pointer to netbuf control block structure.
 * @return ONEBOX_STATUS_SUCCESS on success else negative number on failure.
 */
uint32 core_tx_data_done(PONEBOX_ADAPTER adapter,netbuf_ctrl_block_t *netbuf_cb)
{
	if(netbuf_cb) 
	{
		adapter->os_intf_ops->onebox_free_pkt(adapter,netbuf_cb,0);
	} 
	else 
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_INFO,(TEXT("\nCORE_MSG: tx done error")));
	}
	return ONEBOX_STATUS_SUCCESS;
}

/**
 * This function does the core initialization.
 *
 * @param pointer to the Adapter structure.
 * return ONEBOX_STATUS_SUCCESS on success else negative number on failure. 
 *
 */
uint32 core_init(PONEBOX_ADAPTER adapter)
{
	uint32 count;
	//adapter->init_done = 0;

	for (count = 0; count < ONEBOX_VAPS_DEFAULT; count++) 
	{
		adapter->sec_mode[count] = IEEE80211_CIPHER_NONE;
	}

	/* attach net80211 */
	//core_net80211_attach(adapter);

	/* Rate adaptation initialization */
	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,(TEXT("%s:Registering Autorate\n"),__func__));
	adapter->sc_rc = (struct onebox_ratectrl *)core_rate_attach(adapter);
	if (adapter->sc_rc == NULL) 
	{
		goto out;
	}
	adapter->core_init_done = 1;
	//adapter->init_done = 1;
	return ONEBOX_STATUS_SUCCESS;
out:
	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,(TEXT("%s:out\n"),__func__));
	return ONEBOX_STATUS_FAILURE;
}

/**
 * This functions deinitializes the core.
 *
 * @param Pointer to driver private structure.
 * return ONEBOX_STATUS_SUCCESS on success else negative number on failure. 
 */
uint32 core_deinit(PONEBOX_ADAPTER adapter)
{ 
	uint8 ii;

	//struct ieee80211com *ic = &adapter->vap_com;
	adapter->core_init_done = 0;

	/* Detach net80211 */
	/* Clean up the Net80211 Module */
	core_net80211_detach(adapter);
	/* Purge the mgmt transmit queue */

	/* Purge the Data transmit queue */
	for (ii = 0; ii < NUM_SOFT_QUEUES; ii++)
	{
		adapter->os_intf_ops->onebox_queue_purge(&adapter->host_tx_queue[ii]);
	}

	/* detach autorate */
	core_rate_detach(adapter);

	return ONEBOX_STATUS_SUCCESS;
}
