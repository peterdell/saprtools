#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <unistd.h>
#include "sidengine.h"

//---------------------------------------SOUNDCARD-VARS--------------
//
#define DEVICE_NAME "/dev/dsp"

#define NSAMPLES 882                    // 44100/50 = 882
static int audio_fd;
static uint16_t soundbuffer[NSAMPLES*2];

//---------------------------------------SID-VARS--------------------
//
static uint16_t init_addr, play_addr;
static uint8_t actual_subsong, max_subsong, speed100Hz;
static char song_name[32], song_author[32], song_copyright[32];

int is_little_endian(void) {
    static const unsigned long a = 1;

    return *(const unsigned char *)&a;

}

void soundcard_init(void) {
    int format, format_backup, stereo, speed;

    format_backup = format = is_little_endian()? AFMT_S16_LE : AFMT_S16_BE;
    stereo = 0;
    speed = 44100;

    if ((audio_fd = open(DEVICE_NAME, O_WRONLY)) == -1) {
        perror(DEVICE_NAME);
        return;
    }
    if (ioctl(audio_fd, SNDCTL_DSP_SETFMT, &format) == -1) {
        perror("SNDCTL_DSP_SETFMT");
        close(audio_fd);
        return;
    }
    if (format != format_backup) {
        perror("FORMAT NOT SUPPORTED!");
        close(audio_fd);
        return;
    }
    if (ioctl(audio_fd, SNDCTL_DSP_STEREO, &stereo) == -1) {
        perror("SNDCTL_DSP_STEREO");
        close(audio_fd);
        return;
    }
    if (stereo != 0) {
        perror("CHANNEL MODE NOT SUPPORTED!");
        close(audio_fd);
        return;
    }
    if (ioctl(audio_fd, SNDCTL_DSP_SPEED, &speed) == -1) {
        perror("SNDCTL_DSP_SPEED");
        close(audio_fd);
        return;
    }
}

void show_info(int argc) {
    fprintf(stderr,
        "TinySID v0.94 Copyright (C) 1999-2006 T. Hinrichs and R. Sinsch.\n"
        "Usage: tinysid [sid-file]\n"
        );
}
int main(int argc, char *argv[]) {
    if (argc < 2) {
        show_info(argc);
        return 1;
    }

    c64Init();
    synth_init(44100);
    soundcard_init();

    if (c64SidLoad
        (argv[1], &init_addr, &play_addr, &actual_subsong, &max_subsong,
         &speed100Hz, song_name, song_author, song_copyright) == 0) {
        fprintf(stderr, "Failed to load %s\n", argv[1]);
        return 1;
    }

    printf("Title     : %s\n", song_name);
    printf("Author    : %s\n", song_author);
    printf("Copyright : %s\n", song_copyright);
    printf("Speed     : %s\n", speed100Hz ? "100Hz" : "50Hz");
    printf("Playing... Press ctrl-C to quit.\n");

    cpuJSR(init_addr, actual_subsong);

    while (1) {
        if (!speed100Hz) {
                cpuJSR(play_addr, 0);
                synth_render(soundbuffer, NSAMPLES);
        } else {
                cpuJSR(play_addr, 0);
                synth_render(soundbuffer, NSAMPLES/2);
                cpuJSR(play_addr, 0);
                synth_render(soundbuffer + (NSAMPLES/2*2), NSAMPLES/2);
        }

        int ret = write(audio_fd, soundbuffer, NSAMPLES*2);

        if (ret < 0) {
            fprintf(stderr, "error writing to audio device!\n");
            return 1;
        }
    }

    return 1;
}
