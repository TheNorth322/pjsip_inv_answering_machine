#include "../headers/call.h"

pj_status_t create_call(pj_pool_t *pool, pj_str_t call_id, struct call_t **call)
{
    (*call) = (struct call_t *)pj_pool_alloc(pool, sizeof(**call));
    if (!call)
    {
        return -1;
    }

    (*call)->pool = pool;
    (*call)->call_id = call_id;
    (*call)->snd_port = NULL;
    (*call)->med_stream = NULL;

    /* Timers */
    (*call)->ringing_timer = (pj_timer_entry *)pj_pool_alloc(pool, sizeof(pj_timer_entry));
    (*call)->media_session_timer = (pj_timer_entry *)pj_pool_alloc(pool, sizeof(pj_timer_entry));

    /* Time values */
    (*call)->ringing_time.sec = RINGING_TIME;
    (*call)->ringing_time.msec = 0;

    (*call)->media_session_time.sec = MEDIA_SESSION_TIME;
    (*call)->media_session_time.msec = 0;

    return PJ_SUCCESS;
}

void free_call(struct call_t *call)
{
    call->socket->occupied = PJ_FALSE;

    if (call->snd_port)
        pjmedia_snd_port_destroy(call->snd_port);

    // if (call->med_stream)
    // pjmedia_stream_destroy(call->med_stream);
}
