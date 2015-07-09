
	while :
	do
		./onebox_util rpine0 do_bgscan 0 1
		i=$(($i+1))
		sleep 1
		if [ "$i" == "10000000000000" ]; then
			break;
		fi
	done
