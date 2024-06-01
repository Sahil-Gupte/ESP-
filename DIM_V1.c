/******Dimming_V1******/

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <driver/gpio.h>

#define ZCD_PIN        4   
#define OUTPUT_PIN     5

void IRAM_ATTR zcd_isr_handler(void* arg) {
    gpio_set_level(OUTPUT_PIN, !gpio_get_level(OUTPUT_PIN));
}

void app_main(void) {
    
    gpio_pad_select_gpio(ZCD_PIN);
    gpio_set_direction(ZCD_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(ZCD_PIN, GPIO_PULLDOWN_ONLY);
    gpio_pad_select_gpio(OUTPUT_PIN);
    gpio_set_direction(OUTPUT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(OUTPUT_PIN, 1); 
    gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
    gpio_isr_handler_add(ZCD_PIN, zcd_isr_handler, NULL);

    printf("\nZCD Dimming!\n");
}