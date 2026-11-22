#ifndef __HTTP_CLIENT_H_
#define __HTTP_CLIENT_H_

#include <curl/curl.h>
#include <zlog.h>

typedef struct {
    char *data;
    size_t len;
}ResponseData;

typedef struct {
    CURL *curl;
    struct curl_slist *header;
    ResponseData res_buf;
}HTTPClient;

void http_init(zlog_category_t *log, HTTPClient *client, const char *url);

void http_cleanup(HTTPClient *client);

void http_set_url(zlog_category_t *log, HTTPClient *client, const char *url);

void http_add_header(zlog_category_t *log, HTTPClient *client, const char *h);

void http_add_post(zlog_category_t *log, HTTPClient *client, const char *data);

int http_perform(zlog_category_t *log, HTTPClient *client, long *state);

#endif