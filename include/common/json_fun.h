#ifndef __JSON_FUN_H_
#define __JSON_FUN_H_

#include <zlog.h>
#include <json-c/json.h>

const char *jsonfun_get_string(zlog_category_t *log, json_object *root, const char *key);

json_object *jsonfun_get_object(zlog_category_t *log, json_object *root, const char *key);

json_object *jsonfun_get_array(zlog_category_t *log, json_object *root, const char *key);

#endif