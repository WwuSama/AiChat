#include "common/http_client.h"
#include "common/config.h"

#include <stdlib.h>
#include <string.h>

static size_t receive (const char *data, size_t size, size_t counts, void *userdata)
{
    size_t bytes = size * counts;
    ResponseData *res_buf = (ResponseData *)userdata;
    char *next = realloc(res_buf->data, res_buf->len + bytes + 1);
    if(next == NULL) {
        zlog_error(comConfig.log, "realloc failed");
        exit(EXIT_FAILURE);
    }
    res_buf->data = next;
    memcpy(res_buf->data + res_buf->len, data, bytes);
    res_buf->len += bytes;
    res_buf->data[res_buf->len] = '\0';
    return bytes;
}

void http_init(zlog_category_t *log, HTTPClient *client, const char *url)
{
    memset(client, 0, sizeof(HTTPClient));
    client->curl = curl_easy_init();
    if(client->curl == NULL) {
        zlog_error(log, "HTTPClient init failed");
        exit(EXIT_FAILURE);
    }
    curl_easy_setopt(client->curl, CURLOPT_URL, url);
    curl_easy_setopt(client->curl, CURLOPT_WRITEFUNCTION, receive);
    curl_easy_setopt(client->curl, CURLOPT_WRITEDATA, &client->res_buf);
}

void http_cleanup(HTTPClient *client)
{
    if(client->curl != NULL) {
        curl_easy_cleanup(client->curl);
        client->curl = NULL;
    }

    if(client->header != NULL) {
        curl_slist_free_all(client->header);
        client->header = NULL;
    }

    if(client->res_buf.data != NULL) {
       free(client->res_buf.data);
       client->res_buf.data = NULL;
    }
    client->res_buf.len = 0;
}

void http_set_url(zlog_category_t *log, HTTPClient *client, const char *url)
{
    if(client->curl == NULL) {
        zlog_error(log, "HTTPClient have not init");
        exit(EXIT_FAILURE);
    }
    curl_easy_setopt(client->curl, CURLOPT_URL, url);
}

void http_add_header(zlog_category_t *log, HTTPClient *client, const char *h)
{
    if(client->curl == NULL) {
        zlog_error(log, "HTTPClient have not init");
        exit(EXIT_FAILURE);
    }
    struct curl_slist *next = curl_slist_append(client->header, h);
    if(next == NULL) {
        zlog_error(log, "header append \"%s\" failed", h);
        exit(EXIT_FAILURE);
    }
    client->header = next;
    curl_easy_setopt(client->curl, CURLOPT_HTTPHEADER, client->header);
}

void http_add_post(zlog_category_t *log, HTTPClient *client, const char *data)
{
    if(client->curl == NULL) {
        zlog_error(log, "HTTPClient have not init");
        exit(EXIT_FAILURE);
    }
    curl_easy_setopt(client->curl, CURLOPT_POSTFIELDS, data);
}

int http_perform(zlog_category_t *log, HTTPClient *client, long *state)
{
    if(client->curl == NULL) {
        zlog_error(log, "HTTPClient have not init");
        exit(EXIT_FAILURE);
    }
    if(client->res_buf.data != NULL) {
       free(client->res_buf.data);
       client->res_buf.data = NULL;
    }
    client->res_buf.len = 0;
    CURLcode err = curl_easy_perform(client->curl);
    if(err != CURLE_OK) {
        zlog_error(log, "perform error= %s", curl_easy_strerror(err));
        return -1;
    }
    curl_easy_getinfo(client->curl, CURLINFO_RESPONSE_CODE, state);
    if(*state < 200 || *state >= 300) {
        zlog_warn(log, "HTTP= %ld, response= %s", *state,
            client->res_buf.data != NULL ? client->res_buf.data : "(empty)");
        return -2;
    }
    return 0;
}
