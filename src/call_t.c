#include "../headers/call_t.h"

void free_call(struct call_t *call) {
    if (call->g_snd_port)
        pjmedia_snd_port_destroy(call->g_snd_port);

    if (call->g_med_stream)
        pjmedia_stream_destroy(call->g_med_stream);
}
