#include <stdio.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "esp_wifi.h"
#include "wifi_task.h"
#include "event_task.h"

BaseType_t xTaskCreatePinnedToCore(TaskFunction_t pvTaskCode, const char *const pcName, const uint32_t usStackDepth, void *const pvParameters,
                                   UBaseType_t uxPriority, TaskHandle_t *const pvCreatedTask, const BaseType_t xCoreID);

char *inet_ntoa_r(struct in_addr in, char *dest, size_t len);
int interfacez_main(int argc, char **argv);

void vApplicationMallocFailedHook(void);
void vApplicationTickHook(void);

/* ESP rom */
#include "esp32/rom/uart.h"


uint64_t pinstate = 0xFFFFFFFFFFFFFFFF;

void vAssertCalled( unsigned long ulLine, const char * const pcFileName )
{
    printf("ASSERT: %s : %d\n", pcFileName, (int)ulLine);
    abort();
}


void vApplicationMallocFailedHook()
{
    abort();
}


void _esp_error_check_failed(esp_err_t rc, const char *file, int line, const char *function, const char *expression)
{
    fprintf(stderr,"CHECK FAILED: %d file %s line %d %s %s\n", rc,file,line,function,expression);
    abort();
}

void vApplicationTickHook(void)
{
}

void esp_restart()
{
    fprintf(stderr,"RESTART REQUESTED!!!!");
    exit(0);
}

BaseType_t xTaskCreatePinnedToCore(TaskFunction_t pvTaskCode, const char *const pcName, const uint32_t usStackDepth, void *const pvParameters, UBaseType_t uxPriority, TaskHandle_t *const pvCreatedTask, const BaseType_t xCoreID)
{
    BaseType_t r = xTaskCreate(pvTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pvCreatedTask);
    return r;
}

STATUS uart_rx_one_char(uint8_t *c)
{
    if (read(0, c, 1)==1) {
        return 0;
    }
    return -1;
}

char *inet_ntoa_r(struct in_addr in, char *dest, size_t len)
{
    char * p =  inet_ntoa(in);
    strncpy(dest,p,len);
    return p;
}

void esp_chip_info(esp_chip_info_t* out_info)
{
    out_info->model = 1;
    out_info->features = 0;
    out_info->cores  = 2;
    out_info->revision = 4;
}

extern void app_main(void);
extern void fpga_init(void);

static void wrap_app_main(void *data)
{
    fpga_init();
    app_main();
}

#include <unistd.h>

char startupdir[512] = {0};

extern int partition_init();

int interfacez_main(int argc, char **argv)
{
    TaskHandle_t h;
    getcwd(startupdir, sizeof(startupdir));

    fcntl(0, F_SETFL, fcntl(0,F_GETFL)|O_NONBLOCK);

    wifi_task_init();
    event_task_init();

    if (partition_init()<0)
        return -1;
    printf("Start WiFi task\n");
    xTaskCreate(wifi_task, "wifi_scan", 4096, NULL, 8, &h);
    printf("Start Event task\n");
    xTaskCreate(event_task, "event_scan", 4096, NULL, 8, &h);
    printf("Start main task\n");
    xTaskCreate(wrap_app_main, "main", 4096, NULL, 7, &h);
    vTaskStartScheduler();
}

const char *esp_err_to_name(int err)
{
    return "Unknown";
};
