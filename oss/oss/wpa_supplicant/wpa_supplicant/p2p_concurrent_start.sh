
#!/bin/sh
#this script is used to start wifi direct driver and wpa_supplicnt .
#etch wpa_supplicant placed it's own folder . e.g /basic/wifi_tools/ra0...
#which k2 is use /basic/wifi_tools folder as it's path .

WPA_CONF=/tmp/wpa.conf
P2P_DEV_CONF=/tmp/p2pdev.conf
if [ "$1" == ath0 ]; then
WPA_BIN=/3rd/wpa_supplicant/atheros
elif [ "$1" == athmag0 ]; then
WPA_BIN=/3rd/wpa_supplicant/atheros
elif [ "$1" == ra0 ]; then
WPA_BIN=/3rd/bin/wpa_supplicant/ralink
elif [ "$1" == rea0 ]; then  
WPA_BIN=/3rd/wpa_supplicant/realtek
fi

MODULE_PATH=/basic/wifi_ko
WSUPP_DEBUG=1
echo 8 > /proc/sys/kernel/printk

if [ "${WSUPP_DEBUG}" = "1" ]; then
    WSUPP_DEBUG_PARAM="-v"
fi

create_wpa_conf() {
    #Note: first line use '>', others use '>>'
    echo -e "ctrl_interface=/var/run/wpa_supplicant" > $WPA_CONF
    echo -e "ap_scan=1" >> $WPA_CONF
    
    echo -e "ctrl_interface=/var/run/wpa_supplicant" > $P2P_DEV_CONF
    echo -e "device_name=p2p-linux" >> $P2P_DEV_CONF
    echo -e "model_name=pb44" >> $P2P_DEV_CONF
    echo -e "device_type=1-0050F204-1" >> $P2P_DEV_CONF
    echo -e "config_methods=display keypad push_button" >> $P2P_DEV_CONF
    echo -e "persistent_reconnect=1" >> $P2P_DEV_CONF
    echo -e "p2p_go_intent=14" >> $P2P_DEV_CONF
    echo -e "p2p_oper_reg_class=81" >> $P2P_DEV_CONF
    echo -e "p2p_oper_channel=1" >> $P2P_DEV_CONF
    echo -e "p2p_listen_reg_class=81" >> $P2P_DEV_CONF
    echo -e "p2p_listen_channel=1" >> $P2P_DEV_CONF
    echo -e "driver_param=shared_interface=wlan0" >> $P2P_DEV_CONF
    
# WPS settings
#    source /etc/wifi.script/ub124/wpa_conf
#    echo -e "device_name=$WPS_CONF_DEVICE_NAME" >> $WPA_CONF
#    echo -e "manufacturer=$WPS_CONF_MANUFACTURER" >> $WPA_CONF
#    echo -e "model_name=$WPS_CONF_MODEL_NAME" >> $WPA_CONF
#    echo -e "model_number=$WPS_CONF_MODEL_NUMBER" >> $WPA_CONF
#    echo -e "serial_number=$WPS_CONF_SERIAL_NUMBER" >> $WPA_CONF
## Device Type: Multimedia Devices - Media Server/Adapter/Extender
#    echo -e "device_type=$WPS_CONF_DEVICE_TYPE" >> $WPA_CONF
## OS Version: MSB is reserved and always set to one.
#    echo -e "os_version=$WPS_CONF_OS_VERSION" >> $WPA_CONF
## Config Methods for WPS2
#    echo -e "config_methods=$WPS_CONF_CONFIG_METHODS" >> $WPA_CONF
## UUID: if not configured, UUID will be generated based on the local MAC address
#    echo -e "uuid=12345678-9abc-def0-1234-56789abcdef0" >> $WPA_CONF

}

install_wlan_driver() {
    echo "Installing WiFi (athk2) driver..."
    insmod $MODULE_PATH/adf.ko
    insmod $MODULE_PATH/asf.ko
    insmod $MODULE_PATH/ath_hif_usb.ko
    insmod $MODULE_PATH/ath_htc_hst.ko
    insmod $MODULE_PATH/ath_hal.ko
    insmod $MODULE_PATH/ath_dfs.ko
    insmod $MODULE_PATH/ath_rate_atheros.ko
    insmod $MODULE_PATH/ath_dev.ko
    insmod $MODULE_PATH/umac.ko
    insmod $MODULE_PATH/ath_usbdrv.ko
    echo "WiFi driver installed"
}

install_usb_driver() {
    echo "Install USB driver..."
    insmod /basic/modules/usbcore.ko
    insmod /basic/modules/mtk_hcd.ko
    echo "USB driver installed"
}
install_ra_driver()
{
    echo  ">>>>>> ins ralink driver!"
    insmod $MODULE_PATH/rtutil5370sta.ko
    insmod $MODULE_PATH/rt5370sta.ko
    insmod $MODULE_PATH/rtnet5370sta.ko
    echo ">>>>>> ralink driver installed"
}

detect_device_add() {
    i=0
    WIFI_FOUND=0
    while [ "$i" != "30" ]
    do
        cat /proc/net/dev | grep wifi0
        if [ "$?" = "0" ]; then
            echo "wifi0 detected..."
            WIFI_FOUND=1
            break
        else
            sleep 1
	    fi
        i=$(($i+1))
    done
    if [ "$WIFI_FOUND" != "1" ]; then
        echo "Fail to detect WLAN interface..."
        exit 1
    fi
}

sta_up() {

    $WPA_BIN/wlanconfig ath0 create wlandev wifi0 wlanmode sta
    $WPA_BIN/iwpriv ath0 shortgi 1
    $WPA_BIN/iwpriv wifi0 ForBiasAuto 1
    $WPA_BIN/iwpriv wifi0 AMPDU 1
    $WPA_BIN/iwpriv wifi0 AMPDUFrames 32
    $WPA_BIN/iwpriv wifi0 AMPDULim 50000
    $WPA_BIN/wlanconfig wlan create wlandev wifi0 wlanmode p2pdev
    ln -sf /3rd/wpa_supplicant/atheros/libiw.so.28 /3rd/lib/libiw.so.28
		ln -sf /3rd/wpa_supplicant/atheros/libiw.so.29 /3rd/lib/libiw.so.29
    ifconfig ath0 up
    $WPA_BIN/wpa_supplicant -Dathr -iath0 -c $WPA_CONF  -dd &
    echo "wpa_supplicant started..."

    i=0
    while [ "$i" != "10" ]
    do
    	$WPA_BIN/wpa_cli -i ath0 PING | grep PONG  > /dev/null
    	if [ "$?" = "0" ]; then
    	    echo "supplicant is ready."
    	    break
    	else
    	    echo "Waiting supplicant to be ready.."
    	    sleep 0.5
    	fi
    	i=$(($i+1))
    done

    echo WLAN is up

}
sta_up_athmag0()
{
    $WPA_BIN/wpa_supplicant -Dathr -iathmag0 -c $WPA_CONF  -dd &
    echo "wpa_supplicant started..."

    i=0
    while [ "$i" != "10" ]
    do
    	$WPA_BIN/wpa_cli -i athmag0 PING | grep PONG  > /dev/null
    	if [ "$?" = "0" ]; then
    	    echo "supplicant is ready."
    	    break
    	else
    	    echo "Waiting supplicant to be ready.."
    	    sleep 0.5
    	fi
    	i=$(($i+1))
    done

    echo WLAN is up
}

sta_up_ra0()
{
  echo ">>>> sta up ra0"
  #ln -sf /3rd/wpa_supplicant/ralink/libnl/lib/libnl.so.1 /3rd/lib/libnl.so.1
  #ln -sf /3rd/wpa_supplicant/ralink/openssl/lib/libssl.so.1.0.0 /3rd/lib/libssl.so.0.9.8
  #ln -sf /3rd/wpa_supplicant/ralink/openssl/lib/libcrypto.so.1.0.0 /3rd/lib/libcrypto.so.0.9.8

  #$WPA_BIN/wpa_supplicant -ira0 -c $WPA_BIN/wpa_supplicant.conf.ralink -d -B
  $WPA_BIN/wpa_supplicant -Dwext -ira0 -c /3rd/wpa_supplicant/wpa.conf -dd &
  i=0
  while [ "$i" != "10" ]
  do
    	$WPA_BIN/wpa_cli -i ra0 PING | grep PONG  > /dev/null
    	if [ "$?" = "0" ]; then
    	    echo "supplicant is ready."
    	    break
    	else
    	    echo "Waiting supplicant to be ready.."
    	    sleep 0.5
    	fi
    	i=$(($i+1))
   done

    echo WLAN is up

}

sta_up_rea0()
{
   echo ">>>> sta up rea0"
   ln -sf /3rd/wpa_supplicant/atheros/libiw.so.28 /3rd/lib/libiw.so.28
	ln -sf /3rd/wpa_supplicant/atheros/libiw.so.29 /3rd/lib/libiw.so.29
   $WPA_BIN/wpa_supplicant -irea0 -c $WPA_CONF -dd &
    i=0
  while [ "$i" != "10" ]
  do
    	$WPA_BIN/wpa_cli -i rea0 PING | grep PONG  > /dev/null
    	if [ "$?" = "0" ]; then
    	    echo "supplicant is ready."
    	    break
    	else
    	    echo "Waiting supplicant to be ready.."
    	    sleep 0.5
    	fi
    	i=$(($i+1))
   done
}

create_wpa_conf
#install_usb_driver may be skipped at auto mode.

#sleep 3
#detect_device_add

if [ "$1" == ra0 ]; then
sta_up_ra0
elif [ "$1" == rea0 ]; then
sta_up_rea0
elif [ "$1" == ath0 ]; then
sta_up
elif [ "$1" == athmag0 ]; then
sta_up_athmag0
fi
exit 0