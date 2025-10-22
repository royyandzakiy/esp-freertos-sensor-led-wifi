#include <Arduino.h>
#include "modules/led_controller/led_controller.h"
#include "modules/wifi_manager/wifi_manager.h"
#include "modules/sensor_reader/sensor_reader.h"

// Module instances
static led_controller_t* led_controller = NULL;
static wifi_manager_t* wifi_manager = NULL;
static sensor_reader_t* sensor_reader = NULL;

void setup() {
  Serial.begin(115200);
  delay(1000);

  ESP_LOGI("Main", "ESP32-S3 RTOS Teaching Example Starting...");
  ESP_LOGI("Main", "Using FreeRTOS for tasks, Arduino for hardware APIs");

  // Initialize modules
  led_controller = led_controller_create(LED_BUILTIN);
  wifi_manager = wifi_manager_create("YOUR_SSID", "YOUR_PASSWORD");
  sensor_reader = sensor_reader_create();

  // Start modules
  if (led_controller) {
    led_controller_start(led_controller);
    led_controller_set_pattern(led_controller, LED_PATTERN_BLINK_SLOW);
  }

  if (wifi_manager) {
    wifi_manager_start(wifi_manager);
    wifi_manager_connect(wifi_manager);
  }

  if (sensor_reader) {
    sensor_reader_start(sensor_reader);
  }

  ESP_LOGI("Main", "All modules initialized and tasks started");
  ESP_LOGI("Main", "FreeRTOS task priorities: Sensor(6) > LED(5) > WiFi(4)");
}

void loop() {
  static TickType_t last_status_time = 0;
  const TickType_t status_interval = pdMS_TO_TICKS(10000); // 10 seconds

  TickType_t current_time = xTaskGetTickCount();
  if (current_time - last_status_time >= status_interval) {
    last_status_time = current_time;

    ESP_LOGI("Main", "=== System Status ===");

    // Sensor
    if (sensor_reader) {
      sensor_data_t data;
      if (sensor_reader_get_latest_data(sensor_reader, &data)) {
        ESP_LOGI("Main", "Sensor - Temp: %.1fC, Hum: %.1f%%, Volt: %.2fV",
                 data.temperature, data.humidity, data.voltage);
      }
    }

    // WiFi
    if (wifi_manager) {
      wifi_state_t state = wifi_manager_get_state(wifi_manager);
      const char* state_str = "Unknown";
      switch (state) {
        case WIFI_STATE_DISCONNECTED: state_str = "Disconnected"; break;
        case WIFI_STATE_CONNECTING: state_str = "Connecting"; break;
        case WIFI_STATE_CONNECTED: state_str = "Connected"; break;
        case WIFI_STATE_FAILED: state_str = "Failed"; break;
      }
      ESP_LOGI("Main", "WiFi - State: %s", state_str);
    }

    // LED
    if (led_controller && wifi_manager) {
      wifi_state_t wifi_state = wifi_manager_get_state(wifi_manager);
      switch (wifi_state) {
        case WIFI_STATE_CONNECTED:
          led_controller_set_pattern(led_controller, LED_PATTERN_SOLID);
          break;
        case WIFI_STATE_CONNECTING:
          led_controller_set_pattern(led_controller, LED_PATTERN_BLINK_FAST);
          break;
        case WIFI_STATE_DISCONNECTED:
        case WIFI_STATE_FAILED:
          led_controller_set_pattern(led_controller, LED_PATTERN_BLINK_SLOW);
          break;
      }
    }

    ESP_LOGI("Main", "Free Heap: %d bytes", esp_get_free_heap_size());
    ESP_LOGI("Main", "Min Heap: %d bytes", esp_get_minimum_free_heap_size());
    ESP_LOGI("Main", "=====================");
  }

  vTaskDelay(pdMS_TO_TICKS(100));
}
