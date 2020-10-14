#include "onchip_nvs.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include <string.h>

static nvs_handle_t nvsh = NVS_NO_HANDLE;

typedef union {
    float f;
    uint32_t u;
} float_uint_t;

void nvs__init()
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}


int nvs__fetch_i32(const char *key, int32_t *value, int32_t def)
{
    esp_err_t err;

    if (nvsh==NVS_NO_HANDLE) {
        err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvsh);
        if (err != ESP_OK) return err;
    }

    err = nvs_get_i32(nvsh, key, value);
    if (err==ESP_ERR_NVS_NOT_FOUND) {
        *value = def;
        return 0;
    }

    if (err != ESP_OK) {
        return -1;
    }
    return 0;
}

int nvs__fetch_u32(const char *key, uint32_t *value, uint32_t def)
{
    esp_err_t err;

    if (nvsh==NVS_NO_HANDLE) {
        err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvsh);
        if (err != ESP_OK) return err;
    }

    err = nvs_get_u32(nvsh, key, value);
    if ((err==ESP_ERR_NVS_NOT_FOUND)) {
        //printf("FLOAT DEF\n");
        *value = def;
        return 0;
    }
    //printf("FLOAT OK %d err %d\n", *value, err);


    if (err != ESP_OK) {
        return err;
    }
    return 0;
}

int nvs__fetch_u8(const char *key, uint8_t *value, uint8_t def)
{
    esp_err_t err;

    if (nvsh==NVS_NO_HANDLE) {
        err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvsh);
        if (err != ESP_OK) return err;
    }

    err = nvs_get_u8(nvsh, key, value);
    if (err==ESP_ERR_NVS_NOT_FOUND) {
        *value = def;
        return 0;
    }

    if (err != ESP_OK) {
        return err;
    }
    return 0;
}

int nvs__fetch_float(const char *key, float *value, float def)
{
    float_uint_t v;
    float_uint_t d;
    d.f = def;

    int r = nvs__fetch_u32(key, &v.u, d.u);
    if (r==0) {
        //printf("FLOAT Conv %f\n", v.f);

        *value = v.f;
    }
    return r;
}

int32_t nvs__i32(const char *key, int32_t def)
{
    int32_t ret;
    ESP_ERROR_CHECK(nvs__fetch_i32(key, &ret, def));
    return ret;
}

uint32_t nvs__u32(const char *key, uint32_t def)
{
    uint32_t ret;
    ESP_ERROR_CHECK(nvs__fetch_u32(key, &ret, def));
    return ret;
}

float nvs__float(const char *key, float def)
{
    float ret;
    ESP_ERROR_CHECK(nvs__fetch_float(key, &ret, def));
    //printf("FLOAT %f\n", ret);

    return ret;
}


uint8_t nvs__u8(const char *key, uint8_t def)
{
    uint8_t ret;
    ESP_ERROR_CHECK(nvs__fetch_u8(key, &ret, def));
    return ret;
}

void nvs__close()
{
    if (nvsh != NVS_NO_HANDLE) {
        // Close
        nvs_close(nvsh);
        nvsh = NVS_NO_HANDLE;
    }
}
int nvs__fetch_str(const char *key, char *value, unsigned maxlen, const char* def)
{
    esp_err_t err;
    size_t len = maxlen;

    if (nvsh==NVS_NO_HANDLE) {
        err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvsh);
        if (err != ESP_OK) return err;
    }

    err = nvs_get_str(nvsh, key, value, &len);
    if (err==ESP_ERR_NVS_NOT_FOUND) {
        len = strlen(def);
        memcpy( value, def, len );
        value[len] = '\0';
        return len;
    }

    if (err != ESP_OK) {
        return -1;
    }

    value[len] = '\0';

    return len;
}

int nvs__str(const char *key, char *value, unsigned maxlen, const char* def)
{
    int r = nvs__fetch_str(key,value,maxlen,def);
    if (r<0) {
        abort();
    }
    return r;
}

int nvs__set_u32(const char *key, uint32_t val)
{
    esp_err_t err;
    if (nvsh==NVS_NO_HANDLE) {
        err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvsh);
        if (err != ESP_OK) return err;
    }
    return nvs_set_u32(nvsh,key,val);
}

int nvs__set_u8(const char *key, uint8_t val)
{
    esp_err_t err;
    if (nvsh==NVS_NO_HANDLE) {
        err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvsh);
        if (err != ESP_OK) return err;
    }
    return nvs_set_u8(nvsh,key,val);

}

int nvs__set_float(const char *key, float val)
{
    float_uint_t v;
    v.f = val;

    esp_err_t err;
    if (nvsh==NVS_NO_HANDLE) {
        err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvsh);
        if (err != ESP_OK) return err;
    }
    return nvs_set_u32(nvsh,key,v.u);

}

int nvs__set_str(const char *key, const char* val)
{
    esp_err_t err;
    if (nvsh==NVS_NO_HANDLE) {
        err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvsh);
        if (err != ESP_OK) return err;
    }
    return nvs_set_str(nvsh,key,val);
}

int nvs__commit()
{
    return nvs_commit(nvsh);
}