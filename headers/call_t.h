#ifndef _CALL_T_H_
#define _CALL_T_H_

#include <pjsip.h>
#include <pjmedia.h>
#include <pjmedia-codec.h>
#include <pjsip_ua.h>
#include <pjsip_simple.h>
#include <pjlib-util.h>
#include <pjlib.h>

struct call_t {
    pjsip_inv_session    *g_inv;         /* Current invite session.  */
    pjmedia_stream       *g_med_stream;  /* Call's audio stream.     */
    pjmedia_snd_port     *g_snd_port;    /* Sound device.            */
    pj_pool_t            *pool;
};

void free_call(struct call_t *call); 

#endif // !_CALL_T_H
