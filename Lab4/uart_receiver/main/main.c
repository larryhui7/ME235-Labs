#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define UART_PORT_NUM UART_NUM_1
#define UART_TX_PIN 1
#define UART_RX_PIN 3
#define BUF_SIZE 1024
#define BAUD_RATE 115200

static const char *TAG = "ME235";

static int threshold_value = 128;
static int blur_value = 5;
static int min_area_value = 1000;
static int display_enabled = 0;

static void send_text(const char *msg)
{
    uart_write_bytes(UART_PORT_NUM, msg, strlen(msg));
}

static void process_command(char *cmd)
{
    if (strcmp(cmd, "I") == 0) {
        send_text("ID:DISPLAY_CONTROLLER\n");
    }
    else if (strcmp(cmd, "A") == 0) {
        display_enabled = 1;
        send_text("DISPLAY:ON\n");
    }
    else if (strcmp(cmd, "B") == 0) {
        display_enabled = 0;
        send_text("DISPLAY:OFF\n");
    }
    else if (strcmp(cmd, "R") == 0) {
        threshold_value = 128;
        blur_value = 5;
        min_area_value = 1000;
        send_text("FILTERS:RESET\n");
    }
    else if (strncmp(cmd, "T", 1) == 0) {
        threshold_value = atoi(cmd + 1);
        char reply[64];
        snprintf(reply, sizeof(reply), "THRESHOLD:%d\n", threshold_value);
        send_text(reply);
    }
    else if (strncmp(cmd, "L", 1) == 0) {
        blur_value = atoi(cmd + 1);
        char reply[64];
        snprintf(reply, sizeof(reply), "BLUR:%d\n", blur_value);
        send_text(reply);
    }
    else if (strncmp(cmd, "M", 1) == 0) {
        min_area_value = atoi(cmd + 1);
        char reply[64];
        snprintf(reply, sizeof(reply), "MINAREA:%d\n", min_area_value);
        send_text(reply);
    }
    else if (strcmp(cmd, "S") == 0) {
        char reply[128];
        snprintf(reply, sizeof(reply),
                 "STATUS DISPLAY=%d THR=%d BLUR=%d MIN=%d\n",
                 display_enabled, threshold_value, blur_value, min_area_value);
        send_text(reply);
    }
    else {
        send_text("ERR:UNKNOWN_COMMAND\n");
    }
}

void app_main(void)
{
    uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    send_text("ME235 READY\n");

    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    while (1) {
        int len = uart_read_bytes(UART_PORT_NUM, data, BUF_SIZE - 1, pdMS_TO_TICKS(100));
        if (len > 0) {
            data[len] = 0;

            char *newline = strpbrk((char *)data, "\r\n");
            if (newline) *newline = 0;

            process_command((char *)data);
        }
    }
}