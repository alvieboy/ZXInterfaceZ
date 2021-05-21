#include <stdio.h>
#include "esp_system.h"
#include "os/task.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "esp_wifi.h"
#include "wifi_task.h"
#include "event_task.h"
#include <pty.h>
#include <unistd.h>

BaseType_t xTaskCreatePinnedToCore(TaskFunction_t pvTaskCode, const char *const pcName, const uint32_t usStackDepth, void *const pvParameters,
                                   UBaseType_t uxPriority, TaskHandle_t *const pvCreatedTask, const BaseType_t xCoreID);

char *inet_ntoa_r(struct in_addr in, char *dest, size_t len);
int interfacez_main(int argc, char **argv);

void vApplicationMallocFailedHook(void);
void vApplicationTickHook(void);

int ptyfd = -1;

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
    BaseType_t r = task__create(pvTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pvCreatedTask);
    return r;
}

void periph_module_reset(uint32_t module)
{
}

unsigned long xPortGetFreeHeapSize()
{
    return 0;
}

STATUS uart_rx_one_char(uint8_t *c)
{
    if (read(0, c, 1)==1) {
        return 0;
    }
    if (ptyfd>=0) {
        int r = read(ptyfd, c, 1);
        if (r==1) {
            printf("DATA %d %02x\n",ptyfd, *c);
            return 0;
        } else {
            if (r<0) {
                switch (errno) {
                case EINTR: /* Fall-through */
                case EWOULDBLOCK:
                    break;
                default:
                    reopen_pty();
                    break;
                }
            }
            return -1;
        }
    }
    return -1;
}

STATUS uart_tx_one_char(uint8_t TxChar)
{
    if (ptyfd>=0) {
        int r = write(ptyfd, &TxChar, 1);
        if (r==1)
            return 0;
        return -1;
    }
    return 0;  // Ignore
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

#include <termios.h>
#include <sys/ioctl.h>

void reopen_pty()
{
    char pts[256];
    int slave;

    struct termios term;
    memset(&term, 0, sizeof(term));

    term.c_lflag &= ~(ISIG | ICANON | ECHO | IEXTEN);
    term.c_cc[VMIN] = 0;
    term.c_cc[VTIME] = 0;

    term.c_cflag |= (CLOCAL | CREAD);
    term.c_cflag &= ~(HUPCL);

    if (ptyfd>=0)
        close(ptyfd);

    int r = openpty(&ptyfd, &slave,
                    pts,
                    &term,
                    NULL);
    if (r<0) {
        printf("Cannot open PTY");
        return ;
    }

    printf("Console PTY %s\n", pts);

    fcntl(ptyfd, F_SETFL, fcntl(ptyfd,F_GETFL)|O_NONBLOCK);


#if 0
    ptyfd = posix_openpt(O_RDWR|O_NOCTTY);

    if (ptyfd>=0) {
        char pts[256];
        grantpt(ptyfd);
        unlockpt(ptyfd);
        if (ptsname_r(ptyfd, pts, sizeof(pts))<0) {
            close(ptyfd);
        }
        struct termios term;

        ioctl(ptyfd, TCGETS, &term);

        term.c_lflag &= ~(ISIG | ICANON | ECHO | IEXTEN);
	term.c_cc[VMIN] = 0;
	term.c_cc[VTIME] = 0;

	term.c_cflag |= (CLOCAL | CREAD);
	term.c_cflag &= ~(HUPCL);

        ioctl(ptyfd, TCSETS, &term);
        fcntl(ptyfd, F_SETFL, ioctl(ptyfd, F_GETFL)| O_NONBLOCK);
        printf("Console PTY %s\n", pts);
    }
#endif


}

extern int run_tests();

int interfacez_main(int argc, char **argv)
{
    TaskHandle_t h;
    getcwd(startupdir, sizeof(startupdir));


    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    setlinebuf(stdout);
    setlinebuf(stderr);


    fcntl(0, F_SETFL, fcntl(0,F_GETFL)|O_NONBLOCK);

    if (run_tests()<0) {
        printf(stderr, "TESTS FAILED!\n");
        abort();
        return -1;
    } else {
        printf("Tests OK\n");
    }

    wifi_task_init();
    event_task_init();

    // Open pty
    reopen_pty();
    if (partition_init()<0)
        return -1;
    printf("Start WiFi task\n");
    xTaskCreate(wifi_task, "wifi_scan", 4096, NULL, 8, &h);

    printf("Start Event task\n");
    xTaskCreate(event_task, "event_scan", 4096, NULL, 8, &h);

    printf("Start main task\n");
    xTaskCreate(wrap_app_main, "main", 4096, NULL, 4, &h);

    vTaskStartScheduler();
}

const char *esp_err_to_name(int err)
{
    return "Unknown";
};
