#include "common/config.h"

#include <alsa/asoundlib.h>
#include <zlog.h>

#define AUDIO_CHANNELS      2
#define AUDIO_RATE          16000

static snd_pcm_t *pcm = NULL;
static zlog_category_t *log = NULL;

void audio_play_init(void)
{
    int err;
    log = zlog_get_category("Play");
    if(log == NULL) {
        zlog_error(comConfig.log, "Play log get failed");
        exit(EXIT_FAILURE);
    }
    
    err = snd_pcm_open(&pcm, "hw:0,0", SND_PCM_STREAM_PLAYBACK, 0);
    if(err < 0) {
        zlog_error(log, "open pcm failed: %s", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    err = snd_pcm_set_params(pcm, SND_PCM_FORMAT_S16_LE, 
        SND_PCM_ACCESS_RW_INTERLEAVED,
        AUDIO_CHANNELS, AUDIO_RATE,
        1, 100000);
    if(err < 0) {
        zlog_error(log, "pcm set params failed: %s", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
}

void audio_play_cleanup(void)
{
    if(pcm != NULL) {
        snd_pcm_close(pcm);
        pcm = NULL;
    }
}

void audio_play_service(const char *data, size_t len)
{
    int err = snd_pcm_prepare(pcm);
    if(err < 0) {
        zlog_error(log, "pcm prepare failed: %s", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    char *samples = malloc(len * 2);
    if(samples == NULL) {
        zlog_error(log, "play malloc failed");
        exit(EXIT_FAILURE);
    }
    
    size_t single_frames = len / 2;
    for(size_t i = 0; i < single_frames; i++) {
        memcpy(samples + i * 4, data + i * 2, 2);
        memcpy(samples + i * 4 + 2, data + i * 2, 2);
    }

    snd_pcm_writei(pcm, samples, single_frames);

    snd_pcm_drop(pcm);
    free(samples);
}