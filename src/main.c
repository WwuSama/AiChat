#include "common/config.h"
#include "audio/audio_record.h"
#include "audio/audio_play.h"
#include "services/asr_serivice.h"
#include "services/llm_service.h"
#include "services/tts_serivice.h"

int main(void)
{
    config_init();
    audio_record_init();
    audio_play_init();
    asr_init();
    llm_init();
    tts_init();
    while(1)
    {
        int err = 0;
        RecordData recording = {0};
        ResponseData *playing = NULL;
        char *user_content = NULL;
        char *ai_answer = NULL;
        
        audio_record_service(&recording);
        
        err = asr_service(&recording, &user_content);
        if(err < 0) {
            goto clean;
        }
        zlog_info(comConfig.log, "user: %s", user_content);
        
        llm_service(user_content, &ai_answer);
        zlog_info(comConfig.log, "ai: %s", ai_answer);

        tts_serivice(ai_answer, &playing);

        audio_play_service(playing->data, playing->len);
        
clean:
        llm_free_answer(ai_answer);
        asr_free_content(user_content);
        audio_record_free(&recording);
    }
    tts_cleanup();
    llm_cleanup();
    asr_cleanup();
    audio_play_cleanup();
    audio_record_cleanup();
    config_cleanup();
}