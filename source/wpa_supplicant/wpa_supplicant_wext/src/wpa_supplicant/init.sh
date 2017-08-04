#gnome-terminal -t "wpa_supplicant" -e "./wpa_supplicant -i ra0 -Dwext -C /etc/Infra -N -i p2p0 -Dwext -C /etc/p2p -d"
#gnome-terminal -t "wpa_cli ra0" -e "./wpa_cli -p /etc/Infra -i ra0"
#gnome-terminal -t "wpa_cli p2p0" -e "./wpa_cli -p /etc/p2p -i p2p0"
./wpa_supplicant -i ra0 -Dwext -C /etc/Infra -N -i p2p0 -Dwext -C /etc/p2p -c ./p2p_dev.conf -d &
./wpa_cli -p /etc/p2p -i p2p0
#./wpa_cli -p /etc/Infra -i ra0
