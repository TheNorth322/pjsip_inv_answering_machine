#include "../headers/signals.h"

pj_status_t signals_longtone_get(pj_pool_t *pool, pjmedia_port **port)
{
    pj_status_t status;

    /* Create long tonegen */
    status = pjmedia_tonegen_create(pool, 
                                    CLOCK_RATE, 
                                    CHANNEL_COUNT, 
                                    SAMPLES_PER_FRAME, 
                                    BITS_PER_SAMPLE, 
                                    PJMEDIA_TONEGEN_LOOP, 
                                    port);
    if (status != PJ_SUCCESS)
    {
        return status;
    }

    /* Init long tone */
    {
        pjmedia_tone_desc tones[1];

        tones[0].freq1 = LONG_TONE_FREQUENCY;
        tones[0].freq2 = 0;
        tones[0].on_msec = -1;
        tones[0].off_msec = 0;
        tones[0].volume = PJMEDIA_TONEGEN_VOLUME;

        status = pjmedia_tonegen_play(*port, 1, tones, 0);
    }

    return status;
}

pj_status_t signals_wav_get(pj_pool_t *pool, pjmedia_port **port)
{
    pj_status_t status;

    /* Create wav player */
    status = pjmedia_wav_player_port_create(pool, WAV_FILE, PTIME, 0, 0, port);

    return status;
}

pj_status_t signals_rbt_get(pj_pool_t *pool, pjmedia_port **port)
{
    pj_status_t status;

    /* Create rbt tonegen */
    status = pjmedia_tonegen_create(pool, 
                                    CLOCK_RATE, 
                                    CHANNEL_COUNT, 
                                    SAMPLES_PER_FRAME, 
                                    BITS_PER_SAMPLE, 
                                    PJMEDIA_TONEGEN_LOOP, 
                                    port);
    if (status != PJ_SUCCESS)
    {
        return status;
    }

    /* Init RBT tone */
    {
        pjmedia_tone_desc tones[1];

        tones[0].freq1 = LONG_TONE_FREQUENCY;
        tones[0].freq2 = 0;
        tones[0].on_msec = RBT_ON_MSEC;
        tones[0].off_msec = RBT_OFF_MSEC;
        tones[0].volume = PJMEDIA_TONEGEN_VOLUME;

        status = pjmedia_tonegen_play(*port, 1, tones, 0);
    }

    return status;
}
