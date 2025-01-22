#ifndef _CALL_T_H_
#define _CALL_T_H_

#include <pj/timer.h>
#include <pjsip.h>
#include <pjmedia.h>
#include <pjmedia-codec.h>
#include <pjsip_ua.h>
#include <pjsip_simple.h>
#include <pjlib-util.h>
#include <pjlib.h>

#include "config.h"

struct call_t {
    pj_str_t             call_id; 
    pjsip_inv_session    *inv;         /* Current invite session.  */
    pjmedia_stream       *med_stream;  /* Call's audio stream.     */
    pjmedia_snd_port     *snd_port;    /* Sound device.            */
    pj_pool_t            *pool;

    pj_timer_entry *ringing_timer;
    pj_timer_entry *media_session_timer;

    pj_time_val ringing_time;
    pj_time_val media_session_time;
};

pj_status_t create_call(pj_pool_t* pool, pj_str_t call_id,  struct call_t** call);

void free_call(struct call_t *call); 

#endif // !_CALL_T_H
