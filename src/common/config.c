#include "common/config.h"
#include "common/json_fun.h"

#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <json-c/json.h>

ConfigStruct comConfig;

static void get_config(json_object *file, const char *name, char **config_str)
{
    const char *text = jsonfun_get_string(
        comConfig.log, file, name);
    if(text == NULL || strlen(text) == 0) {
        zlog_error(comConfig.log, "\"%s\" is empty", name);
        exit(EXIT_FAILURE);
    }
    *config_str = strdup(text);
}

void config_init(void)
{
    if(zlog_init("config/zlog.conf")) {
        fprintf(stderr, "zlog init failed\n");
        exit(EXIT_FAILURE);
    }

    comConfig.log = zlog_get_category("System");
    if(!comConfig.log) {
        fprintf(stderr, "zlog system log init failed\n");
        exit(EXIT_FAILURE);
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    json_object *file = json_object_from_file("config/config.json");
    if(file == NULL) {
        zlog_error(comConfig.log, "config.json get error");
        exit(EXIT_FAILURE);
    }

    get_config(file, "aliyun_WorkspaceId", 
        &comConfig.aliyun_workspaceid);

    get_config(file, "aliyun_APIKEY",
        &comConfig.aliyun_apikey);

    get_config(file, "deepseek_APIKEY",
        &comConfig.deepseek_apikey);

    get_config(file, "llm_role",
        &comConfig.llm_role);

    get_config(file, "tts_voice",
        &comConfig.tts_voice);

    json_object_put(file);
}

void config_cleanup(void)
{
    if(comConfig.aliyun_apikey != NULL) {
        free(comConfig.aliyun_apikey);
        comConfig.aliyun_apikey = NULL;
    }

    if(comConfig.aliyun_workspaceid != NULL) {
        free(comConfig.aliyun_workspaceid);
        comConfig.aliyun_workspaceid = NULL;
    }

    if(comConfig.deepseek_apikey != NULL) {
        free(comConfig.deepseek_apikey);
        comConfig.deepseek_apikey = NULL;
    }

    if(comConfig.llm_role != NULL) {
        free(comConfig.llm_role);
        comConfig.llm_role = NULL;
    }

    if(comConfig.tts_voice != NULL) {
        free(comConfig.tts_voice);
        comConfig.tts_voice = NULL;
    }
    zlog_fini();
    curl_global_cleanup();
}