#include "common/config.h"
#include "common/http_client.h"
#include "common/json_fun.h"
#include "audio/audio_record.h"

#include <zlog.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include <resolv.h>

#define ASR_MODEL       "qwen-audio-3.1-asr-flash"

static HTTPClient http;
static zlog_category_t *log = NULL;
static json_object *post = NULL;
static json_object *audio_data = NULL;

static void create_post(void)
{
    post = json_object_new_object();
    json_object_object_add(post, "model", json_object_new_string(ASR_MODEL));
    json_object *input = json_object_new_object();
    json_object_object_add(post, "input", input);
    json_object *parameters = json_object_new_object();
    json_object_object_add(post, "parameters", parameters);

    json_object *messages = json_object_new_array();
    json_object_object_add(input, "messages", messages);

    json_object *message = json_object_new_object();
    json_object_array_add(messages, message);

    json_object_object_add(message, "role", json_object_new_string("user"));
    json_object *content = json_object_new_array();
    json_object_object_add(message, "content", content);

    json_object *content_ = json_object_new_object();
    json_object_array_add(content, content_);
    
    json_object_object_add(content_, "type", json_object_new_string("input_audio"));
    json_object *input_audio = json_object_new_object();
    json_object_object_add(content_, "input_audio", input_audio);
    
    audio_data = json_object_new_string("");
    json_object_object_add(input_audio, "data", audio_data);

    json_object_object_add(parameters, "format", json_object_new_string("wav"));
    json_object_object_add(parameters, "sample_rate", json_object_new_string("16000"));   
}

static char *base64_encoder(const char *data, size_t data_size)
{
    const char prefix[] = "data:audio/wav;base64,";
    size_t prefix_len = sizeof(prefix) - 1;

    size_t encoded_size = 4 * ((data_size + 2) / 3) + 1;
    char *audio_data_str = malloc(prefix_len + encoded_size);
    memcpy(audio_data_str, prefix, prefix_len);
    if(audio_data_str == NULL) {
        zlog_error(log, "base64 malloc failed");
        exit(EXIT_FAILURE);
    }

    int len = b64_ntop((const unsigned char *)data, 
        data_size, 
        audio_data_str + prefix_len,
        encoded_size
    );
    if(len < 0) {
        zlog_error(log, "base64 encode failed");
        exit(EXIT_FAILURE);
    }
    return audio_data_str;
}

void asr_init(void)
{
    log = zlog_get_category("ASR");
    if(log == NULL) {
        zlog_error(comConfig.log, "ASR log get failed");
        exit(EXIT_FAILURE);
    }

    char *url = malloc(strlen(comConfig.aliyun_workspaceid) + 
        sizeof("https://.cn-beijing.maas.aliyuncs.com" 
        "/api/v1/services/aigc/multimodal-generation"
        "/generation")
    );
    sprintf(url, "https://%s.cn-beijing.maas.aliyuncs.com"
        "/api/v1/services/aigc/multimodal-generation"
        "/generation", comConfig.aliyun_workspaceid
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
    http_add_header(log, &http, "X-DashScope-SSE: disable");

    free(url);
    free(auth);

    create_post();
}

void asr_cleanup(void)
{
    if(post != NULL) {
        json_object_put(post);
        post = NULL;
    }
    http_cleanup(&http);
}

int asr_service(RecordData *record_data, char **user_content)
{
    char *b64_str = base64_encoder(record_data->data, record_data->len);
    
    json_object_set_string(audio_data, b64_str);
    http_add_post(log, &http, json_object_to_json_string(post));

    long state;
    int err = http_perform(log, &http, &state);
    if(err == -1) {
        zlog_error(log, "http perform err= %d", err);
        exit(EXIT_FAILURE);
    } else if(err == -2 && state == 400) {
        zlog_info(log, "record_data content is empty");
        free(b64_str);
        return err;
    }

    json_object *asr_res = json_tokener_parse(http.res_buf.data);
    json_object *sentence = jsonfun_get_object(log, asr_res, "sentence");
    char *temp = strdup(jsonfun_get_string(log, sentence, "text"));
    if(temp == NULL) {
        zlog_error(log, "text malloc failed");
        exit(EXIT_FAILURE);
    }
    *user_content = temp;

    json_object_put(asr_res);
    free(b64_str);

    return err;
}

void asr_free_content(char *content)
{
    if(content != NULL) {
        free(content);
    }
}