#!/bin/bash
echo ">>>> this script is used check whether supplicant is ready"

if [ "$1" == ath0 ]; then
WPA_CLI=$2"/atheros/wpa_cli -i ath0";
elif [ "$1" == athmag0 ]; then
WPA_CLI=$2"/atheros/wpa_cli -i athmag0";
elif [ "$1" == ra0 ]; then
WPA_CLI=$2"/common/wpa_cli -i ra0";
elif [ "$1" == wlan0 ]; then
WPA_CLI=$2"/common/wpa_cli -i wlan0";
elif [ "$1" == rea0 ]; then
WPA_CLI=$2"/realtek/wpa_cli -i rea0";
fi

echo "check whether wpa_supplicnat is started ..."
i=0
WIFI_UP=0
while [ "$i" != "30" ]
do
  	$WPA_CLI PING | grep PONG  > /dev/null
  	if [ "$?" = "0" ]; then
  	    echo "supplicant is ready."
  	    WIFI_UP=1
  	    break
  	else
  	    echo "Waiting supplicant to be ready..."
  	    sleep 0.5
  	fi
  	i=$(($i+1))
done
    if [ "$WIFI_UP" != "1" ]; then
        echo "Fail to start supplicant."
        exit 1
    fi
    exit 0

   
