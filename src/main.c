#include "../headers/answering_machine_t.h"
#include <pj/types.h>
#include <pjmedia/port.h>

int main(void)
{
    struct answering_machine_t machine;
    pj_status_t status;
    pjmedia_port* long_tone_port;
    pjmedia_port* wav_port;
    pjmedia_port* rbt_port;

    answering_machine_create(&machine);
    
    status = pjmedia_tonegen_create(machine.pool, 8000, CHANNEL_COUNT, 64, 16, PJMEDIA_TONEGEN_LOOP, &long_tone_port);
    if (status != PJ_SUCCESS) {
        app_perror("main", "tonegen not created", status);
    }
    /* Init long tone */
    {
        pjmedia_tone_desc tones[1];

        tones[0].freq1 = LONG_TONE_FREQUENCY;
        tones[0].freq2 = 0;
        tones[0].on_msec = -1;
        tones[0].off_msec = 0;
        tones[0].volume = PJMEDIA_TONEGEN_VOLUME;

        status = pjmedia_tonegen_play(long_tone_port, 1, tones, 0);
        if (status != PJ_SUCCESS) {
            app_perror("main", "Error in playing tonegen", status);
        }
    }


    /* Create wav player */
    status = pjmedia_wav_player_port_create(machine.pool, WAV_FILE, PTIME, 0, 0, &wav_port);
    if (status != PJ_SUCCESS) {
        app_perror("main", "Error in creating wav", status);
    }

    /* Create rbt tonegen */
    status = pjmedia_tonegen_create(machine.pool, 8000, CHANNEL_COUNT, 64, 16, PJMEDIA_TONEGEN_LOOP, &rbt_port);
    if (status != PJ_SUCCESS) {
        app_perror("main", "Error in creating tonegen", status);
    }

    /* Init RBT tone */
    {
        pjmedia_tone_desc tones[1];
        
        tones[0].freq1 = LONG_TONE_FREQUENCY;
        tones[0].freq2 = 0;
        tones[0].on_msec = RBT_ON_MSEC;
        tones[0].off_msec = RBT_OFF_MSEC;
        tones[0].volume = PJMEDIA_TONEGEN_VOLUME;

        status = pjmedia_tonegen_play(rbt_port, 1, tones, 0);
        if (status != PJ_SUCCESS) {
            app_perror("main", "Error in playing tonegen", status);
        }
    }

    answering_machine_calls_recv();  
    
    return 0;
}
