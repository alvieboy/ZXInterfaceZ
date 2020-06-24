#include <cJSON.h>


cJSON *json__load_from_file(const char *filename);
char *json__get_string_alloc(cJSON*, const char*);
const char *json__get_string(cJSON*, const char*);
