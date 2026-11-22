#ifndef __TTS_SERIVICE_H_
#define __TTS_SERIVICE_H_

#include "common/http_client.h"

void tts_init(void);

void tts_cleanup(void);

void tts_serivice(const char *ai_answer, ResponseData **res_audio);

#endif