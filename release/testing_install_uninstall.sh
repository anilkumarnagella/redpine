i=1000
while test $i != 0
do
	echo "$i"
	sh remove.sh
	sh insert.sh
	./onebox_util rpine0 create_vap vap0 sta no_sta_beacons
	./wpa_supplicant -i vap0 -D nl80211 -c sta_settings.conf -dddddt > wpa_log &
	while :
	do
		`iw dev vap0 link > /home/rsi/release/log`
		state=`grep -ri "Connected to" /home/rsi/release/log | cut -f 1,2 -d " "`
		#echo "$state"
		#state=`./wpa_cli -i vap0 status | grep -ir "wpa_state" | cut -f 2 -d "="`
		if [ "$state" == "Connected to" ]; then
			echo connected to AP
			break;
		fi
	done
	dhclient vap0
	echo "got the ip address"
	ifconfig vap0
	#ping 10.0.0.220 -A -s 30000 -c 20
	i=`expr $i - 1`
done

