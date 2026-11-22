#ifndef __CONFIG_H_
#define __CONFIG_H_

#include <zlog.h>

typedef struct {
    zlog_category_t *log;
    char *aliyun_workspaceid;
    char *aliyun_apikey;
    char *deepseek_apikey;

    char *llm_role;
    char *tts_voice;
}ConfigStruct;

extern ConfigStruct comConfig;

void config_init(void);

void config_cleanup(void);

#define TEST(x) do{    \
    zlog_debug(comConfig.log, "test%d", (x));   \
}while(0)

#endif