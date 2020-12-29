#include "poke.h"
#include "fileaccess.h"
#include "text.h"
#include "esp_log.h"
#include "rom.h"

#define TAG "POK"


void poke__init(poke_t *poke)
{
    poke->f = NULL;
    poke->ask_fun = NULL;
    poke->write_fun = NULL;
}

int poke__setaskfunction(poke_t *poke, poke_value_ask_fun_t askfun, void *user)
{
    poke->ask_fun = askfun;
    poke->ask_fun_user = user;
    return 0;
}

int poke__setmemorywriter(poke_t *poke, poke_mem_write_fun_t writefun, void *user)
{
    poke->write_fun = writefun;
    poke->write_fun_user = user;
    return 0;
}


static int poke__readline(poke_t *poke, char *dest, unsigned len)
{
    if (feof(poke->f))
    {
        return -2;
    }

    char *l = fgets(dest, len, poke->f);

    if (l==NULL)
        return -1;

    return 0;
}

int poke__openfile(poke_t *poke, const char *filename)
{
    char fullfile[256];

    fullpath(filename, fullfile, sizeof(fullfile)-1);

    poke->f = __fopen(fullfile,"r");

    if (poke->f == NULL)
        return -1;

    return 0;;
}

int poke__loadentries(poke_t *poke, void (*handler)(void *, const char *), void *user)
{
    pokeline_t line;
    int r;
    ESP_LOGI(TAG, "Loading POK entries");
    rewind(poke->f);
    do {
        r = poke__readline(poke, line, sizeof(line));
        if (r==-1) {
            ESP_LOGE(TAG, "Short line read");
            return -1;
        }
        if (r==-2)
            break;  // EOF

        // Remove newlines
        chomp(line);
        ESP_LOGD(TAG, "Line: %s", line);
        switch (line[0]) {
        case 'N':
            handler(user, &line[1]);
            break;
        case 'M': /* Fall-through */
        case 'Z': /* Fall-through */
            break;
        case 'Y':
            break;
        case '\0':
            break;
        default:
            ESP_LOGE(TAG, "Unexpected marker '%c'", line[0]);
            return -1; // Unexpected
            break;
        }
    } while (1);
    return 0;
}

void poke__set_memory(poke_t *poke, uint8_t bank, uint16_t address, uint8_t value)
{
    if (poke->write_fun)
        poke->write_fun(poke->write_fun_user, bank, address, value);
}


static int poke__apply_poke(poke_t *poke, char *line)
{
    char *toks[4];
    char *endp;

    if (*line!=' ') {
        ESP_LOGE(TAG, "Malformed POK: not a space");
        return -1;
    }
    line++;

    toks[0] = strtok(line," "); // bank
    toks[1] = strtok(NULL," "); // Address
    toks[2] = strtok(NULL," "); // value
    toks[3] = strtok(NULL," "); // original

    if (toks[1]==NULL || toks[2]==NULL || toks[3]==NULL) {
        ESP_LOGE(TAG, "Malformed POK: missing fields");
        return -1;
    }

    // Convert them into integers

    endp = NULL;
    unsigned long bank = strtoul(toks[0], &endp, 0);
    if (NULL==endp || (*endp)!='\0') {
        ESP_LOGE(TAG, "Malformed POK: invalid bank");
        return -1;
    }
    endp = NULL;
    unsigned long address = strtoul(toks[1], &endp, 0);
    if (NULL==endp || (*endp)!='\0') {
        ESP_LOGE(TAG, "Malformed POK: invalid address");
        return -1;
    }

    endp = NULL;
    unsigned long value = strtoul(toks[2], &endp, 0);
    if (NULL==endp || (*endp)!='\0') {
        ESP_LOGE(TAG, "Malformed POK: invalid value");
        return -1;
    }
#if 0
    endp = NULL;
    unsigned long old = strtoul(toks[3], &endp, 0);
    if (NULL==endp || (*endp)!='\0') {
        ESP_LOGE(TAG, "Malformed POK: invalid old value");
        return -1;
    }
#endif

    if (value==256) {
        ESP_LOGI(TAG, "Ask user");
        if (poke->ask_fun)
            value = poke->ask_fun(poke->ask_fun_user);
        ESP_LOGI(TAG, "User data 0x%02x (%d)", (unsigned)value, (unsigned)value);
    }

    /*
    The POKE line has the format:

    lbbb aaaaa vvv ooo

Where l determines the content, bbb is the bank, aaaaa is the address to be poked with value vvv and ooo is the original value of aaaaa. All values are decimal, and separated by one or more spaces, apart from between l and bbb; however, the bank value is never larger than 8, so you will always see 2 spaces in front of the bank. The field bank field is built from;

    bit 0-2 : bank value
    bit 3 : ignore bank (1=yes, always set for 48K games)

    If the field [value] is in range 0-255, this is the value to be POKEd. If it is 256, a requester should pop up where the user can enter a value.
    */
    // M 8 34801 195 0
    ESP_LOGD(TAG, "Applying value");
    poke__set_memory(poke, bank & 0xff, address & 0xFFFF, value & 0xff);

    return 0;
}

int poke__apply_trainer(poke_t *poke, const char *name)
{
    pokeline_t line;
    bool found = false;
    bool eof = false;

    rewind(poke->f);

    ESP_LOGI(TAG,"Applying trainer '%s'", name);

    do {
        ESP_LOGD(TAG, "Reading poke line");
        if (poke__readline(poke, line, sizeof(line))<0)
            return -1;

        chomp(line);
        ESP_LOGD(TAG, "Line: %s", line);
        switch (line[0]) {
        case 'N':
            if (found)
                return 0; // Done
            if (strcmp(&line[1], name)==0) {
                ESP_LOGI(TAG, "Found trainer '%s'", name);
                found = true;
            }
            break;
        case 'Z': /* Fall-through */
        case 'M':
            if (found) {
                if (poke__apply_poke(poke, &line[1])<0) {
                    ESP_LOGE(TAG, "Cannot apply poke");
                    return -1;
                }
            }
            break;
        case 'Y': /* Fall-through */
        case '\0':
            eof = true;
            break;
        default:
            ESP_LOGE(TAG, "Unexpected marker '%c'", line[0]);
            return -1; // Unexpected
            break;
        }
    } while (!eof);

    if (!found) {
        ESP_LOGI(TAG,"Could not find trainer");
        return -1;
    }
    ESP_LOGI(TAG, "Trainer applied successfully");
    return 0;
}


void poke__close(poke_t *poke)
{
    fclose(poke->f);
}



