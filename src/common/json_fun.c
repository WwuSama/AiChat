#include "common/json_fun.h"

#include <json-c/json.h>

const char *jsonfun_get_string(zlog_category_t *log, json_object *root, const char *key)
{
    json_object *obj = NULL;
    json_object_object_get_ex(root, key, &obj);
    if(!obj || !json_object_is_type(obj, json_type_string)) {
        zlog_error(log, "get \"%s\" failed", key);
        exit(EXIT_FAILURE);
    }
    return json_object_get_string(obj);
}

json_object *jsonfun_get_object(zlog_category_t *log, json_object *root, const char *key)
{
    json_object *obj = NULL;
    json_object_object_get_ex(root, key, &obj);
    if(!obj || !json_object_is_type(obj, json_type_object)) {
        zlog_error(log, "get \"%s\" failed", key);
        exit(EXIT_FAILURE);
    }
    return obj;
}

json_object *jsonfun_get_array(zlog_category_t *log, json_object *root, const char *key)
{
    json_object *obj = NULL;
    json_object_object_get_ex(root, key, &obj);
    if(!obj || !json_object_is_type(obj, json_type_array)) {
        zlog_error(log, "get \"%s\" failed", key);
        exit(EXIT_FAILURE);
    }
    return obj;
}