/* *
 * This file is part of Feng
 *
 * Copyright (C) 2009 by LScube team <team@lscube.org>
 * See AUTHORS for more details
 *
 * feng is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * feng is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with feng; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * */

/**
 * @file
 * @brief SDP session description
 *
 * This file contains the function used to generate the SDP session
 * description as needed by @ref RTSP_describe.
 */

#include <string.h>
#include <stdbool.h>

#include "fnc_log.h"
#include "sdp2.h"
#include "mediathread/description.h"
#include <netembryo/wsocket.h>
#include <netembryo/url.h>

#define SDP2_EL "\r\n"
#define DEFAULT_TTL 32

#define NTP_time(t) ((float)t + 2208988800U)

/**
 * @brief Simple pair for compound parameters in foreach functions
 *
 * This data structure is used to pass parameters along with foreach
 * functions, like @ref sdp_mdescr_private_append.
 */
typedef struct {
    /** The string to append the SDP2 description to */
    GString *descr;
    /** The currently-described media description object */
    MediaDescr *media;
} sdp_mdescr_append_pair;

/**
 * @brief Append media private field information to an SDP2 string
 *
 * @param element An sdp_field object in the list
 * @param user_data An sdp_mdescr_append_pair object
 *
 * @internal This function is only to be called by g_list_foreach().
 */
static void sdp_mdescr_private_append(gpointer element, gpointer user_data)
{
    sdp_field *private = (sdp_field *)element;
    sdp_mdescr_append_pair *pair = (sdp_mdescr_append_pair *)user_data;

    switch (private->type) {
    case empty_field:
        g_string_append_printf(pair->descr, "%s"SDP2_EL,
                               private->field);
        break;
    case fmtp:
        g_string_append_printf(pair->descr, "a=fmtp:%u %s"SDP2_EL,
                               m_descr_rtp_pt(pair->media),
                               private->field);
        break;
    case rtpmap:
        g_string_append_printf(pair->descr, "a=rtpmap:%u %s"SDP2_EL,
                               m_descr_rtp_pt(pair->media),
                               private->field);
        break;
    default: /* Ignore other private fields */
        break;
    }
}

/**
 * @brief Append all the private fields for a media description to an
 *        SDP2 string.
 *
 * @param element The media description to append the fields of
 * @param user_data The GString object to append the description to
 *
 * @internal This function is only to be called by g_list_foreach().
 */
static void sdp_mdescr_private_list_append(gpointer element, gpointer user_data)
{
    sdp_mdescr_append_pair pair = {
        .descr = (GString *)user_data,
        .media = (MediaDescr *)element
    };

    g_list_foreach(m_descr_sdp_private(pair.media),
                   sdp_mdescr_private_append,
                   &pair);
}

/**
 * @brief Append the payload type for the media description to an SDP2
 *        string.
 *
 * @param element The media description to append the fields of
 * @param user_data The GString object to append the description to
 *
 * @internal This function is only to be called by g_list_foreach().
 */
static void sdp_mdescr_pt_append(gpointer element, gpointer user_data)
{
    MediaDescr *mdescr = (MediaDescr *)element;
    GString *descr = (GString *)user_data;

    g_string_append_printf(descr, " %u",
                           m_descr_rtp_pt(mdescr));
}

/**
 * @brief Append the description for a given media to an SDP
 *        description.
 *
 * @param element List of MediaDescr instances to fetch the data
 *                     from.
 * @param user_data GString instance to append the description to
 *
 * @internal This function is only to be called by g_ptr_array_foreach().
 */
static void sdp_media_descr(gpointer element, gpointer user_data)
{
    MediaDescrList m_descr_list = (MediaDescrList)element;
    GString *descr = (GString *)user_data;
    MediaDescr *m_descr = m_descr_list ? (MediaDescr *)m_descr_list->data : NULL;
    char *encoded_media_name;

    /* The following variables are used to read the data out of the
     * m_descr pointer, without calling the same inline function
     * twice.
     */
    MediaType type;
    float frame_rate;
    const char *commons_deed;
    const char *rdf_page;
    const char *title;
    const char *author;

    if (!m_descr)
        return;

    // m=
    /// @TODO Convert this to a string table
    switch ( (type = m_descr_type(m_descr)) ) {
    case MP_audio:
        g_string_append(descr, "m=audio ");
        break;
    case MP_video:
        g_string_append(descr, "m=video ");
        break;
    case MP_application:
        g_string_append(descr, "m=application ");
        break;
    case MP_data:
        g_string_append(descr, "m=data ");
        break;
    case MP_control:
        g_string_append(descr, "m=control ");
        break;
    }

    /// @TODO shawill: probably the transport should not be hard coded,
    /// but obtained in some way
    g_string_append_printf(descr, "%d RTP/AVP",
                           m_descr_rtp_port(m_descr));

    g_list_foreach(m_descr_list,
                   sdp_mdescr_pt_append,
                   descr);

    g_string_append(descr, SDP2_EL);

    // i=*
    // c=*
    // b=*
    // k=*
    // a=*
    encoded_media_name = g_uri_escape_string(m_descr_name(m_descr), NULL, false);

    g_string_append_printf(descr, "a=control:TrackID=%s"SDP2_EL,
                           encoded_media_name);
    g_free(encoded_media_name);

    if ( (frame_rate = m_descr_frame_rate(m_descr))
         && type == MP_video)
        g_string_append_printf(descr, "a=framerate:%f"SDP2_EL,
                               frame_rate);

    g_list_foreach(m_descr_list,
                    sdp_mdescr_private_list_append,
                    descr);

    // CC licenses *
    if ( (commons_deed = m_descr_commons_deed(m_descr)) )
        g_string_append_printf(descr, "a=uriLicense:%s"SDP2_EL,
                               commons_deed);
    if ( (rdf_page = m_descr_rdf_page(m_descr)) )
        g_string_append_printf(descr, "a=uriMetadata:%s"SDP2_EL,
                               rdf_page);
    if ( (title = m_descr_title(m_descr)) )
        g_string_append_printf(descr, "a=title:%s"SDP2_EL,
                               title);
    if ( (author = m_descr_author(m_descr)) )
        g_string_append_printf(descr, "a=author:%s"SDP2_EL,
                               author);
}

/**
 * @brief Append resource private field information to an SDP2 string
 *
 * @param element An sdp_field object in the list
 * @param user_data The GString object to append the data to
 *
 * @internal This function is only to be called by g_list_foreach().
 */
static void sdp_rdescr_private_append(gpointer element, gpointer user_data)
{
    sdp_field *private = (sdp_field *)element;
    GString *descr = (GString *)user_data;

    switch (private->type) {
    case empty_field:
        g_string_append_printf(descr, "%s"SDP2_EL,
                               private->field);
        break;

    default: /* Ignore other private fields */
        break;
    }
}

/**
 * @brief Create description for an SDP session
 *
 * @param srv Pointer to the server-specific data instance.
 * @param url Url of the resource to describe
 *
 * @return A new GString containing the complete description of the
 *         session or NULL if the resource was not found or no demuxer
 *         was found to handle it.
 */
GString *sdp_session_descr(struct feng *srv, const Url *url)
{
    GString *descr = NULL;
    ResourceDescr *r_descr;
    double duration;

    const char *resname;
    time_t restime;
    float currtime_float, restime_float;

    fnc_log(FNC_LOG_DEBUG, "[SDP2] opening %s", url->path);
    if ( !(r_descr=r_descr_get(srv, url->path)) ) {
        fnc_log(FNC_LOG_ERR, "[SDP2] %s not found", url->path);
        return NULL;
    }

    descr = g_string_new("v=0"SDP2_EL);

    /* Near enough approximation to run it now */
    currtime_float = NTP_time(time(NULL));
    restime = r_descr_last_change(r_descr);
    restime_float = restime ? NTP_time(restime) : currtime_float;

    if ( (resname = r_descr_name(r_descr)) == NULL )
        resname = "RTSP Session";

    /* Network type: Internet; Address type: IP4. */
    g_string_append_printf(descr, "o=- %.0f %.0f IN IP4 %s"SDP2_EL,
                           currtime_float, restime_float, url->hostname);

    g_string_append_printf(descr, "s=%s"SDP2_EL,
                           resname);
    // u=
    if (r_descr_descrURI(r_descr))
        g_string_append_printf(descr, "u=%s"SDP2_EL,
                               r_descr_descrURI(r_descr));

    // e=
    if (r_descr_email(r_descr))
        g_string_append_printf(descr, "e=%s"SDP2_EL,
                               r_descr_email(r_descr));
    // p=
    if (r_descr_phone(r_descr))
        g_string_append_printf(descr, "p=%s"SDP2_EL,
                               r_descr_phone(r_descr));

    // c=
    /* Network type: Internet. */
    /* Address type: IP4. */
    g_string_append(descr, "c=IN IP4 ");

    if(r_descr_multicast(r_descr)) {
        g_string_append_printf(descr, "%s/",
                               r_descr_multicast(r_descr));
        if (r_descr_ttl(r_descr))
            g_string_append_printf(descr, "%s"SDP2_EL,
                                   r_descr_ttl(r_descr));
        else
            /* @TODO the possibility to change ttl.
             * See multicast.h, RTSP_setup.c, send_setup_reply.c*/
            g_string_append_printf(descr, "%d"SDP2_EL,
                                   DEFAULT_TTL);
    } else
        g_string_append(descr, "0.0.0.0"SDP2_EL);

    // b=
    // t=
    g_string_append(descr, "t=0 0"SDP2_EL);
    // r=
    // z=
    // k=
    // a=
    // type attribute. We offer only broadcast
    g_string_append(descr, "a=type:broadcast"SDP2_EL);
    // tool attribute. Feng promo
    /// @TODO choose a better session description
    g_string_append_printf(descr, "a=tool:%s %s Streaming Server"SDP2_EL,
                           PACKAGE, VERSION);
    // control attribute. We should look if aggregate metod is supported?
    g_string_append(descr, "a=control:*"SDP2_EL);

    if ((duration = r_descr_time(r_descr)) > 0)
        g_string_append_printf(descr, "a=range:npt=0-%f"SDP2_EL, duration);

    // other private data
    g_list_foreach(r_descr_sdp_private(r_descr),
                   sdp_rdescr_private_append,
                   descr);

    g_ptr_array_foreach(r_descr_get_media(r_descr),
                        sdp_media_descr,
                        descr);

    fnc_log(FNC_LOG_INFO, "[SDP2] description:\n%s", descr->str);

    return descr;
}