#include "wifi_manager.h"

static const char* TAG = "WiFiManager";

static void wifi_manager_task(void* arg) {
    wifi_manager_t* manager = (wifi_manager_t*)arg;
    
    ESP_LOGI(TAG, "WiFi manager task started");
    
    // Set up WiFi event handlers using Arduino callbacks
    auto onWiFiConnected = [](arduino_event_id_t event) {
        ESP_LOGI(TAG, "WiFi Connected (Arduino event)");
        // We'll handle state changes in the main task loop instead
    };
    
    auto onWiFiGotIP = [](arduino_event_id_t event) {
        ESP_LOGI(TAG, "WiFi Got IP (Arduino event)");
    };
    
    auto onWiFiDisconnected = [](arduino_event_id_t event) {
        ESP_LOGI(TAG, "WiFi Disconnected (Arduino event)");
    };
    
    WiFi.onEvent(onWiFiConnected, ARDUINO_EVENT_WIFI_STA_CONNECTED);
    WiFi.onEvent(onWiFiGotIP, ARDUINO_EVENT_WIFI_STA_GOT_IP);
    WiFi.onEvent(onWiFiDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    
    while (manager->task_running) {
        // Check WiFi status using Arduino API
        wl_status_t status = WiFi.status();
        
        switch (status) {
            case WL_CONNECTED:
                if (manager->current_state != WIFI_STATE_CONNECTED) {
                    manager->current_state = WIFI_STATE_CONNECTED;
                    xEventGroupSetBits(manager->event_group, WIFI_EVENT_CONNECTED);
                    ESP_LOGI(TAG, "WiFi connected to: %s", WiFi.SSID().c_str());
                    ESP_LOGI(TAG, "IP Address: %s", WiFi.localIP().toString().c_str());
                }
                break;
                
            case WL_IDLE_STATUS:
            case WL_NO_SSID_AVAIL:
            case WL_SCAN_COMPLETED:
                if (manager->current_state != WIFI_STATE_CONNECTING) {
                    manager->current_state = WIFI_STATE_CONNECTING;
                    ESP_LOGI(TAG, "WiFi connecting...");
                }
                break;
                
            case WL_CONNECT_FAILED:
            case WL_CONNECTION_LOST:
                if (manager->current_state != WIFI_STATE_FAILED) {
                    manager->current_state = WIFI_STATE_FAILED;
                    xEventGroupSetBits(manager->event_group, WIFI_EVENT_FAILED);
                    ESP_LOGE(TAG, "WiFi connection failed");
                    
                    // Attempt reconnection after delay
                    vTaskDelay(pdMS_TO_TICKS(5000));
                    WiFi.begin(manager->ssid, manager->password);
                }
                break;
                
            case WL_DISCONNECTED:
                if (manager->current_state != WIFI_STATE_DISCONNECTED) {
                    manager->current_state = WIFI_STATE_DISCONNECTED;
                    xEventGroupSetBits(manager->event_group, WIFI_EVENT_DISCONNECTED);
                    ESP_LOGI(TAG, "WiFi disconnected");
                    
                    // Attempt reconnection after delay
                    vTaskDelay(pdMS_TO_TICKS(5000));
                    WiFi.begin(manager->ssid, manager->password);
                }
                break;
        }
        
        // If we're supposed to be connected but we're not, try to reconnect
        if (manager->current_state == WIFI_STATE_DISCONNECTED || 
            manager->current_state == WIFI_STATE_FAILED) {
            unsigned long now = millis();
            if (now - manager->connection_start_time > 10000) { // Every 10 seconds
                ESP_LOGI(TAG, "Attempting to reconnect to WiFi...");
                WiFi.begin(manager->ssid, manager->password);
                manager->connection_start_time = now;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    WiFi.disconnect(true);
    ESP_LOGI(TAG, "WiFi manager task exiting");
    vTaskDelete(NULL);
}

wifi_manager_t* wifi_manager_create(const char* ssid, const char* password) {
    wifi_manager_t* manager = (wifi_manager_t*)malloc(sizeof(wifi_manager_t));
    if (!manager) {
        ESP_LOGE(TAG, "Failed to allocate WiFi manager");
        return NULL;
    }
    
    strncpy(manager->ssid, ssid, sizeof(manager->ssid) - 1);
    strncpy(manager->password, password, sizeof(manager->password) - 1);
    manager->current_state = WIFI_STATE_DISCONNECTED;
    manager->task_handle = NULL;
    manager->task_running = false;
    manager->connection_start_time = 0;
    
    // Create event group
    manager->event_group = xEventGroupCreate();
    if (!manager->event_group) {
        ESP_LOGE(TAG, "Failed to create event group");
        free(manager);
        return NULL;
    }
    
    ESP_LOGI(TAG, "WiFi manager created for SSID: %s", ssid);
    return manager;
}

void wifi_manager_destroy(wifi_manager_t* manager) {
    if (!manager) return;
    
    wifi_manager_stop(manager);
    wifi_manager_disconnect(manager);
    
    if (manager->event_group) {
        vEventGroupDelete(manager->event_group);
    }
    
    free(manager);
    ESP_LOGI(TAG, "WiFi manager destroyed");
}

bool wifi_manager_connect(wifi_manager_t* manager) {
    if (!manager) return false;
    
    manager->current_state = WIFI_STATE_CONNECTING;
    manager->connection_start_time = millis();
    
    // Use Arduino WiFi API
    WiFi.mode(WIFI_STA);
    WiFi.begin(manager->ssid, manager->password);
    
    ESP_LOGI(TAG, "Attempting to connect to WiFi: %s", manager->ssid);
    return true;
}

void wifi_manager_disconnect(wifi_manager_t* manager) {
    if (!manager) return;
    
    WiFi.disconnect(true);
    manager->current_state = WIFI_STATE_DISCONNECTED;
    
    ESP_LOGI(TAG, "WiFi disconnected");
}

wifi_state_t wifi_manager_get_state(wifi_manager_t* manager) {
    return manager ? manager->current_state : WIFI_STATE_DISCONNECTED;
}

bool wifi_manager_start(wifi_manager_t* manager) {
    if (!manager || manager->task_running) {
        return false;
    }
    
    BaseType_t result = xTaskCreate(
        wifi_manager_task,
        "wifi_manager",
        8192,
        manager,
        4,  // Lower priority than sensor reading
        &manager->task_handle
    );
    
    if (result == pdPASS) {
        manager->task_running = true;
        ESP_LOGI(TAG, "WiFi manager task started successfully");
        return true;
    }
    
    ESP_LOGE(TAG, "Failed to start WiFi manager task");
    return false;
}

void wifi_manager_stop(wifi_manager_t* manager) {
    if (!manager || !manager->task_running) {
        return;
    }
    
    manager->task_running = false;
    
    if (manager->task_handle) {
        vTaskDelay(pdMS_TO_TICKS(100));
        vTaskDelete(manager->task_handle);
        manager->task_handle = NULL;
    }
    
    ESP_LOGI(TAG, "WiFi manager stopped");
}