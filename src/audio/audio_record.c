#include "common/config.h"
#include "audio/audio_record.h"

#include <zlog.h>
#include <math.h>
#include <alsa/asoundlib.h>
#include <stdbool.h>

#define AUDIO_CHANNELS      2
#define AUDIO_RATE          16000

static zlog_category_t *log_ = NULL;
static snd_pcm_t *pcm = NULL;

#define BUF_SIZE            (AUDIO_RATE * AUDIO_CHANNELS / 50)      //20ms
static short record_buf[BUF_SIZE];

#define MAVHEADER_SIZE      44
static void write_4Bytes(char *addr, uint32_t num)
{
    addr[0] = num;
    addr[1] = (num >> 8);
    addr[2] = (num >> 16);
    addr[3] = (num >> 24);
}

static void write_2Bytes(char *addr, uint16_t num)
{
    addr[0] = num;
    addr[1] = (num >> 8);
}

static void audio_add_mavheader(char *data, size_t data_size)
{
    memcpy(data, "RIFF", 4);
    write_4Bytes(data + 4, data_size + 36);
    memcpy(data + 8, "WAVE", 4);
    memcpy(data + 12, "fmt ", 4);
    write_4Bytes(data + 16, 16);
    write_2Bytes(data + 20, 1);
    write_2Bytes(data + 22, AUDIO_CHANNELS);
    write_4Bytes(data + 24, AUDIO_RATE);
    write_4Bytes(data + 28, AUDIO_RATE * AUDIO_CHANNELS * 2);
    write_2Bytes(data + 32, 4);
    write_2Bytes(data + 34, 16);
    memcpy(data + 36, "data", 4);
    write_4Bytes(data + 40, data_size);
}

static void set_right_gain(long gain)
{
    snd_ctl_t *ctl = NULL;
    snd_ctl_elem_value_t *value = NULL;

    int err = snd_ctl_open(&ctl, "hw:0", 0);
    if(err < 0) {
        zlog_error(log_, "open ctl failed: %s", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    snd_ctl_elem_value_alloca(&value);
    snd_ctl_elem_value_set_interface(value, SND_CTL_ELEM_IFACE_MIXER);
    snd_ctl_elem_value_set_name(value, "Right Channel Capture Volume");
    snd_ctl_elem_value_set_integer(value, 0, gain);
    err = snd_ctl_elem_write(ctl, value);
    if (err < 0) {
        zlog_error(log_, "open elem write failed: %s", snd_strerror(err));
        exit(EXIT_FAILURE);  
    }
    snd_ctl_close(ctl);
}

static void audio_record_copy2left(long frames)
{
    for(long i = 0; i < frames; i++) {
        record_buf[i * 2] = record_buf[i * 2 + 1];
    }
}

#define RMS_MIN     50
#define RMS_MAX     200
static double get_rms(long frames)
{
    double sum = 0;
    for(int i = 0; i < frames; i++) {
        double x = record_buf[i] / 32768.0;
        sum += x * x;
    }
    double rms = sqrt(sum / frames) * 32768.0;
    return rms;
}

void audio_record_init(void)
{
    int err;

    log_ = zlog_get_category("Record");
    if(log_ == NULL) {
        zlog_error(comConfig.log, "Record log get failed");
        exit(EXIT_FAILURE);
    }

    err = snd_pcm_open(&pcm, "hw:0,0", SND_PCM_STREAM_CAPTURE, 0);
    if(err < 0) {
        zlog_error(log_, "open pcm failed: %s", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    err = snd_pcm_set_params(pcm, SND_PCM_FORMAT_S16_LE, 
        SND_PCM_ACCESS_RW_INTERLEAVED,
        AUDIO_CHANNELS, AUDIO_RATE,
        1, 100000);
    if(err < 0) {
        zlog_error(log_, "pcm set params failed: %s", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    set_right_gain(8);
}

void audio_record_cleanup(void)
{
    if(pcm != NULL) {
        snd_pcm_close(pcm);
        pcm = NULL;
    }
}

void audio_record_service(RecordData *record_data)
{
    int err, no_audio_counts = 0;
    bool recording = false;
    size_t recording_malloc_size = AUDIO_RATE * AUDIO_CHANNELS * 2;

    audio_record_free(record_data);

    err = snd_pcm_prepare(pcm);
    if(err < 0) {
        zlog_error(log_, "pcm prepare failed: %s", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    err = snd_pcm_start(pcm);
    if(err < 0) {
        zlog_error(log_, "pcm start failed: %s", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    while(1)
    {
        snd_pcm_sframes_t frames = snd_pcm_readi(pcm, record_buf, BUF_SIZE / 2);
        audio_record_copy2left(frames);
        double rms = get_rms(frames);

        if(recording == false && rms > RMS_MAX) {
            recording = true;
            zlog_debug(log_, "recording ...");
            char *temp = malloc(recording_malloc_size + MAVHEADER_SIZE);
            if(temp == NULL) {
                zlog_error(log_, "record_data malloc failed");
                exit(EXIT_FAILURE);
            }
            record_data->data = temp;
            record_data->len += MAVHEADER_SIZE;
        }

        if(recording) {
            size_t frames_size = frames * 2 * AUDIO_CHANNELS;

            no_audio_counts = rms < RMS_MIN ? no_audio_counts + 1 : 0;

            if(record_data->len + frames_size >= recording_malloc_size) {
                recording_malloc_size *= 2;
                char *temp = realloc(record_data->data, 
                    recording_malloc_size + MAVHEADER_SIZE);
                if(temp == NULL) {
                    zlog_error(log_, "record_data realloc failed");
                    exit(EXIT_FAILURE);
                }
                record_data->data = temp;
            }

            memcpy(record_data->data + record_data->len, 
                record_buf, frames_size);
            record_data->len += frames_size;

            if(no_audio_counts == 18) {
                audio_add_mavheader(record_data->data, 
                    record_data->len - MAVHEADER_SIZE);
                zlog_debug(log_, "recording end");
                break;
            }
        }
    }

    snd_pcm_drop(pcm);
}

void audio_record_free(RecordData *record_data)
{
    if(record_data->data != NULL) {
        free(record_data->data);
        record_data->data = NULL;
    }
    record_data->len = 0;
}
