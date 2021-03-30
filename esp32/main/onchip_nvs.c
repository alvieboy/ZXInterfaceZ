#include "onchip_nvs.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include <string.h>

/**
 * \defgroup nvs
 * \brief Non-volatile storage
 */


static nvs_handle_t nvsh = NVS_NO_HANDLE;

typedef union {
    float f;
    uint32_t u;
} float_uint_t;

/**
 * \ingroup nvs
 * \brief Intitalise the NVS subsystem
 */
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

/**
 * \ingroup nvs
 * \brief Fetch a signed 32-bit integer from NVS
 *
 * Fetch will have the default value def if the key is not found.
 * \param key The key to fetch
 * \param value pointer to location where the value will be stored
 * \param def default value to store if key is not found
 * \return 0 if successful
 */
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

/**
 * \ingroup nvs
 * \brief Fetch an unsigned 32-bit integer from NVS
 *
 * Fetch will have the default value def if the key is not found.
 * \param key The key to fetch
 * \param value pointer to location where the value will be stored
 * \param def default value to store if key is not found
 * \return 0 if successful
 */
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

/**
 * \ingroup nvs
 * \brief Fetch an unsigned 8-bit integer from NVS
 *
 * Fetch will have the default value def if the key is not found.
 * \param key The key to fetch
 * \param value pointer to location where the value will be stored
 * \param def default value to store if key is not found
 * \return 0 if successful
 */
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

/**
 * \ingroup nvs
 * \brief Fetch a float number from NVS
 *
 * Fetch will have the default value def if the key is not found.
 * \param key The key to fetch
 * \param value pointer to location where the value will be stored
 * \param def default value to store if key is not found
 * \return 0 if successful
 */
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

/**
 * \ingroup nvs
 * \brief Quick fetch a signed 32-bit integer from NVS
 *
 * The system will panic if there is any error fetching the value.
 * Returns the default value def if the key is not found.
 *
 * \param key The key to fetch
 * \param def default value to store if key is not found
 * \return The key value.
 */
int32_t nvs__i32(const char *key, int32_t def)
{
    int32_t ret;
    ESP_ERROR_CHECK(nvs__fetch_i32(key, &ret, def));
    return ret;
}

/**
 * \ingroup nvs
 * \brief Quick fetch an unsigned 32-bit integer from NVS
 *
 * The system will panic if there is any error fetching the value.
 * Returns the default value def if the key is not found.
 *
 * \param key The key to fetch
 * \param def default value to store if key is not found
 * \return The key value.
 */
uint32_t nvs__u32(const char *key, uint32_t def)
{
    uint32_t ret;
    ESP_ERROR_CHECK(nvs__fetch_u32(key, &ret, def));
    return ret;
}

/**
 * \ingroup nvs
 * \brief Quick fetch a float number from NVS
 *
 * The system will panic if there is any error fetching the value.
 * Returns the default value def if the key is not found.
 *
 * \param key The key to fetch
 * \param def default value to store if key is not found
 * \return The key value.
 */

float nvs__float(const char *key, float def)
{
    float ret;
    ESP_ERROR_CHECK(nvs__fetch_float(key, &ret, def));
    //printf("FLOAT %f\n", ret);

    return ret;
}


/**
 * \ingroup nvs
 * \brief Quick fetch an unsigned 8-bit integer from NVS
 *
 * The system will panic if there is any error fetching the value.
 * Returns the default value def if the key is not found.
 *
 * \param key The key to fetch
 * \param def default value to store if key is not found
 * \return The key value.
 */
uint8_t nvs__u8(const char *key, uint8_t def)
{
    uint8_t ret;
    ESP_ERROR_CHECK(nvs__fetch_u8(key, &ret, def));
    return ret;
}

/**
 * \ingroup nvs
 * \brief Close the NVS subsystem
 */
void nvs__close()
{
    if (nvsh != NVS_NO_HANDLE) {
        // Close
        nvs_close(nvsh);
        nvsh = NVS_NO_HANDLE;
    }
}

/**
 * \ingroup nvs
 * \brief Fetch a string from NVS
 *
 * Fetch will hold the default string def contents if the key is not found.
 * \param key The key to fetch
 * \param value pointer to location where the string will be stored
 * \param maxlen maximum size of buffer.
 * \param def default value to store if key is not found
 * \return 0 if successful
 */
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

/**
 * \ingroup nvs
 * \brief Quick fetch a string from NVS
 *
 * Fetch will hold the default string def contents if the key is not found.
 *
 * The system will panic if there is any error fetching the value.
 *
 * \param key The key to fetch
 * \param value pointer to location where the string will be stored
 * \param maxlen maximum size of buffer.
 * \param def default value to store if key is not found
 * \return 0 if successful
 */
int nvs__str(const char *key, char *value, unsigned maxlen, const char* def)
{
    int r = nvs__fetch_str(key,value,maxlen,def);
    if (r<0) {
        abort();
    }
    return r;
}

/**
 * \ingroup nvs
 * \brief Save an unsigned 32-bit value to NVS
 *
 * \param key The key to store
 * \param value value to store
 * \return 0 if successful
 */
int nvs__set_u32(const char *key, uint32_t val)
{
    esp_err_t err;
    if (nvsh==NVS_NO_HANDLE) {
        err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvsh);
        if (err != ESP_OK) return err;
    }
    return nvs_set_u32(nvsh,key,val);
}

/**
 * \ingroup nvs
 * \brief Save an unsigned 8-bit value to NVS
 *
 * \param key The key to store
 * \param value value to store
 * \return 0 if successful
 */
int nvs__set_u8(const char *key, uint8_t val)
{
    esp_err_t err;
    if (nvsh==NVS_NO_HANDLE) {
        err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvsh);
        if (err != ESP_OK) return err;
    }
    return nvs_set_u8(nvsh,key,val);

}

/**
 * \ingroup nvs
 * \brief Save a float number to NVS
 *
 * \param key The key to store
 * \param value value to store
 * \return 0 if successful
 */
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

/**
 * \ingroup nvs
 * \brief Save a string to NVS
 *
 * \param key The key to store
 * \param value string to store
 * \return 0 if successful
 */
int nvs__set_str(const char *key, const char* val)
{
    esp_err_t err;
    if (nvsh==NVS_NO_HANDLE) {
        err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvsh);
        if (err != ESP_OK) return err;
    }
    return nvs_set_str(nvsh,key,val);
}

/**
 * \ingroup nvs
 * \brief Commit NVS changes
 *
 * This should be called after setting values.
 *
 * \return 0 if successful
 */

int nvs__commit()
{
    return nvs_commit(nvsh);
}
