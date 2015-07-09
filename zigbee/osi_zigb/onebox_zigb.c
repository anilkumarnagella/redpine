#include "onebox_core.h"

int32 core_zigb_init(PONEBOX_ADAPTER adapter)
{
	ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("Entered core_bt_init in %s function\n"),__func__));
	
	adapter->os_intf_ops->onebox_netbuf_queue_init(&adapter->zigb_rx_queue);
	ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("Finished netbuf_queue_init in %s function\n"),__func__));

	ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("Before onebox_init_proc in %s function\n"),__func__));
	/********************************************************************************
	      			adding  for Netlink sockets
		  *********************************************************************/
	if (adapter->zigb_osd_ops->onebox_zigb_register_genl(adapter)) {
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		    (TEXT("Finished Zigbeestack_init in %s function\n"), __func__));
		return ONEBOX_STATUS_FAILURE;
	}

	adapter->core_init_done = 1;

	return ONEBOX_STATUS_SUCCESS;
}

int32 core_zigb_deinit(PONEBOX_ADAPTER adapter)
{
	adapter->core_init_done = 0;
	adapter->os_intf_ops->onebox_queue_purge(&adapter->zigb_rx_queue);

	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,(TEXT("%s: Uninitialized Procfs\n"), __func__));   
	/**********************************************************************
   				adding for Netlink sockets
  		*******************************************************************/
	if(adapter->zigb_osd_ops->onebox_zigb_deregister_genl(adapter)) {
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,(TEXT("Finished Zigbeestack_ deinit in %s function\n"),__func__));
		return ONEBOX_STATUS_FAILURE;
	}

	return 0;	
}

/**
 * This routine dump the given data through the debugger..
 *
 * @param  Debug zone.  
 * @param  Pointer to data that has to dump.  
 * @param  Length of the data to dump.  
 * @return VOID. 
 */
void onebox_print_dump(int32 zone,UCHAR *vdata, int32 len )
{
	uint16 ii;

	if(!zone || !(zone & onebox_zigb_zone_enabled))
		return;

	for(ii=0; ii< len; ii++)
	{
		if(!(ii % 16) && ii)
		{
			//ONEBOX_DEBUG(zone, (TEXT("\n%04d: "), ii));
			ONEBOX_DEBUG(zone, (TEXT("\n")));
		}
		ONEBOX_DEBUG(zone,(TEXT("%02x "),(vdata[ii])));
	}
	ONEBOX_DEBUG(zone, (TEXT("\n")));
}
