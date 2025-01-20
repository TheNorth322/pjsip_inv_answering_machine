#ifndef _ANSWERING_MACHINE_T_H_
#define _ANSWERING_MACHINE_T_H_

#include <pj/types.h>
#include <pjsip.h>
#include <pjmedia.h>
#include <pjmedia-codec.h>
#include <pjsip_ua.h>
#include <pjsip_simple.h>
#include <pjlib-util.h>
#include <pjlib.h>
#include <stdio.h>
#include <pj/hash.h>

#include "util.h"
#include "call_t.h"
#include "config.h"

#define AF              pj_AF_INET() 
#define SIP_PORT        6222         /* Listening SIP port              */
#define RTP_PORT        4000         /* RTP port                        */

#define POOL_SIZE       4000
#define POOL_INC        4000
#define THIS_FILE       "answering_machine_t.c"
#define MAX_MEDIA_CNT   1            /* Media count, set to 1 for audio */

#define MAX_CALLS 25

struct answering_machine_t 
{
    pjsip_endpoint       *g_endpt;       /* SIP endpoint.            */
    pj_caching_pool      *cp;            /* Global pool factory.     */
    pj_pool_t            *pool;
    pj_pool_t            *media_pool;
    pjmedia_endpt        *g_med_endpt;   /* Media endpoint.          */
    pj_hash_table_t      *table;
    struct call_t**       calls;
    
    int calls_count;
    int calls_capacity;

    pjmedia_transport_info g_med_tpinfo[MAX_MEDIA_CNT]; 
                                                /* Socket info for media    */
    pjmedia_transport    *g_med_transport[MAX_MEDIA_CNT];
                                                /* Media stream transport   */
    pjmedia_sock_info     g_sock_info[MAX_MEDIA_CNT];  

    pjsip_module mod_simpleua;

    pjsip_module msg_logger; 
};

pj_status_t answering_machine_create();

pj_status_t answering_machine_ua_module_init(pjsip_module *module);

pj_status_t answering_machine_logger_module_init(pjsip_module *module);

pj_status_t answering_machine_global_endpt_init();

pj_status_t answering_machine_transport_init();

pj_status_t answering_machine_invite_module_init();

pj_status_t answering_machine_media_endpt_init();

pj_status_t answering_machine_media_transport_create();

pj_status_t answering_machine_add_call(struct call_t* call);

pj_status_t answering_machine_find_call(const pj_str_t* dlg_id, struct call_t** call);

pj_status_t answering_machine_delete_call(const pj_str_t* dlg_id);

void answering_machine_calls_recv();

void answering_machine_free(struct answering_machine_t* machine_ptr);

/* Notification on incoming messages */
static pj_bool_t logging_on_rx_msg(pjsip_rx_data *rdata);

/* Notification on outgoing messages */
static pj_status_t logging_on_tx_msg(pjsip_tx_data *tdata);

/* Callback to be called when SDP negotiation is done in the call: */
static void call_on_media_update(pjsip_inv_session *inv,
                                 pj_status_t status);

/* Callback to be called when invite session's state has changed: */
static void call_on_state_changed(pjsip_inv_session *inv, 
                                  pjsip_event *e);

/* Callback to be called when dialog has forked: */
static void call_on_forked(pjsip_inv_session *inv, pjsip_event *e);

static void on_ringing_timer_callback(pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry);

static void on_media_state_timer_callback(pj_timer_heap_t *timer_heap, struct pj_timer_entry *entry);

/* Callback to be called to handle incoming requests outside dialogs: */
static pj_bool_t on_rx_request(pjsip_rx_data *rdata );

#endif // !_ANSWERING_MACHINE_T_H_
