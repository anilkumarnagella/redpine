sh post_vap.sh 
sleep 0.1
iwpriv vap0 setparam 96 3
sleep 0.1
iwpriv vap0 short_gi 3
sleep 0.1
iwconfig vap0 essid Gdvr
sleep 0.1
ifconfig vap0 up
sleep 0.1
iwconfig
sleep 0.1
ifconfig vap0 192.168.5.15
sleep 0.1
echo 0 > /proc/onebox_ap/debug_zone 
sleep 0.1
ifconfig vap0 192.168.5.15
