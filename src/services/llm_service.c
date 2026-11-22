#include "common/config.h"
#include "common/http_client.h"
#include "common/json_fun.h"

#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>

static HTTPClient http;
static zlog_category_t *log = NULL;
static json_object *data = NULL;
static json_object *messages = NULL;

static void add_message(const char *role, const char *content)
{
    json_object *message = json_object_new_object();
    json_object_object_add(message, "role", json_object_new_string(role));
    json_object_object_add(message, "content", json_object_new_string(content));
    json_object_array_add(messages, message);
}

static void create_post(void)
{
    data = json_object_new_object();
    json_object_object_add(data, "model", json_object_new_string("deepseek-flash"));
    messages = json_object_new_array();
    json_object_object_add(data, "messages", messages);
    json_object *thinking = json_object_new_object();
    json_object_object_add(data, "thinking", thinking);

    add_message("system", comConfig.llm_role);

    json_object_object_add(thinking, "type", json_object_new_string("disabled"));
}

void llm_init(void)
{
    log = zlog_get_category("LLM");
    if(log == NULL) {
        zlog_error(comConfig.log, "LLM log get failed");
        exit(EXIT_FAILURE);
    }

    char *auth = malloc(strlen(comConfig.aliyun_apikey) +
        sizeof("Authorization: Bearer ")
    );
    sprintf(auth, "Authorization: Bearer %s"
        , comConfig.deepseek_apikey
    );

    http_init(log, &http, "https://api.deepseek.com/chat/completions");
    http_add_header(log, &http, auth);
    http_add_header(log, &http, "Content-Type: application/json");

    create_post();

    free(auth);
}

void llm_cleanup(void)
{
    json_object_put(data);
    http_cleanup(&http);
}

void llm_service(const char *user_content, char **answer)
{
    add_message("user", user_content);

    http_add_post(log, &http, json_object_to_json_string(data));

    long state;
    http_perform(log, &http, &state);

    json_object *llm_res = json_tokener_parse(http.res_buf.data);
    json_object *choices = jsonfun_get_array(log, llm_res, "choices");
    json_object *get_choice = json_object_array_get_idx(choices, 0);
    json_object *message = jsonfun_get_object(log, get_choice, "message");
    char *assistant = strdup(jsonfun_get_string(log, message, "content"));
    if(assistant == NULL) {
        zlog_error(log, "answer malloc failed");
        exit(EXIT_FAILURE);
    }
    *answer = assistant;
    add_message("assistant", assistant);

    json_object_put(llm_res);
}

void llm_free_answer(char *answer)
{
    if(answer != NULL) {
        free(answer);
        answer = NULL;
    }
}