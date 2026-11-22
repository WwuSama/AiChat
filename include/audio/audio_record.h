#ifndef __AUDIO_RECORD_H_
#define __AUDIO_RECORD_H_

#include <stddef.h>

typedef struct {
    char *data;
    size_t len;
}RecordData;

void audio_record_init(void);

void audio_record_cleanup(void);

void audio_record_service(RecordData *record_data);

void audio_record_free(RecordData *record_data);

#endif