/*
 * Driver interaction with generic Linux Wireless Extensions
 * Copyright (c) 2003-2010, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 *
 * This file implements a driver interface for the Linux Wireless Extensions.
 * When used with WE-18 or newer, this interface can be used as-is with number
 * of drivers. In addition to this, some of the common functions in this file
 * can be used by other driver interface implementations that use generic WE
 * ioctls, but require private ioctls for some of the functionality.
 */

#include "includes.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <net/if_arp.h>

#include "wireless_copy.h"
#include "common.h"

#include "eloop.h"
#include "common/ieee802_11_defs.h"
#include "common/wpa_common.h"
#include "priv_netlink.h"
#include "netlink.h"
#include "linux_ioctl.h"
#include "rfkill.h"
#include "driver.h"
#include "driver_wext.h"

#include "common/wpa_ctrl.h"
#include "../../wpa_supplicant_i.h"
#include "../../config_ssid.h"
#include "p2p/p2p.h"



static int wpa_driver_wext_flush_pmkid(void *priv);
static int wpa_driver_wext_get_range(void *priv);
static int wpa_driver_wext_finish_drv_init(struct wpa_driver_wext_data *drv);
static void wpa_driver_wext_disconnect(struct wpa_driver_wext_data *drv);
static int wpa_driver_wext_set_auth_alg(void *priv, int auth_alg);


#define WFA_STAUT_IF "p2p0"
#define WIFI_STAUT_IF "ra0"

#if WIRELESS_EXT <= 11
#ifndef SIOCDEVPRIVATE
#define SIOCDEVPRIVATE					0x8BE0
#endif
#define SIOCIWFIRSTPRIV					SIOCDEVPRIVATE
#endif


#define RT_PRIV_IOCTL                   (SIOCIWFIRSTPRIV + 0x01)
#define RTPRIV_IOCTL_SET                (SIOCIWFIRSTPRIV + 0x02)

#define OID_GET_SET_TOGGLE              0x8000

#define OID_802_DOT1X_IDLE_TIMEOUT      0x0545
#define RT_OID_802_DOT1X_IDLE_TIMEOUT   (OID_GET_SET_TOGGLE | OID_802_DOT1X_IDLE_TIMEOUT)

#define IWEVCUSTOM_P2P_DEVICE_FIND		0x010A
#define IWEVCUSTOM_P2P_RECV_PROV_REQ	0x010B
#define IWEVCUSTOM_P2P_RECV_PROV_RSP	0x010C
#define IWEVCUSTOM_P2P_RECV_INVITE_REQ	0x010D
#define IWEVCUSTOM_P2P_RECV_INVITE_RSP	0x010E
#define IWEVCUSTOM_P2P_RECV_GO_NEGO_REQ	0x010F
#define IWEVCUSTOM_P2P_RECV_GO_NEGO_RSP	0x0110
#define IWEVCUSTOM_P2P_GO_NEG_COMPLETED	0x0111
#define IWEVCUSTOM_P2P_GO_NEG_FAIL		0x0112
#define IWEVCUSTOM_P2P_WPS_COMPLETED	0x0113
#define IWEVCUSTOM_P2P_CONNECTED		0x0114
#define IWEVCUSTOM_P2P_DISCONNECTED		0x0115
#define IWEVCUSTOM_P2P_CONNECT_FAIL				0x0116
#define IWEVCUSTOM_P2P_LEGACY_CONNECTED			0x0117
#define IWEVCUSTOM_P2P_LEGACY_DISCONNECTED		0x0118
#define IWEVCUSTOM_P2P_AP_STA_CONNECTED		0x0119
#define IWEVCUSTOM_P2P_AP_STA_DISCONNECTED		0x011A
#define IWEVCUSTOM_P2P_DEVICE_TABLE_ITEM_DELETE	0x011B
#define IWEVCUSTOM_P2P_GO_NEGO_FAIL_INTENT			0x011C
#define IWEVCUSTOM_P2P_GO_NEGO_FAIL_RECV_RESP		0x011D
#define IWEVCUSTOM_P2P_GO_NEGO_FAIL_RECV_CONFIRM	0x011E
#define IWEVCUSTOM_P2P_WPS_FAIL						0x011F
#define IWEVCUSTOM_P2P_WPS_TIMEOUT					0x0120
#define IWEVCUSTOM_P2P_WPS_2MINS_TIMEOUT			0x0121
#define IWEVCUSTOM_P2P_WPS_PBC_SESSION_OVERLAP		0x0122
#define IWEVCUSTOM_P2P_WPS_SEND_M2D					0x0123
#define IWEVCUSTOM_P2P_WPS_RECEIVE_NACK     		0x0124
#define IWEVCUSTOM_P2P_STOP_CONNECT     			0x0125
#define IWEVCUSTOM_P2P_DEAUTH_AP     				0x0126

typedef struct _RT_802_11_OID_ASSOCIATED_AP_INFO {
	char			name[IFNAMSIZ];
	unsigned char	channel;
	s32			rate;
} RT_802_11_OID_ASSOCIATED_AP_INFO, *PRT_802_11_OID_ASSOCIATED_AP_INFO;

typedef struct _RT_802_11_LINK_STATUS { 										   
	unsigned long CurrTxRate;	   /* in units of 0.5Mbps */				  
	unsigned long ChannelQuality;   /* 0..100 % */							 
	unsigned long TxByteCount; 	 /* both ok and fail */ 					
	unsigned long RxByteCount;		/* both ok and fail */					   
	unsigned long CentralChannel;   /* 40MHz central channel number */ 
	} RT_802_11_LINK_STATUS, *PRT_802_11_LINK_STATUS; 						 

typedef struct _NDIS_802_11_STATISTICS {
	unsigned long Length;		/* Length of structure */
	LARGE_INTEGER TransmittedFragmentCount;
	LARGE_INTEGER MulticastTransmittedFrameCount;
	LARGE_INTEGER FailedCount;
	LARGE_INTEGER RetryCount;
	LARGE_INTEGER MultipleRetryCount;
	LARGE_INTEGER RTSSuccessCount;
	LARGE_INTEGER RTSFailureCount;
	LARGE_INTEGER ACKFailureCount;
	LARGE_INTEGER FrameDuplicateCount;
	LARGE_INTEGER ReceivedFragmentCount;
	LARGE_INTEGER MulticastReceivedFrameCount;
	LARGE_INTEGER FCSErrorCount;
	LARGE_INTEGER TransmittedFrameCount;
	LARGE_INTEGER WEPUndecryptableCount;
	LARGE_INTEGER TKIPLocalMICFailures;
	LARGE_INTEGER TKIPRemoteMICErrors;
	LARGE_INTEGER TKIPICVErrors;
	LARGE_INTEGER TKIPCounterMeasuresInvoked;
	LARGE_INTEGER TKIPReplays;
	LARGE_INTEGER CCMPFormatErrors;
	LARGE_INTEGER CCMPReplays;
	LARGE_INTEGER CCMPDecryptErrors;
	LARGE_INTEGER FourWayHandshakeFailures;
} NDIS_802_11_STATISTICS, *PNDIS_802_11_STATISTICS;

#ifndef GNU_PACKED
#define GNU_PACKED	__attribute__ ((packed))
#endif

typedef struct GNU_PACKED _DOT1X_IDLE_TIMEOUT {
	UCHAR StaAddr[6];
	UINT32 idle_timeout;
} DOT1X_IDLE_TIMEOUT, *PDOT1X_IDLE_TIMEOUT;

int wfa_driver_ralink_set_oid(struct wpa_driver_wext_data *drv, int oid, u8 *data, int len)
{
	struct iwreq iwr;
	int err;


	memset (&iwr, 0, sizeof(iwr));
	strncpy(iwr.ifr_name, WFA_STAUT_IF, 4);
	iwr.u.data.flags = OID_GET_SET_TOGGLE | oid;
	iwr.u.data.pointer = data;
	iwr.u.data.length = len;

	if ( (err = ioctl(drv->ioctl_sock, RT_PRIV_IOCTL, &iwr)) < 0)
	{
		printf("ERR(%d)::  %s in 0x%04x.\n", err, __FUNCTION__, oid);
		return -1;
	}

	return 0;
}

int wfa_driver_ralink_get_oid(struct wpa_driver_wext_data *drv, int oid, u8 *data, int len)
{
	struct iwreq iwr;
	int err;


	os_memset (&iwr, 0, sizeof(iwr));
	os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	iwr.u.data.flags = oid;
	iwr.u.data.pointer = data;
	iwr.u.data.length = len;

	if ( (err = ioctl(drv->ioctl_sock, RT_PRIV_IOCTL, &iwr)) < 0)
	{
		wpa_printf(MSG_ERROR, "%s: ERR(%d) (ifname=%s, oid=0x%04x)", __FUNCTION__, err, drv->ifname, oid);
		return -1;
	}
	wpa_printf(MSG_DEBUG, "%s:SUCCESS (ifname=%s,oid=0x%04x)", __FUNCTION__, drv->ifname, oid);

	return 0;
}


int wpa_driver_wext_set_auth_param(struct wpa_driver_wext_data *drv,
				   int idx, u32 value)
{
	struct iwreq iwr;
	int ret = 0;

	os_memset(&iwr, 0, sizeof(iwr));
	os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	iwr.u.param.flags = idx & IW_AUTH_INDEX;
	iwr.u.param.value = value;

	if (ioctl(drv->ioctl_sock, SIOCSIWAUTH, &iwr) < 0) {
		if (errno != EOPNOTSUPP) {
			wpa_printf(MSG_DEBUG, "WEXT: SIOCSIWAUTH(param %d "
				   "value 0x%x) failed: %s)",
				   idx, value, strerror(errno));
		}
		ret = errno == EOPNOTSUPP ? -2 : -1;
	}

	return ret;
}


/**
 * wpa_driver_wext_get_bssid - Get BSSID, SIOCGIWAP
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * @bssid: Buffer for BSSID
 * Returns: 0 on success, -1 on failure
 */
int wpa_driver_wext_get_bssid(void *priv, u8 *bssid)
{
	struct wpa_driver_wext_data *drv = priv;
	struct iwreq iwr;
	int ret = 0;

	os_memset(&iwr, 0, sizeof(iwr));
	os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);

	if (ioctl(drv->ioctl_sock, SIOCGIWAP, &iwr) < 0) {
		perror("ioctl[SIOCGIWAP]");
		ret = -1;
	}
	os_memcpy(bssid, iwr.u.ap_addr.sa_data, ETH_ALEN);

	return ret;
}


/**
 * wpa_driver_wext_set_bssid - Set BSSID, SIOCSIWAP
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * @bssid: BSSID
 * Returns: 0 on success, -1 on failure
 */
int wpa_driver_wext_set_bssid(void *priv, const u8 *bssid)
{
	struct wpa_driver_wext_data *drv = priv;
	struct iwreq iwr;
	int ret = 0;

	os_memset(&iwr, 0, sizeof(iwr));
	os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	iwr.u.ap_addr.sa_family = ARPHRD_ETHER;
	if (bssid)
		os_memcpy(iwr.u.ap_addr.sa_data, bssid, ETH_ALEN);
	else
		os_memset(iwr.u.ap_addr.sa_data, 0, ETH_ALEN);

	if (ioctl(drv->ioctl_sock, SIOCSIWAP, &iwr) < 0) {
		perror("ioctl[SIOCSIWAP]");
		ret = -1;
	}

	return ret;
}


/**
 * wpa_driver_wext_get_ssid - Get SSID, SIOCGIWESSID
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * @ssid: Buffer for the SSID; must be at least 32 bytes long
 * Returns: SSID length on success, -1 on failure
 */
int wpa_driver_wext_get_ssid(void *priv, u8 *ssid)
{
	struct wpa_driver_wext_data *drv = priv;
	struct iwreq iwr;
	int ret = 0;

	os_memset(&iwr, 0, sizeof(iwr));
	os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	iwr.u.essid.pointer = (caddr_t) ssid;
	iwr.u.essid.length = 32;

	if (ioctl(drv->ioctl_sock, SIOCGIWESSID, &iwr) < 0) {
		perror("ioctl[SIOCGIWESSID]");
		ret = -1;
	} else {
		ret = iwr.u.essid.length;
		if (ret > 32)
			ret = 32;
		/* Some drivers include nul termination in the SSID, so let's
		 * remove it here before further processing. WE-21 changes this
		 * to explicitly require the length _not_ to include nul
		 * termination. */
		if (ret > 0 && ssid[ret - 1] == '\0' &&
		    drv->we_version_compiled < 21)
			ret--;
	}

	return ret;
}


/**
 * wpa_driver_wext_set_ssid - Set SSID, SIOCSIWESSID
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * @ssid: SSID
 * @ssid_len: Length of SSID (0..32)
 * Returns: 0 on success, -1 on failure
 */
int wpa_driver_wext_set_ssid(void *priv, const u8 *ssid, size_t ssid_len)
{
	struct wpa_driver_wext_data *drv = priv;
	struct iwreq iwr;
	int ret = 0;
	char buf[33];

	if (ssid_len > 32)
		return -1;

	os_memset(&iwr, 0, sizeof(iwr));
	os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	/* flags: 1 = ESSID is active, 0 = not (promiscuous) */
	iwr.u.essid.flags = (ssid_len != 0);
	os_memset(buf, 0, sizeof(buf));
	os_memcpy(buf, ssid, ssid_len);
	iwr.u.essid.pointer = (caddr_t) buf;
	if (drv->we_version_compiled < 21) {
		/* For historic reasons, set SSID length to include one extra
		 * character, C string nul termination, even though SSID is
		 * really an octet string that should not be presented as a C
		 * string. Some Linux drivers decrement the length by one and
		 * can thus end up missing the last octet of the SSID if the
		 * length is not incremented here. WE-21 changes this to
		 * explicitly require the length _not_ to include nul
		 * termination. */
		if (ssid_len)
			ssid_len++;
	}
	iwr.u.essid.length = ssid_len;

	if (ioctl(drv->ioctl_sock, SIOCSIWESSID, &iwr) < 0) {
		perror("ioctl[SIOCSIWESSID]");
		ret = -1;
	}

	return ret;
}


/**
 * wpa_driver_wext_set_freq - Set frequency/channel, SIOCSIWFREQ
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * @freq: Frequency in MHz
 * Returns: 0 on success, -1 on failure
 */
int wpa_driver_wext_set_freq(void *priv, int freq)
{
	struct wpa_driver_wext_data *drv = priv;
	struct iwreq iwr;
	int ret = 0;

	os_memset(&iwr, 0, sizeof(iwr));
	os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	iwr.u.freq.m = freq * 100000;
	iwr.u.freq.e = 1;

	if (ioctl(drv->ioctl_sock, SIOCSIWFREQ, &iwr) < 0) {
		perror("ioctl[SIOCSIWFREQ]");
		ret = -1;
	}

	return ret;
}


static void
wpa_driver_wext_event_wireless_custom(void *ctx, char *custom, int flag, int event_length)
{
	union wpa_event_data data;

	wpa_printf(MSG_DEBUG, "WEXT: Custom wireless event: '%s', sizeof data = %d\n",
		   custom, sizeof(data));

	os_memset(&data, 0, sizeof(data));
	/* Host AP driver */
	if (os_strncmp(custom, "MLME-MICHAELMICFAILURE.indication", 33) == 0) {
		data.michael_mic_failure.unicast =
			os_strstr(custom, " unicast ") != NULL;
		/* TODO: parse parameters(?) */
		wpa_supplicant_event(ctx, EVENT_MICHAEL_MIC_FAILURE, &data);
	} else if (os_strncmp(custom, "ASSOCINFO(ReqIEs=", 17) == 0) {
		char *spos;
		int bytes;
		u8 *req_ies = NULL, *resp_ies = NULL;

		spos = custom + 17;

		bytes = strspn(spos, "0123456789abcdefABCDEF");
		if (!bytes || (bytes & 1))
			return;
		bytes /= 2;

		req_ies = os_malloc(bytes);
		if (req_ies == NULL ||
		    hexstr2bin(spos, req_ies, bytes) < 0)
			goto done;
		data.assoc_info.req_ies = req_ies;
		data.assoc_info.req_ies_len = bytes;

		spos += bytes * 2;

		data.assoc_info.resp_ies = NULL;
		data.assoc_info.resp_ies_len = 0;

		if (os_strncmp(spos, " RespIEs=", 9) == 0) {
			spos += 9;

			bytes = strspn(spos, "0123456789abcdefABCDEF");
			if (!bytes || (bytes & 1))
				goto done;
			bytes /= 2;

			resp_ies = os_malloc(bytes);
			if (resp_ies == NULL ||
			    hexstr2bin(spos, resp_ies, bytes) < 0)
				goto done;
			data.assoc_info.resp_ies = resp_ies;
			data.assoc_info.resp_ies_len = bytes;
		}

		wpa_supplicant_event(ctx, EVENT_ASSOCINFO, &data);

	done:
		os_free(resp_ies);
		os_free(req_ies);
#ifdef CONFIG_PEERKEY
	} else if (os_strncmp(custom, "STKSTART.request=", 17) == 0) {
		if (hwaddr_aton(custom + 17, data.stkstart.peer)) {
			wpa_printf(MSG_DEBUG, "WEXT: unrecognized "
				   "STKSTART.request '%s'", custom + 17);
			return;
		}
		wpa_supplicant_event(ctx, EVENT_STKSTART, &data);
#endif /* CONFIG_PEERKEY */

#ifdef ANDROID
	} else if (os_strncmp(custom, "STOP", 4) == 0) {
		wpa_msg(ctx, MSG_INFO, WPA_EVENT_DRIVER_STATE "STOPPED");
	} else if (os_strncmp(custom, "START", 5) == 0) {
		wpa_msg(ctx, MSG_INFO, WPA_EVENT_DRIVER_STATE "STARTED");
	} else if (os_strncmp(custom, "HANG", 4) == 0) {
		wpa_msg(ctx, MSG_INFO, WPA_EVENT_DRIVER_STATE "HANGED");
#endif /* ANDROID */

	}
	else
	{
		int struct_size;
		struct_size = sizeof(data);

		u8 *ptr = (u8 *)custom;

		switch (flag)
		{
		case IWEVCUSTOM_P2P_DEVICE_FIND:
			struct_size = sizeof(struct _p2p_all_info);
			if (event_length != struct_size) {
				wpa_printf(MSG_ERROR, "IWEVCUSTOM_P2P_DEVICE_FIND: Event Structure size is unmatch with Driver!!!");
				return;
			}

			os_memcpy(&data.p2p_all_info, ptr, event_length);
			wpa_supplicant_event(ctx, EVENT_P2P_DEV_FOUND, &data);
			break;

		case IWEVCUSTOM_P2P_RECV_GO_NEGO_REQ:
			struct_size = sizeof(struct _p2p_all_info);
			if (event_length != struct_size) {
				wpa_printf(MSG_ERROR, "IWEVCUSTOM_P2P_RECV_GO_NEGO_REQ: Event Structure size is unmatch with Driver!!!");
				return;
			}

			os_memcpy(&data.p2p_all_info, ptr, event_length);
			wpa_supplicant_event(ctx, EVENT_P2P_GO_NEG_REQ_RX, &data);
			break;

		case IWEVCUSTOM_P2P_RECV_GO_NEGO_RSP:
			printf("%s:: P2P RECV GO NEG RSP.\n", __FUNCTION__);
			break;

		case IWEVCUSTOM_P2P_GO_NEG_COMPLETED:
			wpa_printf(MSG_ERROR, "%s:: P2P NEG COMPLETED!\n", __FUNCTION__);
			wpa_supplicant_event(ctx, EVENT_P2P_GO_NEG_COMPLETED, &data);
			break;

		case IWEVCUSTOM_P2P_RECV_PROV_REQ:
			struct_size = sizeof(struct _p2p_all_info);
			if (event_length != struct_size) {
				wpa_printf(MSG_ERROR, "IWEVCUSTOM_P2P_RECV_PROV_REQ: Event Structure size is unmatch with Driver!!!");
				return;
			}

			os_memcpy(&data.p2p_all_info, ptr, event_length);
			wpa_supplicant_event(ctx, EVENT_P2P_PROV_DISC_REQUEST, &data);
			break;

		case IWEVCUSTOM_P2P_RECV_PROV_RSP:
			wpa_printf(MSG_DEBUG,"%s:: P2P RECV PROV RSP!\n", __FUNCTION__);
			data.p2p_prov_disc_resp.peer = ptr;
			ptr += 6;
			os_memcpy(&data.p2p_prov_disc_resp.config_methods, ptr, 2);
			printf("peer = %02x %02x %02x %02x %02x %02x\n", 
					*(data.p2p_prov_disc_resp.peer), *(data.p2p_prov_disc_resp.peer+1),
					*(data.p2p_prov_disc_resp.peer+2), *(data.p2p_prov_disc_resp.peer+3),
					*(data.p2p_prov_disc_resp.peer+4), *(data.p2p_prov_disc_resp.peer+5));
			printf("config method = %04x\n",  data.p2p_prov_disc_resp.config_methods);
			wpa_supplicant_event(ctx, EVENT_P2P_PROV_DISC_RESPONSE, &data);
			break;

		case IWEVCUSTOM_P2P_RECV_INVITE_REQ:
			printf("%s:: P2P RECV INVITE REQ!\n", __FUNCTION__);
			data.p2p_prov_disc_req.peer = ptr;
			ptr += 6;
			data.p2p_prov_disc_req.dev_addr = ptr;
			ptr += 6;
			data.p2p_prov_disc_req.pri_dev_type = ptr;
			ptr += 8;
			data.p2p_prov_disc_req.dev_name = (char *)ptr;
			ptr += 33;
			wpa_supplicant_event(ctx, EVENT_P2P_INVITE_REQUEST, &data);
			break;

		case IWEVCUSTOM_P2P_WPS_COMPLETED:
			printf("%s:: P2P WPS COMPLETED!\n", __FUNCTION__);
			wpa_supplicant_event(ctx, EVENT_P2P_WPS_COMPLETED, &data);
			break;

		case IWEVCUSTOM_P2P_CONNECTED:
			struct_size = sizeof(struct _p2p_all_info);
			if (event_length != struct_size) {
				wpa_printf(MSG_ERROR, "IWEVCUSTOM_P2P_CONNECTED: Event Structure size is unmatch with Driver!!!");
				return;
			}

			os_memcpy(&data.p2p_all_info, ptr, event_length);
			wpa_supplicant_event(ctx, EVENT_P2P_CONNECTED, &data);
			break;

		case IWEVCUSTOM_P2P_DISCONNECTED:
			os_memcpy(&data.p2p_group_report.Rule, ptr, 1);
			wpa_supplicant_event(ctx, EVENT_P2P_DISCONNECTED, &data);
			break;

		case IWEVCUSTOM_P2P_LEGACY_CONNECTED:
			data.new_sta.addr = (u8 *)custom;
			wpa_supplicant_event(ctx, EVENT_P2P_LEGACY_CONNECTED, &data);
			break;

		case IWEVCUSTOM_P2P_LEGACY_DISCONNECTED:
			data.new_sta.addr = (u8 *)custom;
			wpa_supplicant_event(ctx, EVENT_P2P_LEGACY_DISCONNECTED, &data);
			break;

		case IWEVCUSTOM_P2P_DEVICE_TABLE_ITEM_DELETE:
			data.p2p_dev_found.addr = ptr;
			ptr += 6;
			data.p2p_dev_found.dev_addr = ptr;
			ptr += 6;
			wpa_supplicant_event(ctx, EVENT_P2P_DEVICE_TABLE_ITEM_DELETE, &data);
			break;

		case IWEVCUSTOM_P2P_AP_STA_CONNECTED:
			os_memcpy(&data.p2p_ap_sta_connected.addr, ptr, 6);

			ptr += 6;
			wpa_printf(MSG_DEBUG, "addr = %02x %02x %02x %02x %02x %02x\n",
					data.p2p_ap_sta_connected.addr[0], data.p2p_ap_sta_connected.addr[1],
					data.p2p_ap_sta_connected.addr[2], data.p2p_ap_sta_connected.addr[3],
					data.p2p_ap_sta_connected.addr[4], data.p2p_ap_sta_connected.addr[5]);

			if (*ptr == 1) {
				ptr += 1;
				os_memcpy(&data.p2p_ap_sta_connected.dev_addr, ptr, 6);
				wpa_printf(MSG_DEBUG, "dev_addr = %02x %02x %02x %02x %02x %02x\n",
					*(data.p2p_ap_sta_connected.dev_addr), *(data.p2p_ap_sta_connected.dev_addr+1),
					*(data.p2p_ap_sta_connected.dev_addr+2), *(data.p2p_ap_sta_connected.dev_addr+3),
					*(data.p2p_ap_sta_connected.dev_addr+4), *(data.p2p_ap_sta_connected.dev_addr+5));
			}
#if defined (CONFIG_HE_SUPPORT) && !defined (ANDROID) //for HE Linux platform event
			else
				ptr += 1;
			ptr += 6;
			data.p2p_ap_sta_connected.bIsGo = *ptr;
			ptr += 1;
			data.p2p_ap_sta_connected.bInsPerTable = *ptr;
			ptr += 1;
#ifdef CONFIG_WFD
			ptr += 1;  // alignment
			os_memcpy(&data.p2p_ap_sta_connected.rtsp_port, ptr, 2);
#endif /* CONFIG_WFD */
#endif /* CONFIG_HE_SUPPORT & !ANDROID */
			wpa_supplicant_event(ctx, EVENT_P2P_AP_STA_CONNECTED, &data);
			break;

		case IWEVCUSTOM_P2P_AP_STA_DISCONNECTED:
			os_memcpy(&data.p2p_ap_sta_connected.addr, ptr, 6);

			ptr += 6;
			wpa_printf(MSG_DEBUG, "addr = %02x %02x %02x %02x %02x %02x\n",
					data.p2p_ap_sta_connected.addr[0], data.p2p_ap_sta_connected.addr[1],
					data.p2p_ap_sta_connected.addr[2], data.p2p_ap_sta_connected.addr[3],
					data.p2p_ap_sta_connected.addr[4], data.p2p_ap_sta_connected.addr[5]);

			if (*ptr == 1) {
				ptr += 1;
				os_memcpy(&data.p2p_ap_sta_connected.dev_addr, ptr, 6);
				wpa_printf(MSG_DEBUG, "dev_addr = %02x %02x %02x %02x %02x %02x\n",
					*(data.p2p_ap_sta_connected.dev_addr), *(data.p2p_ap_sta_connected.dev_addr+1),
					*(data.p2p_ap_sta_connected.dev_addr+2), *(data.p2p_ap_sta_connected.dev_addr+3),
					*(data.p2p_ap_sta_connected.dev_addr+4), *(data.p2p_ap_sta_connected.dev_addr+5));
			}

			wpa_supplicant_event(ctx, EVENT_P2P_AP_STA_DISCONNECTED, &data);
			break;
#if defined (CONFIG_HE_SUPPORT) && !defined (ANDROID) //for HE Linux platform event
		case IWEVCUSTOM_P2P_GO_NEGO_FAIL_INTENT:
			data.new_sta.addr = (u8 *)custom;
			wpa_supplicant_event(ctx, EVENT_P2P_GO_NEGO_FAIL_INTENT, &data);
			break;
		case IWEVCUSTOM_P2P_GO_NEGO_FAIL_RECV_RESP:
			data.fail_status.addr = ptr;
			ptr += 6;
			data.fail_status.rv = *ptr;
			wpa_supplicant_event(ctx, EVENT_P2P_GO_NEGO_FAIL_RECV_RESP, &data);
			break;
		case IWEVCUSTOM_P2P_GO_NEGO_FAIL_RECV_CONFIRM:
			data.fail_status.addr = ptr;
			ptr += 6;
			data.fail_status.rv = *ptr;
			wpa_supplicant_event(ctx, EVENT_P2P_GO_NEGO_FAIL_RECV_CONFIRM, &data);
			break;
		case IWEVCUSTOM_P2P_WPS_FAIL:
			data.fail_status.addr = ptr;
			ptr += 6;
			data.fail_status.rv = *ptr;
			wpa_supplicant_event(ctx, EVENT_P2P_WPS_FAIL, &data);
			break;
		case IWEVCUSTOM_P2P_WPS_TIMEOUT:
			data.new_sta.addr = (u8 *)custom;
			wpa_supplicant_event(ctx, EVENT_P2P_WPS_TIMEOUT, &data);
			break;
		case IWEVCUSTOM_P2P_WPS_2MINS_TIMEOUT:
			wpa_supplicant_event(ctx, EVENT_P2P_WPS_2MINS_TIMEOUT, &data);
			break;
		case IWEVCUSTOM_P2P_WPS_PBC_SESSION_OVERLAP:
			wpa_supplicant_event(ctx, EVENT_P2P_WPS_PBC_SESSION_OVERLAP, &data);
			break;
		case IWEVCUSTOM_P2P_WPS_SEND_M2D:
			wpa_supplicant_event(ctx, EVENT_P2P_WPS_SEND_M2D, &data);
			break;
		case IWEVCUSTOM_P2P_WPS_RECEIVE_NACK:
			wpa_supplicant_event(ctx, EVENT_P2P_WPS_RECEIVE_NACK, &data);
			break;
		case IWEVCUSTOM_P2P_STOP_CONNECT:
			data.new_sta.addr = (u8 *)custom;
			wpa_supplicant_event(ctx, EVENT_P2P_STOP_CONNECT, &data);
			break;
		case IWEVCUSTOM_P2P_DEAUTH_AP:
			data.new_sta.addr = (u8 *)custom;
			wpa_supplicant_event(ctx, EVENT_P2P_DEAUTH_AP, &data);
			break;
#endif /* CONFIG_HE_SUPPORT & !ANDROID */
		default:
			printf("%s:: unknown command type=%x\n", __FUNCTION__, flag);
		}
	}
}


static int wpa_driver_wext_event_wireless_michaelmicfailure(
	void *ctx, const char *ev, size_t len)
{
	const struct iw_michaelmicfailure *mic;
	union wpa_event_data data;

	if (len < sizeof(*mic))
		return -1;

	mic = (const struct iw_michaelmicfailure *) ev;

	wpa_printf(MSG_DEBUG, "Michael MIC failure wireless event: "
		   "flags=0x%x src_addr=" MACSTR, mic->flags,
		   MAC2STR(mic->src_addr.sa_data));

	os_memset(&data, 0, sizeof(data));
	data.michael_mic_failure.unicast = !(mic->flags & IW_MICFAILURE_GROUP);
	wpa_supplicant_event(ctx, EVENT_MICHAEL_MIC_FAILURE, &data);

	return 0;
}


static int wpa_driver_wext_event_wireless_pmkidcand(
	struct wpa_driver_wext_data *drv, const char *ev, size_t len)
{
	const struct iw_pmkid_cand *cand;
	union wpa_event_data data;
	const u8 *addr;

	if (len < sizeof(*cand))
		return -1;

	cand = (const struct iw_pmkid_cand *) ev;
	addr = (const u8 *) cand->bssid.sa_data;

	wpa_printf(MSG_DEBUG, "PMKID candidate wireless event: "
		   "flags=0x%x index=%d bssid=" MACSTR, cand->flags,
		   cand->index, MAC2STR(addr));

	os_memset(&data, 0, sizeof(data));
	os_memcpy(data.pmkid_candidate.bssid, addr, ETH_ALEN);
	data.pmkid_candidate.index = cand->index;
	data.pmkid_candidate.preauth = cand->flags & IW_PMKID_CAND_PREAUTH;
	wpa_supplicant_event(drv->ctx, EVENT_PMKID_CANDIDATE, &data);

	return 0;
}


static int wpa_driver_wext_event_wireless_assocreqie(
	struct wpa_driver_wext_data *drv, const char *ev, int len)
{
	if (len < 0)
		return -1;

	wpa_hexdump(MSG_DEBUG, "AssocReq IE wireless event", (const u8 *) ev,
		    len);
	os_free(drv->assoc_req_ies);
	drv->assoc_req_ies = os_malloc(len);
	if (drv->assoc_req_ies == NULL) {
		drv->assoc_req_ies_len = 0;
		return -1;
	}
	os_memcpy(drv->assoc_req_ies, ev, len);
	drv->assoc_req_ies_len = len;

	return 0;
}


static int wpa_driver_wext_event_wireless_assocrespie(
	struct wpa_driver_wext_data *drv, const char *ev, int len)
{
	if (len < 0)
		return -1;

	wpa_hexdump(MSG_DEBUG, "AssocResp IE wireless event", (const u8 *) ev,
		    len);
	os_free(drv->assoc_resp_ies);
	drv->assoc_resp_ies = os_malloc(len);
	if (drv->assoc_resp_ies == NULL) {
		drv->assoc_resp_ies_len = 0;
		return -1;
	}
	os_memcpy(drv->assoc_resp_ies, ev, len);
	drv->assoc_resp_ies_len = len;

	return 0;
}


static void wpa_driver_wext_event_assoc_ies(struct wpa_driver_wext_data *drv)
{
	union wpa_event_data data;

	if (drv->assoc_req_ies == NULL && drv->assoc_resp_ies == NULL)
		return;

	os_memset(&data, 0, sizeof(data));
	if (drv->assoc_req_ies) {
		data.assoc_info.req_ies = drv->assoc_req_ies;
		data.assoc_info.req_ies_len = drv->assoc_req_ies_len;
	}
	if (drv->assoc_resp_ies) {
		data.assoc_info.resp_ies = drv->assoc_resp_ies;
		data.assoc_info.resp_ies_len = drv->assoc_resp_ies_len;
	}

	wpa_supplicant_event(drv->ctx, EVENT_ASSOCINFO, &data);

	os_free(drv->assoc_req_ies);
	drv->assoc_req_ies = NULL;
	os_free(drv->assoc_resp_ies);
	drv->assoc_resp_ies = NULL;
}


static void wpa_driver_wext_event_wireless(struct wpa_driver_wext_data *drv,
					   char *data, int len)
{
	struct iw_event iwe_buf, *iwe = &iwe_buf;
	char *pos, *end, *custom, *buf;

	pos = data;
	end = data + len;

	while (pos + IW_EV_LCP_LEN <= end) {
		/* Event data may be unaligned, so make a local, aligned copy
		 * before processing. */
		os_memcpy(&iwe_buf, pos, IW_EV_LCP_LEN);
		wpa_printf(MSG_DEBUG, "Wireless event: cmd=0x%x len=%d",
			   iwe->cmd, iwe->len);
		if (iwe->len <= IW_EV_LCP_LEN)
			return;

		custom = pos + IW_EV_POINT_LEN;
		if (drv->we_version_compiled > 18 &&
		    (iwe->cmd == IWEVMICHAELMICFAILURE ||
		     iwe->cmd == IWEVCUSTOM ||
		     iwe->cmd == IWEVASSOCREQIE ||
		     iwe->cmd == IWEVASSOCRESPIE ||
		     iwe->cmd == IWEVPMKIDCAND)) {
			/* WE-19 removed the pointer from struct iw_point */
			char *dpos = (char *) &iwe_buf.u.data.length;
			int dlen = dpos - (char *) &iwe_buf;
			os_memcpy(dpos, pos + IW_EV_LCP_LEN,
				  sizeof(struct iw_event) - dlen);
		} else {
			os_memcpy(&iwe_buf, pos, sizeof(struct iw_event));
			custom += IW_EV_POINT_OFF;
		}

		switch (iwe->cmd) {
		case SIOCGIWAP:
			wpa_printf(MSG_DEBUG, "Wireless event: new AP: "
				   MACSTR,
				   MAC2STR((u8 *) iwe->u.ap_addr.sa_data));
			if (is_zero_ether_addr(
				    (const u8 *) iwe->u.ap_addr.sa_data) ||
			    os_memcmp(iwe->u.ap_addr.sa_data,
				      "\x44\x44\x44\x44\x44\x44", ETH_ALEN) ==
			    0) {
				wpa_printf(MSG_DEBUG, "WTF case");
				os_free(drv->assoc_req_ies);
				drv->assoc_req_ies = NULL;
				os_free(drv->assoc_resp_ies);
				drv->assoc_resp_ies = NULL;
#ifdef ANDROID
				if (!drv->skip_disconnect) {
					drv->skip_disconnect = 1;
#endif
				wpa_printf(MSG_DEBUG, "skip_disconnect");
				wpa_supplicant_event(drv->ctx, EVENT_DISASSOC,
						     NULL);
#ifdef ANDROID
				}
#endif
			} else {
#ifdef ANDROID
				drv->skip_disconnect = 0;
#endif
				wpa_driver_wext_event_assoc_ies(drv);
				wpa_supplicant_event(drv->ctx, EVENT_ASSOC,
						     NULL);
			}
			break;
		case IWEVMICHAELMICFAILURE:
			if (custom + iwe->u.data.length > end) {
				wpa_printf(MSG_DEBUG, "WEXT: Invalid "
					   "IWEVMICHAELMICFAILURE length");
				return;
			}
			wpa_driver_wext_event_wireless_michaelmicfailure(
				drv->ctx, custom, iwe->u.data.length);
			break;
		case IWEVCUSTOM:
			if (custom + iwe->u.data.length > end) {
				wpa_printf(MSG_DEBUG, "WEXT: Invalid "
					   "IWEVCUSTOM length");
				return;
			}
			buf = os_malloc(iwe->u.data.length + 1);
			if (buf == NULL)
				return;
			os_memcpy(buf, custom, iwe->u.data.length);
			buf[iwe->u.data.length] = '\0';

			wpa_driver_wext_event_wireless_custom(drv->ctx, buf, iwe->u.data.flags, iwe->u.data.length);
			os_free(buf);
			break;
		case SIOCGIWSCAN:
			drv->scan_complete_events = 1;
			eloop_cancel_timeout(wpa_driver_wext_scan_timeout,
					     drv, drv->ctx);
			wpa_supplicant_event(drv->ctx, EVENT_SCAN_RESULTS,
					     NULL);
			break;
		case IWEVASSOCREQIE:
			if (custom + iwe->u.data.length > end) {
				wpa_printf(MSG_DEBUG, "WEXT: Invalid "
					   "IWEVASSOCREQIE length");
				return;
			}
			wpa_driver_wext_event_wireless_assocreqie(
				drv, custom, iwe->u.data.length);
			break;
		case IWEVASSOCRESPIE:
			if (custom + iwe->u.data.length > end) {
				wpa_printf(MSG_DEBUG, "WEXT: Invalid "
					   "IWEVASSOCRESPIE length");
				return;
			}
			wpa_driver_wext_event_wireless_assocrespie(
				drv, custom, iwe->u.data.length);
			break;
		case IWEVPMKIDCAND:
			if (custom + iwe->u.data.length > end) {
				wpa_printf(MSG_DEBUG, "WEXT: Invalid "
					   "IWEVPMKIDCAND length");
				return;
			}
			wpa_driver_wext_event_wireless_pmkidcand(
				drv, custom, iwe->u.data.length);
			break;
		}

		pos += iwe->len;
	}
}


static void wpa_driver_wext_event_link(struct wpa_driver_wext_data *drv,
				       char *buf, size_t len, int del)
{
	union wpa_event_data event;

	os_memset(&event, 0, sizeof(event));
	if (len > sizeof(event.interface_status.ifname))
		len = sizeof(event.interface_status.ifname) - 1;
	os_memcpy(event.interface_status.ifname, buf, len);
	event.interface_status.ievent = del ? EVENT_INTERFACE_REMOVED :
		EVENT_INTERFACE_ADDED;

	wpa_printf(MSG_DEBUG, "RTM_%sLINK, IFLA_IFNAME: Interface '%s' %s",
		   del ? "DEL" : "NEW",
		   event.interface_status.ifname,
		   del ? "removed" : "added");

	if (os_strcmp(drv->ifname, event.interface_status.ifname) == 0) {
		if (del)
			drv->if_removed = 1;
		else
			drv->if_removed = 0;
	}

	wpa_supplicant_event(drv->ctx, EVENT_INTERFACE_STATUS, &event);
}


static int wpa_driver_wext_own_ifname(struct wpa_driver_wext_data *drv,
				      u8 *buf, size_t len)
{
	int attrlen, rta_len;
	struct rtattr *attr;

	attrlen = len;
	attr = (struct rtattr *) buf;

	rta_len = RTA_ALIGN(sizeof(struct rtattr));
	while (RTA_OK(attr, attrlen)) {
		if (attr->rta_type == IFLA_IFNAME) {
			if (os_strcmp(((char *) attr) + rta_len, drv->ifname)
			    == 0)
				return 1;
			else
				break;
		}
		attr = RTA_NEXT(attr, attrlen);
	}

	return 0;
}


static int wpa_driver_wext_own_ifindex(struct wpa_driver_wext_data *drv,
				       int ifindex, u8 *buf, size_t len)
{
	if (drv->ifindex == ifindex || drv->ifindex2 == ifindex)
		return 1;

	if (drv->if_removed && wpa_driver_wext_own_ifname(drv, buf, len)) {
		drv->ifindex = if_nametoindex(drv->ifname);
		wpa_printf(MSG_DEBUG, "WEXT: Update ifindex for a removed "
			   "interface");
		wpa_driver_wext_finish_drv_init(drv);
		return 1;
	}

	return 0;
}


static void wpa_driver_wext_event_rtm_newlink(void *ctx, struct ifinfomsg *ifi,
					      u8 *buf, size_t len)
{
	struct wpa_driver_wext_data *drv = ctx;
	int attrlen, rta_len;
	struct rtattr *attr;

	if (!wpa_driver_wext_own_ifindex(drv, ifi->ifi_index, buf, len)) {
		wpa_printf(MSG_DEBUG, "Ignore event for foreign ifindex %d",
			   ifi->ifi_index);
		return;
	}

	wpa_printf(MSG_DEBUG, "RTM_NEWLINK: operstate=%d ifi_flags=0x%x "
		   "(%s%s%s%s)",
		   drv->operstate, ifi->ifi_flags,
		   (ifi->ifi_flags & IFF_UP) ? "[UP]" : "",
		   (ifi->ifi_flags & IFF_RUNNING) ? "[RUNNING]" : "",
		   (ifi->ifi_flags & IFF_LOWER_UP) ? "[LOWER_UP]" : "",
		   (ifi->ifi_flags & IFF_DORMANT) ? "[DORMANT]" : "");

	if (!drv->if_disabled && !(ifi->ifi_flags & IFF_UP)) {
		wpa_printf(MSG_DEBUG, "WEXT: Interface down");
		drv->if_disabled = 1;
		wpa_supplicant_event(drv->ctx, EVENT_INTERFACE_DISABLED, NULL);
	}

	if (drv->if_disabled && (ifi->ifi_flags & IFF_UP)) {
		wpa_printf(MSG_DEBUG, "WEXT: Interface up");
		drv->if_disabled = 0;
		wpa_supplicant_event(drv->ctx, EVENT_INTERFACE_ENABLED, NULL);
	}

	/*
	 * Some drivers send the association event before the operup event--in
	 * this case, lifting operstate in wpa_driver_wext_set_operstate()
	 * fails. This will hit us when wpa_supplicant does not need to do
	 * IEEE 802.1X authentication
	 */
	if (drv->operstate == 1 &&
	    (ifi->ifi_flags & (IFF_LOWER_UP | IFF_DORMANT)) == IFF_LOWER_UP &&
	    !(ifi->ifi_flags & IFF_RUNNING))
		netlink_send_oper_ifla(drv->netlink, drv->ifindex,
				       -1, IF_OPER_UP);

	attrlen = len;
	attr = (struct rtattr *) buf;

	rta_len = RTA_ALIGN(sizeof(struct rtattr));
	while (RTA_OK(attr, attrlen)) {
		if (attr->rta_type == IFLA_WIRELESS) {
			wpa_driver_wext_event_wireless(
				drv, ((char *) attr) + rta_len,
				attr->rta_len - rta_len);
		} else if (attr->rta_type == IFLA_IFNAME) {
			wpa_driver_wext_event_link(drv,
						   ((char *) attr) + rta_len,
						   attr->rta_len - rta_len, 0);
		}
		attr = RTA_NEXT(attr, attrlen);
	}
}


static void wpa_driver_wext_event_rtm_dellink(void *ctx, struct ifinfomsg *ifi,
					      u8 *buf, size_t len)
{
	struct wpa_driver_wext_data *drv = ctx;
	int attrlen, rta_len;
	struct rtattr *attr;

	attrlen = len;
	attr = (struct rtattr *) buf;

	rta_len = RTA_ALIGN(sizeof(struct rtattr));
	while (RTA_OK(attr, attrlen)) {
		if (attr->rta_type == IFLA_IFNAME) {
			wpa_driver_wext_event_link(drv,
						   ((char *) attr) + rta_len,
						   attr->rta_len - rta_len, 1);
		}
		attr = RTA_NEXT(attr, attrlen);
	}
}


static void wpa_driver_wext_rfkill_blocked(void *ctx)
{
	wpa_printf(MSG_DEBUG, "WEXT: RFKILL blocked");
	/*
	 * This may be for any interface; use ifdown event to disable
	 * interface.
	 */
}


static void wpa_driver_wext_rfkill_unblocked(void *ctx)
{
	struct wpa_driver_wext_data *drv = ctx;
	wpa_printf(MSG_DEBUG, "WEXT: RFKILL unblocked");
	if (linux_set_iface_flags(drv->ioctl_sock, drv->ifname, 1)) {
		wpa_printf(MSG_DEBUG, "WEXT: Could not set interface UP "
			   "after rfkill unblock");
		return;
	}
	/* rtnetlink ifup handler will report interface as enabled */
}


static void wext_get_phy_name(struct wpa_driver_wext_data *drv)
{
	/* Find phy (radio) to which this interface belongs */
	char buf[90], *pos;
	int f, rv;

	drv->phyname[0] = '\0';
	snprintf(buf, sizeof(buf) - 1, "/sys/class/net/%s/phy80211/name",
		 drv->ifname);
	f = open(buf, O_RDONLY);
	if (f < 0) {
		wpa_printf(MSG_DEBUG, "Could not open file %s: %s",
			   buf, strerror(errno));
		return;
	}

	rv = read(f, drv->phyname, sizeof(drv->phyname) - 1);
	close(f);
	if (rv < 0) {
		wpa_printf(MSG_DEBUG, "Could not read file %s: %s",
			   buf, strerror(errno));
		return;
	}

	drv->phyname[rv] = '\0';
	pos = os_strchr(drv->phyname, '\n');
	if (pos)
		*pos = '\0';
	wpa_printf(MSG_DEBUG, "wext: interface %s phy: %s",
		   drv->ifname, drv->phyname);
}


/**
 * wpa_driver_wext_init - Initialize WE driver interface
 * @ctx: context to be used when calling wpa_supplicant functions,
 * e.g., wpa_supplicant_event()
 * @ifname: interface name, e.g., wlan0
 * Returns: Pointer to private data, %NULL on failure
 */
void * wpa_driver_wext_init(void *ctx, const char *ifname)
{
	struct wpa_driver_wext_data *drv;
	struct netlink_config *cfg;
	struct rfkill_config *rcfg;
	char path[128];
	struct stat buf;

	drv = os_zalloc(sizeof(*drv));
	if (drv == NULL)
		return NULL;
	drv->ctx = ctx;
	os_strlcpy(drv->ifname, ifname, sizeof(drv->ifname));

	os_snprintf(path, sizeof(path), "/sys/class/net/%s/phy80211", ifname);
	if (stat(path, &buf) == 0) {
		wpa_printf(MSG_DEBUG, "WEXT: cfg80211-based driver detected");
		drv->cfg80211 = 1;
		wext_get_phy_name(drv);
	}

	drv->ioctl_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (drv->ioctl_sock < 0) {
		perror("socket(PF_INET,SOCK_DGRAM)");
		goto err1;
	}

	cfg = os_zalloc(sizeof(*cfg));
	if (cfg == NULL)
		goto err1;
	cfg->ctx = drv;
	cfg->newlink_cb = wpa_driver_wext_event_rtm_newlink;
	cfg->dellink_cb = wpa_driver_wext_event_rtm_dellink;
	drv->netlink = netlink_init(cfg);
	if (drv->netlink == NULL) {
		os_free(cfg);
		goto err2;
	}

	rcfg = os_zalloc(sizeof(*rcfg));
	if (rcfg == NULL)
		goto err3;
	rcfg->ctx = drv;
	os_strlcpy(rcfg->ifname, ifname, sizeof(rcfg->ifname));
	rcfg->blocked_cb = wpa_driver_wext_rfkill_blocked;
	rcfg->unblocked_cb = wpa_driver_wext_rfkill_unblocked;
	drv->rfkill = rfkill_init(rcfg);
	if (drv->rfkill == NULL) {
		wpa_printf(MSG_DEBUG, "WEXT: RFKILL status not available");
		os_free(rcfg);
	}

	drv->mlme_sock = -1;
#ifdef ANDROID
	drv->errors = 0;
	drv->driver_is_loaded = TRUE;
	drv->skip_disconnect = 0;
	//drv->bgscan_enabled = 0;
#endif
	if (wpa_driver_wext_finish_drv_init(drv) < 0)
		goto err3;

	wpa_driver_wext_set_auth_param(drv, IW_AUTH_WPA_ENABLED, 1);

	return drv;

err3:
	rfkill_deinit(drv->rfkill);
	netlink_deinit(drv->netlink);
err2:
	close(drv->ioctl_sock);
err1:
	os_free(drv);
	return NULL;
}


static void wpa_driver_wext_send_rfkill(void *eloop_ctx, void *timeout_ctx)
{
	wpa_supplicant_event(timeout_ctx, EVENT_INTERFACE_DISABLED, NULL);
}


static int wpa_driver_wext_finish_drv_init(struct wpa_driver_wext_data *drv)
{
	int send_rfkill_event = 0;

	if (linux_set_iface_flags(drv->ioctl_sock, drv->ifname, 1) < 0) {
		if (rfkill_is_blocked(drv->rfkill)) {
			wpa_printf(MSG_DEBUG, "WEXT: Could not yet enable "
				   "interface '%s' due to rfkill",
				   drv->ifname);
			drv->if_disabled = 1;
			send_rfkill_event = 1;
		} else {
			wpa_printf(MSG_ERROR, "WEXT: Could not set "
				   "interface '%s' UP", drv->ifname);
			return -1;
		}
	}

	/*
	 * Make sure that the driver does not have any obsolete PMKID entries.
	 */
	wpa_driver_wext_flush_pmkid(drv);

	if (wpa_driver_wext_set_mode(drv, 0) < 0) {
		wpa_printf(MSG_DEBUG, "Could not configure driver to use "
			   "managed mode");
		/* Try to use it anyway */
	}

	wpa_driver_wext_get_range(drv);

	/*
	 * Unlock the driver's BSSID and force to a random SSID to clear any
	 * previous association the driver might have when the supplicant
	 * starts up.
	 */
	wpa_driver_wext_disconnect(drv);

	drv->ifindex = if_nametoindex(drv->ifname);

	if (os_strncmp(drv->ifname, "wlan", 4) == 0) {
		/*
		 * Host AP driver may use both wlan# and wifi# interface in
		 * wireless events. Since some of the versions included WE-18
		 * support, let's add the alternative ifindex also from
		 * driver_wext.c for the time being. This may be removed at
		 * some point once it is believed that old versions of the
		 * driver are not in use anymore.
		 */
		char ifname2[IFNAMSIZ + 1];
		os_strlcpy(ifname2, drv->ifname, sizeof(ifname2));
		os_memcpy(ifname2, "wifi", 4);
		wpa_driver_wext_alternative_ifindex(drv, ifname2);
	}

	netlink_send_oper_ifla(drv->netlink, drv->ifindex,
			       1, IF_OPER_DORMANT);

	if (send_rfkill_event) {
		eloop_register_timeout(0, 0, wpa_driver_wext_send_rfkill,
				       drv, drv->ctx);
	}

	return 0;
}


/**
 * wpa_driver_wext_deinit - Deinitialize WE driver interface
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 *
 * Shut down driver interface and processing of driver events. Free
 * private data buffer if one was allocated in wpa_driver_wext_init().
 */
void wpa_driver_wext_deinit(void *priv)
{
	struct wpa_driver_wext_data *drv = priv;

	wpa_driver_wext_set_auth_param(drv, IW_AUTH_WPA_ENABLED, 0);

	eloop_cancel_timeout(wpa_driver_wext_scan_timeout, drv, drv->ctx);

	/*
	 * Clear possibly configured driver parameters in order to make it
	 * easier to use the driver after wpa_supplicant has been terminated.
	 */
	wpa_driver_wext_disconnect(drv);

	netlink_send_oper_ifla(drv->netlink, drv->ifindex, 0, IF_OPER_UP);
	netlink_deinit(drv->netlink);
	rfkill_deinit(drv->rfkill);

	if (drv->mlme_sock >= 0)
		eloop_unregister_read_sock(drv->mlme_sock);

	(void) linux_set_iface_flags(drv->ioctl_sock, drv->ifname, 0);

	close(drv->ioctl_sock);
	if (drv->mlme_sock >= 0)
		close(drv->mlme_sock);
	os_free(drv->assoc_req_ies);
	os_free(drv->assoc_resp_ies);
	os_free(drv);
}


/**
 * wpa_driver_wext_scan_timeout - Scan timeout to report scan completion
 * @eloop_ctx: Unused
 * @timeout_ctx: ctx argument given to wpa_driver_wext_init()
 *
 * This function can be used as registered timeout when starting a scan to
 * generate a scan completed event if the driver does not report this.
 */
void wpa_driver_wext_scan_timeout(void *eloop_ctx, void *timeout_ctx)
{
	wpa_printf(MSG_DEBUG, "Scan timeout - try to get results");
	wpa_supplicant_event(timeout_ctx, EVENT_SCAN_RESULTS, NULL);
}

static int ralink_set_oid(struct wpa_driver_wext_data *drv,
			  unsigned short oid, char *data, int len)
{
	char *buf;
	struct iwreq iwr;

	buf = os_zalloc(len);
	if (buf == NULL)
		return -1;
	os_memset(&iwr, 0, sizeof(iwr));
	os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	iwr.u.data.flags = oid;
	iwr.u.data.flags |= OID_GET_SET_TOGGLE;

	if (data)
		os_memcpy(buf, data, len);

	iwr.u.data.pointer = (caddr_t) buf;
	iwr.u.data.length = len;

	if (ioctl(drv->ioctl_sock, RT_PRIV_IOCTL, &iwr) < 0) {
		wpa_printf(MSG_DEBUG, "%s: oid=0x%x len (%d) failed",
			   __func__, oid, len);
		os_free(buf);
		return -1;
	}
	os_free(buf);
	return 0;
}

/**
 * wpa_driver_wext_scan - Request the driver to initiate scan
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * @param: Scan parameters (specific SSID to scan for (ProbeReq), etc.)
 * Returns: 0 on success, -1 on failure
 */
int wpa_driver_wext_scan(void *priv, struct wpa_driver_scan_params *params)
{
	struct wpa_driver_wext_data *drv = priv;
	struct iwreq iwr;
	int ret = 0, timeout;
	struct iw_scan_req req;
	const u8 *ssid = params->ssids[0].ssid;
	size_t ssid_len = params->ssids[0].ssid_len;
#ifdef ANDROID
	struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)(drv->ctx);
	int scan_probe_flag = 0;
#endif

	if (ssid_len > IW_ESSID_MAX_SIZE) {
		wpa_printf(MSG_DEBUG, "%s: too long SSID (%lu)",
			   __FUNCTION__, (unsigned long) ssid_len);
		return -1;
	}

	if (ralink_set_oid(drv, RT_OID_WPS_PROBE_REQ_IE,
			   (char *) params->extra_ies, params->extra_ies_len) <
	    0) {
		wpa_printf(MSG_DEBUG, "RALINK: Failed to set "
			   "RT_OID_WPS_PROBE_REQ_IE");
	}

	os_memset(&iwr, 0, sizeof(iwr));
	os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);

	if (ssid && ssid_len) {
		os_memset(&req, 0, sizeof(req));
		req.essid_len = ssid_len;
		req.bssid.sa_family = ARPHRD_ETHER;
		os_memset(req.bssid.sa_data, 0xff, ETH_ALEN);
		os_memcpy(req.essid, ssid, ssid_len);
		iwr.u.data.pointer = (caddr_t) &req;
		iwr.u.data.length = sizeof(req);
		iwr.u.data.flags = IW_SCAN_THIS_ESSID;
	}

	if (ioctl(drv->ioctl_sock, SIOCSIWSCAN, &iwr) < 0) {
		perror("ioctl[SIOCSIWSCAN]");
		ret = -1;
	}

	/* Not all drivers generate "scan completed" wireless event, so try to
	 * read results after a timeout. */
	timeout = 10;
	if (drv->scan_complete_events) {
		/*
		 * The driver seems to deliver SIOCGIWSCAN events to notify
		 * when scan is complete, so use longer timeout to avoid race
		 * conditions with scanning and following association request.
		 */
		timeout = 30;
	}
	wpa_printf(MSG_DEBUG, "Scan requested (ret=%d) - scan timeout %d "
		   "seconds", ret, timeout);
	eloop_cancel_timeout(wpa_driver_wext_scan_timeout, drv, drv->ctx);
	eloop_register_timeout(timeout, 0, wpa_driver_wext_scan_timeout, drv,
			       drv->ctx);

	return ret;
}


static u8 * wpa_driver_wext_giwscan(struct wpa_driver_wext_data *drv,
				    size_t *len)
{
	struct iwreq iwr;
	u8 *res_buf;
	size_t res_buf_len;

	res_buf_len = IW_SCAN_MAX_DATA;
	for (;;) {
		res_buf = os_malloc(res_buf_len);
		if (res_buf == NULL)
			return NULL;
		os_memset(&iwr, 0, sizeof(iwr));
		os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
		iwr.u.data.pointer = res_buf;
		iwr.u.data.length = res_buf_len;

		if (ioctl(drv->ioctl_sock, SIOCGIWSCAN, &iwr) == 0)
			break;

		if (errno == E2BIG && res_buf_len < 65535) {
			os_free(res_buf);
			res_buf = NULL;
			res_buf_len *= 2;
			if (res_buf_len > 65535)
				res_buf_len = 65535; /* 16-bit length field */
			wpa_printf(MSG_DEBUG, "Scan results did not fit - "
				   "trying larger buffer (%lu bytes)",
				   (unsigned long) res_buf_len);
		} else {
			perror("ioctl[SIOCGIWSCAN]");
			os_free(res_buf);
			return NULL;
		}
	}

	if (iwr.u.data.length > res_buf_len) {
		os_free(res_buf);
		return NULL;
	}
	*len = iwr.u.data.length;

	return res_buf;
}


/*
 * Data structure for collecting WEXT scan results. This is needed to allow
 * the various methods of reporting IEs to be combined into a single IE buffer.
 */
struct wext_scan_data {
	struct wpa_scan_res res;
	u8 *ie;
	size_t ie_len;
	u8 ssid[32];
	size_t ssid_len;
	int maxrate;
};


static void wext_get_scan_mode(struct iw_event *iwe,
			       struct wext_scan_data *res)
{
	if (iwe->u.mode == IW_MODE_ADHOC)
		res->res.caps |= IEEE80211_CAP_IBSS;
	else if (iwe->u.mode == IW_MODE_MASTER || iwe->u.mode == IW_MODE_INFRA)
		res->res.caps |= IEEE80211_CAP_ESS;
}


static void wext_get_scan_ssid(struct iw_event *iwe,
			       struct wext_scan_data *res, char *custom,
			       char *end)
{
	int ssid_len = iwe->u.essid.length;
	if (custom + ssid_len > end)
		return;
	if (iwe->u.essid.flags &&
	    ssid_len > 0 &&
	    ssid_len <= IW_ESSID_MAX_SIZE) {
		os_memcpy(res->ssid, custom, ssid_len);
		res->ssid_len = ssid_len;
	}
}


static void wext_get_scan_freq(struct iw_event *iwe,
			       struct wext_scan_data *res)
{
	int divi = 1000000, i;

	if (iwe->u.freq.e == 0) {
		/*
		 * Some drivers do not report frequency, but a channel.
		 * Try to map this to frequency by assuming they are using
		 * IEEE 802.11b/g.  But don't overwrite a previously parsed
		 * frequency if the driver sends both frequency and channel,
		 * since the driver may be sending an A-band channel that we
		 * don't handle here.
		 */

		if (res->res.freq)
			return;

		if (iwe->u.freq.m >= 1 && iwe->u.freq.m <= 13) {
			res->res.freq = 2407 + 5 * iwe->u.freq.m;
			return;
		} else if (iwe->u.freq.m == 14) {
			res->res.freq = 2484;
			return;
		}
	}

	if (iwe->u.freq.e > 6) {
		wpa_printf(MSG_DEBUG, "Invalid freq in scan results (BSSID="
			   MACSTR " m=%d e=%d)",
			   MAC2STR(res->res.bssid), iwe->u.freq.m,
			   iwe->u.freq.e);
		return;
	}

	for (i = 0; i < iwe->u.freq.e; i++)
		divi /= 10;
	res->res.freq = iwe->u.freq.m / divi;
}


static void wext_get_scan_qual(struct wpa_driver_wext_data *drv,
			       struct iw_event *iwe,
			       struct wext_scan_data *res)
{
	res->res.qual = iwe->u.qual.qual;
	res->res.noise = iwe->u.qual.noise;
	res->res.level = iwe->u.qual.level;
	if (iwe->u.qual.updated & IW_QUAL_QUAL_INVALID)
		res->res.flags |= WPA_SCAN_QUAL_INVALID;
	if (iwe->u.qual.updated & IW_QUAL_LEVEL_INVALID)
		res->res.flags |= WPA_SCAN_LEVEL_INVALID;
	if (iwe->u.qual.updated & IW_QUAL_NOISE_INVALID)
		res->res.flags |= WPA_SCAN_NOISE_INVALID;
	if (iwe->u.qual.updated & IW_QUAL_DBM)
		res->res.flags |= WPA_SCAN_LEVEL_DBM;
	if ((iwe->u.qual.updated & IW_QUAL_DBM) ||
	    ((iwe->u.qual.level != 0) &&
	     (iwe->u.qual.level > drv->max_level))) {
		if (iwe->u.qual.level >= 64)
			res->res.level -= 0x100;
		if (iwe->u.qual.noise >= 64)
			res->res.noise -= 0x100;
	}
}


static void wext_get_scan_encode(struct iw_event *iwe,
				 struct wext_scan_data *res)
{
	if (!(iwe->u.data.flags & IW_ENCODE_DISABLED))
		res->res.caps |= IEEE80211_CAP_PRIVACY;
}


static void wext_get_scan_rate(struct iw_event *iwe,
			       struct wext_scan_data *res, char *pos,
			       char *end)
{
	int maxrate;
	char *custom = pos + IW_EV_LCP_LEN;
	struct iw_param p;
	size_t clen;

	clen = iwe->len;
	if (custom + clen > end + IW_EV_LCP_LEN)
		return;
	maxrate = 0;
	while (((ssize_t) clen) >= (ssize_t) sizeof(struct iw_param)) {
		/* Note: may be misaligned, make a local, aligned copy */
		os_memcpy(&p, custom, sizeof(struct iw_param));
		if (p.value > maxrate)
			maxrate = p.value;
		clen -= sizeof(struct iw_param);
		custom += sizeof(struct iw_param);
	}

	/* Convert the maxrate from WE-style (b/s units) to
	 * 802.11 rates (500000 b/s units).
	 */
	res->maxrate = maxrate / 500000;
	res->res.max_rate = res->maxrate;
}


static void wext_get_scan_iwevgenie(struct iw_event *iwe,
				    struct wext_scan_data *res, char *custom,
				    char *end)
{
	char *genie, *gpos, *gend;
	u8 *tmp;

	if (iwe->u.data.length == 0)
		return;

	gpos = genie = custom;
	gend = genie + iwe->u.data.length;
	if (gend > end) {
		wpa_printf(MSG_INFO, "IWEVGENIE overflow");
		return;
	}

	tmp = os_realloc(res->ie, res->ie_len + gend - gpos);
	if (tmp == NULL)
		return;
	os_memcpy(tmp + res->ie_len, gpos, gend - gpos);
	res->ie = tmp;
	res->ie_len += gend - gpos;
}


static void wext_get_scan_custom(struct iw_event *iwe,
				 struct wext_scan_data *res, char *custom,
				 char *end)
{
	size_t clen;
	u8 *tmp;

	clen = iwe->u.data.length;
	if (custom + clen > end)
		return;

	if (clen > 7 && os_strncmp(custom, "wpa_ie=", 7) == 0) {
		char *spos;
		int bytes;
		spos = custom + 7;
		bytes = custom + clen - spos;
		if (bytes & 1 || bytes == 0)
			return;
		bytes /= 2;
		tmp = os_realloc(res->ie, res->ie_len + bytes);
		if (tmp == NULL)
			return;
		res->ie = tmp;
		if (hexstr2bin(spos, tmp + res->ie_len, bytes) < 0)
			return;
		res->ie_len += bytes;
	} else if (clen > 7 && os_strncmp(custom, "rsn_ie=", 7) == 0) {
		char *spos;
		int bytes;
		spos = custom + 7;
		bytes = custom + clen - spos;
		if (bytes & 1 || bytes == 0)
			return;
		bytes /= 2;
		tmp = os_realloc(res->ie, res->ie_len + bytes);
		if (tmp == NULL)
			return;
		res->ie = tmp;
		if (hexstr2bin(spos, tmp + res->ie_len, bytes) < 0)
			return;
		res->ie_len += bytes;
	} else if (clen > 4 && os_strncmp(custom, "tsf=", 4) == 0) {
		char *spos;
		int bytes;
		u8 bin[8];
		spos = custom + 4;
		bytes = custom + clen - spos;
		if (bytes != 16) {
			wpa_printf(MSG_INFO, "Invalid TSF length (%d)", bytes);
			return;
		}
		bytes /= 2;
		if (hexstr2bin(spos, bin, bytes) < 0) {
			wpa_printf(MSG_DEBUG, "WEXT: Invalid TSF value");
			return;
		}
		res->res.tsf += WPA_GET_BE64(bin);
	}
}


static int wext_19_iw_point(struct wpa_driver_wext_data *drv, u16 cmd)
{
	return drv->we_version_compiled > 18 &&
		(cmd == SIOCGIWESSID || cmd == SIOCGIWENCODE ||
		 cmd == IWEVGENIE || cmd == IWEVCUSTOM);
}


static void wpa_driver_wext_add_scan_entry(struct wpa_scan_results *res,
					   struct wext_scan_data *data)
{
	struct wpa_scan_res **tmp;
	struct wpa_scan_res *r;
	size_t extra_len;
	u8 *pos, *end, *ssid_ie = NULL, *rate_ie = NULL;

	/* Figure out whether we need to fake any IEs */
	pos = data->ie;
	end = pos + data->ie_len;
	while (pos && pos + 1 < end) {
		if (pos + 2 + pos[1] > end)
			break;
		if (pos[0] == WLAN_EID_SSID)
			ssid_ie = pos;
		else if (pos[0] == WLAN_EID_SUPP_RATES)
			rate_ie = pos;
		else if (pos[0] == WLAN_EID_EXT_SUPP_RATES)
			rate_ie = pos;
		pos += 2 + pos[1];
	}

	extra_len = 0;
	if (ssid_ie == NULL)
		extra_len += 2 + data->ssid_len;
	if (rate_ie == NULL && data->maxrate)
		//extra_len += 3;
		extra_len += 4;

	r = os_zalloc(sizeof(*r) + extra_len + data->ie_len);
	if (r == NULL)
		return;
	os_memcpy(r, &data->res, sizeof(*r));
	r->ie_len = extra_len + data->ie_len;
	pos = (u8 *) (r + 1);
	if (ssid_ie == NULL) {
		/*
		 * Generate a fake SSID IE since the driver did not report
		 * a full IE list.
		 */
		*pos++ = WLAN_EID_SSID;
		*pos++ = data->ssid_len;
		os_memcpy(pos, data->ssid, data->ssid_len);
		pos += data->ssid_len;
	}
	if (rate_ie == NULL && data->maxrate) {
		/*
		 * Generate a fake Supported Rates IE since the driver did not
		 * report a full IE list.
		 */
		*pos++ = WLAN_EID_SUPP_RATES;
#if 0
		*pos++ = 1;
		*pos++ = data->maxrate;
#else
		*pos++ = 2;
		*pos++ = 0xFF & data->maxrate;
		*pos++ = (0xFF00 & data->maxrate) >> 8;
#endif
	}
	if (data->ie)
		os_memcpy(pos, data->ie, data->ie_len);

	tmp = os_realloc(res->res,
			 (res->num + 1) * sizeof(struct wpa_scan_res *));
	if (tmp == NULL) {
		os_free(r);
		return;
	}
	tmp[res->num++] = r;
	res->res = tmp;
}
				      

/**
 * wpa_driver_wext_get_scan_results - Fetch the latest scan results
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * Returns: Scan results on success, -1 on failure
 */
struct wpa_scan_results * wpa_driver_wext_get_scan_results(void *priv)
{
	struct wpa_driver_wext_data *drv = priv;
	size_t ap_num = 0, len;
	int first;
	u8 *res_buf;
	struct iw_event iwe_buf, *iwe = &iwe_buf;
	char *pos, *end, *custom;
	struct wpa_scan_results *res;
	struct wext_scan_data data;

	res_buf = wpa_driver_wext_giwscan(drv, &len);
	if (res_buf == NULL)
		return NULL;

	ap_num = 0;
	first = 1;

	res = os_zalloc(sizeof(*res));
	if (res == NULL) {
		os_free(res_buf);
		return NULL;
	}

	pos = (char *) res_buf;
	end = (char *) res_buf + len;
	os_memset(&data, 0, sizeof(data));

	while (pos + IW_EV_LCP_LEN <= end) {
		/* Event data may be unaligned, so make a local, aligned copy
		 * before processing. */
		os_memcpy(&iwe_buf, pos, IW_EV_LCP_LEN);
		if (iwe->len <= IW_EV_LCP_LEN)
			break;

		custom = pos + IW_EV_POINT_LEN;
		if (wext_19_iw_point(drv, iwe->cmd)) {
			/* WE-19 removed the pointer from struct iw_point */
			char *dpos = (char *) &iwe_buf.u.data.length;
			int dlen = dpos - (char *) &iwe_buf;
			os_memcpy(dpos, pos + IW_EV_LCP_LEN,
				  sizeof(struct iw_event) - dlen);
		} else {
			os_memcpy(&iwe_buf, pos, sizeof(struct iw_event));
			custom += IW_EV_POINT_OFF;
		}

		switch (iwe->cmd) {
		case SIOCGIWAP:
			if (!first)
				wpa_driver_wext_add_scan_entry(res, &data);
			first = 0;
			os_free(data.ie);
			os_memset(&data, 0, sizeof(data));
			os_memcpy(data.res.bssid,
				  iwe->u.ap_addr.sa_data, ETH_ALEN);
			break;
		case SIOCGIWNAME:
			os_memcpy(data.res.name,
				  iwe->u.name, IFNAMSIZ);
			//printf("%s\n", iwe->u.name);
			break;
		case SIOCGIWMODE:
			wext_get_scan_mode(iwe, &data);
			break;
		case SIOCGIWESSID:
			wext_get_scan_ssid(iwe, &data, custom, end);
			break;
		case SIOCGIWFREQ:
			wext_get_scan_freq(iwe, &data);
			break;
		case IWEVQUAL:
			wext_get_scan_qual(drv, iwe, &data);
			break;
		case SIOCGIWENCODE:
			wext_get_scan_encode(iwe, &data);
			break;
		case SIOCGIWRATE:
			wext_get_scan_rate(iwe, &data, pos, end);
			break;
		case IWEVGENIE:
			wext_get_scan_iwevgenie(iwe, &data, custom, end);
			break;
		case IWEVCUSTOM:
			wext_get_scan_custom(iwe, &data, custom, end);
			break;
		}

		pos += iwe->len;
	}
	os_free(res_buf);
	res_buf = NULL;
	if (!first)
		wpa_driver_wext_add_scan_entry(res, &data);
	os_free(data.ie);

	wpa_printf(MSG_DEBUG, "Received %lu bytes of scan results (%lu BSSes)",
		   (unsigned long) len, (unsigned long) res->num);

	return res;
}


static int wpa_driver_wext_get_range(void *priv)
{
	struct wpa_driver_wext_data *drv = priv;
	struct iw_range *range;
	struct iwreq iwr;
	int minlen;
	size_t buflen;

	/*
	 * Use larger buffer than struct iw_range in order to allow the
	 * structure to grow in the future.
	 */
	buflen = sizeof(struct iw_range) + 500;
	range = os_zalloc(buflen);
	if (range == NULL)
		return -1;

	os_memset(&iwr, 0, sizeof(iwr));
	os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	iwr.u.data.pointer = (caddr_t) range;
	iwr.u.data.length = buflen;

	minlen = ((char *) &range->enc_capa) - (char *) range +
		sizeof(range->enc_capa);

	if (ioctl(drv->ioctl_sock, SIOCGIWRANGE, &iwr) < 0) {
		perror("ioctl[SIOCGIWRANGE]");
		os_free(range);
		return -1;
	} else if (iwr.u.data.length >= minlen &&
		   range->we_version_compiled >= 18) {
		wpa_printf(MSG_DEBUG, "SIOCGIWRANGE: WE(compiled)=%d "
			   "WE(source)=%d enc_capa=0x%x",
			   range->we_version_compiled,
			   range->we_version_source,
			   range->enc_capa);
		drv->has_capability = 1;
		drv->we_version_compiled = range->we_version_compiled;
		if (range->enc_capa & IW_ENC_CAPA_WPA) {
			drv->capa.key_mgmt |= WPA_DRIVER_CAPA_KEY_MGMT_WPA |
				WPA_DRIVER_CAPA_KEY_MGMT_WPA_PSK;
		}
		if (range->enc_capa & IW_ENC_CAPA_WPA2) {
			drv->capa.key_mgmt |= WPA_DRIVER_CAPA_KEY_MGMT_WPA2 |
				WPA_DRIVER_CAPA_KEY_MGMT_WPA2_PSK;
		}
		drv->capa.enc |= WPA_DRIVER_CAPA_ENC_WEP40 |
			WPA_DRIVER_CAPA_ENC_WEP104;
		if (range->enc_capa & IW_ENC_CAPA_CIPHER_TKIP)
			drv->capa.enc |= WPA_DRIVER_CAPA_ENC_TKIP;
		if (range->enc_capa & IW_ENC_CAPA_CIPHER_CCMP)
			drv->capa.enc |= WPA_DRIVER_CAPA_ENC_CCMP;
		if (range->enc_capa & IW_ENC_CAPA_4WAY_HANDSHAKE)
			drv->capa.flags |= WPA_DRIVER_FLAGS_4WAY_HANDSHAKE;
		drv->capa.auth = WPA_DRIVER_AUTH_OPEN |
			WPA_DRIVER_AUTH_SHARED |
			WPA_DRIVER_AUTH_LEAP;
		drv->capa.max_scan_ssids = 1;

		wpa_printf(MSG_DEBUG, "  capabilities: key_mgmt 0x%x enc 0x%x "
			   "flags 0x%x",
			   drv->capa.key_mgmt, drv->capa.enc, drv->capa.flags);
	} else {
		wpa_printf(MSG_DEBUG, "SIOCGIWRANGE: too old (short) data - "
			   "assuming WPA is not supported");
	}

	drv->max_level = range->max_qual.level;

	os_free(range);
	return 0;
}


static int wpa_driver_wext_set_psk(struct wpa_driver_wext_data *drv,
				   const u8 *psk)
{
	struct iw_encode_ext *ext;
	struct iwreq iwr;
	int ret;

	wpa_printf(MSG_DEBUG, "%s", __FUNCTION__);

	if (!(drv->capa.flags & WPA_DRIVER_FLAGS_4WAY_HANDSHAKE))
		return 0;

	if (!psk)
		return 0;

	os_memset(&iwr, 0, sizeof(iwr));
	os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);

	ext = os_zalloc(sizeof(*ext) + PMK_LEN);
	if (ext == NULL)
		return -1;

	iwr.u.encoding.pointer = (caddr_t) ext;
	iwr.u.encoding.length = sizeof(*ext) + PMK_LEN;
	ext->key_len = PMK_LEN;
	os_memcpy(&ext->key, psk, ext->key_len);
	ext->alg = IW_ENCODE_ALG_PMK;

	ret = ioctl(drv->ioctl_sock, SIOCSIWENCODEEXT, &iwr);
	if (ret < 0)
		perror("ioctl[SIOCSIWENCODEEXT] PMK");
	os_free(ext);

	return ret;
}


static int wpa_driver_wext_set_key_ext(void *priv, enum wpa_alg alg,
				       const u8 *addr, int key_idx,
				       int set_tx, const u8 *seq,
				       size_t seq_len,
				       const u8 *key, size_t key_len)
{
	struct wpa_driver_wext_data *drv = priv;
	struct iwreq iwr;
	int ret = 0;
	struct iw_encode_ext *ext;

	if (seq_len > IW_ENCODE_SEQ_MAX_SIZE) {
		wpa_printf(MSG_DEBUG, "%s: Invalid seq_len %lu",
			   __FUNCTION__, (unsigned long) seq_len);
		return -1;
	}

	ext = os_zalloc(sizeof(*ext) + key_len);
	if (ext == NULL)
		return -1;
	os_memset(&iwr, 0, sizeof(iwr));
	os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	iwr.u.encoding.flags = key_idx + 1;
	iwr.u.encoding.flags |= IW_ENCODE_TEMP;
	if (alg == WPA_ALG_NONE)
		iwr.u.encoding.flags |= IW_ENCODE_DISABLED;
	iwr.u.encoding.pointer = (caddr_t) ext;
	iwr.u.encoding.length = sizeof(*ext) + key_len;

	if (addr == NULL || is_broadcast_ether_addr(addr))
		ext->ext_flags |= IW_ENCODE_EXT_GROUP_KEY;
	if (set_tx)
		ext->ext_flags |= IW_ENCODE_EXT_SET_TX_KEY;

	ext->addr.sa_family = ARPHRD_ETHER;
	if (addr)
		os_memcpy(ext->addr.sa_data, addr, ETH_ALEN);
	else
		os_memset(ext->addr.sa_data, 0xff, ETH_ALEN);
	if (key && key_len) {
		os_memcpy(ext + 1, key, key_len);
		ext->key_len = key_len;
	}
	switch (alg) {
	case WPA_ALG_NONE:
		ext->alg = IW_ENCODE_ALG_NONE;
		break;
	case WPA_ALG_WEP:
		ext->alg = IW_ENCODE_ALG_WEP;
		break;
	case WPA_ALG_TKIP:
		ext->alg = IW_ENCODE_ALG_TKIP;
		break;
	case WPA_ALG_CCMP:
		ext->alg = IW_ENCODE_ALG_CCMP;
		break;
	case WPA_ALG_PMK:
		ext->alg = IW_ENCODE_ALG_PMK;
		break;
#ifdef CONFIG_SET_REKEY_INFO		
	case WPA_ALG_KCK:
		ext->alg = IW_ENCODE_ALG_KCK;
		break;
	case WPA_ALG_KEK:
		ext->alg = IW_ENCODE_ALG_KEK;
		break;
#endif		
#ifdef CONFIG_IEEE80211W
	case WPA_ALG_IGTK:
		ext->alg = IW_ENCODE_ALG_AES_CMAC;
		break;
#endif /* CONFIG_IEEE80211W */
	default:
		wpa_printf(MSG_DEBUG, "%s: Unknown algorithm %d",
			   __FUNCTION__, alg);
		os_free(ext);
		return -1;
	}

	if (seq && seq_len) {
		ext->ext_flags |= IW_ENCODE_EXT_RX_SEQ_VALID;
		os_memcpy(ext->rx_seq, seq, seq_len);
	}

	if (ioctl(drv->ioctl_sock, SIOCSIWENCODEEXT, &iwr) < 0) {
		ret = errno == EOPNOTSUPP ? -2 : -1;
		if (errno == ENODEV) {
			/*
			 * ndiswrapper seems to be returning incorrect error
			 * code.. */
			ret = -2;
		}

		perror("ioctl[SIOCSIWENCODEEXT]");
	}

	os_free(ext);
	return ret;
}


/**
 * wpa_driver_wext_set_key - Configure encryption key
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * @priv: Private driver interface data
 * @alg: Encryption algorithm (%WPA_ALG_NONE, %WPA_ALG_WEP,
 *	%WPA_ALG_TKIP, %WPA_ALG_CCMP); %WPA_ALG_NONE clears the key.
 * @addr: Address of the peer STA or ff:ff:ff:ff:ff:ff for
 *	broadcast/default keys
 * @key_idx: key index (0..3), usually 0 for unicast keys
 * @set_tx: Configure this key as the default Tx key (only used when
 *	driver does not support separate unicast/individual key
 * @seq: Sequence number/packet number, seq_len octets, the next
 *	packet number to be used for in replay protection; configured
 *	for Rx keys (in most cases, this is only used with broadcast
 *	keys and set to zero for unicast keys)
 * @seq_len: Length of the seq, depends on the algorithm:
 *	TKIP: 6 octets, CCMP: 6 octets
 * @key: Key buffer; TKIP: 16-byte temporal key, 8-byte Tx Mic key,
 *	8-byte Rx Mic Key
 * @key_len: Length of the key buffer in octets (WEP: 5 or 13,
 *	TKIP: 32, CCMP: 16)
 * Returns: 0 on success, -1 on failure
 *
 * This function uses SIOCSIWENCODEEXT by default, but tries to use
 * SIOCSIWENCODE if the extended ioctl fails when configuring a WEP key.
 */
int wpa_driver_wext_set_key(const char *ifname, void *priv, enum wpa_alg alg,
			    const u8 *addr, int key_idx,
			    int set_tx, const u8 *seq, size_t seq_len,
			    const u8 *key, size_t key_len)
{
	struct wpa_driver_wext_data *drv = priv;
	struct iwreq iwr;
	int ret = 0;

	wpa_printf(MSG_DEBUG, "%s: alg=%d key_idx=%d set_tx=%d seq_len=%lu "
		   "key_len=%lu",
		   __FUNCTION__, alg, key_idx, set_tx,
		   (unsigned long) seq_len, (unsigned long) key_len);

	ret = wpa_driver_wext_set_key_ext(drv, alg, addr, key_idx, set_tx,
					  seq, seq_len, key, key_len);
	if (ret == 0)
		return 0;

	if (ret == -2 &&
	    (alg == WPA_ALG_NONE || alg == WPA_ALG_WEP)) {
		wpa_printf(MSG_DEBUG, "Driver did not support "
			   "SIOCSIWENCODEEXT, trying SIOCSIWENCODE");
		ret = 0;
	} else {
		wpa_printf(MSG_DEBUG, "Driver did not support "
			   "SIOCSIWENCODEEXT");
		return ret;
	}

	os_memset(&iwr, 0, sizeof(iwr));
	os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	iwr.u.encoding.flags = key_idx + 1;
	iwr.u.encoding.flags |= IW_ENCODE_TEMP;
	if (alg == WPA_ALG_NONE)
		iwr.u.encoding.flags |= IW_ENCODE_DISABLED;
	iwr.u.encoding.pointer = (caddr_t) key;
	iwr.u.encoding.length = key_len;

	if (ioctl(drv->ioctl_sock, SIOCSIWENCODE, &iwr) < 0) {
		perror("ioctl[SIOCSIWENCODE]");
		ret = -1;
	}

	if (set_tx && alg != WPA_ALG_NONE) {
		os_memset(&iwr, 0, sizeof(iwr));
		os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
		iwr.u.encoding.flags = key_idx + 1;
		iwr.u.encoding.flags |= IW_ENCODE_TEMP;
		iwr.u.encoding.pointer = (caddr_t) NULL;
		iwr.u.encoding.length = 0;
		if (ioctl(drv->ioctl_sock, SIOCSIWENCODE, &iwr) < 0) {
			perror("ioctl[SIOCSIWENCODE] (set_tx)");
			ret = -1;
		}
	}

	return ret;
}


static int wpa_driver_wext_set_countermeasures(void *priv,
					       int enabled)
{
	struct wpa_driver_wext_data *drv = priv;
	wpa_printf(MSG_DEBUG, "%s", __FUNCTION__);
	return wpa_driver_wext_set_auth_param(drv,
					      IW_AUTH_TKIP_COUNTERMEASURES,
					      enabled);
}


static int wpa_driver_wext_set_drop_unencrypted(void *priv,
						int enabled)
{
	struct wpa_driver_wext_data *drv = priv;
	wpa_printf(MSG_DEBUG, "%s", __FUNCTION__);
	drv->use_crypt = enabled;
	return wpa_driver_wext_set_auth_param(drv, IW_AUTH_DROP_UNENCRYPTED,
					      enabled);
}


static int wpa_driver_wext_mlme(struct wpa_driver_wext_data *drv,
				const u8 *addr, int cmd, int reason_code)
{
	struct iwreq iwr;
	struct iw_mlme mlme;
	int ret = 0;

	os_memset(&iwr, 0, sizeof(iwr));
	os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	os_memset(&mlme, 0, sizeof(mlme));
	mlme.cmd = cmd;
	mlme.reason_code = reason_code;
	mlme.addr.sa_family = ARPHRD_ETHER;
	os_memcpy(mlme.addr.sa_data, addr, ETH_ALEN);
	iwr.u.data.pointer = (caddr_t) &mlme;
	iwr.u.data.length = sizeof(mlme);

	if (ioctl(drv->ioctl_sock, SIOCSIWMLME, &iwr) < 0) {
		perror("ioctl[SIOCSIWMLME]");
		ret = -1;
	}

	return ret;
}


static void wpa_driver_wext_disconnect(struct wpa_driver_wext_data *drv)
{
	struct iwreq iwr;
	const u8 null_bssid[ETH_ALEN] = { 0, 0, 0, 0, 0, 0 };
	u8 ssid[32];
	int i;

	/*
	 * Only force-disconnect when the card is in infrastructure mode,
	 * otherwise the driver might interpret the cleared BSSID and random
	 * SSID as an attempt to create a new ad-hoc network.
	 */
	os_memset(&iwr, 0, sizeof(iwr));
	os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	if (ioctl(drv->ioctl_sock, SIOCGIWMODE, &iwr) < 0) {
		perror("ioctl[SIOCGIWMODE]");
		iwr.u.mode = IW_MODE_INFRA;
	}

	if (iwr.u.mode == IW_MODE_INFRA) {
		if (drv->cfg80211) {
			/*
			 * cfg80211 supports SIOCSIWMLME commands, so there is
			 * no need for the random SSID hack, but clear the
			 * BSSID and SSID.
			 */
			if (wpa_driver_wext_set_bssid(drv, null_bssid) < 0 ||
#ifdef ANDROID
			    0) {
#else
			    wpa_driver_wext_set_ssid(drv, (u8 *) "", 0) < 0) {
#endif
				wpa_printf(MSG_DEBUG, "WEXT: Failed to clear "
					   "to disconnect");
			}
			return;
		}
		/*
		 * Clear the BSSID selection and set a random SSID to make sure
		 * the driver will not be trying to associate with something
		 * even if it does not understand SIOCSIWMLME commands (or
		 * tries to associate automatically after deauth/disassoc).
		 */
		for (i = 0; i < 32; i++)
			ssid[i] = rand() & 0xFF;
		if (wpa_driver_wext_set_bssid(drv, null_bssid) < 0 ||
#ifdef ANDROID
		    0) {
#else
		    wpa_driver_wext_set_ssid(drv, ssid, 32) < 0) {
#endif
			wpa_printf(MSG_DEBUG, "WEXT: Failed to set bogus "
				   "BSSID/SSID to disconnect");
		}
	}
}


static int wpa_driver_wext_deauthenticate(void *priv, const u8 *addr,
					  int reason_code)
{
	struct wpa_driver_wext_data *drv = priv;
	int ret;
	wpa_printf(MSG_DEBUG, "%s", __FUNCTION__);
	ret = wpa_driver_wext_mlme(drv, addr, IW_MLME_DEAUTH, reason_code);
	wpa_driver_wext_disconnect(drv);
	return ret;
}


static int wpa_driver_wext_disassociate(void *priv, const u8 *addr,
					int reason_code)
{
	struct wpa_driver_wext_data *drv = priv;
	int ret;
	wpa_printf(MSG_DEBUG, "%s", __FUNCTION__);
	ret = wpa_driver_wext_mlme(drv, addr, IW_MLME_DISASSOC, reason_code);
	wpa_driver_wext_disconnect(drv);
	return ret;
}


static int wpa_driver_wext_set_gen_ie(void *priv, const u8 *ie,
				      size_t ie_len)
{
	struct wpa_driver_wext_data *drv = priv;
	struct iwreq iwr;
	int ret = 0;

	os_memset(&iwr, 0, sizeof(iwr));
	os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	iwr.u.data.pointer = (caddr_t) ie;
	iwr.u.data.length = ie_len;

	if (ioctl(drv->ioctl_sock, SIOCSIWGENIE, &iwr) < 0) {
		perror("ioctl[SIOCSIWGENIE]");
		ret = -1;
	}

	return ret;
}


int wpa_driver_wext_cipher2wext(int cipher)
{
	switch (cipher) {
	case CIPHER_NONE:
		return IW_AUTH_CIPHER_NONE;
	case CIPHER_WEP40:
		return IW_AUTH_CIPHER_WEP40;
	case CIPHER_TKIP:
		return IW_AUTH_CIPHER_TKIP;
	case CIPHER_CCMP:
		return IW_AUTH_CIPHER_CCMP;
	case CIPHER_WEP104:
		return IW_AUTH_CIPHER_WEP104;
	default:
		return 0;
	}
}


int wpa_driver_wext_keymgmt2wext(int keymgmt)
{
	switch (keymgmt) {
	case KEY_MGMT_802_1X:
	case KEY_MGMT_802_1X_NO_WPA:
		return IW_AUTH_KEY_MGMT_802_1X;
	case KEY_MGMT_PSK:
		return IW_AUTH_KEY_MGMT_PSK;
#ifdef CONFIG_WPS //snowpin
	case KEY_MGMT_WPS:
		return IW_AUTH_KEY_MGMT_WPS;
#endif /* CONFIG_WPS */
	default:
		return 0;
	}
}


static int
wpa_driver_wext_auth_alg_fallback(struct wpa_driver_wext_data *drv,
				  struct wpa_driver_associate_params *params)
{
	struct iwreq iwr;
	int ret = 0;

	wpa_printf(MSG_DEBUG, "WEXT: Driver did not support "
		   "SIOCSIWAUTH for AUTH_ALG, trying SIOCSIWENCODE");

	os_memset(&iwr, 0, sizeof(iwr));
	os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	/* Just changing mode, not actual keys */
	iwr.u.encoding.flags = 0;
	iwr.u.encoding.pointer = (caddr_t) NULL;
	iwr.u.encoding.length = 0;

	/*
	 * Note: IW_ENCODE_{OPEN,RESTRICTED} can be interpreted to mean two
	 * different things. Here they are used to indicate Open System vs.
	 * Shared Key authentication algorithm. However, some drivers may use
	 * them to select between open/restricted WEP encrypted (open = allow
	 * both unencrypted and encrypted frames; restricted = only allow
	 * encrypted frames).
	 */

	if (!drv->use_crypt) {
		iwr.u.encoding.flags |= IW_ENCODE_DISABLED;
	} else {
		if (params->auth_alg & WPA_AUTH_ALG_OPEN)
			iwr.u.encoding.flags |= IW_ENCODE_OPEN;
		if (params->auth_alg & WPA_AUTH_ALG_SHARED)
			iwr.u.encoding.flags |= IW_ENCODE_RESTRICTED;
	}

	if (ioctl(drv->ioctl_sock, SIOCSIWENCODE, &iwr) < 0) {
		perror("ioctl[SIOCSIWENCODE]");
		ret = -1;
	}

	return ret;
}


int wpa_driver_wext_associate(void *priv,
			      struct wpa_driver_associate_params *params)
{
	struct wpa_driver_wext_data *drv = priv;
	int ret = 0;
	int allow_unencrypted_eapol;
	int value, flags;

	wpa_printf(MSG_DEBUG, "%s", __FUNCTION__);
	
#ifdef ANDROID
	drv->skip_disconnect = 0;
#endif

	if (drv->cfg80211) {
		/*
		 * Stop cfg80211 from trying to associate before we are done
		 * with all parameters.
		 */
		wpa_driver_wext_set_ssid(drv, (u8 *) "", 0);
	}

	if (wpa_driver_wext_set_drop_unencrypted(drv, params->drop_unencrypted)
	    < 0)
		ret = -1;
	if (wpa_driver_wext_set_auth_alg(drv, params->auth_alg) < 0)
		ret = -1;
	if (wpa_driver_wext_set_mode(drv, params->mode) < 0)
		ret = -1;

	/*
	 * If the driver did not support SIOCSIWAUTH, fallback to
	 * SIOCSIWENCODE here.
	 */
	if (drv->auth_alg_fallback &&
	    wpa_driver_wext_auth_alg_fallback(drv, params) < 0)
		ret = -1;

	if (!params->bssid &&
	    wpa_driver_wext_set_bssid(drv, NULL) < 0)
		ret = -1;

	/* TODO: should consider getting wpa version and cipher/key_mgmt suites
	 * from configuration, not from here, where only the selected suite is
	 * available */
	if (wpa_driver_wext_set_gen_ie(drv, params->wpa_ie, params->wpa_ie_len)
	    < 0)
		ret = -1;
	if (params->wpa_ie == NULL || params->wpa_ie_len == 0)
		value = IW_AUTH_WPA_VERSION_DISABLED;
	else if (params->wpa_ie[0] == WLAN_EID_RSN)
		value = IW_AUTH_WPA_VERSION_WPA2;
	else
		value = IW_AUTH_WPA_VERSION_WPA;
	if (wpa_driver_wext_set_auth_param(drv,
					   IW_AUTH_WPA_VERSION, value) < 0)
		ret = -1;
	value = wpa_driver_wext_cipher2wext(params->pairwise_suite);
	if (wpa_driver_wext_set_auth_param(drv,
					   IW_AUTH_CIPHER_PAIRWISE, value) < 0)
		ret = -1;
	value = wpa_driver_wext_cipher2wext(params->group_suite);
	if (wpa_driver_wext_set_auth_param(drv,
					   IW_AUTH_CIPHER_GROUP, value) < 0)
		ret = -1;
	value = wpa_driver_wext_keymgmt2wext(params->key_mgmt_suite);
	if (wpa_driver_wext_set_auth_param(drv,
					   IW_AUTH_KEY_MGMT, value) < 0)
		ret = -1;
	value = params->key_mgmt_suite != KEY_MGMT_NONE ||
		params->pairwise_suite != CIPHER_NONE ||
		params->group_suite != CIPHER_NONE ||
		params->wpa_ie_len;
	if (wpa_driver_wext_set_auth_param(drv,
					   IW_AUTH_PRIVACY_INVOKED, value) < 0)
		ret = -1;

	/* Allow unencrypted EAPOL messages even if pairwise keys are set when
	 * not using WPA. IEEE 802.1X specifies that these frames are not
	 * encrypted, but WPA encrypts them when pairwise keys are in use. */
	if (params->key_mgmt_suite == KEY_MGMT_802_1X ||
	    params->key_mgmt_suite == KEY_MGMT_PSK)
		allow_unencrypted_eapol = 0;
	else
		allow_unencrypted_eapol = 1;

	if (wpa_driver_wext_set_psk(drv, params->psk) < 0)
		ret = -1;
	if (wpa_driver_wext_set_auth_param(drv,
					   IW_AUTH_RX_UNENCRYPTED_EAPOL,
					   allow_unencrypted_eapol) < 0)
		ret = -1;
#ifdef CONFIG_IEEE80211W
	switch (params->mgmt_frame_protection) {
	case NO_MGMT_FRAME_PROTECTION:
		value = IW_AUTH_MFP_DISABLED;
		break;
	case MGMT_FRAME_PROTECTION_OPTIONAL:
		value = IW_AUTH_MFP_OPTIONAL;
		break;
	case MGMT_FRAME_PROTECTION_REQUIRED:
		value = IW_AUTH_MFP_REQUIRED;
		break;
	};
	if (wpa_driver_wext_set_auth_param(drv, IW_AUTH_MFP, value) < 0)
		ret = -1;
#endif /* CONFIG_IEEE80211W */
	if (params->freq && wpa_driver_wext_set_freq(drv, params->freq) < 0)
		ret = -1;
	if (!drv->cfg80211 &&
	    wpa_driver_wext_set_ssid(drv, params->ssid, params->ssid_len) < 0)
		ret = -1;
	if (params->bssid &&
	    wpa_driver_wext_set_bssid(drv, params->bssid) < 0)
		ret = -1;
	if (drv->cfg80211 &&
	    wpa_driver_wext_set_ssid(drv, params->ssid, params->ssid_len) < 0)
		ret = -1;

	return ret;
}


static int wpa_driver_wext_set_auth_alg(void *priv, int auth_alg)
{
	struct wpa_driver_wext_data *drv = priv;
	int algs = 0, res;

	if (auth_alg & WPA_AUTH_ALG_OPEN)
		algs |= IW_AUTH_ALG_OPEN_SYSTEM;
	if (auth_alg & WPA_AUTH_ALG_SHARED)
		algs |= IW_AUTH_ALG_SHARED_KEY;
	if (auth_alg & WPA_AUTH_ALG_LEAP)
		algs |= IW_AUTH_ALG_LEAP;
	if (algs == 0) {
		/* at least one algorithm should be set */
		algs = IW_AUTH_ALG_OPEN_SYSTEM;
	}

	res = wpa_driver_wext_set_auth_param(drv, IW_AUTH_80211_AUTH_ALG,
					     algs);
	drv->auth_alg_fallback = res == -2;
	return res;
}


/**
 * wpa_driver_wext_set_mode - Set wireless mode (infra/adhoc), SIOCSIWMODE
 * @priv: Pointer to private wext data from wpa_driver_wext_init()
 * @mode: 0 = infra/BSS (associate with an AP), 1 = adhoc/IBSS
 * Returns: 0 on success, -1 on failure
 */
int wpa_driver_wext_set_mode(void *priv, int mode)
{
	struct wpa_driver_wext_data *drv = priv;
	struct iwreq iwr;
	int ret = -1;
	unsigned int new_mode = mode ? IW_MODE_ADHOC : IW_MODE_INFRA;

	os_memset(&iwr, 0, sizeof(iwr));
	os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	iwr.u.mode = new_mode;
	if (ioctl(drv->ioctl_sock, SIOCSIWMODE, &iwr) == 0) {
		ret = 0;
		goto done;
	}

	if (errno != EBUSY) {
		perror("ioctl[SIOCSIWMODE]");
		goto done;
	}

	/* mac80211 doesn't allow mode changes while the device is up, so if
	 * the device isn't in the mode we're about to change to, take device
	 * down, try to set the mode again, and bring it back up.
	 */
	if (ioctl(drv->ioctl_sock, SIOCGIWMODE, &iwr) < 0) {
		perror("ioctl[SIOCGIWMODE]");
		goto done;
	}

	if (iwr.u.mode == new_mode) {
		ret = 0;
		goto done;
	}

	if (linux_set_iface_flags(drv->ioctl_sock, drv->ifname, 0) == 0) {
		/* Try to set the mode again while the interface is down */
		iwr.u.mode = new_mode;
		if (ioctl(drv->ioctl_sock, SIOCSIWMODE, &iwr) < 0)
			perror("ioctl[SIOCSIWMODE]");
		else
			ret = 0;

		(void) linux_set_iface_flags(drv->ioctl_sock, drv->ifname, 1);
	}

done:
	return ret;
}


static int wpa_driver_wext_pmksa(struct wpa_driver_wext_data *drv,
				 u32 cmd, const u8 *bssid, const u8 *pmkid)
{
	struct iwreq iwr;
	struct iw_pmksa pmksa;
	int ret = 0;

	os_memset(&iwr, 0, sizeof(iwr));
	os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	os_memset(&pmksa, 0, sizeof(pmksa));
	pmksa.cmd = cmd;
	pmksa.bssid.sa_family = ARPHRD_ETHER;
	if (bssid)
		os_memcpy(pmksa.bssid.sa_data, bssid, ETH_ALEN);
	if (pmkid)
		os_memcpy(pmksa.pmkid, pmkid, IW_PMKID_LEN);
	iwr.u.data.pointer = (caddr_t) &pmksa;
	iwr.u.data.length = sizeof(pmksa);

	if (ioctl(drv->ioctl_sock, SIOCSIWPMKSA, &iwr) < 0) {
		if (errno != EOPNOTSUPP)
			perror("ioctl[SIOCSIWPMKSA]");
		ret = -1;
	}

	return ret;
}


static int wpa_driver_wext_add_pmkid(void *priv, const u8 *bssid,
				     const u8 *pmkid)
{
	struct wpa_driver_wext_data *drv = priv;
	return wpa_driver_wext_pmksa(drv, IW_PMKSA_ADD, bssid, pmkid);
}


static int wpa_driver_wext_remove_pmkid(void *priv, const u8 *bssid,
		 			const u8 *pmkid)
{
	struct wpa_driver_wext_data *drv = priv;
	return wpa_driver_wext_pmksa(drv, IW_PMKSA_REMOVE, bssid, pmkid);
}


static int wpa_driver_wext_flush_pmkid(void *priv)
{
	struct wpa_driver_wext_data *drv = priv;
	return wpa_driver_wext_pmksa(drv, IW_PMKSA_FLUSH, NULL, NULL);
}


int wpa_driver_wext_get_capa(void *priv, struct wpa_driver_capa *capa)
{
	struct wpa_driver_wext_data *drv = priv;
	if (!drv->has_capability)
		return -1;
	drv->capa.flags |= (WPA_DRIVER_FLAGS_P2P_MGMT | WPA_DRIVER_FLAGS_P2P_CAPABLE);
	drv->capa.flags |= WPA_DRIVER_FLAGS_P2P_CONCURRENT;//Carter add for Force GO support.
	drv->capa.flags |= WPA_DRIVER_FLAGS_P2P_MGMT_AND_NON_P2P;//Carter add for Force GO support.
	drv->capa.flags |= WPA_DRIVER_FLAGS_AP;//Carter add for Force GO support.
	os_memcpy(capa, &drv->capa, sizeof(*capa));
	return 0;
}


int wpa_driver_wext_alternative_ifindex(struct wpa_driver_wext_data *drv,
					const char *ifname)
{
	if (ifname == NULL) {
		drv->ifindex2 = -1;
		return 0;
	}

	drv->ifindex2 = if_nametoindex(ifname);
	if (drv->ifindex2 <= 0)
		return -1;

	wpa_printf(MSG_DEBUG, "Added alternative ifindex %d (%s) for "
		   "wireless events", drv->ifindex2, ifname);

	return 0;
}


int wpa_driver_wext_set_operstate(void *priv, int state)
{
	struct wpa_driver_wext_data *drv = priv;

	wpa_printf(MSG_DEBUG, "%s: operstate %d->%d (%s)",
		   __func__, drv->operstate, state, state ? "UP" : "DORMANT");
	drv->operstate = state;
	return netlink_send_oper_ifla(drv->netlink, drv->ifindex, -1,
				      state ? IF_OPER_UP : IF_OPER_DORMANT);
}


int wpa_driver_wext_get_version(struct wpa_driver_wext_data *drv)
{
	return drv->we_version_compiled;
}


static const char * wext_get_radio_name(void *priv)
{
	struct wpa_driver_wext_data *drv = priv;
	return drv->phyname;
}


#define	RT_OID_VERSION_INFO							0x0608
#define RT_OID_802_11_QUERY_LINK_STATUS				0x060c 
#define	RT_OID_802_11_STATISTICS					0x060e
#define RT_OID_WPA_SUPPLICANT_SUPPORT               		0x0621
#define RT_OID_802_11_QUERY_ASSOCIATED_AP_INFO		0x0626

static int ralink_get_driver_version(void *priv, char *ver)
{
	struct wpa_driver_wext_data *drv = priv;
	struct iwreq iwr;

	os_memset(&iwr, 0, sizeof(iwr));
	os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	iwr.u.data.flags = RT_OID_VERSION_INFO;
	iwr.u.data.pointer = ver;

	if (ioctl(drv->ioctl_sock, RT_PRIV_IOCTL, &iwr) < 0) {
		wpa_printf(MSG_DEBUG, "%s: failed", __func__);
		return -1;
	}

	os_memcpy(ver, iwr.u.data.pointer, iwr.u.data.length);
	printf("%s\n", ver);

	return 0;
}


static int ralink_set_wpa_supplicant_support(void *priv, u8 enable)
{
	struct wpa_driver_wext_data *drv = priv;
	u8 wpa_supplicant_support = enable;
	struct iwreq iwr;
	int err;

	printf("%s:: 0x%x\n", __FUNCTION__, enable);
	
	//memset (&iwr, 0, sizeof(iwr));
	//strncpy(iwr.ifr_name, WIFI_STAUT_IF, 3);
	os_memset(&iwr, 0, sizeof(iwr));
	os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);

	iwr.u.data.flags = OID_GET_SET_TOGGLE | RT_OID_WPA_SUPPLICANT_SUPPORT;
	iwr.u.data.pointer = &wpa_supplicant_support;
	iwr.u.data.length = sizeof(u8);

	if ( (err = ioctl(drv->ioctl_sock, RT_PRIV_IOCTL, &iwr)) < 0)
	{
		printf("ERR(%d)::  %s in 0x%04x.\n", err, __FUNCTION__, RT_OID_WPA_SUPPLICANT_SUPPORT);
		return -1;
	}

	return 0;
	
}


static int ralink_get_driver_status(void *priv, u8 *quailty, u8 *level, u8 *noise)
{
	struct wpa_driver_wext_data *drv = priv;
	struct iwreq iwr;
	struct iw_statistics status;

	os_memset(&iwr, 0, sizeof(iwr));
	os_memset(&status, 0, sizeof(status));
	os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	iwr.u.data.pointer = &status;
	iwr.u.data.length = sizeof(status);

	if (ioctl(drv->ioctl_sock, SIOCGIWSTATS, &iwr) < 0) {
		wpa_printf(MSG_DEBUG, "%s: failed", __func__);
		return -1;
	}

	*quailty = status.qual.qual;
	*level = status.qual.level;
	*noise = status.qual.noise;

	return 0;
}


static int ralink_get_bitrate(void *priv, s32 *bitrate)
{
	struct wpa_driver_wext_data *drv = priv;
	struct iwreq iwr;
	struct iw_statistics status;

	os_memset(&iwr, 0, sizeof(iwr));
	os_memset(&status, 0, sizeof(status));
	os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);

	if (ioctl(drv->ioctl_sock, SIOCGIWRATE, &iwr) < 0) {
		wpa_printf(MSG_DEBUG, "%s: failed", __func__);
		return -1;
	}

	*bitrate = (iwr.u.bitrate.value) / 1000000;

	return 0;
}


static int ralink_get_associated_ap_info(void *priv, char *name, u8 *channel, s32 *bitrate)
{
	struct wpa_driver_wext_data *drv = priv;
	struct iwreq iwr;
	RT_802_11_OID_ASSOCIATED_AP_INFO	ap_info;

	os_memset(&iwr, 0, sizeof(iwr));
	os_memset(&ap_info, 0, sizeof(ap_info));
	os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	iwr.u.data.flags = RT_OID_802_11_QUERY_ASSOCIATED_AP_INFO;
	iwr.u.data.pointer = &ap_info;

	if (ioctl(drv->ioctl_sock, RT_PRIV_IOCTL, &iwr) < 0) {
		wpa_printf(MSG_DEBUG, "%s: failed", __func__);
		return -1;
	}

	os_memcpy(name, ap_info.name, IFNAMSIZ);
	*channel = ap_info.channel;
	*bitrate = ap_info.rate/1000000;

	return 0;
}


static int ralink_get_channel_status_info(void *priv, u8 *channel)
{
        struct wpa_driver_wext_data *drv = priv;
        struct iwreq iwr;
        RT_802_11_LINK_STATUS    channel_status;

        os_memset(&iwr, 0, sizeof(iwr));
        os_memset(&channel_status, 0, sizeof(channel_status));
        os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
        iwr.u.data.flags = RT_OID_802_11_QUERY_LINK_STATUS;
        iwr.u.data.pointer = &channel_status;

        if (ioctl(drv->ioctl_sock, RT_PRIV_IOCTL, &iwr) < 0) {
                wpa_printf(MSG_DEBUG, "%s: failed", __func__);
                return -1;
        }

        *channel = channel_status.CentralChannel;

        return 0;

}

#ifdef ANDROID
static char *wpa_driver_get_country_code(int channels)
{
	char *country = "US"; /* WEXT_NUMBER_SCAN_CHANNELS_FCC */

	if (channels == WEXT_NUMBER_SCAN_CHANNELS_ETSI)
		country = "EU";
	else if( channels == WEXT_NUMBER_SCAN_CHANNELS_MKK1)
		country = "JP";
	return country;
}

static int wpa_driver_priv_driver_cmd( void *priv, char *cmd, char *buf, size_t buf_len )
{
	struct wpa_driver_wext_data *drv = priv;
	struct wpa_supplicant *wpa_s = (struct wpa_supplicant *)(drv->ctx);
	struct iwreq iwr;
	int ret = 0, flags;

	wpa_printf(MSG_DEBUG, "%s %s len = %d", __func__, cmd, buf_len);

	if (!drv->driver_is_loaded && (os_strcasecmp(cmd, "START") != 0)) {
		wpa_printf(MSG_ERROR,"WEXT: Driver not initialized yet");
		return -1;
	}

	if (os_strcasecmp(cmd, "RSSI-APPROX") == 0) {
		os_strncpy(cmd, "RSSI", MAX_DRV_CMD_SIZE);
	}
	else if( os_strncasecmp(cmd, "SCAN-CHANNELS", 13) == 0 ) {
		int no_of_chan;

		no_of_chan = atoi(cmd + 13);
		os_snprintf(cmd, MAX_DRV_CMD_SIZE, "COUNTRY %s",
			wpa_driver_get_country_code(no_of_chan));
	}
	else if (os_strcasecmp(cmd, "STOP") == 0) {
		//if ((wpa_driver_wext_get_ifflags(drv, &flags) == 0) &&
		//    (flags & IFF_UP)) {
			wpa_printf(MSG_ERROR, "WEXT: %s when iface is UP", cmd);
		//	wpa_driver_wext_set_ifflags(drv, flags & ~IFF_UP);
		//}
	}
	else if( os_strcasecmp(cmd, "RELOAD") == 0 ) {
		wpa_printf(MSG_DEBUG,"Reload command");
		wpa_msg(drv->ctx, MSG_INFO, WPA_EVENT_DRIVER_STATE "HANGED");
		return ret;
	}

	os_memset(&iwr, 0, sizeof(iwr));
	os_strncpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	os_memcpy(buf, cmd, strlen(cmd) + 1);
	iwr.u.data.pointer = buf;
	iwr.u.data.length = buf_len;

	if ((ret = ioctl(drv->ioctl_sock, SIOCSIWPRIV, &iwr)) < 0) {
		perror("ioctl[SIOCSIWPRIV]");
	}

	if (ret < 0) {
		wpa_printf(MSG_ERROR, "%s failed (%d): %s", __func__, ret, cmd);
		drv->errors++;
		if (drv->errors > WEXT_NUMBER_SEQUENTIAL_ERRORS) {
			drv->errors = 0;
			wpa_msg(drv->ctx, MSG_INFO, WPA_EVENT_DRIVER_STATE "HANGED");
		}
	}
	else {
		drv->errors = 0;
		ret = 0;
		if ((os_strcasecmp(cmd, "RSSI") == 0) ||
		    (os_strcasecmp(cmd, "LINKSPEED") == 0) ||
		    (os_strcasecmp(cmd, "MACADDR") == 0)) {
			ret = strlen(buf);
		}
		else if (os_strcasecmp(cmd, "START") == 0) {
			drv->driver_is_loaded = TRUE;
			/* os_sleep(0, WPA_DRIVER_WEXT_WAIT_US);
			wpa_msg(drv->ctx, MSG_INFO, WPA_EVENT_DRIVER_STATE "STARTED"); */
		}
		else if (os_strcasecmp(cmd, "STOP") == 0) {
			drv->driver_is_loaded = FALSE;
			/* wpa_msg(drv->ctx, MSG_INFO, WPA_EVENT_DRIVER_STATE "STOPPED"); */
		}
		wpa_printf(MSG_DEBUG, "%s %s len = %d, %d", __func__, buf, ret, strlen(buf));
	}
	return ret;
}
#endif

#ifdef CONFIG_P2P
#define OID_802_11_P2P_MODE					0x0801
#define OID_802_11_P2P_DEVICE_NAME			0x0802
#define OID_802_11_P2P_LISTEN_CHANNEL			0x0803
#define OID_802_11_P2P_OPERATION_CHANNEL		0x0804
#define OID_802_11_P2P_DEV_ADDR				0x0805
#define OID_802_11_P2P_SCAN_LIST				0x0806
#define OID_802_11_P2P_GO_INT					0x080c

#define OID_802_11_P2P_CTRL_STATUS			0x0807
#define OID_802_11_P2P_DISC_STATUS			0x0808
#define OID_802_11_P2P_GOFORM_STATUS			0x0809
#define OID_P2P_WSC_PIN_CODE					0x080a
#define OID_802_11_P2P_CLEAN_TABLE			0x080b
#define OID_802_11_P2P_SCAN					0x080d
#define OID_802_11_P2P_WscMode				0x080e
#define OID_802_11_P2P_WscConf					0x080f
/* 0x0810 ~ 0x0814 Reserved for iNIC USERDEF_GPIO_SUPPORT */
/* 0x0820 ~ 0x0822 Reserved for iNIC USERDEF_GPIO_SUPPORT */
#define OID_802_11_P2P_Link						0x0830
#define OID_802_11_P2P_Connected_MAC			0x0831
#define OID_802_11_P2P_RESET					0x0832
#define OID_802_11_P2P_SIGMA_ENABLE			0x0833
#define OID_802_11_P2P_SSID					0x0834
#define OID_802_11_P2P_CONNECT_ADDR			0x0835
#define OID_802_11_P2P_CONNECT_STATUS		0x0836
#define OID_802_11_P2P_PEER_GROUP_ID		0x0837
#define OID_802_11_P2P_ENTER_PIN			0x0838
#define OID_802_11_P2P_PROVISION			0x0839
#define OID_802_11_P2P_DEL_CLIENT			0x083a
#define OID_802_11_P2P_PASSPHRASE			0x0840
#define OID_802_11_P2P_ASSOCIATE_TAB		0x0841
#define OID_802_11_P2P_PROVISION_MAC		0x0842
#define OID_802_11_P2P_LINK_DOWN				0x0843
#define OID_802_11_P2P_PRI_DEVICE_TYPE		0x0844
#define OID_802_11_P2P_INVITE				0x0845
#define OID_802_11_P2P_PERSISTENT_TABLE     (0x0846)
#define OID_802_11_P2P_TRIGGER_WSC		0x0848
#define OID_802_11_P2P_WSC_CONF_MODE    0x0849
#define OID_802_11_P2P_PERSISTENT_ENABLE    (0x084a)
#define OID_802_11_P2P_WSC_CANCEL       0x084b
#define OID_802_11_P2P_WSC_MODE         0x0850
#define OID_802_11_P2P_PIN_CODE         0x0851
#define OID_802_11_P2P_AUTO_ACCEPT		0x0852
#define OID_802_11_P2P_CHECK_PEER_CHANNEL		0x0853
#ifdef CONFIG_QUERY_P2P_CONNECT_STATE
#define OID_802_11_P2P_GET_CONNECT_STATE			(0x0854)
#endif

#ifdef CONFIG_WFD
#define OID_802_11_WFD_ENABLE				0x0859
#define OID_802_11_WFD_DEVICE_TYPE					0x0860
#define OID_802_11_WFD_SOURCE_COUPLED				0x0861
#define OID_802_11_WFD_SINK_COUPLED					0x0862
#define OID_802_11_WFD_SESSION_AVAILABLE 			0x0863
#define OID_802_11_WFD_RTSP_PORT					0x0864
#define OID_802_11_WFD_MAX_THROUGHPUT				0x0865
#define OID_802_11_WFD_SESSION_ID				0x0866
#define OID_802_11_P2P_PEER_RTSP_PORT				0x0867
#define OID_802_11_WFD_CONTENT_PROTECT				0x086a
#endif /* CONFIG_WFD */

/* USB-suspend */
#define RT_OID_USB_SUSPEND					0x0920
#define RT_OID_USB_RESUME					0x0921
/* Endo of USB-suspend */

#define RT_OID_MULTI_CHANNEL_ENABLE			0x0930

static int wpa_driver_wext_p2p_find(void *priv, unsigned int timeout, int type)
{
	struct wpa_driver_wext_data *drv = priv;
	u8 scan = 1;
	wpa_printf(MSG_DEBUG, "%s(timeout=%u)", __func__, timeout);

	return wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_SCAN, &scan, sizeof(char));
}


static int wpa_driver_wext_p2p_stop_find(void *priv)
{
	struct wpa_driver_wext_data *drv = priv;
	u8 scan = 0;

	wpa_printf(MSG_DEBUG, "%s", __func__);

	return wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_SCAN, &scan, sizeof(char));
}


static int wpa_driver_wext_p2p_connect(void *priv, const u8 *peer_addr,
				       int wps_method, int go_intent,
				       const u8 *own_interface_addr,
				       unsigned int force_freq,
				       int persistent_group, const u8 *pin)
{
	struct wpa_driver_wext_data *drv = priv;

	u8 Addr[18]={0}, Pin[10] = {0};
	u8 intent = go_intent;
	u8 ZERO_MAC_ADDR[6]  = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	wpa_printf(MSG_DEBUG, "%s(peer_addr=" MACSTR " wps_method=%d "
		   "go_intent=%d "
		   "own_interface_addr=" MACSTR " force_freq=%u "
		   "persistent_group=%d)",
		   __func__, MAC2STR(peer_addr), wps_method, go_intent,
		   MAC2STR(own_interface_addr), force_freq, persistent_group);

	if ( !memcmp(ZERO_MAC_ADDR, peer_addr, 6) ) 
	{
		u8 conn_stat = 1;
		wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_LINK_DOWN, &conn_stat, sizeof(char));
		return 0;
	}

	if (wps_method == WPS_PIN_DISPLAY)
	{
		u8 wps_mthd = 1;
		wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_WscMode, &wps_mthd, sizeof(u8));
		wps_mthd = 1;
		wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_WscConf, &wps_mthd, sizeof(u8));
	}
	else if (wps_method == WPS_PIN_KEYPAD)
	{
		u8 wps_mthd = 1;
		wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_WscMode, &wps_mthd, sizeof(u8));
		wps_mthd = 2;
		wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_WscConf, &wps_mthd, sizeof(u8));
		snprintf((char *)Pin, sizeof(Pin), "%s", pin);
		wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_ENTER_PIN, Pin, sizeof(Pin));

	}
	else if (wps_method == WPS_PBC)
	{
		u8 wps_mthd = 2;
		wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_WscMode, &wps_mthd, sizeof(u8));
		wps_mthd = 3;
		wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_WscConf, &wps_mthd, sizeof(u8));
	}		

	wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_GO_INT, &intent, sizeof(u8));
	snprintf((char *)Addr, sizeof(Addr), "%02x:%02x:%02x:%02x:%02x:%02x", *(peer_addr), *(peer_addr+1), *(peer_addr+2), *(peer_addr+3),
				*(peer_addr+4), *(peer_addr+5));
	wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_CONNECT_ADDR, Addr, sizeof(Addr));

	return 0;
}

#define WPS_CONFIG_USBA 0x0001
#define WPS_CONFIG_ETHERNET 0x0002
#define WPS_CONFIG_LABEL 0x0004
#define WPS_CONFIG_DISPLAY 0x0008
#define WPS_CONFIG_EXT_NFC_TOKEN 0x0010
#define WPS_CONFIG_INT_NFC_TOKEN 0x0020
#define WPS_CONFIG_NFC_INTERFACE 0x0040
#define WPS_CONFIG_PUSHBUTTON 0x0080
#define WPS_CONFIG_KEYPAD 0x0100

static int wpa_driver_wext_p2p_provision(void *priv, const u8 *peer_addr,
				 u16 config_methods)
{
	struct wpa_driver_wext_data *drv = priv;

	u8 Addr[18]={0}/*, Pin[10] = {0}*/;
	//u8 intent = go_intent;
	wpa_printf(MSG_DEBUG, "%s(peer_addr=" MACSTR " config_methods=%d",
		   __func__, MAC2STR(peer_addr), config_methods);

	if (config_methods == WPS_CONFIG_KEYPAD)
	{
		u8 wps_mthd = 1;
		wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_WscMode, &wps_mthd, sizeof(u8));
		wps_mthd = 1;
		wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_WscConf, &wps_mthd, sizeof(u8));
	}
	else if (config_methods == WPS_CONFIG_DISPLAY)
	{
		u8 wps_mthd = 1;
		wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_WscMode, &wps_mthd, sizeof(u8));
		wps_mthd = 2;
		wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_WscConf, &wps_mthd, sizeof(u8));
//		snprintf(Pin, sizeof(Pin), "%s", pin);
//		wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_ENTER_PIN, &Pin, sizeof(Pin));

	}
	else if (config_methods == WPS_CONFIG_PUSHBUTTON)
	{
		u8 wps_mthd = 2;
		wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_WscMode, &wps_mthd, sizeof(u8));
		wps_mthd = 3;
		wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_WscConf, &wps_mthd, sizeof(u8));
	}		

//	wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_GO_INT, &intent, sizeof(u8));
	snprintf((char *)Addr, sizeof(Addr), "%02x:%02x:%02x:%02x:%02x:%02x", *(peer_addr), *(peer_addr+1), *(peer_addr+2), *(peer_addr+3),
				*(peer_addr+4), *(peer_addr+5));
	wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_PROVISION_MAC, Addr, sizeof(Addr));

	return 0;
}

static int wpa_driver_wext_p2p_display_pin(void *priv,
				 unsigned int generated_pin)
{
	struct wpa_driver_wext_data *drv = priv;
	u8 pin_code[9];
	os_snprintf( (char *)pin_code, sizeof(pin_code), "%08d",
			    generated_pin);
	//printf("%s:: generated_pin = %d. pin_code = %s\n", __func__, generated_pin, pin_code);
	wpa_printf(MSG_DEBUG, "%s:: generated_pin = %d. pin_code = %s\n", __func__, generated_pin, pin_code);
	wfa_driver_ralink_set_oid(drv, OID_P2P_WSC_PIN_CODE, pin_code, 8);
	return 0;
}

static int wpa_driver_wext_p2p_set_params(void *priv,
					  const struct p2p_params *params)
{
	struct wpa_driver_wext_data *drv = priv;
	wpa_printf(MSG_DEBUG, "%s", __func__);
	u8 listen_channel = params->listen_channel;
	u8 op_channel = params->op_channel;
	u8 intent = params->intent;
	u8 dev_name[33] = {0};

	//printf("--------->%s\n", __func__);
	/* set GO Intent */
	wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_GO_INT, &intent, sizeof(u8));
	/* set Listen Channel currently */
	wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_LISTEN_CHANNEL, &listen_channel, sizeof(u8));
	/* set Operation Channel currently */
	wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_OPERATION_CHANNEL, &op_channel, sizeof(u8));
	if (params->dev_name)
	{
		strcpy((char *)dev_name, params->dev_name);
		wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_DEVICE_NAME, dev_name, strlen(params->dev_name));
	}

	wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_PRI_DEVICE_TYPE, (u8 *)params->pri_dev_type, sizeof(params->pri_dev_type));

#ifdef CONFIG_WFD
	u8 devType = params->devType;
	wfa_driver_ralink_set_oid(drv, OID_802_11_WFD_DEVICE_TYPE, &devType, sizeof(u8));
	
	u8 sourceCoup = params->sourceCoup;
	wfa_driver_ralink_set_oid(drv, OID_802_11_WFD_SOURCE_COUPLED, &sourceCoup, sizeof(u8));
	
	u8 sinkCoup = params->sinkCoup;
	wfa_driver_ralink_set_oid(drv, OID_802_11_WFD_SINK_COUPLED, &sinkCoup, sizeof(u8));
	
	u8 sessionAvail = params->sessionAvail;
	wfa_driver_ralink_set_oid(drv, OID_802_11_WFD_SESSION_AVAILABLE, &sessionAvail, sizeof(u8));
	
	int rtspPort = params->rtspPort;
	wfa_driver_ralink_set_oid(drv, OID_802_11_WFD_RTSP_PORT, &rtspPort, sizeof(int));
	
	int maxThroughput = params->maxThroughput;
	wfa_driver_ralink_set_oid(drv, OID_802_11_WFD_MAX_THROUGHPUT, &maxThroughput, sizeof(int));

	int contentProtect = params->contentProtect;
	wfa_driver_ralink_set_oid(drv, OID_802_11_WFD_CONTENT_PROTECT, &contentProtect, sizeof(int));
	
	u8 wfdEnable = params->wfdEnable;
	wfa_driver_ralink_set_oid(drv, OID_802_11_WFD_ENABLE, &wfdEnable, sizeof(u8));
	
#endif /* CONFIG_WFD */
#ifdef CONFIG_P2P
	#ifdef CONFIG_P2P_PERSISTANT_ENABLE
	int p2persistant_en = params->p2pPersistentEn;
	wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_PERSISTENT_ENABLE, &p2persistant_en, sizeof(int));
	
	#endif
#endif


	return 0;
}
#if 1
static int wpa_driver_wext_if_add(void *priv, enum wpa_driver_if_type type,
			      const char *ifname, const u8 *addr,
			      void *bss_ctx, void **drv_priv,
			      char *force_ifname, u8 *if_addr,
			      const char *bridge)
{
	struct wpa_driver_wext_data *drv = priv;

	wpa_printf(MSG_DEBUG, "%s(type=%d ifname=%s bss_ctx=%p)",
		   __func__, type, ifname, bss_ctx);
	if (!addr &&
	    linux_get_ifhwaddr(drv->ioctl_sock, drv->ifname, if_addr) < 0) {
		wpa_printf(MSG_ERROR, "%s(fail to get Mac Address)",
		   __func__);
		return -1;
	}

	return 0;
}
#endif
static int ralink_get_p2p_association_table(void *priv,struct _P2P_STA_ASSOC_LIST *AssocList)
{
	struct wpa_driver_wext_data *drv = priv;
	//P2P_STA_ASSOC_LIST AssocList;
	struct iwreq iwr;
	int ret = 0;

	os_memset(&iwr, 0, sizeof(iwr));
	os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	iwr.u.data.flags = OID_802_11_P2P_ASSOCIATE_TAB;
	iwr.u.data.pointer = AssocList;

	if (ioctl(drv->ioctl_sock, RT_PRIV_IOCTL, &iwr) < 0) {
		wpa_printf(MSG_DEBUG, "%s: failed", __func__);
		ret = -1;
	}

	return ret;
}

static int ralink_get_p2p_operation_channel(void *priv, char* OperationChannel)
{
	struct wpa_driver_wext_data *drv = priv;
	struct iwreq iwr;
	int ret = 0;

	os_memset(&iwr, 0, sizeof(iwr));
	os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	iwr.u.data.flags = OID_802_11_P2P_OPERATION_CHANNEL ;
	iwr.u.data.pointer = OperationChannel;

	if (ioctl(drv->ioctl_sock, RT_PRIV_IOCTL, &iwr) < 0) {
		wpa_printf(MSG_DEBUG, "%s: failed", __func__);
		ret = -1;
	}

	return ret;
}

#ifdef CONFIG_QUERY_P2P_CONNECT_STATE
static int ralink_get_p2p_connect_state(void *priv, int* ConnectState)
{
	struct wpa_driver_wext_data *drv = priv;
	struct iwreq iwr;
	int ret = 0;

	os_memset(&iwr, 0, sizeof(iwr));
	os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
	iwr.u.data.flags = OID_802_11_P2P_GET_CONNECT_STATE ;
	iwr.u.data.pointer = ConnectState;

	if (ioctl(drv->ioctl_sock, RT_PRIV_IOCTL, &iwr) < 0) {
		wpa_printf(MSG_DEBUG, "%s: failed", __func__);
		ret = -1;
	}

	return ret;
}
#endif

static int ralink_get_802_11_statistics(void *priv, unsigned long *pFailedCount, unsigned long *pRetryCount)
{
        struct wpa_driver_wext_data *drv = priv;
        struct iwreq iwr;
		NDIS_802_11_STATISTICS Statistics;
 
        os_memset(&iwr, 0, sizeof(iwr));
        os_memset(&Statistics, 0, sizeof(Statistics));
        os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
        iwr.u.data.flags = RT_OID_802_11_STATISTICS;
        iwr.u.data.pointer = &Statistics;

        if (ioctl(drv->ioctl_sock, RT_PRIV_IOCTL, &iwr) < 0) {
                wpa_printf(MSG_DEBUG, "%s: failed", __func__);
                return -1;
        }

		*pFailedCount = Statistics.FailedCount.u.LowPart;
		*pRetryCount = Statistics.RetryCount.u.LowPart;

        return 0;
}

static int ralink_set_p2p_force_go(void *priv, u8 enable, struct p2p_go_neg_results *params)
{
	struct wpa_driver_wext_data *drv = priv;
	char op_mode;

	printf("%s:: 0x%x\n", __FUNCTION__, enable);

	if ( enable )
	{
		
		op_mode = 1;
		wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_MODE, &op_mode, sizeof(char));	
		wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_SSID, params->ssid, params->ssid_len);
		wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_PASSPHRASE, params->passphrase, strlen(params->passphrase));
		wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_SSID, params->ssid, params->ssid_len);
	} else {
		int status;
		char conn_stat = 1;
		op_mode = 0;
		wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_MODE, &op_mode, sizeof(char));	
		//status = wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_LINK_DOWN, &conn_stat, sizeof(char));
		return status;
	}

	return 0;
	
}

static int ralink_set_p2p_go_ssid(void *priv, const char *ssid, const char *wpapsk)
{
	struct wpa_driver_wext_data *drv = priv;
	wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_PASSPHRASE, wpapsk, strlen(wpapsk));
	wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_SSID, ssid, strlen(ssid));

	return 0;
	
}

static int wpa_driver_wext_p2p_listen(void *priv, unsigned int timeout)
{
                struct wpa_driver_wext_data *drv = priv;
                u8 scan = 2;
                
                wpa_printf(MSG_DEBUG, "%s(timeout=%u)", __func__, timeout);

                return wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_SCAN, &scan, sizeof(char));
}

static int ralink_set_p2p_pin(void *priv, const char *pin)
{
	struct wpa_driver_wext_data *drv = priv;
	int i = os_strlen(pin);
	u8 pin_to_driver[i];
	char wsc_conf_mode[] = "7";
	char wsc_mode[] = "1";
	char wsc_trigger[] = "1";
	
	if ((i != 4) && (i != 8))
	{
		wpa_printf(MSG_ERROR, "%s Pin Length error!!!", __func__, i);
		return -1;
	}

	os_memset(pin_to_driver, 0, i);
	os_strncpy(pin_to_driver, pin,  i);

	wpa_printf(MSG_ERROR, "%s:: pin_to_driver = %s", __func__, pin_to_driver);

	wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_WSC_CONF_MODE, &wsc_conf_mode, sizeof(wsc_conf_mode));
	wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_ENTER_PIN, &pin_to_driver, i);
	wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_WSC_MODE, &wsc_mode, sizeof(wsc_mode));
	
	return wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_TRIGGER_WSC, &wsc_trigger, sizeof(wsc_trigger));
}

static int ralink_set_p2p_pbc(void *priv, u8 *enable)
{
	struct wpa_driver_wext_data *drv = priv;
	char wsc_conf_mode[] = "7";
	char wsc_mode[] = "2";
	char wsc_trigger[] = "1";
	
	os_strncpy(wsc_trigger, enable, 1);

	wpa_printf(MSG_ERROR, "%s:: pbc_trigger_driver = %d", __func__, strtol(wsc_trigger, NULL, 0));
	
	wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_WSC_CONF_MODE, &wsc_conf_mode, sizeof(wsc_conf_mode));
	wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_WSC_MODE, &wsc_mode, sizeof(wsc_mode));

	return wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_TRIGGER_WSC, &wsc_trigger, sizeof(wsc_trigger));
}

static int ralink_set_p2p_del_client(void *priv, char *addr)
{
	struct wpa_driver_wext_data *drv = priv;

    if (!addr)
        return -1;
    wpa_printf(MSG_ERROR, "%s:: addr=[%s]", __func__, addr);
    return wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_DEL_CLIENT, addr, os_strlen(addr));
}

static int ralink_set_p2p_customized_cmd(void *priv, char *cmdstr, char *data, u32 len)
{
	struct wpa_driver_wext_data *drv = priv;
    u16 oid = 0;

    if (!priv || !cmdstr)
        return -1;
    if (os_strcasecmp(cmdstr, "p2p_persistent_enable") == 0)
        oid = OID_802_11_P2P_PERSISTENT_ENABLE;
    else
    {
        wpa_printf(MSG_ERROR, "%s: Unknown cmd=%s", __func__, cmdstr);
        return -1;
    }
    wpa_printf(MSG_ERROR, "%s: oid=%d", __func__, oid);
    return wfa_driver_ralink_set_oid(drv, oid, data, len);
}

static int ralink_get_p2p_customized_cmd(void *priv, char *cmdstr, char *data, u32 len)
{
	struct wpa_driver_wext_data *drv = priv;
    u16 oid = 0;

    if (!priv || !cmdstr)
        return -1;
    if (os_strcasecmp(cmdstr, "P2P_PER_TAB") == 0)
    {
        oid = OID_802_11_P2P_PERSISTENT_TABLE;
    }
    else
    {
        wpa_printf(MSG_ERROR, "%s: Unknown cmd=%s", __func__, cmdstr);
        return -1;
    }
    wpa_printf(MSG_ERROR, "%s: oid=0x%x", __func__, oid);
    return wfa_driver_ralink_get_oid(drv, oid, data, len);
}

static int ralink_set_p2p_auto_accept(void *priv, u8 enable)
{
	struct wpa_driver_wext_data *drv = priv;

	wpa_printf(MSG_ERROR, "%s:: ralink_set_p2p_auto_accept = %d", __func__, enable);

	return wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_AUTO_ACCEPT, &enable, sizeof(u8));
}

static int ralink_set_p2p_check_peer_channel(void *priv, u8 mode)
{
	struct wpa_driver_wext_data *drv = priv;

	wpa_printf(MSG_ERROR, "%s:: ralink_set_p2p_check_peer_channel = %d", __func__, mode);

	return wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_CHECK_PEER_CHANNEL, &mode, sizeof(u8));
}

static int ralink_set_p2p_client_ageout(void *priv, const u8 *addr, unsigned int ageout)
{
	struct wpa_driver_wext_data *drv = priv;
	DOT1X_IDLE_TIMEOUT cli_ageout;		
    if (!addr)
        return -1;
	
	os_memcpy(cli_ageout.StaAddr, addr, ETH_ALEN);
	cli_ageout.idle_timeout = ageout;

    wpa_printf(MSG_ERROR, "%s:: addr=[%02X:%02X:%02X:%02X:%02X:%02X] ageout=%d seconds", __func__, 
		*(cli_ageout.StaAddr), *(cli_ageout.StaAddr+1), *(cli_ageout.StaAddr+2), *(cli_ageout.StaAddr+3), *(cli_ageout.StaAddr+4), *(cli_ageout.StaAddr+5), ageout);

    return wfa_driver_ralink_set_oid(drv, OID_802_DOT1X_IDLE_TIMEOUT, &cli_ageout, sizeof(DOT1X_IDLE_TIMEOUT));

}

static int wpa_driver_wext_p2p_link_down(void *priv)
{
	struct wpa_driver_wext_data *drv = priv;
	u8 conn_stat = 1;
	return wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_LINK_DOWN, &conn_stat, sizeof(char));
}

#endif /* CONFIG_P2P */

static int ralink_set_usb_suspend(void *priv, u32 param)
{
    struct wpa_driver_wext_data *drv = priv;
    struct iwreq iwr;
    int err;
    
    os_memset(&iwr, 0, sizeof(iwr));
    os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
    if (param == 1)
        iwr.u.data.flags = OID_GET_SET_TOGGLE | RT_OID_USB_SUSPEND;
    else
        iwr.u.data.flags = OID_GET_SET_TOGGLE | RT_OID_USB_RESUME;
    iwr.u.data.pointer = &param;
    iwr.u.data.length = sizeof(u32);

    wpa_printf(MSG_ERROR, "%s:: set usb=%s", __func__, (param == 1)?"suspend":"resume");    
    if ( (err = ioctl(drv->ioctl_sock, RT_PRIV_IOCTL, &iwr)) < 0)
    {
        printf("ERR(%d):: %s in 0x%04x.\n", err, __FUNCTION__, RT_OID_USB_SUSPEND);
        return -1;
    }    
	return 0;
}

static int ralink_set_multi_channel_enable(void *priv, u8 enable)
{
    struct wpa_driver_wext_data *drv = priv;
    struct iwreq iwr;
    int err;
    
    os_memset(&iwr, 0, sizeof(iwr));
    os_strlcpy(iwr.ifr_name, drv->ifname, IFNAMSIZ);
    iwr.u.data.flags = OID_GET_SET_TOGGLE | RT_OID_MULTI_CHANNEL_ENABLE;
    iwr.u.data.pointer = &enable;
    iwr.u.data.length = sizeof(u8);

    wpa_printf(MSG_ERROR, "%s:: set multiple channel enable=%d", __func__, enable);    
    if ( (err = ioctl(drv->ioctl_sock, RT_PRIV_IOCTL, &iwr)) < 0)
    {
        printf("ERR(%d):: %s in 0x%04x.\n", err, __FUNCTION__, RT_OID_MULTI_CHANNEL_ENABLE);
        return -1;
    }    
	return 0;
}

static int ralink_set_p2p_invite(void *priv, const u8 *peer, int role,
			  const u8 *bssid, const u8 *ssid, size_t ssid_len,
			  const u8 *go_dev_addr, int persistent_group)
{
    struct wpa_driver_wext_data *drv = priv;
    u8 Addr[18] = {0};
    if (!peer)
        return -1;
    snprintf((char *)Addr, sizeof(Addr), "%02x:%02x:%02x:%02x:%02x:%02x", *(peer), *(peer+1), *(peer+2), *(peer+3),
				*(peer+4), *(peer+5));
    wpa_printf(MSG_ERROR, "%s:: addr=%s", __func__, Addr);
    return wfa_driver_ralink_set_oid(drv, OID_802_11_P2P_INVITE, &Addr, sizeof(Addr));
}
static int ralink_set_p2p_wps_cancel(void *priv)
{
    struct wpa_driver_wext_data *drv = priv;
    u8 wps_cancel=1;
    wpa_printf(MSG_ERROR, "Set P2P WPS cancel\n");
    return wfa_driver_ralink_set_oid(drv,OID_802_11_P2P_WSC_CANCEL,wps_cancel,sizeof(char));
}

const struct wpa_driver_ops wpa_driver_wext_ops = {
	.name = "wext",
	.desc = "Linux wireless extensions (generic)",
	.get_bssid = wpa_driver_wext_get_bssid,
	.get_ssid = wpa_driver_wext_get_ssid,
	.set_key = wpa_driver_wext_set_key,
	.set_countermeasures = wpa_driver_wext_set_countermeasures,
	.scan2 = wpa_driver_wext_scan,
	.get_scan_results2 = wpa_driver_wext_get_scan_results,
	.deauthenticate = wpa_driver_wext_deauthenticate,
	.disassociate = wpa_driver_wext_disassociate,
	.associate = wpa_driver_wext_associate,
	.init = wpa_driver_wext_init,
	.deinit = wpa_driver_wext_deinit,
	.add_pmkid = wpa_driver_wext_add_pmkid,
	.remove_pmkid = wpa_driver_wext_remove_pmkid,
	.flush_pmkid = wpa_driver_wext_flush_pmkid,
	.get_capa = wpa_driver_wext_get_capa,
	.set_operstate = wpa_driver_wext_set_operstate,
	.get_radio_name = wext_get_radio_name,
	.get_driver_version = ralink_get_driver_version,
	.set_wpa_supplicant_support = ralink_set_wpa_supplicant_support,
	.get_status = ralink_get_driver_status,
	.get_bitrate = ralink_get_bitrate,
	.get_associated_ap_info = ralink_get_associated_ap_info,
	.get_channel_status_info = ralink_get_channel_status_info,
	.set_usb_suspend = ralink_set_usb_suspend,
	.set_multi_channel_enable = ralink_set_multi_channel_enable,
	.get_802_11_statistics = ralink_get_802_11_statistics,
#ifdef CONFIG_P2P
	.if_add = wpa_driver_wext_if_add,
	.p2p_find = wpa_driver_wext_p2p_find,
	.p2p_stop_find = wpa_driver_wext_p2p_stop_find,
	.p2p_connect = wpa_driver_wext_p2p_connect,
	.p2p_set_params = wpa_driver_wext_p2p_set_params,
	.p2p_prov_disc_req = wpa_driver_wext_p2p_provision,
	.p2p_display_pin = wpa_driver_wext_p2p_display_pin,
	.get_p2p_association_table = ralink_get_p2p_association_table,
	.get_p2p_operation_channel = ralink_get_p2p_operation_channel,
	.set_p2p_force_go = ralink_set_p2p_force_go,
	.p2p_listen = wpa_driver_wext_p2p_listen,
	.p2p_start_pin = ralink_set_p2p_pin,
	.set_p2p_go_ssid = ralink_set_p2p_go_ssid,
	.p2p_start_pbc = ralink_set_p2p_pbc,
	.p2p_del_client = ralink_set_p2p_del_client,
	.p2p_invite = ralink_set_p2p_invite,
	.p2p_set_customized_cmd = ralink_set_p2p_customized_cmd,
	.p2p_get_customized_cmd = ralink_get_p2p_customized_cmd,
	.set_p2p_auto_accept = ralink_set_p2p_auto_accept,
	.p2p_wps_cancel = ralink_set_p2p_wps_cancel,
	.set_p2p_check_peer_channel = ralink_set_p2p_check_peer_channel,
	.set_p2p_client_ageout = ralink_set_p2p_client_ageout,
	.p2p_disconnect = wpa_driver_wext_p2p_link_down,
	#ifdef CONFIG_QUERY_P2P_CONNECT_STATE
	.get_p2p_connect_state = ralink_get_p2p_connect_state,
	#endif
#endif /* CONFIG_P2P */
#ifdef ANDROID
    //.signal_poll = wpa_driver_signal_poll,
	.driver_cmd = wpa_driver_priv_driver_cmd,
#endif

};
