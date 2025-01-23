#ifndef _ANSWERING_MACHINE_H_
#define _ANSWERING_MACHINE_H_

#include <pj/hash.h>
#include <pj/types.h>
#include <pjlib-util.h>
#include <pjlib.h>
#include <pjmedia-codec.h>
#include <pjmedia.h>
#include <pjmedia/conference.h>
#include <pjsip.h>
#include <pjsip_simple.h>
#include <pjsip_ua.h>
#include <stdio.h>
#include <pjmedia/conference.h>
#include <pjmedia/frame.h>
#include <pjmedia/port.h>

#include "call.h"
#include "config.h"
#include "media_socket.h"
#include "util.h"

#define AF pj_AF_INET()
#define SIP_PORT 6222 /* Listening SIP port              */
#define RTP_PORT 4000 /* RTP port                        */

#define POOL_SIZE 4000
#define POOL_INC 4000
#define THIS_FILE "answering_machine.c"
#define MAX_MEDIA_CNT 29 /* Media count, set to 1 for audio */

#define MAX_CALLS 35

struct answering_machine_t
{
    pjsip_endpoint *g_endpt; /* SIP endpoint.            */

    pj_caching_pool *cp; /* Global pool factory.     */
    pj_pool_t *pool;
    pj_pool_t *media_pool;

    pjmedia_endpt *g_med_endpt; /* Media endpoint.          */

    struct call_t **calls;
    struct media_socket_t **med_sockets;
    pj_hash_table_t *table;

    pjmedia_sock_info g_sock_info[MAX_MEDIA_CNT];

    pjmedia_conf *conf;

    pjsip_module mod_simpleua;

    pjsip_module msg_logger;

    int calls_count;
    int calls_capacity;
};

pj_status_t answering_machine_create(pj_pool_t **pool);

void answering_machine_signal_add(pjmedia_port *signal, char *username);

void answering_machine_calls_recv();

#endif  // !_ANSWERING_MACHINE_H_
