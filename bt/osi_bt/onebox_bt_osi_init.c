#include <linux/module.h>
#include <linux/kernel.h>
#include "onebox_datatypes.h"
#include "onebox_common.h"

uint32 onebox_bt_zone_enabled = ONEBOX_ZONE_INFO |
				ONEBOX_ZONE_INIT |
				ONEBOX_ZONE_OID |
				ONEBOX_ZONE_MGMT_SEND |
				ONEBOX_ZONE_MGMT_RCV |
				ONEBOX_ZONE_DATA_SEND |
				ONEBOX_ZONE_DATA_RCV |
				ONEBOX_ZONE_FSM | 
				ONEBOX_ZONE_ISR |
				ONEBOX_ZONE_MGMT_DUMP |
				ONEBOX_ZONE_DATA_DUMP |
				ONEBOX_ZONE_DEBUG |
				ONEBOX_ZONE_AUTORATE |
				ONEBOX_ZONE_PWR_SAVE |
				ONEBOX_ZONE_ERROR |
				0;  
EXPORT_SYMBOL(onebox_bt_zone_enabled);


ONEBOX_STATIC int32 onebox_bt_nongpl_module_init(VOID)
{
	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,(TEXT("onebox_nongpl_module_init called and registering the nongpl driver\n")));
	return 0;
}

ONEBOX_STATIC VOID onebox_bt_nongpl_module_exit(VOID)
{
	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,(TEXT("onebox_nongpl_module_exit called and unregistering the nongpl driver\n")));
	printk("BT : NONGPL module exit\n");
	return;
}

module_init(onebox_bt_nongpl_module_init);
module_exit(onebox_bt_nongpl_module_exit);
