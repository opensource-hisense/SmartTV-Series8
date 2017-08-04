#!/bin/bash
echo ">>>> this script is used to connect wifi with specitial ap"
echo ">>>> note : please set ap ssid as A ,secure mode as wpa/psk ,key as 11111111"
echo ">>>> note : 1st parameter:(0:internal atheros dongle, 1:external atheros dongle , 2: ralink dongle);2nd parameter(ap ssid) "
if [ $1 = 0 ];then
	INTERFACE="ath0";
	WPA_CLI="/3rd/wpa_supplicant/atheros/wpa_cli -i $INTERFACE"
elif [ $1 = 1 ];then
	INTERFACE="athmag0";
	WPA_CLI="/3rd/wpa_supplicant/atheros/wpa_cli -i $INTERFACE"
elif [ $1 = 2 ];then 
	INTERFACE="ra0";
	WPA_CLI="/3rd/bin/wpa_supplicant/ralink/wpa_cli -ira0"
fi
echo INTERFACE=$INTERFACE
echo SSID=$2

$WPA_CLI PING | grep PONG 

if [ "$?" != "0" ]; then
  sh /3rd/wpa_supplicant/p2p_concurrent_start.sh $INTERFACE ;
  sleep 5;
  ifconfig $INTERFACE  up;
  sleep 1;
fi

echo WPA_CLI=$WPA_CLI
#WPA_CLI="/3rd/wpa_supplicant/ralink/wpa_cli -i $INTERFACE"

$WPA_CLI disconnect | grep OK;
if [ "$?" = "0" ]; then
	echo "disconect ok"
fi
sleep 1
$WPA_CLI remove_network all | grep OK;
if [ "$?" = "0" ]; then
        echo "remove network ok"
fi

$WPA_CLI add_network | grep 0 ;
if [ "$?" = "0" ] ; then
	echo "add network 0 ok";
fi

$WPA_CLI set_network 0 key_mgmt NONE | grep OK ;
if [ "$?" = "0" ] ; then
        echo "set_network network keymgmt ok";
fi

$WPA_CLI set_network 0 auth_alg OPEN | grep OK;
if [ "$?" = "0" ] ; then
        echo "set_network network auth_alg ok";
fi;
sleep 0.5

$WPA_CLI set_network 0 scan_ssid 1 | grep OK ;
if [ "$?" = "0" ] ; then
        echo "set_network network scan_ssid ok";
fi;
sleep 0.5

$WPA_CLI set_network 0 ssid \"$2\" | grep OK ;
if [ "$?" = "0" ] ; then
        echo "set_network network ssid \"$2\" ok";
fi;

$WPA_CLI enable_network 0 | grep OK;
if [ "$?" = "0" ] ; then
        echo "enable_network 0 ok";
fi;
sleep 1

$WPA_CLI reconnect | grep OK ;
if [ "$?" = "0" ] ; then
        echo "reconnect  ok";
fi;
echo ">>>>>>> connection is in progress ,just wait..."
sleep 8;

i=0
WIFI_COMPLETED=0
while [ "$i" != "30" ] 
do
    $WPA_CLI status | grep COMPLETED
    if [ "$?" = "0" ];then
        echo "Wifi Completed..."
        WIFI_COMPLETED=1
        break
    else
        sleep 1
    fi
        i=$(($i+1))

done

    
if [ "$WIFI_COMPLETED" != "1" ];then
    echo "WIFI not completed..."
fi


udhcpc -i $INTERFACE -n -q



if [ "$?" = "0" ];then 
   echo "############### connect ok!"
   ifconfig;
   exit 0;
fi
   echo ">>>>>>>>>>>>>> connect faild!"

exit 1;

   
