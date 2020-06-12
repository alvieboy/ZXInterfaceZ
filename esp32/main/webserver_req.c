#include <inttypes.h>
#include <stdbool.h>
#include "webserver_req.h"
#include "esp_log.h"
#include "defs.h"
#include "version.h"
#include "fpga.h"
#include "esp_system.h"
#include "fileaccess.h"
#include <unistd.h>
#include <dirent.h>
#include "webserver.h"

struct webserver_req_entry {
    const char *path;
    webserver_req_handler_t req;
};

static void webserver_req__send_and_free_json(httpd_req_t *req, cJSON *response)
{
    char *data = cJSON_Print(response);
    httpd_resp_sendstr(req, data);
    free(data);
    cJSON_Delete(response);
}

static esp_err_t webserver_req__version(httpd_req_t *req, cJSON *params)
{
    cJSON *root = cJSON_CreateObject();

    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    cJSON_AddStringToObject(root, "esp_chip", "ESP32");
    cJSON_AddStringToObject(root, "esp_version", IDF_VER);
    cJSON_AddNumberToObject(root, "esp_revision", chip_info.revision);
    cJSON_AddNumberToObject(root, "esp_cores", chip_info.cores);
    cJSON_AddStringToObject(root, "esp_flash", "Unknown");

    char versionstring[64];

    unsigned fpga_id = fpga__read_id();

    sprintf(versionstring,"%02x.%02x r%d",
            (fpga_id>>16) & 0xff,
            (fpga_id>>8) & 0xff,
            (fpga_id) & 0xff);

    cJSON_AddStringToObject(root, "software_version", version);
    cJSON_AddStringToObject(root, "fpga_version", versionstring);

    webserver_req__send_and_free_json(req, root);
    return ESP_OK;
}

static esp_err_t webserver_req__send_error(httpd_req_t *req, int error, const char *reason)
{
    httpd_resp_send_err(req, error, reason);
    return ESP_FAIL;
}

static esp_err_t webserver_req__list(httpd_req_t *req, cJSON *params)
{
    char fullpath[FILE_PATH_MAX];
    char query[FILE_PATH_MAX];
    char path[FILE_PATH_MAX];
    char *pathptr = path;

    ESP_LOGI(TAG, "Genrating list");

    if (httpd_req_get_url_query_str(req, query, sizeof(query))!=ESP_OK) {
        ESP_LOGI(TAG,"Missing parameters");
        return webserver_req__send_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid path");
        return ESP_FAIL;
    }

    webserver__decodeurl(query);

    if (httpd_query_key_value(query, "path", path, sizeof(path))!=ESP_OK) {
        ESP_LOGI(TAG,"Missing parameters");
        return webserver_req__send_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid path");
        return ESP_FAIL;
    }
    // Validate path.

    if (strchr(path,'.')!=NULL) {
        ESP_LOGI(TAG,"Path invalid chars");
        return webserver_req__send_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid path");
    }

    if (*pathptr=='/') {
        pathptr++;
    }
    if ((strlen(pathptr) + 8)>=sizeof(fullpath)) {
        ESP_LOGI(TAG,"Path too long");

        return webserver_req__send_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Path too long");
    }
#ifdef __linux
    sprintf(fullpath,"%s/sdcard/%s", startupdir, pathptr);
#else
    sprintf(fullpath,"/sdcard/%s", pathptr);
#endif

    DIR *dir = opendir(fullpath);
    struct dirent *ent;

    if (NULL==dir) {
        ESP_LOGI(TAG, "Could not open path %s", fullpath);
        return webserver_req__send_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No such path");;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON *earray = cJSON_CreateArray();

    while ((ent=readdir(dir))) {

        cJSON *entry = cJSON_CreateObject();

        if (ent->d_type == DT_DIR) {
            cJSON_AddStringToObject(entry, "type", "dir");
        } else {
            cJSON_AddStringToObject(entry, "type", "file");
        }
        cJSON_AddStringToObject(entry, "name", ent->d_name);

        cJSON_AddNumberToObject(entry, "size", file_size(fullpath, ent->d_name) );

        cJSON_AddItemToArray(earray, entry);
    }

    cJSON_AddStringToObject(root, "path", path);
    cJSON_AddItemToObject(root, "entries", earray);

    webserver_req__send_and_free_json(req, root);
    return ESP_OK;
}

static esp_err_t webserver_req__upload(httpd_req_t *req, cJSON *params)
{
    return ESP_FAIL;
}


static const struct webserver_req_entry req_handlers[] = {
    { "version", &webserver_req__version },
    { "list",    &webserver_req__list },
    { "upload",  &webserver_req__upload }
};


webserver_req_handler_t webserver_req__find_handler(const char *path)
{
    unsigned int i;
    char reqname[128];
    char *delim;

    if (strlen(path)>=sizeof(reqname))
        return NULL;

    strcpy(reqname, path);

    char *end = strchr(reqname,'?');
    if (end) {
        *end = '\0';
    }

    ESP_LOGI(TAG,"Scanning req '%s'", reqname);

    delim = strrchr(reqname,'/');
    if (delim) {
        delim++;
    } else {
        delim = reqname;
    }

    ESP_LOGI(TAG,"Parsed as '%s'", delim);

    for (i=0; i<sizeof(req_handlers)/sizeof(req_handlers[0]); i++) {
        if (strcmp(req_handlers[i].path, delim)==0) {
            return req_handlers[i].req;
        }
    }
    return NULL;
}
