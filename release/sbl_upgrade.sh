cat /dev/null > /var/log/messages
dmesg -c > /dev/null
#sh dump_msg.sh &
#dmesg -n 7

SKIP_FW_LOAD=0

#Upgrade Software Boot loader
DRIVER_MODE=9
FW_LOAD_MODE=1

insmod onebox_nongpl.ko driver_mode=$DRIVER_MODE firmware_path=$PWD/firmware/ onebox_zone_enabled=0xffffffff skip_fw_load=$SKIP_FW_LOAD fw_load_mode=$FW_LOAD_MODE
insmod onebox_gpl.ko
