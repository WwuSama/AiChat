#ifndef __AUDIO_PLAY_H_
#define __AUDIO_PLAY_H_

#include <stddef.h>

void audio_play_init(void);

void audio_play_cleanup(void);

void audio_play_service(const char *data, size_t len);

#endif