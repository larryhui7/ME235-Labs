/* ME235 Communication Protocol */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

#define CONFIG_EXAMPLE_UART_TXD 1
#define CONFIG_EXAMPLE_UART_RXD 3
#define CONFIG_EXAMPLE_UART_PORT_NUM 0
#define CONFIG_EXAMPLE_UART_BAUD_RATE 115200
#define CONFIG_EXAMPLE_TASK_STACK_SIZE 4096

#define ECHO_TEST_TXD (CONFIG_EXAMPLE_UART_TXD)
#define ECHO_TEST_RXD (CONFIG_EXAMPLE_UART_RXD)
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

#define ECHO_UART_PORT_NUM   (CONFIG_EXAMPLE_UART_PORT_NUM)
#define ECHO_UART_BAUD_RATE  (CONFIG_EXAMPLE_UART_BAUD_RATE)
#define ECHO_TASK_STACK_SIZE (CONFIG_EXAMPLE_TASK_STACK_SIZE)

#define BUF_SIZE (1024)

static int threshold_value = 128;
static int blur_value = 5;
static int min_area_value = 1000;
static int display_enabled = 0;

TaskHandle_t myTaskHandle = NULL;

static void echo_task(void *arg)
{
    uart_config_t uart_config = {
        .baud_rate = ECHO_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));

    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    // Initial boot message
    uart_write_bytes(ECHO_UART_PORT_NUM, "Commands Ready\r\n", strlen("Commands Ready\r\n"));

    while (1)
    {
        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);

        if (len)
        {
            data[len] = '\0';

            switch (data[0])
            {
                case 'I':
                    uart_write_bytes(ECHO_UART_PORT_NUM, "ESP32\r\n", strlen("ESP32\r\n"));
                    break;

                case 'T':
                {
                    char *ptr = (char *)data + 1;
                    char reply[96];

                    threshold_value = 0;
                    while (*ptr >= '0' && *ptr <= '9')
                    {
                        threshold_value = threshold_value * 10 + (*ptr - '0');
                        ptr++;
                    }

                    if (*ptr == 'L')
                    {
                        ptr++;
                        blur_value = 0;
                        while (*ptr >= '0' && *ptr <= '9')
                        {
                            blur_value = blur_value * 10 + (*ptr - '0');
                            ptr++;
                        }
                    }

                    if (*ptr == 'M')
                    {
                        ptr++;
                        min_area_value = 0;
                        while (*ptr >= '0' && *ptr <= '9')
                        {
                            min_area_value = min_area_value * 10 + (*ptr - '0');
                            ptr++;
                        }
                    }

                    snprintf(reply, sizeof(reply), "THRESHOLD:%d BLUR:%d MINAREA:%d\r\n",
                             threshold_value, blur_value, min_area_value);
                    uart_write_bytes(ECHO_UART_PORT_NUM, reply, strlen(reply));
                    break;
                }

                case 'A':
                    display_enabled = 1;
                    uart_write_bytes(ECHO_UART_PORT_NUM, "DISPLAY:ON\r\n", strlen("DISPLAY:ON\r\n"));
                    break;

                case 'B':
                    display_enabled = 0;
                    uart_write_bytes(ECHO_UART_PORT_NUM, "DISPLAY:OFF\r\n", strlen("DISPLAY:OFF\r\n"));
                    break;

                case 'R':
                    threshold_value = 128;
                    blur_value = 5;
                    min_area_value = 1000;
                    uart_write_bytes(ECHO_UART_PORT_NUM, "FILTERS:RESET\r\n", strlen("FILTERS:RESET\r\n"));
                    break;

                case 'S':
                {
                    char reply[128];
                    snprintf(reply, sizeof(reply), "STATUS DISPLAY=%d THR=%d BLUR=%d MIN=%d\r\n",
                             display_enabled, threshold_value, blur_value, min_area_value);
                    uart_write_bytes(ECHO_UART_PORT_NUM, reply, strlen(reply));
                    break;
                }
            }
        }
    }
}

void app_main(void)
{
    gpio_reset_pin(13);
    gpio_set_direction(13, GPIO_MODE_OUTPUT);

    xTaskCreate(echo_task, "uart_echo_task", CONFIG_EXAMPLE_TASK_STACK_SIZE, NULL, 10, NULL);
}