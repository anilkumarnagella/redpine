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
*/

/* include files */
#include "onebox_common.h"
#include "onebox_linux.h"
#include "onebox_pktpro.h"

/**
 * This function read frames from the SD card.
 *
 * @param  Pointer to driver adapter structure.  
 * @param  Pointer to received packet.  
 * @param  Pointer to length of the received packet.  
 * @return 0 if success else -1. 
 */
ONEBOX_STATUS zigb_read_pkt(PONEBOX_ADAPTER adapter, netbuf_ctrl_block_t *netbuf_cb1)
{
	FUNCTION_ENTRY(ONEBOX_ZONE_DATA_RCV);
	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,(TEXT("adapter = %p\n"),
	             adapter));
  	/* Queuing the packets here so that zigbee will poll and receive them*/
  	adapter->os_intf_ops->onebox_genl_app_send(adapter->genl_cb, netbuf_cb1);

	FUNCTION_EXIT(ONEBOX_ZONE_INFO);

	return ONEBOX_STATUS_SUCCESS;
}
EXPORT_SYMBOL(zigb_read_pkt);

