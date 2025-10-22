#include "led_controller.h"

static const char* TAG = "LedController";

static void led_controller_task(void* arg) {
    led_controller_t* controller = (led_controller_t*)arg;
    bool led_state = false;
    uint32_t blink_counter = 0;
    
    ESP_LOGI(TAG, "LED controller task started on pin %d", controller->pin);
    
    pinMode(controller->pin, OUTPUT);
    digitalWrite(controller->pin, LOW);
    
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(100);
    
    while (controller->task_running) {
        // Update LED based on current pattern
        switch (controller->current_pattern) {
            case LED_PATTERN_OFF:
                digitalWrite(controller->pin, LOW);
                break;
                
            case LED_PATTERN_SOLID:
                digitalWrite(controller->pin, HIGH);
                break;
                
            case LED_PATTERN_BLINK_SLOW:
                if (blink_counter % 10 == 0) { // 1 second interval
                    led_state = !led_state;
                    digitalWrite(controller->pin, led_state ? HIGH : LOW);
                }
                break;
                
            case LED_PATTERN_BLINK_FAST:
                if (blink_counter % 2 == 0) { // 200ms interval
                    led_state = !led_state;
                    digitalWrite(controller->pin, led_state ? HIGH : LOW);
                }
                break;
                
            case LED_PATTERN_BREATHE:
                // Simple breathe pattern using PWM (simulated)
                {
                    uint8_t breathe_value = (blink_counter % 20) * 12;
                    digitalWrite(controller->pin, (breathe_value > 100) ? HIGH : LOW);
                }
                break;
        }
        
        blink_counter++;
        
        // Wait for next cycle or event
        EventBits_t events = xEventGroupWaitBits(
            controller->event_group,
            LED_EVENT_PATTERN_CHANGED,
            pdTRUE,  // Clear on exit
            pdFALSE, // Don't wait for all bits
            frequency
        );
        
        vTaskDelayUntil(&last_wake_time, frequency);
    }
    
    digitalWrite(controller->pin, LOW);
    ESP_LOGI(TAG, "LED controller task exiting");
    vTaskDelete(NULL);
}

led_controller_t* led_controller_create(uint8_t led_pin) {
    led_controller_t* controller = (led_controller_t*)malloc(sizeof(led_controller_t));
    if (!controller) {
        ESP_LOGE(TAG, "Failed to allocate LED controller");
        return NULL;
    }
    
    controller->pin = led_pin;
    controller->current_pattern = LED_PATTERN_OFF;
    controller->task_handle = NULL;
    controller->task_running = false;
    
    // Create event group
    controller->event_group = xEventGroupCreate();
    if (!controller->event_group) {
        ESP_LOGE(TAG, "Failed to create event group");
        free(controller);
        return NULL;
    }
    
    ESP_LOGI(TAG, "LED controller created for pin %d", led_pin);
    return controller;
}

void led_controller_destroy(led_controller_t* controller) {
    if (!controller) return;
    
    led_controller_stop(controller);
    
    if (controller->event_group) {
        vEventGroupDelete(controller->event_group);
    }
    
    free(controller);
    ESP_LOGI(TAG, "LED controller destroyed");
}

bool led_controller_start(led_controller_t* controller) {
    if (!controller || controller->task_running) {
        return false;
    }
    
    BaseType_t result = xTaskCreate(
        led_controller_task,
        "led_controller",
        2048,
        controller,
        5,
        &controller->task_handle
    );
    
    if (result == pdPASS) {
        controller->task_running = true;
        ESP_LOGI(TAG, "LED controller task started successfully");
        return true;
    }
    
    ESP_LOGE(TAG, "Failed to start LED controller task");
    return false;
}

void led_controller_stop(led_controller_t* controller) {
    if (!controller || !controller->task_running) {
        return;
    }
    
    controller->task_running = false;
    
    if (controller->task_handle) {
        vTaskDelay(pdMS_TO_TICKS(100));
        vTaskDelete(controller->task_handle);
        controller->task_handle = NULL;
    }
    
    ESP_LOGI(TAG, "LED controller stopped");
}

void led_controller_set_pattern(led_controller_t* controller, led_pattern_t pattern) {
    if (!controller) return;
    
    controller->current_pattern = pattern;
    xEventGroupSetBits(controller->event_group, LED_EVENT_PATTERN_CHANGED);
    
    ESP_LOGI(TAG, "LED pattern changed to: %d", pattern);
}