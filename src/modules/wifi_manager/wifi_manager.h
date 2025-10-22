#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "WiFi.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WIFI_STATE_DISCONNECTED = 0,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_FAILED
} wifi_state_t;

typedef struct {
    char ssid[32];
    char password[64];
    wifi_state_t current_state;
    TaskHandle_t task_handle;
    EventGroupHandle_t event_group;
    bool task_running;
    unsigned long connection_start_time;
} wifi_manager_t;

// Events
#define WIFI_EVENT_CONNECTED    (1 << 0)
#define WIFI_EVENT_DISCONNECTED (1 << 1)
#define WIFI_EVENT_FAILED       (1 << 2)

wifi_manager_t* wifi_manager_create(const char* ssid, const char* password);
void wifi_manager_destroy(wifi_manager_t* manager);
bool wifi_manager_connect(wifi_manager_t* manager);
void wifi_manager_disconnect(wifi_manager_t* manager);
wifi_state_t wifi_manager_get_state(wifi_manager_t* manager);
bool wifi_manager_start(wifi_manager_t* manager);
void wifi_manager_stop(wifi_manager_t* manager);

#ifdef __cplusplus
}
#endif

#endif