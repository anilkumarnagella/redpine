cat /dev/null > /var/log/messages
dmesg -c > /dev/null
#sh dump_msg.sh &
#dmesg -n 7

SKIP_FW_LOAD=0

#Master Boot Record and Software Bootloader flashing modes

#Driver Mode 8 Flashing with Calib data from EEPROM, 
#            10 Flashing with Calib data from calib_data file
DRIVER_MODE=8
FW_LOAD_MODE=4

insmod onebox_nongpl.ko driver_mode=$DRIVER_MODE firmware_path=$PWD/firmware/ onebox_zone_enabled=0x1 skip_fw_load=$SKIP_FW_LOAD fw_load_mode=$FW_LOAD_MODE
insmod onebox_gpl.ko
