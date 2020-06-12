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

    const char *path = cJSON_GetObjectItem(params, "path")->valuestring;

    // Validate path.
    if (!path) {
        return webserver_req__send_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid path");
    }

    if (strchr(path,'.')!=NULL) {
        return webserver_req__send_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid path");
    }

    if (*path=='/') {
        path++;
    }
    if ((strlen(path) + 8)>=sizeof(fullpath)) {
        return webserver_req__send_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Path too long");
    }

    sprintf(fullpath,"/sdcard/%s", path);


    DIR *dir = opendir(fullpath);
    struct dirent *ent;

    if (NULL==dir) {
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



static const struct webserver_req_entry req_handlers[] = {
    { "version", &webserver_req__version },
    { "list",    &webserver_req__list }
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
