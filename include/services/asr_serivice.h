#ifndef __ASR_SERIVICE_H_
#define __ASR_SERIVICE_H_

#include "audio/audio_record.h"

void asr_init(void);

void asr_cleanup(void);

int asr_service(RecordData *record_data, char **user_content);

void asr_free_content(char *content);

#endif