/*
 * Driver interaction with extended Linux CFG8021
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 */

#include "hardware_legacy/driver_nl80211.h"
#include "wpa_supplicant_i.h"
#include "config.h"
//#ifdef ANDROID
#include "android_drv.h"
//#endif

#define CONFIG_MTK_HE_SUPPORT
#define CONFIG_ACS_SUPPORT
#define CONFIG_ACL_SUPPORT
#define CONFIG_WOW_SUPPORT
#define CONFIG_ACTIVE_CHANNEL_SCAN_SUPPORT


/******* Mediatek Proprietary Driver Commands ********/
#ifndef SIOCIWFIRSTPRIV
#define SIOCIWFIRSTPRIV	                            0x8BE0
#endif
#define RT_PRIV_IOCTL                               (SIOCIWFIRSTPRIV + 0x01)
#define OID_GET_SET_TOGGLE                          0x8000
#define OID_BW_CAPABILITY                           0x06B0 /* get/set My HT BW Capability */
#define OID_CURRENT_ASSOCIATED_BW                   0x06B4 /* Get currently associated Bandwidth */
#define OID_HE_GET_STA_INFO 						0x06B1
#define OID_HE_GET_PRODUCT_INFO 					0x06B2
#ifdef CONFIG_ACS_SUPPORT
#define OID_GET_ACS_RANK_LIST                       0x06B7 /* Get AutoChannelSelection Rank list */
#endif /* CONFIG_ACS_SUPPORT */
#define	RT_OID_DEVICE_NAME							0x0607
#define	RT_OID_VERSION_INFO							0x0608

#define	RT_OID_802_11_QUERY_EEPROM_VERSION			0x0611
#define	RT_OID_802_11_QUERY_FIRMWARE_VERSION		0x0612

#define RT_OID_802_11_MANUFACTURERNAME			0x0701
#define RT_OID_802_11_PRODUCTID					0x0710
#define RT_OID_802_11_MANUFACTUREID				0x0711


/* HE OIDs for Ioctl-based Interfaces */
#ifdef CONFIG_ACL_SUPPORT
#define OID_HE_SET_ACL_POLICY				0x06C0
#define OID_HE_ADD_ACL_ADDR					0x06C1
#define OID_HE_DEL_ACL_ADDR					0x06C2
#define OID_HE_CLR_ACL						0x06C3
#define OID_HE_GET_ACL						0x06C4
#endif
#ifdef CONFIG_ACTIVE_CHANNEL_SCAN_SUPPORT
#define OID_HE_DO_ACTIVE_SCAN				0x06C5
#endif /* CONFIG_ACTIVE_CHANNEL_SCAN_SUPPORT */
#ifdef CONFIG_WOW_SUPPORT
#define OID_HE_SET_WOW_IP					0x06CA
#define OID_HE_SET_UDP_PORT					0x06CB
#define OID_HE_GET_WOW_STAT					0x06CC
#define OID_HE_SET_TCP_PORT					0x06CD
#define OID_HE_SET_REPLAY_COUNTER			0x06CE
#define OID_HE_SET_PTK						0x06CF
#endif /* CONFIG_WOW_SUPPORT */


typedef struct android_wifi_priv_cmd {
	char *buf;
	int used_len;
	int total_len;
} android_wifi_priv_cmd;

struct  iw_point
{
    void    *pointer;   /* Pointer to the data  (in user space) */
    u16     length;     /* number of fields or size in bytes */
    u16     flags;      /* Optional params */
};

union   iwreq_data
{
        struct iw_point data;       /* Other large parameters */
};

struct  iwreq
{
    union
    {
        char    ifrn_name[IFNAMSIZ];    /* if name, e.g. "eth0" */
    } ifr_ifrn;

    /* Data part (defined just above) */
    union   iwreq_data  u;
};


int linux_driver_mtk_set_oid(int ioctl_sock, const char *ifname, int oid, u8 *data, int len)
{
	struct iwreq iwr;
	int err;


	memset (&iwr, 0, sizeof(iwr));
	strncpy(iwr.ifr_name, ifname, os_strlen(ifname));
	iwr.u.data.flags = OID_GET_SET_TOGGLE | oid;
	iwr.u.data.pointer = data;
	iwr.u.data.length = len;

	if ( (err = ioctl(ioctl_sock, RT_PRIV_IOCTL, &iwr)) < 0)
	{
		wpa_printf(MSG_ERROR, "ERR(%d)::  %s in 0x%04x", err, __FUNCTION__, oid);
		return -1;
	}

	return 0;
}

int linux_driver_mtk_get_oid(int ioctl_sock, const char *ifname, int oid, u8 *data, int len)
{
	struct iwreq iwr;
	int err;


	memset (&iwr, 0, sizeof(iwr));
	strncpy(iwr.ifr_name, ifname, os_strlen(ifname));
	iwr.u.data.flags = oid;
	iwr.u.data.pointer = data;
	iwr.u.data.length = len;

	if ( (err = ioctl(ioctl_sock, RT_PRIV_IOCTL, &iwr)) < 0)
	{
		wpa_printf(MSG_ERROR, "%s: ERR(%d) (ifname=%s, oid=0x%04x)", __FUNCTION__, err, ifname, oid);
		return -1;
	}
	wpa_printf(MSG_DEBUG, "%s:SUCCESS (ifname=%s,oid=0x%04x)", __FUNCTION__, ifname, oid);
    
	return iwr.u.data.length;
}



int linux_set_htbw(int sock, const char *ifname, int bw)
{

    wpa_printf(MSG_DEBUG, "%s: sock=0x%x, ifname=%s, bw=%d(%s)", __FUNCTION__, sock, ifname, bw, (bw==0?"BW_20":"BW_40"));
    return linux_driver_mtk_set_oid(sock, ifname, OID_BW_CAPABILITY, (u8*)&bw, (int)sizeof(bw));
}

int linux_get_current_associated_htbw(int sock, const char *ifname, int *bw)
{
    linux_driver_mtk_get_oid(sock, ifname, OID_CURRENT_ASSOCIATED_BW, (u8*)bw, (int)sizeof(*bw));
    return 0;
}

int linux_get_acs_rank_list(int sock, const char *ifname, u8 *buf, int buf_len)
{
    return linux_driver_mtk_get_oid(sock, ifname, OID_GET_ACS_RANK_LIST, buf, buf_len);
}

int linux_get_fw_ver(int sock, const char *ifname, u8 *fwVer, int len)
{
    linux_driver_mtk_get_oid(sock, ifname, RT_OID_802_11_QUERY_FIRMWARE_VERSION, (u8*)fwVer, len);
    return 0;
}

int linux_get_sta_info(int sock, const char *ifname, u8 *staInfo, int len)
{
    linux_driver_mtk_get_oid(sock, ifname, OID_HE_GET_STA_INFO, (u8*)staInfo, len);
    return 0;
}

int linux_get_product_info(int sock, const char *ifname, u8 *productInfo, int len)
{
    linux_driver_mtk_get_oid(sock, ifname, OID_HE_GET_PRODUCT_INFO, (u8*)productInfo, len);
    return 0;
}


/* ACL-related Interfaces */
#ifdef CONFIG_ACL_SUPPORT
int linux_set_aclPolicy(int sock, const char *ifname, int policy)
{
    return linux_driver_mtk_set_oid(sock, ifname, OID_HE_SET_ACL_POLICY, (u8*)&policy, (int)sizeof(policy));
}

int linux_add_aclAddr(int sock, const char *ifname, u8 *data, int len)
{
    return linux_driver_mtk_set_oid(sock, ifname, OID_HE_ADD_ACL_ADDR, (u8*)data, len);
}

int linux_del_aclAddr(int sock, const char *ifname, u8 *data, int len)
{
	return linux_driver_mtk_set_oid(sock, ifname, OID_HE_DEL_ACL_ADDR, (u8*)data, len);
}

int linux_clear_acl(int sock, const char *ifname)
{
	return linux_driver_mtk_set_oid(sock, ifname, OID_HE_CLR_ACL, NULL, 0);
}

int linux_get_current_acl(int sock, const char *ifname, u8 *data, int *len)
{
	struct iwreq iwr;
	int err;


	memset (&iwr, 0, sizeof(iwr));
	strncpy(iwr.ifr_name, ifname, os_strlen(ifname));
	iwr.u.data.flags = OID_HE_GET_ACL;
	iwr.u.data.pointer = data;
	iwr.u.data.length = *len;

	if ( (err = ioctl(sock, RT_PRIV_IOCTL, &iwr)) < 0)
	{
		wpa_printf(MSG_ERROR, "%s: ERR(%d) (ifname=%s, oid=0x%04x)", __FUNCTION__, err, ifname, OID_HE_GET_ACL);
		return -1;
	}
	return 0;

}
#endif /* CONFIG_ACL_SUPPORT */
#ifdef CONFIG_WOW_SUPPORT
int linux_set_ptk(int sock, const char *ifname, u8 *data, int len)
{
    return linux_driver_mtk_set_oid(sock, ifname, OID_HE_SET_PTK, (u8*)data, len);
}

int linux_set_replay_counter(int sock, const char *ifname, u8 *data, int len)
{
	return linux_driver_mtk_set_oid(sock, ifname, OID_HE_SET_REPLAY_COUNTER, (u8*)data, len);
}
int linux_get_wow_stat(int sock, const char *ifname, u8 *data)
{
	linux_driver_mtk_get_oid(sock, ifname, OID_HE_GET_WOW_STAT, (u8*)data, (int)sizeof(data));
	return 0;
}
int linux_set_tcp_port(int sock, const char *ifname, int *data, int len)
{
	return linux_driver_mtk_set_oid(sock, ifname, OID_HE_SET_TCP_PORT, (u8*)data, len * sizeof(int));
}
int linux_set_udp_port(int sock, const char *ifname, int *data, int len)
{
	return linux_driver_mtk_set_oid(sock, ifname, OID_HE_SET_UDP_PORT, (u8*)data, len * sizeof(int));
}
int linux_set_wow_ip(int sock, const char *ifname, int *data, int len)
{
	return linux_driver_mtk_set_oid(sock, ifname, OID_HE_SET_WOW_IP, (u8*)data, len);
}
#endif /* CONFIG_WOW_SUPPORT */
#ifdef CONFIG_ACTIVE_CHANNEL_SCAN_SUPPORT
int linux_do_active_scan(int sock, const char *ifname)
{
	return linux_driver_mtk_set_oid(sock, ifname, OID_HE_DO_ACTIVE_SCAN, NULL, 0);
}
#endif /* CONFIG_ACTIVE_CHANNEL_SCAN_SUPPORT */


#ifdef CONFIG_ACS_SUPPORT
int wpa_driver_nl80211_get_acs_rank_list(void *priv, u8 *buf, int buf_len)
{
    struct i802_bss *bss = priv;
    struct wpa_driver_nl80211_data *drv = bss->drv;

    return linux_get_acs_rank_list(drv->global->ioctl_sock, bss->ifname, buf, buf_len);
}
#endif /* CONFIG_ACS_SUPPORT */

#if 0
int wpa_driver_nl80211_driver_cmd(void *priv, char *cmd, char *buf,
                                size_t buf_len )
{
    struct i802_bss *bss = priv;
    struct wpa_driver_nl80211_data *drv = bss->drv;
    struct ifreq ifr;
    android_wifi_priv_cmd priv_cmd;
    int ret = 0;

    if (os_strcasecmp(cmd, "STOP") == 0) {
        linux_set_iface_flags(drv->global->ioctl_sock, bss->ifname, 0);
        wpa_msg(drv->ctx, MSG_INFO, WPA_EVENT_DRIVER_STATE "STOPPED");
    } else if (os_strcasecmp(cmd, "START") == 0) {
        linux_set_iface_flags(drv->global->ioctl_sock, bss->ifname, 1);
        wpa_msg(drv->ctx, MSG_INFO, WPA_EVENT_DRIVER_STATE "STARTED");
    } else if (os_strcasecmp(cmd, "MACADDR") == 0) {
        u8 macaddr[ETH_ALEN] = {};

        ret = linux_get_ifhwaddr(drv->global->ioctl_sock, bss->ifname, macaddr);
        if (!ret)
            ret = os_snprintf(buf, buf_len,
                            "Macaddr = " MACSTR "\n", MAC2STR(macaddr));
    } else if (os_strcasecmp(cmd, "RELOAD") == 0) {
        wpa_msg(drv->ctx, MSG_INFO, WPA_EVENT_DRIVER_STATE "HANGED");
    } else { /* Use private command */
        return 0;
    }
    return ret;
}
#else
int wpa_driver_nl80211_driver_cmd(void *priv, char *cmd, char *buf,
                                size_t buf_len )
{
    struct i802_bss *bss = priv;
    struct wpa_driver_nl80211_data *drv = bss->drv;
    int ret = 0;
	
    if (os_strcasecmp(cmd, "STOP") == 0) {
        linux_set_iface_flags(drv->global->ioctl_sock, bss->ifname, 0);
        wpa_msg(drv->ctx, MSG_INFO, WPA_EVENT_DRIVER_STATE "STOPPED");
    } else if (os_strcasecmp(cmd, "START") == 0) {
        linux_set_iface_flags(drv->global->ioctl_sock, bss->ifname, 1);
        wpa_msg(drv->ctx, MSG_INFO, WPA_EVENT_DRIVER_STATE "STARTED");
    } else if (os_strcasecmp(cmd, "MACADDR") == 0) {
        u8 macaddr[ETH_ALEN] = {};

        ret = linux_get_ifhwaddr(drv->global->ioctl_sock, bss->ifname, macaddr);
        if (!ret)
            ret = os_snprintf(buf, buf_len,
                            "Macaddr = " MACSTR "\n", MAC2STR(macaddr));
    } else if (os_strcasecmp(cmd, "RELOAD") == 0) {
        wpa_msg(drv->ctx, MSG_INFO, WPA_EVENT_DRIVER_STATE "HANGED");
    /* Mediatek proprietary cmds */
#ifdef CONFIG_MTK_HE_SUPPORT	
    } else if (os_strncasecmp(cmd, "HT_BW ", 6) == 0) {
        int htbw = 0;
        htbw = atoi(cmd+6);
        ret = linux_set_htbw(drv->global->ioctl_sock, bss->ifname, htbw);
    } else if (os_strncasecmp(cmd, "GET_ASSOC_HT_BW", 15) == 0) {
        int htbw = 0;
        ret = linux_get_current_associated_htbw(drv->global->ioctl_sock, bss->ifname, &htbw);
        if (!ret) {
            ret = os_snprintf(buf, buf_len,
                            "GET_ASSOC_HT_BW=%d\n", htbw);
        }
#ifdef CONFIG_ACS_SUPPORT
    } else if (os_strncasecmp(cmd, "GET_ACS_RANK_LIST", 17) == 0) {
        u8 acs_list[64] = {0x00};
        int list_len = sizeof(acs_list);
        wpa_printf(MSG_DEBUG, "Get ACS RANK LIST");
        list_len = linux_get_acs_rank_list(drv->global->ioctl_sock, bss->ifname, acs_list, list_len);
        if (list_len) {
            wpa_printf(MSG_DEBUG, "Get ACS_RANK_LIST len=%d ", list_len);
            wpa_hexdump(MSG_DEBUG, "ACS list:", acs_list, list_len);
            ret = 0;
        }
        else
            ret = -1;
#endif /* CONFIG_ACS_SUPPORT */
    } else if (os_strncasecmp(cmd, "GET_FW_VER", 10) == 0) {
        u8 fwVer[13] = {'\0'};
        ret = linux_get_fw_ver(drv->global->ioctl_sock, bss->ifname, fwVer, 4);
        if (!ret) {
            ret = os_snprintf(buf, buf_len,
                            "Firmware Version: %s\n", fwVer);
        }
    } else if (os_strncasecmp(cmd, "GET_STA_INFO", 12) == 0) {
        u8 staInfo[50] = {'\0'};
        ret = linux_get_sta_info(drv->global->ioctl_sock, bss->ifname, staInfo, 4);
        if (!ret) {
            ret = os_snprintf(buf, buf_len,
                            "STA Info:\n%s\n", staInfo);
        }
    } else if (os_strncasecmp(cmd, "GET_PRODUCT_INFO", 16) == 0) {
        u8 productInfo[256] = {'\0'};
        ret = linux_get_product_info(drv->global->ioctl_sock, bss->ifname, productInfo, 4);
        if (!ret) {
            ret = os_snprintf(buf, buf_len,
                            "Product Info:\n%s\n", productInfo);
        }		
#ifdef CONFIG_ACL_SUPPORT			
    } else if (os_strncasecmp(cmd, "SetACLMode ", 11) == 0) {
        int acl_policy = 0;
        acl_policy = atoi(cmd+11);
        ret = linux_set_aclPolicy(drv->global->ioctl_sock, bss->ifname, acl_policy);	
	} else if (os_strncasecmp(cmd, "AddACL ", 7) == 0) {
		ret = linux_add_aclAddr(drv->global->ioctl_sock, bss->ifname, (u8*)cmd+7, 17);	
	} else if (os_strncasecmp(cmd, "RemoveACL ", 10) == 0) {
		ret = linux_del_aclAddr(drv->global->ioctl_sock, bss->ifname, (u8*)cmd+10, 17);	
	} else if (os_strncasecmp(cmd, "ClearACL", 8) == 0) {
		ret = linux_clear_acl(drv->global->ioctl_sock, bss->ifname);			
	} else if (os_strncasecmp(cmd, "GetACL", 6) == 0) {
	    int listLen = 0;
		int i = 0;
		u8 List[(ETH_ALEN * 20)+1] = {'\0'};

		ret = linux_get_current_acl(drv->global->ioctl_sock, bss->ifname, List, &listLen);	
		for(i=0; i<List[0]; i++)
		{
			os_snprintf(buf, buf_len,
				i<(List[0]-1)?MACSTR",":MACSTR, 
				List[(i*ETH_ALEN)+1], 
				List[(i*ETH_ALEN)+2], 
				List[(i*ETH_ALEN)+3], 
				List[(i*ETH_ALEN)+4], 
				List[(i*ETH_ALEN)+5], 
				List[(i*ETH_ALEN)+6]);
			
			ret += i<(List[0]-1)?18:17;
			buf += i<(List[0]-1)?18:17;
			buf_len -= i<(List[0]-1)?18:17;
		}	
#endif /* CONFIG_ACL_SUPPORT */
#ifdef CONFIG_WOW_SUPPORT
	} else if (os_strncasecmp(cmd, "GET_WOW_STAT", 12) == 0) {
		u8 WowStat = 0;
		ret = linux_get_wow_stat(drv->global->ioctl_sock, bss->ifname, &WowStat);
        if (!ret) {
            ret = os_snprintf(buf, buf_len,
                            "WoW Stat: %s\n", (WowStat==0?"OFF":"ON"));
        }
	} else if (os_strncasecmp(cmd, "SET_TCP_PORT ", 13) == 0) {
		int tcp_port[64];
		int tcp_port_count = 0, i = 0, cmd_length = os_strlen(cmd);
		i = 12;
		/* 
		Expect format :
			wpa_cli -iwlan0 driver SET_TCP_PORT 1 23 456 789 1011 121 3
		Parsing the TCP port with " 1 23 456 789 1011 121 3"
		*/
		while(i<cmd_length && tcp_port_count<64)
		{
			if(*(cmd+i) == ' ')
				tcp_port[tcp_port_count++] = atoi(cmd+i);	
			i++;
		}

		ret = linux_set_tcp_port(drv->global->ioctl_sock, bss->ifname, tcp_port, tcp_port_count);
        if (!ret) {
			ret = 0;
			for(i=0;i<tcp_port_count;i++)
			{
		        os_snprintf(buf, buf_len,
		                        "TCP_PORT[%2d]:%5d\n", i, tcp_port[i]);
				ret += 19;
				buf += 19;
				buf_len -= 19;
			}
        }	
	} else if (os_strncasecmp(cmd, "SET_UDP_PORT ", 13) == 0) {
		int udp_port[64];
		int udp_port_count = 0, i = 0, cmd_length = os_strlen(cmd);
		i = 12;
		/* 
		Expect format :
			wpa_cli -iwlan0 driver SET_UDP_PORT 1 23 456 789 1011 121 3
		Parsing the UDP port with " 1 23 456 789 1011 121 3"
		*/
		while(i<cmd_length && udp_port_count<64)
		{
			if(*(cmd+i) == ' ')
				udp_port[udp_port_count++] = atoi(cmd+i);	
			i++;
		}

		ret = linux_set_udp_port(drv->global->ioctl_sock, bss->ifname, udp_port, udp_port_count);
        if (!ret) {
			ret = 0;
			for(i=0;i<udp_port_count;i++)
			{
		        os_snprintf(buf, buf_len,
		                        "UDP_PORT[%2d]:%5d\n", i, udp_port[i]);
				ret += 19;
				buf += 19;
				buf_len -= 19;
			}
        }	
	} else if (os_strncasecmp(cmd, "SET_WOW_IP ", 11)== 0) {
		unsigned char IP[16];
		int cmd_length = os_strlen(cmd);
		os_strlcpy((char *)IP,cmd+11,16);
		if(!os_strncmp((char *)IP,"any",3))
		{
			ret = linux_set_wow_ip(drv->global->ioctl_sock, bss->ifname, (int*)IP, 3);
            ret = os_snprintf(buf, buf_len,
                "Set IP to be any\n");
		}
		else
		{
			ret = linux_set_wow_ip(drv->global->ioctl_sock, bss->ifname, (int*)IP, cmd_length - 11);
			ret = os_snprintf(buf, buf_len,
                "Set IP to %s\n",IP);
		}	
#endif
#ifdef CONFIG_ACTIVE_CHANNEL_SCAN_SUPPORT
	} else if (os_strncasecmp(cmd, "ACTIVE_SCAN", 11) == 0) {		
		/* Set the active scan flag first */
		ret = linux_do_active_scan(drv->global->ioctl_sock, bss->ifname);
		/* Run the scan, and after scan is done, the active scan flag will be cleared in driver */
		struct wpa_supplicant *wpa_s = drv->ctx;
		if (!wpa_s->scanning &&
			    ((wpa_s->wpa_state <= WPA_SCANNING) ||
			     (wpa_s->wpa_state == WPA_COMPLETED))) {
				wpa_s->normal_scans = 0;
				wpa_s->scan_req = 2;
				wpa_supplicant_req_scan(wpa_s, 0, 0);
			} else if (wpa_s->sched_scanning) {
				wpa_printf(MSG_DEBUG, "Stop ongoing "
					   "sched_scan to allow requested "
					   "full scan to proceed");
				wpa_supplicant_cancel_sched_scan(wpa_s);
				wpa_s->scan_req = 2;
				wpa_supplicant_req_scan(wpa_s, 0, 0);
			} else {
				wpa_printf(MSG_DEBUG, "Ongoing scan action - "
					   "reject new request");
				ret = os_snprintf(buf, buf_len,
					"FAIL-BUSY\n");
			}
#endif /* CONFIG_ACTIVE_CHANNEL_SCAN_SUPPORT */
#endif /* CONFIG_MTK_HE_SUPPORT */
    } else { /* Use private command */
        return -1;
    }
    return ret;
}
#endif

int wpa_driver_set_p2p_noa(void *priv, u8 count, int start, int duration) {
        return 0;
}
int wpa_driver_get_p2p_noa(void *priv, u8 *buf, size_t len) {
        return 0;
}
int wpa_driver_set_p2p_ps(void *priv, int legacy_ps, int opp_ps, int ctwindow) {
        return -1;
}
int wpa_driver_set_ap_wps_p2p_ie(void *priv, const struct wpabuf *beacon,
                                 const struct wpabuf *proberesp,
                                 const struct wpabuf *assocresp) {
        return 0;
}



