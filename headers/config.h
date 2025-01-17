#ifndef _CONFIG_H_
#define _CONFIG_H_

#define RINGING_TIME        3
#define MEDIA_SESSION_TIME  10

#define CHANNEL_COUNT       1
#define PORT_COUNT          255
#define MAX_URI             16
#define PORTS               16
#define CALLS               255

/* Constants */
#define CLOCK_RATE          44100
#define NSAMPLES            (CLOCK_RATE * 20 / 1000)
#define NCHANNELS           1
#define NBITS               16

/* First tone */
#define LONG_TONE_FREQUENCY 425

/* Second audio message */
#define WAV_FILE            "../etc/example3.wav"
#define WAV_BITRATE         64000
#define WAV_FREQUENCY       8000
#define PTIME               20

/* Third tone */
#define RBT_FREQUENCY       425
#define RBT_ON_MSEC         1000
#define RBT_OFF_MSEC        4000

#endif // !_CONFIG_H_
