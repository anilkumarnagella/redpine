/**
* @file
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
* The file contains the preparation of datapacket header  for sending on air 
*/

#include "onebox_common.h"
#include "onebox_pktpro.h"

/* Sending zigbee packet to device */
ONEBOX_STATUS send_zigb_pkt(PONEBOX_ADAPTER adapter, 
                          netbuf_ctrl_block_t *netbuf_cb)
{
	uint32 status;

	/* Do any processing on the packet here if needed */
	netbuf_cb->tx_pkt_type = ZIGB_TX_Q;
	status = adapter->onebox_send_pkt_to_coex(netbuf_cb, ZIGB_Q);
	if (status != ONEBOX_STATUS_SUCCESS) 
	{ 
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		             (TEXT("%s: Failed To Write The Packet\n"),__func__));
	}
	return status;    
}

