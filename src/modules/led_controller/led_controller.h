#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "Arduino.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LED_PATTERN_OFF = 0,
    LED_PATTERN_SOLID,
    LED_PATTERN_BLINK_SLOW,
    LED_PATTERN_BLINK_FAST,
    LED_PATTERN_BREATHE
} led_pattern_t;

typedef struct {
    uint8_t pin;
    led_pattern_t current_pattern;
    TaskHandle_t task_handle;
    EventGroupHandle_t event_group;
    bool task_running;
} led_controller_t;

// Events
#define LED_EVENT_PATTERN_CHANGED (1 << 0)

led_controller_t* led_controller_create(uint8_t led_pin);
void led_controller_destroy(led_controller_t* controller);
void led_controller_set_pattern(led_controller_t* controller, led_pattern_t pattern);
bool led_controller_start(led_controller_t* controller);
void led_controller_stop(led_controller_t* controller);

#ifdef __cplusplus
}
#endif

#endif