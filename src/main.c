#include "../headers/answering_machine.h"
#include "../headers/signals.h"

int main(void)
{
    pjmedia_port *longtone;
    pjmedia_port *wav;
    pjmedia_port *rbt;
    pj_pool_t *pool;
    
    answering_machine_create(&pool);
    
    /* Create signals */
    signals_longtone_get(pool, &longtone);
    signals_wav_get(pool, &wav);
    signals_rbt_get(pool, &rbt);
    
    /* Add signals to answering machine */
    answering_machine_signal_add(longtone, "longtone");
    answering_machine_signal_add(wav, "wav");
    answering_machine_signal_add(rbt, "rbt");

    answering_machine_calls_recv();

    return 0;
}
