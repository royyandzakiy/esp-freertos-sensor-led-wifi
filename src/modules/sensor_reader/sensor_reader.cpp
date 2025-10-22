#include "sensor_reader.h"
#include <stdlib.h>

static const char* TAG = "SensorReader";

static void sensor_reader_task(void* arg) {
    sensor_reader_t* reader = (sensor_reader_t*)arg;
    
    ESP_LOGI(TAG, "Sensor reader task started");
    
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(2000); // 2 second interval
    
    while (reader->task_running) {
        // Generate fake sensor data
        reader->fake_sensor_counter++;
        
        reader->latest_data.timestamp = xTaskGetTickCount();
        
        // Generate somewhat realistic fake data with some variation
        reader->latest_data.raw_value = 1000 + (rand() % 1000);
        reader->latest_data.voltage = 1.5f + ((rand() % 1000) / 1000.0f); // 1.5-2.5V
        reader->latest_data.temperature = 20.0f + ((rand() % 200) / 10.0f); // 20-40°C
        reader->latest_data.humidity = 30.0f + ((rand() % 500) / 10.0f); // 30-80% RH
        
        // Notify new data available
        xEventGroupSetBits(reader->event_group, SENSOR_EVENT_NEW_DATA);
        
        ESP_LOGI(TAG, "Sensor Data - Raw: %lu, Temp: %.1fC, Hum: %.1f%%, Volt: %.2fV", 
                 reader->latest_data.raw_value, 
                 reader->latest_data.temperature,
                 reader->latest_data.humidity,
                 reader->latest_data.voltage);
        
        vTaskDelayUntil(&last_wake_time, frequency);
    }
    
    ESP_LOGI(TAG, "Sensor reader task exiting");
    vTaskDelete(NULL);
}

sensor_reader_t* sensor_reader_create() {
    sensor_reader_t* reader = (sensor_reader_t*)malloc(sizeof(sensor_reader_t));
    if (!reader) {
        ESP_LOGE(TAG, "Failed to allocate sensor reader");
        return NULL;
    }
    
    reader->task_handle = NULL;
    reader->task_running = false;
    reader->fake_sensor_counter = 0;
    
    // Initialize with default data
    reader->latest_data.raw_value = 0;
    reader->latest_data.voltage = 0.0f;
    reader->latest_data.temperature = 0.0f;
    reader->latest_data.humidity = 0.0f;
    reader->latest_data.timestamp = 0;
    
    // Create event group
    reader->event_group = xEventGroupCreate();
    if (!reader->event_group) {
        ESP_LOGE(TAG, "Failed to create event group");
        free(reader);
        return NULL;
    }
    
    ESP_LOGI(TAG, "Sensor reader created");
    return reader;
}

void sensor_reader_destroy(sensor_reader_t* reader) {
    if (!reader) return;
    
    sensor_reader_stop(reader);
    
    if (reader->event_group) {
        vEventGroupDelete(reader->event_group);
    }
    
    free(reader);
    ESP_LOGI(TAG, "Sensor reader destroyed");
}

bool sensor_reader_get_latest_data(sensor_reader_t* reader, sensor_data_t* data) {
    if (!reader || !data) return false;
    
    *data = reader->latest_data;
    return true;
}

bool sensor_reader_start(sensor_reader_t* reader) {
    if (!reader || reader->task_running) {
        return false;
    }
    
    BaseType_t result = xTaskCreate(
        sensor_reader_task,
        "sensor_reader",
        4096,
        reader,
        6,  // Higher priority than WiFi
        &reader->task_handle
    );
    
    if (result == pdPASS) {
        reader->task_running = true;
        ESP_LOGI(TAG, "Sensor reader task started successfully");
        return true;
    }
    
    ESP_LOGE(TAG, "Failed to start sensor reader task");
    return false;
}

void sensor_reader_stop(sensor_reader_t* reader) {
    if (!reader || !reader->task_running) {
        return;
    }
    
    reader->task_running = false;
    
    if (reader->task_handle) {
        vTaskDelay(pdMS_TO_TICKS(100));
        vTaskDelete(reader->task_handle);
        reader->task_handle = NULL;
    }
    
    ESP_LOGI(TAG, "Sensor reader stopped");
}