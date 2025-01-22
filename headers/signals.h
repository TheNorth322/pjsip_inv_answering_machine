#ifndef _SIGNALS_H_
#define _SIGNALS_H_

#include <pjmedia.h>

/* First tone */
#define LONG_TONE_FREQUENCY 425

/* Second audio message */
#define WAV_FILE "../etc/example3.wav"
#define WAV_BITRATE 64000
#define WAV_FREQUENCY 8000
#define PTIME 20

/* Third tone */
#define RBT_FREQUENCY 425
#define RBT_ON_MSEC 1000
#define RBT_OFF_MSEC 4000

#define CHANNEL_COUNT 1

pj_status_t get_longtone_signal(pj_pool_t *pool, pjmedia_port **port);

pj_status_t get_wav_signal(pj_pool_t *pool, pjmedia_port **port);

pj_status_t get_rbt_signal(pj_pool_t *pool, pjmedia_port **port);

#endif  // !_SIGNALS_H_
