#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "mysql.h"

#define RS485_RX_PIN 16
#define RS485_TX_PIN 17
#define RS485_UART_NUM UART_NUM_1
#define RS485_UART_BAUDRATE 9600

static const char *TAG = "RS485_MYSQL";

#define WIFI_SSID ""
#define WIFI_PASS ""
#define MYSQL_HOST ""
#define MYSQL_PORT 3306
#define MYSQL_USER ""
#define MYSQL_PASS ""
#define MYSQL_DB ""

#define DATA_BUFFER_SIZE 128
static char data_buffer[DATA_BUFFER_SIZE];

void rs485_init() {
    uart_config_t uart_config = {
        .baud_rate = RS485_UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_param_config(RS485_UART_NUM, &uart_config);
    uart_set_pin(RS485_UART_NUM, RS485_TX_PIN, RS485_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(RS485_UART_NUM, 1024, 0, 0, NULL, 0);
}

void wifi_init_sta() {
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void mysql_task(void *pvParameters) {
    MYSQL *mysql = mysql_init(NULL);
    if (mysql == NULL) {
        ESP_LOGE(TAG, "mysql_init failed");
        vTaskDelete(NULL);
    }

    if (!mysql_real_connect(mysql, MYSQL_HOST, MYSQL_USER, MYSQL_PASS, MYSQL_DB, MYSQL_PORT, NULL, 0)) {
        ESP_LOGE(TAG, "mysql_real_connect failed: %s", mysql_error(mysql));
        mysql_close(mysql);
        vTaskDelete(NULL);
    }

    while (1) {
        int len = uart_read_bytes(RS485_UART_NUM, (uint8_t *)data_buffer, DATA_BUFFER_SIZE - 1, 20 / portTICK_RATE_MS);
        if (len > 0) {
            data_buffer[len] = '\0';
            
            char query[128];
            snprintf(query, sizeof(query), "INSERT INTO sensor_data (value) VALUES ('%s')", data_buffer);
            if (mysql_query(mysql, query) != 0) {
                ESP_LOGE(TAG, "mysql_query failed: %s", mysql_error(mysql));
            } else {
                ESP_LOGI(TAG, "Data stored in database");
            }

            vTaskDelay(1000 / portTICK_RATE_MS);
        }
    }

    mysql_close(mysql);
    vTaskDelete(NULL);
}

void app_main() {
    ESP_ERROR_CHECK(nvs_flash_init());
    rs485_init();
    wifi_init_sta();
    xTaskCreate(mysql_task, "mysql_task", 4096, NULL, 5, NULL);
}
