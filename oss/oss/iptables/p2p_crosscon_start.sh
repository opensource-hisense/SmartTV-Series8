
#!/bin/sh
#this script is used to enable NAT and redirect packet & dns query.

IPTABLES_BIN=/3rd/bin/iptables/iptables-1.4.15

if [ "$1" == ra0 ]; then
echo "1" >> /proc/sys/net/ipv4/ip_forward
$IPTABLES_BIN/xtables-multi iptables -F -t nat
$IPTABLES_BIN/xtables-multi iptables -t nat -A POSTROUTING -s 192.168.5.0/24 -o ra0 -j MASQUERADE
$IPTABLES_BIN/xtables-multi iptables -t nat -A PREROUTING -p udp -i p2p0 --dport 53 -j DNAT --to-destination $2:53
$IPTABLES_BIN/xtables-multi iptables -L -t nat -nv
elif [ "$1" == eth0 ]; then
echo "1" >> /proc/sys/net/ipv4/ip_forward
$IPTABLES_BIN/xtables-multi iptables -F -t nat
$IPTABLES_BIN/xtables-multi iptables -t nat -A POSTROUTING -s 192.168.5.0/24 -o eth0 -j MASQUERADE
$IPTABLES_BIN/xtables-multi iptables -t nat -A PREROUTING -p udp -i p2p0 --dport 53 -j DNAT --to-destination $2:53
$IPTABLES_BIN/xtables-multi iptables -L -t nat -nv
fi

exit 0
