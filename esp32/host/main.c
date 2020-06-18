#include <stdio.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <netinet/in.h>
#include <arpa/inet.h>

void vAssertCalled( unsigned long ulLine, const char * const pcFileName )
{
    printf("ASSERT: %s : %d\n", pcFileName, (int)ulLine);
    while(1);
}

void vApplicationMallocFailedHook()
{
}
void _esp_error_check_failed(esp_err_t rc, const char *file, int line, const char *function, const char *expression)
{
    while(1);
}

void vApplicationTickHook()
{
}

void esp_restart()
{
    abort();
}

BaseType_t xTaskCreatePinnedToCore(TaskFunction_t pvTaskCode, const char *const pcName, const uint32_t usStackDepth, void *const pvParameters, UBaseType_t uxPriority, TaskHandle_t *const pvCreatedTask, const BaseType_t xCoreID)
{
    BaseType_t r = xTaskCreate(pvTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pvCreatedTask);
    return r;
}

int uart_rx_one_char()
{
    return -1;
}

char *inet_ntoa_r(struct in_addr in)
{
    return inet_ntoa(in);
}

void esp_chip_info(esp_chip_info_t* out_info)
{
    out_info->model = 1;
    out_info->features = 0;
    out_info->cores  = 2;
    out_info->revision = 4;
}

extern void app_main(void);
extern void fpga_init();
void wrap_app_main(void *data)
{
    fpga_init();
    app_main();
}

#include <unistd.h>

char startupdir[512] = {0};

int main(int argc, char **argv)
{
    TaskHandle_t h;
    getcwd(startupdir, sizeof(startupdir));
    xTaskCreate(wrap_app_main, "main", 4096, NULL, 7, &h);
    vTaskStartScheduler();
}
