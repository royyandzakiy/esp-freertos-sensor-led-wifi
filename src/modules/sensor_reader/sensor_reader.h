#ifndef SENSOR_READER_H
#define SENSOR_READER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t raw_value;
    float voltage;
    float temperature;
    float humidity;
    TickType_t timestamp;
} sensor_data_t;

typedef struct {
    sensor_data_t latest_data;
    TaskHandle_t task_handle;
    EventGroupHandle_t event_group;
    bool task_running;
    uint32_t fake_sensor_counter;
} sensor_reader_t;

// Events
#define SENSOR_EVENT_NEW_DATA (1 << 0)

sensor_reader_t* sensor_reader_create();
void sensor_reader_destroy(sensor_reader_t* reader);
bool sensor_reader_get_latest_data(sensor_reader_t* reader, sensor_data_t* data);
bool sensor_reader_start(sensor_reader_t* reader);
void sensor_reader_stop(sensor_reader_t* reader);

#ifdef __cplusplus
}
#endif

#endif