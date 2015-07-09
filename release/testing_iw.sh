i=100
ssid=$1
bssid=$2
echo "bssid is $bssid"
ifconfig vap0 up
while test $i != 0
do
	echo "$i"
	iw dev vap0 scan > log &
        iw dev vap0 connect $ssid $bssid	
	while :
	do
		`iw dev vap0 link > /home/rsi/release/log`
		state=`grep -ri "Connected to" /home/rsi/release/log | cut -f 1,2 -d " "`
		#echo "$state"
		#state=`./wpa_cli -i vap0 status | grep -ir "wpa_state" | cut -f 2 -d "="`
		if [ "$state" == "Connected to" ]; then
			echo connected to AP
			break;
		#else
			#echo "Not connected.. Wait"
		fi
	done
	dhclient vap0
	echo "got the ip address"
	ping 10.0.0.220 -A -s 30000 -c 20
	i=`expr $i - 1`
	pkill dhclient
	iw dev vap0 disconnect
done
