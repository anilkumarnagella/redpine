
/**
 * @file onebox_hal_ops.h
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
 * This file contians the function prototypes of the callbacks used across
 * differnet layers in the driver
 * 
 */
#include "onebox_common.h"

#ifdef USE_USB_INTF
#include <linux/usb.h>
#endif

struct onebox_osi_zigb_ops {
	int32	(*onebox_core_init)(PONEBOX_ADAPTER adapter);
	int32	(*onebox_core_deinit)(PONEBOX_ADAPTER adapter);
	void	(*onebox_dump)(int32 dbg_zone_l, PUCHAR msg_to_print_p,int32 len);
	ONEBOX_STATUS	(*onebox_send_pkt)(PONEBOX_ADAPTER adapter,
					   netbuf_ctrl_block_t *netbuf_cb);
};

struct onebox_zigb_osd_operations {
  int32	(*onebox_zigb_register_genl)(PONEBOX_ADAPTER adapter);
  int32	(*onebox_zigb_deregister_genl)(PONEBOX_ADAPTER adapter);
  int32	(*onebox_zigb_app_send)(PONEBOX_ADAPTER adapter, netbuf_ctrl_block_t *netbuf_cb);
};
/* EOF */
