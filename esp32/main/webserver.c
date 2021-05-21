#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>
#include <ctype.h>
#include "flash_resource.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "defs.h"
#include <limits.h>
#include "webserver.h"
#include "fileaccess.h"
#include "firmware_ws.h"

#define TAG "WEBSERVER"

static httpd_handle_t server = NULL;

#if 0
size_t
strlcpy(char *dst, const char *src, size_t siz);
#endif

#define IS_FILE_EXT(filename, ext) \
    (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)

#define SCRATCH_BUFSIZE  2048
static uint8_t scratch[SCRATCH_BUFSIZE];

static esp_err_t webserver__set_content_type_from_file(httpd_req_t *req, const char *filename)
{
    if (IS_FILE_EXT(filename, ".pdf")) {
        return httpd_resp_set_type(req, "application/pdf");
    } else if (IS_FILE_EXT(filename, ".html")) {
        return httpd_resp_set_type(req, "text/html");
    } else if (IS_FILE_EXT(filename, ".jpeg")) {
        return httpd_resp_set_type(req, "image/jpeg");
    } else if (IS_FILE_EXT(filename, ".ico")) {
        return httpd_resp_set_type(req, "image/x-icon");
    } else if (IS_FILE_EXT(filename, ".css")) {
        return httpd_resp_set_type(req, "text/css");
    } else if (IS_FILE_EXT(filename, ".css.gz")) {
        return httpd_resp_set_type(req, "text/css");
    } else if (IS_FILE_EXT(filename, ".js")) {
        return httpd_resp_set_type(req, "text/javascript");
    } else if (IS_FILE_EXT(filename, ".js.gz")) {
        return httpd_resp_set_type(req, "text/javascript");
    }
    /* This is a limited set only */
    /* For any other type always set as plain text */
    return httpd_resp_set_type(req, "text/plain");
}


static const char* webserver__get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize)
{
    const size_t base_pathlen = strlen(base_path);
    size_t pathlen = strlen(uri);

    const char *quest = strchr(uri, '?');
    if (quest) {
        pathlen = MIN(pathlen, quest - uri);
    }
    const char *hash = strchr(uri, '#');
    if (hash) {
        pathlen = MIN(pathlen, hash - uri);
    }

    if (base_pathlen + pathlen + 1 > destsize) {
        /* Full path string won't fit into destination buffer */
        return NULL;
    }

    /* Construct full path (base + path) */
    strcpy(dest, base_path);
    strlcpy(dest + base_pathlen, uri, pathlen + 1);

    /* Return pointer to path, skipping the base */
    return dest + base_pathlen;
}

static const char *webserver__rootpath()
{
    return "/spiffs";
}

static esp_err_t webserver__get_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];
    FILE *fd = NULL;
    struct stat file_stat;


    const char *filename = webserver__get_path_from_uri(filepath,
                                                        webserver__rootpath(),
                                                        req->uri, sizeof(filepath));

    ESP_LOGI(TAG, "File path: %s", filepath);
    ESP_LOGI(TAG, "Filename: %s", filename? filename:"NULL");

    if (!filename) {
        ESP_LOGE(TAG, "Filename is too long");
        /* Respond with 400 Bad request */
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Filename too long");
        return ESP_FAIL;
    }

    if (filename[strlen(filename) - 1] == '/') {

        // If serving '/' respond with index.html
        if (!strcmp(filename, "/")) {
            strcpy(filepath, "/spiffs/index.html");
        } else {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Filename can not end with /");
            return ESP_FAIL;
        }
    }

    // Check if we have a compressed version of the file.

    char *oldpathend = filepath + strlen(filepath);
    strcpy(oldpathend,".gz");

    ESP_LOGI(TAG, "Scanning for compressed %s", filepath);
    if (__lstat(filepath, &file_stat) == -1) {
        *oldpathend = '\0'; // no
    } else {
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    }


    if (__lstat(filepath, &file_stat) == -1) {
        if (strchr(filename, '.')==NULL) {
            // No  extension on request, fall back to index.html
            ESP_LOGI(TAG, "Failed to stat file : %s, falling back to index.html", filepath);
            strcpy(filepath, "/spiffs/index.html");
        } else {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
            return ESP_FAIL;
        }
    }

    fd = __fopen(filepath, "r");
    if (!fd) {
        ESP_LOGE(TAG, "Failed to read existing file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Sending file : %s (%ld bytes)...", filename, file_stat.st_size);
    webserver__set_content_type_from_file(req, filename);

    /* Retrieve the pointer to scratch buffer for temporary storage */
    char *chunk = (char*)scratch;
    size_t chunksize;
    do {
        /* Read file in chunks into the scratch buffer */
        chunksize = fread(chunk, 1, SCRATCH_BUFSIZE, fd);

        if (chunksize > 0) {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK) {
                fclose(fd);
                ESP_LOGE(TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
               return ESP_FAIL;
           }
        }

        /* Keep looping till the whole file is sent */
    } while (chunksize != 0);

    /* Close file after sending complete */
    fclose(fd);
    ESP_LOGI(TAG, "File sending complete");

    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

#include "webserver_req.h"

static esp_err_t webserver__req_get_handler(httpd_req_t *req)
{
    char query[FILE_PATH_MAX];
    char *qptr = query;
    webserver_req_handler_t handler = webserver_req__find_handler(req->uri);

    if (handler==NULL) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not found");
        return ESP_FAIL;
    }

    // TODO Maybe use httpd_rq_get_url_query_len to allocate query buffer.
    int r = httpd_req_get_url_query_str(req, query, sizeof(query));
    switch (r) {
    case ESP_OK:
        break;
    case ESP_ERR_NOT_FOUND:
        qptr = NULL;
        break;
    default:
        ESP_LOGE(TAG,"Cannot get query or invalid query string");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid query string");
        return ESP_FAIL;
    }

    if (qptr) {
        webserver__decodeurl(qptr);
    }

    httpd_resp_set_type(req, "application/json");

    return handler(req, qptr);
}

static esp_err_t webserver__req_post_handler(httpd_req_t *req)
{

    char query[FILE_PATH_MAX];
    char *qptr = query;

    webserver_req_handler_t handler = webserver_req__find_post_handler(req->uri);

    if (handler==NULL) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not found");
        return ESP_FAIL;
    }

    int r = httpd_req_get_url_query_str(req, query, sizeof(query));
    switch (r) {
    case ESP_OK:
        break;
    case ESP_ERR_NOT_FOUND:
        qptr = NULL;
        break;
    default:
        ESP_LOGE(TAG,"Cannot get query or invalid query string");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid query string");
        return ESP_FAIL;
    }
    if (qptr) {
        ESP_LOGI(TAG, "Query string: '%s'", qptr);
        webserver__decodeurl(qptr);
    }

    httpd_resp_set_type(req, "application/json");

    return handler(req, qptr);
}
int webserver__init(void)
{
    /* Allocate memory for server data */
    /*
     server_data = calloc(1, sizeof(struct file_server_data));

    if (!server_data) {
        ESP_LOGE(TAG, "Failed to allocate memory for server data");
        return ESP_ERR_NO_MEM;
    }
    strlcpy(server_data->base_path,
            base_path,
            sizeof(server_data->base_path));
     */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.max_open_sockets = 4;
#ifdef __linux__
    config.server_port = 8000;
    config.task_priority = 4;
#endif
    /* Use the URI wildcard matching function in order to
     * allow the same handler to respond to multiple different
     * target URIs which match the wildcard scheme */
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(TAG, "Starting HTTP Server");
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start file server!");
        return ESP_FAIL;
    }

    httpd_uri_t req_get = {
        .uri       = "/req/*",  // Match all URIs of type /path/to/file
        .method    = HTTP_GET,
        .handler   = &webserver__req_get_handler,
        .user_ctx  = NULL    // Pass server data as context
    };
    httpd_register_uri_handler(server, &req_get);

    httpd_uri_t req_post = {
        .uri       = "/upload/*",   // Match all URIs of type /upload/path/to/file
        .method    = HTTP_POST,
        .handler   = &webserver__req_post_handler,
        .user_ctx  = NULL    // Pass server data as context
    };

    httpd_register_uri_handler(server, &req_post);

    // WS seems not to pass the full request URI once connected.
    // So we tie this to a single handler for now.

    httpd_uri_t ws_get = {
        .uri       = "/ws/fwupgrade",   // Match all URIs of type /upload/path/to/file
        .method    = HTTP_GET,
        .handler   = &firmware_ws__firmware_upgrade,
        .user_ctx  = NULL, 
        .is_websocket = true
    };

    httpd_register_uri_handler(server, &ws_get);

    /* URI handler for getting uploaded files */
    httpd_uri_t file_download = {
        .uri       = "/*",  // Match all URIs of type /path/to/file
        .method    = HTTP_GET,
        .handler   = &webserver__get_handler,
        .user_ctx  = NULL    // Pass server data as context
    };

    httpd_register_uri_handler(server, &file_download);




    ESP_LOGI(TAG,"Registered all handlers");
    return ESP_OK;
}

void webserver__decodeurl(char *src)
{
    char a, b;
    char *dst = src;
    while (*src) {
        if ((*src == '%') &&
            ((a = src[1]) && (b = src[2])) &&
            (isxdigit(a) && isxdigit(b))) {
            if (a >= 'a')
                a -= 'a'-'A';
            if (a >= 'A')
                a -= ('A' - 10);
            else
                a -= '0';
            if (b >= 'a')
                b -= 'a'-'A';
            if (b >= 'A')
                b -= ('A' - 10);
            else
                b -= '0';
            *dst++ = 16*a+b;
            src+=3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst++ = '\0';
}

httpd_handle_t webserver__get_handle()
{
    return server;
}
