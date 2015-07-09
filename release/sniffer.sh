cat /dev/null > /var/log/messages
#dmesg -c > /dev/null
#sh dump_msg.sh &
#dmesg -n 7

#Sniffer mode
DRIVER_MODE=7
COEX_MODE=1
TA_AGGR=4
SKIP_FW_LOAD=0
FW_LOAD_MODE=1

insmod onebox_nongpl.ko	driver_mode=$DRIVER_MODE firmware_path=$PWD/firmware/ onebox_zone_enabled=0x1 coex_mode=$COEX_MODE ta_aggr=$TA_AGGR skip_fw_load=$SKIP_FW_LOAD fw_load_mode=$FW_LOAD_MODE
insmod onebox_gpl.ko

insmod wlan.ko
insmod wlan_wep.ko
insmod wlan_tkip.ko
insmod wlan_ccmp.ko
insmod wlan_acl.ko
insmod wlan_xauth.ko
insmod wlan_scan_sta.ko
insmod onebox_wlan_nongpl.ko
insmod onebox_wlan_gpl.ko

#./onebox_util rpine0 create_vap wifi0 mon
#ifconfig wifi0 up


