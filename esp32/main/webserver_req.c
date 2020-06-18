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

static esp_err_t webserver_req__version(httpd_req_t *req, const char *querystring)
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

static inline void webserver_req__get_sdcard_path(const char *relative, char *absolute, size_t len)
{
#if 0 //def __linux
    snprintf(absolute, len, "%s/sdcard/%s", startupdir, relative);
#else
    strcpy(absolute,"/sdcard/");
    strncat(absolute, relative, len-8);
#endif
}


static esp_err_t webserver_req__list(httpd_req_t *req, const char *querystr)
{
    char fullpath[FILE_PATH_MAX+ESP_VFS_PATH_MAX];
    char path[FILE_PATH_MAX];
    char *pathptr = path;

    ESP_LOGI(TAG, "Generating file list");

    if (httpd_query_key_value(querystr, "path", path, sizeof(path))!=ESP_OK) {
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

    webserver_req__get_sdcard_path(pathptr, fullpath, sizeof(fullpath));

    DIR *dir = __opendir(fullpath);
    struct dirent *ent;

    if (NULL==dir) {
        ESP_LOGI(TAG, "Could not open path %s", fullpath);
        return webserver_req__send_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No such path");;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON *earray = cJSON_CreateArray();

    while ((ent=__readdir(dir))) {

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

static esp_err_t webserver_req__upload(httpd_req_t *req, const char *querystr)
{
    return ESP_FAIL;
}

static esp_err_t webserver_req__delete(httpd_req_t *req, const char *querystr)
{
    char fullpath[FILE_PATH_MAX];
    char path[FILE_PATH_MAX - 8];

    if (httpd_query_key_value(querystr, "path", path, sizeof(path))!=ESP_OK) {
        ESP_LOGI(TAG,"Missing parameters");
        return webserver_req__send_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid path");
        return ESP_FAIL;
    }

    webserver_req__get_sdcard_path(path, fullpath, sizeof(fullpath));

    filetype_t type = file_type(fullpath);

    if (type!=TYPE_FILE) {
        ESP_LOGI(TAG,"Not a file");
        return webserver_req__send_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid path");
        return ESP_FAIL;
    }
    // Unlink.
    int r = unlink(fullpath);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "success", r==0? "true":"false");
    cJSON_AddStringToObject(root, "path", path);

    webserver_req__send_and_free_json(req, root);
    return ESP_OK;
}

static esp_err_t webserver_req__rename(httpd_req_t *req, const char *querystr)
{
    char fullpath[FILE_PATH_MAX];
    char fullnewpath[FILE_PATH_MAX];
    char path[FILE_PATH_MAX - 8];

    if (httpd_query_key_value(querystr, "path", path, sizeof(path))!=ESP_OK) {
        ESP_LOGI(TAG,"Missing parameters");
        return webserver_req__send_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid path");
        return ESP_FAIL;
    }

    webserver_req__get_sdcard_path(path, fullpath, sizeof(fullpath));

    if (httpd_query_key_value(querystr, "newpath", path, sizeof(path))!=ESP_OK) {
        ESP_LOGI(TAG,"Missing parameters");
        return webserver_req__send_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid path");
        return ESP_FAIL;
    }

    webserver_req__get_sdcard_path(path, fullnewpath, sizeof(fullnewpath));


    filetype_t type = file_type(fullpath);

    if (type!=TYPE_FILE) {
        ESP_LOGI(TAG,"Not a file");
        return webserver_req__send_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid path");
        return ESP_FAIL;
    }
    // Rename.
    int r = rename(fullpath, fullnewpath);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "success", r==0? "true":"false");
    cJSON_AddStringToObject(root, "path", path);
    webserver_req__send_and_free_json(req, root);
    return ESP_OK;
}

static esp_err_t webserver_req__mkdir(httpd_req_t *req, const char *querystr)
{
    char fullpath[FILE_PATH_MAX];
    char path[FILE_PATH_MAX - 8];

    if (httpd_query_key_value(querystr, "path", path, sizeof(path))!=ESP_OK) {
        ESP_LOGI(TAG,"Missing parameters");
        return webserver_req__send_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid path");
        return ESP_FAIL;
    }

    webserver_req__get_sdcard_path(path, fullpath, sizeof(fullpath));

    int r = mkdir(fullpath, 0666);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "success", r==0? "true":"false");
    cJSON_AddStringToObject(root, "path", path);
    webserver_req__send_and_free_json(req, root);
    return ESP_OK;
}

static esp_err_t webserver_req__rmdir(httpd_req_t *req, const char *querystr)
{
    char fullpath[FILE_PATH_MAX];
    char path[FILE_PATH_MAX - 8];

    if (httpd_query_key_value(querystr, "path", path, sizeof(path))!=ESP_OK) {
        ESP_LOGI(TAG,"Missing parameters");
        return webserver_req__send_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid path");
        return ESP_FAIL;
    }

    webserver_req__get_sdcard_path(path, fullpath, sizeof(fullpath));

    int r = rmdir(fullpath);

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "success", r==0? "true":"false");
    cJSON_AddStringToObject(root, "path", path);
    webserver_req__send_and_free_json(req, root);
    return ESP_OK;
}


static const struct webserver_req_entry req_handlers[] = {
    { "version", &webserver_req__version },
    { "list",    &webserver_req__list },
    { "upload",  &webserver_req__upload },
    { "delete",  &webserver_req__delete },
    { "rename",  &webserver_req__rename },
    { "mkdir",   &webserver_req__mkdir },
    { "rmdir",   &webserver_req__rmdir },
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


webserver_req_post_handler_t webserver_req__find_post_handler(const char *path)
{
    return NULL;
}
