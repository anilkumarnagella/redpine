/**
 * @file onebox_osd_main.c
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
 * This file contains all the Linux network device specific code.
 */


#include "onebox_common.h"
#include "onebox_linux.h"
#include "onebox_sdio_intf.h"

/**
 * This function is invoked when the module is loaded into the
 * kernel. It registers the client driver.
 *
 * @param  VOID.  
 * @return On success 0 is returned else a negative value. 
 */
ONEBOX_STATIC int32 __init onebox_module_init(VOID)
{
	struct onebox_osd_host_intf_operations *osd_host_intf_ops;
	struct driver_assets *d_assets = onebox_get_driver_asset();

	/*Unregistering the client driver*/
	osd_host_intf_ops = onebox_get_osd_host_intf_operations();
	d_assets->card_state = 0;
	d_assets->global_priv = NULL;

	d_assets->techs[WLAN_ID].drv_state = MODULE_REMOVED;
	d_assets->techs[WLAN_ID].fw_state = FW_INACTIVE;

	d_assets->techs[BT_ID].drv_state = MODULE_REMOVED;
	d_assets->techs[BT_ID].fw_state = FW_INACTIVE;

	d_assets->techs[ZB_ID].drv_state = MODULE_REMOVED;
	d_assets->techs[ZB_ID].fw_state = FW_INACTIVE;

	/* registering the client driver */ 
	if (osd_host_intf_ops->onebox_register_drv() == 0) {
		ONEBOX_DEBUG(ONEBOX_ZONE_INIT,
		             (TEXT("%s: Successfully registered gpl driver\n"),
		              __func__));
		return ONEBOX_STATUS_SUCCESS;
	} else {
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		             (TEXT("%s: Unable to register gpl driver\n"), __func__));
		/*returning a negative value to imply error condition*/
		return ONEBOX_STATUS_FAILURE;
	} /* End if <condition> */    
}/* End of <onebox_module_init> */

/**
 * At the time of removing/unloading the module, this function is 
 * called. It unregisters the client driver.
 *
 * @param  VOID.  
 * @return VOID. 
 */
ONEBOX_STATIC VOID __exit onebox_module_exit(VOID)
{
	struct onebox_osd_host_intf_operations *osd_host_intf_ops;
	/*Unregistering the client driver*/	
	osd_host_intf_ops = onebox_get_osd_host_intf_operations();
	
	if (osd_host_intf_ops->onebox_remove() == 0)
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
		             (TEXT( "%s: called onebox_remove function\n"), __func__));
	}
	else    
	{
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
		             (TEXT("%s: Unable to unregister GPL driver\n"),
		             __func__));
	}

	osd_host_intf_ops->onebox_unregister_drv();
	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
	             (TEXT("%s: Unregistered the GPL driver\n"), __func__));
	return;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Redpine Signals Inc");
MODULE_DESCRIPTION("Coexistance Solution From Redpine Signals");
MODULE_SUPPORTED_DEVICE("Godavari RS911x WLAN Modules");

module_init(onebox_module_init);
module_exit(onebox_module_exit);
