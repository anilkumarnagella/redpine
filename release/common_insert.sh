
cat /dev/null > /var/log/messages
#dmesg -c > /dev/null
#sh dump_msg.sh &
#dmesg -n 7

#Driver Mode 1 END-TO-END mode, 
#            2 RF Evaluation Mode

DRIVER_MODE=1

#COEX MODE   	1 WIFI ALONE
# 	   	2 WIFI+BT Classic
#		3 WIFI+ZIGBEE					
# 	     	4 WIFI+BT LE (Low Engergy)
COEX_MODE=1

#To enable TA-level SDIO aggregation set 1 else set 0 to disable it.
TA_AGGR=4

#Disable Firmware load set 1 to skip FW loading through Driver else set to 0.
SKIP_FW_LOAD=0

#FW Download Mode	
#	1 - Full Flash mode with Secondary Boot Loader
#	2 - Full RAM mode with Secondary Boot Loader
#	3 - Flash + RAM mode with Secondary Boot Loader
#	4 - Firmware loading WITHOUT Secondary Boot Loader
# Recommended to use the default mode 1
FW_LOAD_MODE=1

#ps_handshake_mode
#   1 - No hand shake Mode
#	2 - Packet hand shake Mode
#	3 - GPIO Hand shake Mode
###########Default is Packet handshake mode=2
HANDSHAKE_MODE=2

PARAMS=" driver_mode=$DRIVER_MODE"
PARAMS=$PARAMS" firmware_path=$PWD/firmware/"
PARAMS=$PARAMS" onebox_zone_enabled=0x1"
PARAMS=$PARAMS" coex_mode=$COEX_MODE"
PARAMS=$PARAMS" ta_aggr=$TA_AGGR"
PARAMS=$PARAMS" skip_fw_load=$SKIP_FW_LOAD"
PARAMS=$PARAMS" fw_load_mode=$FW_LOAD_MODE"
#PARAMS=$PARAMS" ps_handshake_mode=$HANDSHAKE_MODE"

insmod onebox_nongpl.ko	$PARAMS
insmod onebox_gpl.ko

