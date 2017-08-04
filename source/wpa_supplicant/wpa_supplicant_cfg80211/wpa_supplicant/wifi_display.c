/*
 * wpa_supplicant - Wi-Fi Display
 * Copyright (c) 2011, Atheros Communications, Inc.
 * Copyright (c) 2011-2012, Qualcomm Atheros, Inc.
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"

#include "common.h"
#include "p2p/p2p.h"
#include "common/ieee802_11_defs.h"
#include "wpa_supplicant_i.h"
#include "wifi_display.h"


#define WIFI_DISPLAY_SUBELEM_HEADER_LEN 3


int wifi_display_init(struct wpa_global *global)
{
	global->wifi_display = 1;
	return 0;
}


void wifi_display_deinit(struct wpa_global *global)
{
	int i;
	for (i = 0; i < MAX_WFD_SUBELEMS; i++) {
		wpabuf_free(global->wfd_subelem[i]);
		global->wfd_subelem[i] = NULL;
	}
}


struct wpabuf * wifi_display_get_wfd_ie(struct wpa_global *global)
{
	struct wpabuf *ie;
	size_t len;
	int i;

	if (global->p2p == NULL)
		return NULL;

	len = 0;
	for (i = 0; i < MAX_WFD_SUBELEMS; i++) {
		if (global->wfd_subelem[i])
			len += wpabuf_len(global->wfd_subelem[i]);
	}

	ie = wpabuf_alloc(len);
	if (ie == NULL)
		return NULL;

	for (i = 0; i < MAX_WFD_SUBELEMS; i++) {
		if (global->wfd_subelem[i])
			wpabuf_put_buf(ie, global->wfd_subelem[i]);
	}

	return ie;
}


static int wifi_display_update_wfd_ie(struct wpa_global *global)
{
	struct wpabuf *ie, *buf;
	size_t len, plen;

	if (global->p2p == NULL)
		return 0;

	wpa_printf(MSG_DEBUG, "WFD: Update WFD IE");

	if (!global->wifi_display) {
		wpa_printf(MSG_DEBUG, "WFD: Wi-Fi Display disabled - do not "
			   "include WFD IE");
		p2p_set_wfd_ie_beacon(global->p2p, NULL);
		p2p_set_wfd_ie_probe_req(global->p2p, NULL);
		p2p_set_wfd_ie_probe_resp(global->p2p, NULL);
		p2p_set_wfd_ie_assoc_req(global->p2p, NULL);
		p2p_set_wfd_ie_invitation(global->p2p, NULL);
		p2p_set_wfd_ie_prov_disc_req(global->p2p, NULL);
		p2p_set_wfd_ie_prov_disc_resp(global->p2p, NULL);
		p2p_set_wfd_ie_go_neg(global->p2p, NULL);
		p2p_set_wfd_dev_info(global->p2p, NULL);
		p2p_set_wfd_assoc_bssid(global->p2p, NULL);
		p2p_set_wfd_coupled_sink_info(global->p2p, NULL);
		return 0;
	}

	p2p_set_wfd_dev_info(global->p2p,
			     global->wfd_subelem[WFD_SUBELEM_DEVICE_INFO]);
	p2p_set_wfd_assoc_bssid(
		global->p2p,
		global->wfd_subelem[WFD_SUBELEM_ASSOCIATED_BSSID]);
	p2p_set_wfd_coupled_sink_info(
		global->p2p, global->wfd_subelem[WFD_SUBELEM_COUPLED_SINK]);

	/*
	 * WFD IE is included in number of management frames. Two different
	 * sets of subelements are included depending on the frame:
	 *
	 * Beacon, (Re)Association Request, GO Negotiation Req/Resp/Conf,
	 * Provision Discovery Req:
	 * WFD Device Info
	 * [Associated BSSID]
	 * [Coupled Sink Info]
	 *
	 * Probe Request:
	 * WFD Device Info
	 * [Associated BSSID]
	 * [Coupled Sink Info]
	 * [WFD Extended Capability]
	 *
	 * Probe Response:
	 * WFD Device Info
	 * [Associated BSSID]
	 * [Coupled Sink Info]
	 * [WFD Extended Capability]
	 * [WFD Session Info]
	 *
	 * (Re)Association Response, P2P Invitation Req/Resp,
	 * Provision Discovery Resp:
	 * WFD Device Info
	 * [Associated BSSID]
	 * [Coupled Sink Info]
	 * [WFD Session Info]
	 */
	len = 0;
	if (global->wfd_subelem[WFD_SUBELEM_DEVICE_INFO])
		len += wpabuf_len(global->wfd_subelem[
					  WFD_SUBELEM_DEVICE_INFO]);
	if (global->wfd_subelem[WFD_SUBELEM_ASSOCIATED_BSSID])
		len += wpabuf_len(global->wfd_subelem[
					  WFD_SUBELEM_ASSOCIATED_BSSID]);
	if (global->wfd_subelem[WFD_SUBELEM_COUPLED_SINK])
		len += wpabuf_len(global->wfd_subelem[
					  WFD_SUBELEM_COUPLED_SINK]);
	if (global->wfd_subelem[WFD_SUBELEM_SESSION_INFO])
		len += wpabuf_len(global->wfd_subelem[
					  WFD_SUBELEM_SESSION_INFO]);
	if (global->wfd_subelem[WFD_SUBELEM_EXT_CAPAB])
		len += wpabuf_len(global->wfd_subelem[WFD_SUBELEM_EXT_CAPAB]);
	buf = wpabuf_alloc(len);
	if (buf == NULL)
		return -1;

	if (global->wfd_subelem[WFD_SUBELEM_DEVICE_INFO])
		wpabuf_put_buf(buf,
			       global->wfd_subelem[WFD_SUBELEM_DEVICE_INFO]);
	if (global->wfd_subelem[WFD_SUBELEM_ASSOCIATED_BSSID])
		wpabuf_put_buf(buf, global->wfd_subelem[
				       WFD_SUBELEM_ASSOCIATED_BSSID]);
	if (global->wfd_subelem[WFD_SUBELEM_COUPLED_SINK])
		wpabuf_put_buf(buf,
			       global->wfd_subelem[WFD_SUBELEM_COUPLED_SINK]);

	ie = wifi_display_encaps(buf);
	wpa_hexdump_buf(MSG_DEBUG, "WFD: WFD IE for Beacon", ie);
	p2p_set_wfd_ie_beacon(global->p2p, ie);

	ie = wifi_display_encaps(buf);
	wpa_hexdump_buf(MSG_DEBUG, "WFD: WFD IE for (Re)Association Request",
			ie);
	p2p_set_wfd_ie_assoc_req(global->p2p, ie);

	ie = wifi_display_encaps(buf);
	wpa_hexdump_buf(MSG_DEBUG, "WFD: WFD IE for GO Negotiation", ie);
	p2p_set_wfd_ie_go_neg(global->p2p, ie);

	ie = wifi_display_encaps(buf);
	wpa_hexdump_buf(MSG_DEBUG, "WFD: WFD IE for Provision Discovery "
			"Request", ie);
	p2p_set_wfd_ie_prov_disc_req(global->p2p, ie);

	plen = buf->used;
	if (global->wfd_subelem[WFD_SUBELEM_EXT_CAPAB])
		wpabuf_put_buf(buf,
			       global->wfd_subelem[WFD_SUBELEM_EXT_CAPAB]);

	ie = wifi_display_encaps(buf);
	wpa_hexdump_buf(MSG_DEBUG, "WFD: WFD IE for Probe Request", ie);
	p2p_set_wfd_ie_probe_req(global->p2p, ie);

	if (global->wfd_subelem[WFD_SUBELEM_SESSION_INFO])
		wpabuf_put_buf(buf,
			       global->wfd_subelem[WFD_SUBELEM_SESSION_INFO]);
	ie = wifi_display_encaps(buf);
	wpa_hexdump_buf(MSG_DEBUG, "WFD: WFD IE for Probe Response", ie);
	p2p_set_wfd_ie_probe_resp(global->p2p, ie);

	/* Remove WFD Extended Capability from buffer */
	buf->used = plen;
	if (global->wfd_subelem[WFD_SUBELEM_SESSION_INFO])
		wpabuf_put_buf(buf,
			       global->wfd_subelem[WFD_SUBELEM_SESSION_INFO]);

	ie = wifi_display_encaps(buf);
	wpa_hexdump_buf(MSG_DEBUG, "WFD: WFD IE for P2P Invitation", ie);
	p2p_set_wfd_ie_invitation(global->p2p, ie);

	ie = wifi_display_encaps(buf);
	wpa_hexdump_buf(MSG_DEBUG, "WFD: WFD IE for Provision Discovery "
			"Response", ie);
	p2p_set_wfd_ie_prov_disc_resp(global->p2p, ie);

	wpabuf_free(buf);

	return 0;
}


void wifi_display_enable(struct wpa_global *global, int enabled)
{
	wpa_printf(MSG_DEBUG, "WFD: Wi-Fi Display %s",
		   enabled ? "enabled" : "disabled");
	global->wifi_display = enabled;
	wifi_display_update_wfd_ie(global);
}


int wifi_display_subelem_set(struct wpa_global *global, char *cmd)
{
	char *pos;
	int subelem;
	size_t len;
	struct wpabuf *e;

	pos = os_strchr(cmd, ' ');
	if (pos == NULL)
		return -1;
	*pos++ = '\0';
	subelem = atoi(cmd);
	if (subelem < 0 || subelem >= MAX_WFD_SUBELEMS)
		return -1;

	len = os_strlen(pos);
	if (len & 1)
		return -1;
	len /= 2;

	if (len == 0) {
		/* Clear subelement */
		e = NULL;
		wpa_printf(MSG_DEBUG, "WFD: Clear subelement %d", subelem);
	} else {
		e = wpabuf_alloc(1 + len);
		if (e == NULL)
			return -1;
		wpabuf_put_u8(e, subelem);
		if (hexstr2bin(pos, wpabuf_put(e, len), len) < 0) {
			wpabuf_free(e);
			return -1;
		}
		wpa_printf(MSG_DEBUG, "WFD: Set subelement %d", subelem);
	}

	wpabuf_free(global->wfd_subelem[subelem]);
	global->wfd_subelem[subelem] = e;
	wifi_display_update_wfd_ie(global);

	return 0;
}


int wifi_display_subelem_set_from_ies(struct wpa_global *global,
				      struct wpabuf *ie)
{
	int subelements[MAX_WFD_SUBELEMS] = {};
	const u8 *pos, *end;
	int len, subelem;
	struct wpabuf *e;

	wpa_printf(MSG_DEBUG, "WFD IEs set: %p - %lu",
		   ie, ie ? (unsigned long) wpabuf_len(ie) : 0);

	if (ie == NULL || wpabuf_len(ie) < 6)
		return -1;

	pos = wpabuf_head(ie);
	end = pos + wpabuf_len(ie);

	while (end > pos) {
		if (pos + 3 > end)
			break;

		len = WPA_GET_BE16(pos + 1) + 3;

		wpa_printf(MSG_DEBUG, "WFD Sub-Element ID %d - len %d",
			   *pos, len - 3);

		if (pos + len > end)
			break;

		subelem = *pos;
		if (subelem < MAX_WFD_SUBELEMS && subelements[subelem] == 0) {
			e = wpabuf_alloc_copy(pos, len);
			if (e == NULL)
				return -1;

			wpabuf_free(global->wfd_subelem[subelem]);
			global->wfd_subelem[subelem] = e;
			subelements[subelem] = 1;
		}

		pos += len;
	}

	for (subelem = 0; subelem < MAX_WFD_SUBELEMS; subelem++) {
		if (subelements[subelem] == 0) {
			wpabuf_free(global->wfd_subelem[subelem]);
			global->wfd_subelem[subelem] = NULL;
		}
	}

	return wifi_display_update_wfd_ie(global);
}


int wifi_display_subelem_get(struct wpa_global *global, char *cmd,
			     char *buf, size_t buflen)
{
	int subelem;

	subelem = atoi(cmd);
	if (subelem < 0 || subelem >= MAX_WFD_SUBELEMS)
		return -1;

	if (global->wfd_subelem[subelem] == NULL)
		return 0;

	return wpa_snprintf_hex(buf, buflen,
				wpabuf_head_u8(global->wfd_subelem[subelem]) +
				1,
				wpabuf_len(global->wfd_subelem[subelem]) - 1);
}

int wifi_display_subelem_memcpy(struct wpa_global *global, int subelem,
			     u8 *buf, size_t buflen)
{
	if (subelem < 0 || subelem >= MAX_WFD_SUBELEMS)
		return -1;

	if (global->wfd_subelem[subelem] == NULL)
		return 0;

	if (buflen < wpabuf_len(global->wfd_subelem[subelem]))
		return -1;
	else
	{
		os_memcpy(buf, wpabuf_head_u8(global->wfd_subelem[subelem]), wpabuf_len(global->wfd_subelem[subelem]));
		return wpabuf_len(global->wfd_subelem[subelem]);
	}
}


char * wifi_display_subelem_hex(const struct wpabuf *wfd_subelems, u8 id)
{
	char *subelem = NULL;
	const u8 *buf;
	size_t buflen;
	size_t i = 0;
	u16 elen;

	if (!wfd_subelems)
		return NULL;

	buf = wpabuf_head_u8(wfd_subelems);
	if (!buf)
		return NULL;

	buflen = wpabuf_len(wfd_subelems);

	while (i + WIFI_DISPLAY_SUBELEM_HEADER_LEN < buflen) {
		elen = WPA_GET_BE16(buf + i + 1);
		if (i + WIFI_DISPLAY_SUBELEM_HEADER_LEN + elen > buflen)
			break; /* truncated subelement */

		if (buf[i] == id) {
			/*
			 * Limit explicitly to an arbitrary length to avoid
			 * unnecessarily large allocations. In practice, this
			 * is limited to maximum frame length anyway, so the
			 * maximum memory allocation here is not really that
			 * large. Anyway, the Wi-Fi Display subelements that
			 * are fetched with this function are even shorter.
			 */
			if (elen > 1000)
				break;
			subelem = os_zalloc(2 * elen + 1);
			if (!subelem)
				return NULL;
			wpa_snprintf_hex(subelem, 2 * elen + 1,
					 buf + i +
					 WIFI_DISPLAY_SUBELEM_HEADER_LEN,
					 elen);
			break;
		}

		i += elen + WIFI_DISPLAY_SUBELEM_HEADER_LEN;
	}

	return subelem;
}
#ifdef CONFIG_MTK_HE_SUPPORT
/* Get WFD Device Information Subelement - WFD Device Infomration field */
/* examples of the allinfo string: 00060011141c0064 (len(4B)+devInfo(4B)+CtrlPort(4B)+MaxTP(4B) */
int wifi_display_subelem_dev_info_get_dev_info_field(char *allinfo, int *val)
{
    char *ptr = NULL;
    char allinfo_tmp[32];

    os_memset(allinfo_tmp, 0, sizeof(allinfo_tmp));
    os_strncpy(allinfo_tmp, allinfo, sizeof(allinfo_tmp));
    ptr = allinfo_tmp;
    ptr += 4; /* get dev_info field byte 2&3 */
    *(ptr+4) = 0x00; /* dev_info is two bytes */
    sscanf(ptr, "%x", val);
    return 0;
}

/* Get WFD Device Information Subelement - Session Management Control Port field */
int wifi_display_subelem_dev_info_get_ctrl_port_field(char *allinfo, int *val)
{
    char *ptr = NULL;
    char allinfo_tmp[32];

    os_memset(allinfo_tmp, 0, sizeof(allinfo_tmp));
    os_strncpy(allinfo_tmp, allinfo, sizeof(allinfo_tmp));
    ptr = allinfo_tmp;
    ptr += 8; /* get ctrl_port field byte 2&3 */
    *(ptr+4) = 0x00; /* ctrl_port is two bytes */
    sscanf(ptr, "%x", val);
    return 0;
}

/* Get WFD Device Information Subelement - Device Max Throughput field */
int wifi_display_subelem_dev_info_get_max_throughput_field(char *allinfo, int *val)
{
    char *ptr = NULL;
    char allinfo_tmp[32];

    os_memset(allinfo_tmp, 0, sizeof(allinfo_tmp));
    os_strncpy(allinfo_tmp, allinfo, sizeof(allinfo_tmp));
    ptr = allinfo_tmp;
    ptr += 12; /* get max_throughput field byte 2&3 */
    *(ptr+4) = 0x00; /* max_throughput is two bytes */
    sscanf(ptr, "%x", val);
    return 0;
}


int wifi_display_set_wfd_dev_info(struct wpa_global *global, char *cmd, int val)
{
    char allinfo[32];
    int dev_info = 0, ctrl_port = 0, max_tput = 0;

    memset(allinfo, 0, sizeof(allinfo));
    /* get cuurent settings */
    wifi_display_subelem_get(global, "0", allinfo, sizeof(allinfo));
    wifi_display_subelem_dev_info_get_dev_info_field(allinfo, &dev_info);
    wifi_display_subelem_dev_info_get_ctrl_port_field(allinfo, &ctrl_port);
    wifi_display_subelem_dev_info_get_max_throughput_field(allinfo, &max_tput);
    
	wpa_printf(MSG_DEBUG, "WFD: set %s = %d", cmd, val);
    
    if (os_strcasecmp(cmd, "wfd_devType") == 0)
    {
        if (val >= WFD_DEV_INFO_DEV_TYPE_SOURE && val <= WFD_DEV_INFO_DEV_TYPE_DUAL)
            dev_info &= ~WFD_DEV_INFO_DEV_TYPE_DUAL;
            dev_info |= val;
    }
    else if (os_strcasecmp(cmd, "wfd_sourceCoup") == 0)
    {
            if (val)
                dev_info |= WFD_DEV_INFO_BITMASK_SO_COUPLE;
            else
                dev_info &= ~WFD_DEV_INFO_BITMASK_SO_COUPLE;
    }
    else if (os_strcasecmp(cmd, "wfd_sinkCoup") == 0)
    {
        if (val)
            dev_info |= WFD_DEV_INFO_BITMASK_SN_COUPLE;
        else
            dev_info &= ~WFD_DEV_INFO_BITMASK_SN_COUPLE;
    }
    else if (os_strcasecmp(cmd, "wfd_sessionAvail") == 0)
    {
        if (val)
        {
            dev_info |= WFD_DEV_INFO_BITMASK_SESS_AVAIL_BIT1;
            dev_info &= ~WFD_DEV_INFO_BITMASK_SESS_AVAIL_BIT2;
        }
        else
            dev_info &= ~WFD_DEV_INFO_BITMASK_SESS_AVAIL;
    }
    else if (os_strcasecmp(cmd, "wfd_contentProtect") == 0)
    {
        if (val)
            dev_info |= WFD_DEV_INFO_BITMASK_CP;
        else
            dev_info &= ~WFD_DEV_INFO_BITMASK_CP;
    }
    else if (os_strcasecmp(cmd, "wfd_preferredConnectivity") == 0)
    {
        if (val)
            dev_info |= WFD_DEV_INFO_BITMASK_PC;
        else
            dev_info &= ~WFD_DEV_INFO_BITMASK_PC;
    }
    else if (os_strcasecmp(cmd, "wfd_rtspPort") == 0)
    {
        ctrl_port = val;
    }
    else if (os_strcasecmp(cmd, "wfd_maxThroughput") == 0)
    {
        max_tput = val;
    }
    else
    {
        wpa_printf(MSG_DEBUG, "WFD: unknown cmd:%s", cmd);
        return -1;
    }

    memset(allinfo, 0, sizeof(allinfo));
    os_snprintf(allinfo, sizeof(allinfo)-1, "0 %04x%04x%04x%04x", 
                6, dev_info, ctrl_port, max_tput);
	wpa_printf(MSG_DEBUG, "wifi_display_subelem_set=[%s]", allinfo);

    return wifi_display_subelem_set(global, allinfo);
}

int wifi_display_set_wfd_bssid(struct wpa_global *global, char *cmd, char *bssid)
{
	if(!bssid || strlen(bssid) != 12)
	{
		wpa_printf(MSG_DEBUG, "wifi_display_set_wfd_bssid bssid parameter error");
		return -1;
	}
	wpa_printf(MSG_DEBUG, "WFD: set %s = %s", cmd, bssid);
	
    if (os_strcasecmp(cmd, "wfd_bssid") == 0)
    {
    	char allinfo[32];
		memset(allinfo, 0, sizeof(allinfo));
		os_snprintf(allinfo, sizeof(allinfo)-1, "1 %04x%s", 
					6, bssid);
		wpa_printf(MSG_DEBUG, "wifi_display_subelem_set=[%s]", allinfo);
		
		return wifi_display_subelem_set(global, allinfo);
    }
	else
	{
        wpa_printf(MSG_DEBUG, "WFD: unknown cmd:%s", cmd);
        return -1;
	}
}

#endif /* CONFIG_MTK_HE_SUPPORT */
