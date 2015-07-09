#service dhcpd stop
#pkill dhcpd
#pkill dhclient
#ifconfig wifi0 down
killall -9 wpa_supplicant
killall -9 hostapd
sleep 2
rmmod onebox_wlan_gpl.ko
rmmod onebox_wlan_nongpl.ko
rmmod wlan_scan_sta.ko
rmmod wlan_xauth.ko
rmmod wlan_acl.ko
rmmod wlan_tkip.ko
rmmod wlan_ccmp.ko
rmmod wlan_wep.ko
rmmod wlan.ko
