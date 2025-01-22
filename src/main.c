#include "../headers/answering_machine.h"
#include "../headers/signals.h"

int main(void)
{
    pjmedia_port *longtone;
    pjmedia_port *wav;
    pjmedia_port *rbt;
    pj_pool_t *pool;

    answering_machine_create(&pool);

    get_longtone_signal(pool, &longtone);
    get_wav_signal(pool, &wav);
    get_rbt_signal(pool, &rbt);

    answering_machine_signal_add(longtone, "longtone");
    answering_machine_signal_add(wav, "wav");
    answering_machine_signal_add(rbt, "rbt");

    answering_machine_calls_recv();

    return 0;
}
