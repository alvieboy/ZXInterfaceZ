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
#include <fcntl.h>
#include "devmap.h"
#include "wifi.h"
#include <errno.h>
#include "firmware.h"
#include "minmax.h"

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

static void webserver_req__simple_json_reply(httpd_req_t *req, const char *errorstr)
{
    cJSON *root = cJSON_CreateObject();

    if (errorstr) {
        cJSON_AddStringToObject(root, "errorstring", errorstr);
    };

    cJSON_AddStringToObject(root, "success", errorstr==NULL?"true":"false");

    webserver_req__send_and_free_json(req, root);
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
    cJSON_AddStringToObject(root, "git_version", gitversion);
    cJSON_AddStringToObject(root, "build_date", builddate);
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


static esp_err_t webserver_req__post_file(httpd_req_t *req, const char *querystr)
{
    char fullpath[FILE_PATH_MAX];
    char path[FILE_PATH_MAX - 8];

    if (httpd_query_key_value(querystr, "path", path, sizeof(path))!=ESP_OK) {
        ESP_LOGI(TAG,"Missing parameters");
        return webserver_req__send_error(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid path");
        return ESP_FAIL;
    }

    webserver_req__get_sdcard_path(path, fullpath, sizeof(fullpath));

    int total_len = req->content_len;
    int received;
    char buf[512];

    // TODO: sanitize path

    // Check if file exists.

    int fd = __open(fullpath,O_CREAT|O_WRONLY|O_TRUNC, 0660);
    if (fd<0)
    {
        ESP_LOGE(TAG, "Cannot create '%s': %d", fullpath, errno);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "File exists");
        return ESP_FAIL;
    }

    while (total_len>0) {
        received = httpd_req_recv(req, buf, MIN(total_len,sizeof(buf)));
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            close(fd);
            unlink(fullpath);
            return ESP_FAIL;
        }
        if (write(fd, buf, received)!=received) {
            close(fd);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Cannot write file");
            unlink(fullpath);
            return ESP_FAIL;
        }
        total_len -= received;
    }
    if (close(fd)<0) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Cannot close file");
        unlink(fullpath);
        return ESP_FAIL;
    }

    webserver_req__simple_json_reply(req, NULL);

    return ESP_OK;
}

int webserver_req__devlist(httpd_req_t *req, const char *querystr)
{
    cJSON *root = cJSON_CreateObject();

    devmap__populate_devices(root);

    webserver_req__send_and_free_json(req, root);
    return ESP_OK;
}


int webserver_req__wifi(httpd_req_t *req, const char *querystr)
{
    cJSON *root = cJSON_CreateObject();

    wifi__get_conf_json(root);

    webserver_req__send_and_free_json(req, root);
    return ESP_OK;
}


int webserver_req__start_scan(httpd_req_t *req, const char *querystr)
{
    cJSON *root = cJSON_CreateObject();

    int r = 0;

    if (wifi__scanning()) {
        r = -1;
        cJSON_AddStringToObject(root, "errorstring", "Scan in already in progress");
    } else {
        r = wifi__scan_json();
        if (r<0) {
            cJSON_AddStringToObject(root, "errorstring", "Internal scan error");
        }
    };

    cJSON_AddStringToObject(root, "success", r==0?"true":"false");
    webserver_req__send_and_free_json(req, root);

    return ESP_OK;
}

int webserver_req__scan(httpd_req_t *req, const char *querystr)
{
    cJSON *root = cJSON_CreateObject();

    if (wifi__scanning()) {
        cJSON_AddStringToObject(root, "errorstring", "Scan in progress");
        cJSON_AddStringToObject(root, "success", "false");

    } else {
        cJSON_AddStringToObject(root, "success", "true");

        cJSON *aplist = wifi__ap_get_json();

        cJSON_AddItemToObject(root, "aplist", aplist);
    };

    webserver_req__send_and_free_json(req, root);


    return ESP_OK;
}

static esp_err_t webserver_req__post_volume(httpd_req_t *req, const char *querystr)
{
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Not implemented");
    return ESP_FAIL;
}

static int firmware_read_wrapper(void *user, uint8_t *buf, size_t size)
{
    int remain = size;
    int read = 0;
    char *rbuf = (char*)buf;
    do {
        int r = httpd_req_recv((httpd_req_t*)user, rbuf, remain);
        if (r<0)
            return -1;
        if (r==0) {
            return read;
        }
        read += r;
        rbuf += r;
        remain -= r;
    } while (remain>0);

#if 0
    ESP_LOGI(TAG, "FW READ: %d %d\n", size, read);
    int i;
    for (i=0;i<size;i++) {
        printf("%02x ", buf[i]);
        if ((i%32)==31)
            printf("\n");
    }
#endif
    return read;
}

static esp_err_t webserver_req__firmware_upgrade(httpd_req_t *req, const char *querystr)
{
    firmware_upgrade_t f;

    firmware__init(&f,
                   firmware_read_wrapper,
                   req);

    int r =firmware__upgrade(&f);
    firmware__deinit(&f);

    if (r<0) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Error");
    } else {
        webserver_req__simple_json_reply(req, NULL);

        request_restart();
    }

    return ESP_OK;
}

static esp_err_t webserver_req__restart(httpd_req_t *req, const char *querystr)
{
    webserver_req__simple_json_reply(req, NULL);
    request_restart();
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
    { "devlist", &webserver_req__devlist },
    { "wifi",    &webserver_req__wifi },
    { "scan",    &webserver_req__scan },
    { "startscan",    &webserver_req__start_scan },
};

static const struct webserver_req_entry post_handlers[] = {
    { "file",    &webserver_req__post_file },
    { "fwupgrade", &webserver_req__firmware_upgrade },
    { "volume",    &webserver_req__post_volume },
    { "restart",    &webserver_req__restart },
};


webserver_req_handler_t webserver_find_handler(const char *path, const struct webserver_req_entry *handlers, unsigned num_entries)
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




    for (i=0; i<num_entries; i++) {
        if (strcmp(handlers[i].path, delim)==0) {
            return handlers[i].req;
        }
    }
    return NULL;
}


webserver_req_handler_t webserver_req__find_handler(const char *path)
{
    return webserver_find_handler(path, req_handlers, sizeof(req_handlers)/sizeof(req_handlers[0]));
}

webserver_req_handler_t webserver_req__find_post_handler(const char *path)
{

    return webserver_find_handler(path, post_handlers, sizeof(post_handlers)/sizeof(post_handlers[0]));
}
