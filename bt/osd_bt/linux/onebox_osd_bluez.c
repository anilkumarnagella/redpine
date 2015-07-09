#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/ptrace.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/tty.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/signal.h>
#include <linux/ioctl.h>
#include <linux/skbuff.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>
#include "onebox_common.h"
#include "onebox_linux.h"
#define VERSION "2.2"

/**
 * callback function for `hdev->open'
 *
 * @hdev - pointer to `struct hci_dev' data
 * @return - 0 on success
 */
static int onebox_hdev_open(struct hci_dev *hdev)
{
	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,
		(TEXT("%s: `%s' open\n"), __func__, hdev->name));

	if(test_and_set_bit(HCI_RUNNING, &hdev->flags))
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
			(TEXT("%s: device `%s' already running\n"),
			 __func__, hdev->name));

	return 0;
}

/**
 * callback function for `hdev->close'
 *
 * @hdev - pointer to `struct hci_dev' data
 * @return - 0 on success
 */
static int onebox_hdev_close(struct hci_dev *hdev)
{
	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,
		(TEXT("%s: `%s' close\n"), __func__, hdev->name));

	if(!test_and_clear_bit(HCI_RUNNING, &hdev->flags))
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
			(TEXT("%s: device `%s' not running\n"),
			 __func__, hdev->name));

	return 0;
}

/**
 * callback function for `hdev->flush'
 *
 * @hdev - pointer to `struct hci_dev' data
 * @return - 0 on success
 */
static int onebox_hdev_flush(struct hci_dev *hdev)
{
	PONEBOX_ADAPTER adapter;

	if (!(adapter = get_hci_drvdata(hdev)))
		return -EFAULT;

	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,
		(TEXT("%s: `%s' flush \n"), __func__, hdev->name));

	adapter->os_intf_ops->onebox_queue_purge(&adapter->bt_tx_queue);

	return 0;
}



/**
 * callback function for `hdev->send'
 *
 * @hdev - pointer to `struct hci_dev' data
 * @skb - pointer to the skb 
 * @return - 0 on success
 * 
 * `hci' sends a packet through this function.
 */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,12,28)
static int onebox_hdev_send_frame(struct sk_buff *skb)
#else
static int onebox_hdev_send_frame(struct hci_dev *hdev, struct sk_buff *skb)
#endif
{
	PONEBOX_ADAPTER adapter;
	netbuf_ctrl_block_t *netbuf_cb = (netbuf_ctrl_block_t *)skb->cb;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,12,28)
	struct hci_dev *hdev = (struct hci_dev *)skb->dev;
#endif

	if (!(adapter = get_hci_drvdata(hdev)))
		return -EFAULT;

	if (!test_bit(HCI_RUNNING, &hdev->flags))
		return -EBUSY;

	switch (bt_cb(skb)->pkt_type) {
	case HCI_COMMAND_PKT:
		hdev->stat.cmd_tx++;
		break;

	case HCI_ACLDATA_PKT:
		hdev->stat.acl_tx++;
		break;

	case HCI_SCODATA_PKT:
		hdev->stat.sco_tx++;
		break;

	default:
		return -EILSEQ;
	}

	if(skb_headroom(skb) < REQUIRED_HEADROOM_FOR_BT_HAL)
	{
		/* Re-allocate one more skb with sufficent headroom make copy of input-skb to new one */
		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR, (TEXT("In %s No sufficient head room\n"), __func__));

		/* Allocate new buffer with required headroom */
		netbuf_cb = adapter->os_intf_ops->onebox_alloc_skb(skb->len + REQUIRED_HEADROOM_FOR_BT_HAL);
		/* Reserve the required headroom */
		adapter->os_intf_ops->onebox_reserve_data(netbuf_cb, REQUIRED_HEADROOM_FOR_BT_HAL);
		/* Prepare the buffer to add data */
		adapter->os_intf_ops->onebox_add_data_to_skb(netbuf_cb, skb->len);
		/* copy the data from skb to new buffer */
		adapter->os_intf_ops->onebox_memcpy(netbuf_cb->data, skb->data, skb->len);
		/* Assign the pkt type to new netbuf_cb */
		netbuf_cb->bt_pkt_type = bt_cb(skb)->pkt_type;
		/* Finally free the old skb */
		dev_kfree_skb(skb);
	}
	else
	{
		netbuf_cb->len = skb->len;
		netbuf_cb->pkt_addr = (VOID *)skb;
		netbuf_cb->data = skb->data;
		netbuf_cb->bt_pkt_type = bt_cb(skb)->pkt_type;
	}
	adapter->osi_bt_ops->onebox_bt_xmit(adapter, netbuf_cb);
	return 0;
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 3, 8)
/**
 * callback function for `hdev->destruct'
 *
 * @hdev - pointer to `struct hci_dev' data
 * @return - void
 */
static void onebox_hdev_destruct(struct hci_dev *hdev)
{
	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
		(TEXT("%s: `%s' destruct \n"), __func__, hdev->name));
	return;
}
#endif

ONEBOX_STATUS send_pkt_to_bluez(PONEBOX_ADAPTER adapter, netbuf_ctrl_block_t *netbuf_cb)
{
 	ONEBOX_STATUS status;
	struct sk_buff *skb = netbuf_cb->pkt_addr;
	struct hci_dev *hdev = adapter->hdev;
 
 	adapter->hdev->stat.byte_rx += netbuf_cb->len;

 	skb->dev = (void *)hdev;
 	bt_cb(skb)->pkt_type = netbuf_cb->bt_pkt_type;

	status = onebox_hci_recv_frame(hdev, skb);
 	if (status < 0)
 		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
			(TEXT("%s: packet to `%s' failed\n"),
			 __func__, hdev->name));

 	return status;
}

/**
 * registers with the bluetooth-hci(BlueZ) interface
 *
 * @adapter - pointer to bluetooth asset's adapter 
 * @return  - 0 on success 
 */
int bluez_init(PONEBOX_ADAPTER adapter)
{
	int err;
	struct hci_dev *hdev;

	ONEBOX_DEBUG(ONEBOX_ZONE_INIT,
		(TEXT("%s: registering with `hci'\n"),__func__));

	hdev = hci_alloc_dev();
	if (!hdev)
		return -ENOMEM;

	hdev->bus = HCI_DEV_BUS;

	set_hci_drvdata(hdev, adapter);

	hdev->dev_type = HCI_BREDR;

	adapter->hdev = hdev;

	hdev->open     = onebox_hdev_open;
	hdev->close    = onebox_hdev_close;
	hdev->flush    = onebox_hdev_flush;
	hdev->send     = onebox_hdev_send_frame;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(3, 3, 8)
	hdev->destruct = onebox_hdev_destruct;
	hdev->owner    = THIS_MODULE;
#endif

	err = hci_register_dev(hdev);
	if (err < 0) {
 		ONEBOX_DEBUG(ONEBOX_ZONE_ERROR,
			(TEXT("%s: hci registration failed\n"), __func__));
		hci_free_dev(hdev);
		adapter->hdev = NULL;
		return err;
	}

 	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
		(TEXT("%s: done registering `%s'\n"), __func__, hdev->name));
	
	return ONEBOX_STATUS_SUCCESS;
}

/**
 * unregisters with the bluetooth-hci(BlueZ) interface
 *
 * @adapter - pointer to bluetooth asset's adapter 
 * @return  - 0 on success 
 */
int bluez_deinit(PONEBOX_ADAPTER adapter)
{
	struct hci_dev *hdev;

	if (!(hdev = adapter->hdev))
		return -EFAULT;

	ONEBOX_DEBUG(ONEBOX_ZONE_INFO,
		(TEXT("%s: deregistering `%s'\n"), 
		__func__, hdev->name));

	onebox_hci_dev_hold(hdev);
	hci_unregister_dev(hdev);
	onebox_hci_dev_put(hdev);

	hci_free_dev(hdev);

	return ONEBOX_STATUS_SUCCESS;
}
