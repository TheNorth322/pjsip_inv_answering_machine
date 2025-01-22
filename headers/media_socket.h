#ifndef _MEDIA_SOCKET_H_
#define _MEDIA_SOCKET_H_

#include <pj/hash.h>
#include <pj/types.h>
#include <pjlib-util.h>
#include <pjlib.h>
#include <pjmedia-codec.h>
#include <pjmedia.h>
#include <pjsip.h>
#include <pjsip_simple.h>
#include <pjsip_ua.h>
#include <stdio.h>

struct media_socket_t
{
    pjmedia_transport_info med_tpinfo;
    pjmedia_transport *med_transport;
    pjmedia_sock_info sock_info;

    pj_bool_t occupied;
};

pj_status_t media_socket_create(
    pj_pool_t *pool, pjmedia_endpt *endpt, const pj_uint16_t af, const int rtp_port, struct media_socket_t **socket);

void media_socket_free(struct media_socket_t *socket);

#endif  // !_MEDIA_SOCKET_H_
