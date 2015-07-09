/**
 * @file
 * @author  Sowjanya
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
 * This file contains all the function pointer initializations for os
 * interface.
 */
#include "onebox_common.h"
#include "onebox_linux.h"

#ifdef USE_BLUEZ_BT_STACK
static struct onebox_bt_osd_operations bt_osd_ops = {
	.onebox_btstack_init   = bluez_init,
	.onebox_btstack_deinit = bluez_deinit,
 	.onebox_send_pkt_to_btstack = send_pkt_to_bluez,
};
#endif

#ifdef USE_GENL_BT_STACK
static struct onebox_bt_osd_operations bt_osd_ops = {
	.onebox_btstack_init   = btgenl_init,
	.onebox_btstack_deinit = btgenl_deinit,
	.onebox_send_pkt_to_btstack = send_pkt_to_btgenl,
};
#endif

inline struct onebox_bt_osd_operations 
*onebox_get_bt_osd_operations_from_origin(void)
{
	return &bt_osd_ops;
}
