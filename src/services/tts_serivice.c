#include "common/config.h"
#include "common/http_client.h"
#include "common/json_fun.h"

#include <zlog.h>
#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>

static HTTPClient http;
static HTTPClient http_download;
static zlog_category_t *log = NULL;
static json_object *post = NULL;
static json_object *text = NULL;

static void create_post(void)
{
    post = json_object_new_object();
    json_object_object_add(post, "model", json_object_new_string("qwen-audio-3.1-tts-flash"));
    json_object *input = json_object_new_object();
    json_object_object_add(post, "input", input);

    text = json_object_new_string("");
    json_object_object_add(input, "text", text);
    json_object_object_add(input, "voice", json_object_new_string(comConfig.tts_voice));
    json_object_object_add(input, "format", json_object_new_string("pcm"));
    json_object_object_add(input, "sample_rate", json_object_new_string("16000"));
}

void tts_init(void)
{
    log = zlog_get_category("TTS");
    if(log == NULL) {
        zlog_error(comConfig.log, "TTS log get failed");
        exit(EXIT_FAILURE);
    }

    char *url = malloc(strlen(comConfig.aliyun_workspaceid) + 
        sizeof("https://.cn-beijing.maas.aliyuncs.com"
            "/api/v1/services/audio"
            "/tts/SpeechSynthesizer")
    );
    sprintf(url, "https://%s.cn-beijing.maas.aliyuncs.com"
        "/api/v1/services/audio"
        "/tts/SpeechSynthesizer", 
        comConfig.aliyun_workspaceid
    );

    char *auth = malloc(strlen(comConfig.aliyun_apikey) +
        sizeof("Authorization: Bearer ")
    );
    sprintf(auth, "Authorization: Bearer %s"
        , comConfig.aliyun_apikey
    );

    http_init(log, &http, url);
    http_add_header(log, &http, auth);
    http_add_header(log, &http, "Content-Type: application/json");
    create_post();
    
    http_init(log, &http_download, NULL);

    free(auth);
    free(url);
}

void tts_cleanup(void)
{
    json_object_put(post);
    http_cleanup(&http_download);
    http_cleanup(&http);
}

void tts_serivice(const char *ai_answer, ResponseData **res_audio)
{
    json_object_set_string(text, ai_answer);
    http_add_post(log, &http, json_object_to_json_string(post));
    long state;
    http_perform(log, &http, &state);
    
    json_object *tts_res = json_tokener_parse(http.res_buf.data);
    json_object *output = jsonfun_get_object(log, tts_res, "output");
    json_object *audio = jsonfun_get_object(log, output, "audio");
    const char *download_url = jsonfun_get_string(log, audio, "url");

    http_set_url(log, &http_download, download_url);
    http_perform(log, &http_download, &state);

    *res_audio = &http_download.res_buf;

    json_object_put(tts_res);
}