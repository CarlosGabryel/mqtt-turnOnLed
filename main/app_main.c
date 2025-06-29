#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "driver/gpio.h"

#define LED_GPIO GPIO_NUM_2  // Defina o pino conectado ao LED
static const char *TAG = "mqtt5_led_client";

static esp_mqtt5_user_property_item_t user_property_arr[] = {
    {"board", "esp32"},
    {"u", "user"},
    {"p", "password"}
};
#define USE_PROPERTY_ARR_SIZE sizeof(user_property_arr)/sizeof(esp_mqtt5_user_property_item_t)

static void log_error_if_nonzero(const char *message, int error_code) {
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void print_user_property(mqtt5_user_property_handle_t user_property) {
    if (user_property) {
        uint8_t count = esp_mqtt5_client_get_user_property_count(user_property);
        if (count) {
            esp_mqtt5_user_property_item_t *item = malloc(count * sizeof(esp_mqtt5_user_property_item_t));
            if (esp_mqtt5_client_get_user_property(user_property, item, &count) == ESP_OK) {
                for (int i = 0; i < count; i++) {
                    esp_mqtt5_user_property_item_t *t = &item[i];
                    ESP_LOGI(TAG, "key: %s, value: %s", t->key, t->value);
                    free((char *)t->key);
                    free((char *)t->value);
                }
            }
            free(item);
        }
    }
}

static void mqtt5_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        print_user_property(event->property->user_property);

        msg_id = esp_mqtt_client_subscribe(client, "/ifpe/ads/embarcados/esp32/led", 0);
        ESP_LOGI(TAG, "Inscrito no tópico do LED, msg_id=%d", msg_id);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);

        if (strncmp(event->topic, "/ifpe/ads/embarcados/esp32/led", event->topic_len) == 0) {
            if (event->data_len == 1 && event->data[0] == '1') {
                gpio_set_level(LED_GPIO, 1);
                ESP_LOGI(TAG, "LED ligado");
            } else if (event->data_len == 1 && event->data[0] == '0') {
                gpio_set_level(LED_GPIO, 0);
                ESP_LOGI(TAG, "LED desligado");
            } else {
                ESP_LOGW(TAG, "Comando inválido recebido: %.*s", event->data_len, event->data);
            }
        }
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("socket errno", event->error_handle->esp_transport_sock_errno);
        }
        break;

    default:
        ESP_LOGI(TAG, "Evento não tratado: %d", event->event_id);
        break;
    }
}

static void mqtt5_app_start(void) {
    esp_mqtt5_connection_property_config_t connect_property = {
        .session_expiry_interval = 10,
        .maximum_packet_size = 1024,
        .receive_maximum = 65535,
        .topic_alias_maximum = 2,
        .request_resp_info = true,
        .request_problem_info = true,
    };

    esp_mqtt_client_config_t mqtt5_cfg = {
        .broker.address.uri = CONFIG_BROKER_URL,  // <- ajuste o IP para o do seu broker
        .session.protocol_ver = MQTT_PROTOCOL_V_5,
        // .credentials.username = "123",
        // .credentials.authentication.password = "456",
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt5_cfg);

    esp_mqtt5_client_set_user_property(&connect_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
    esp_mqtt5_client_set_connect_property(client, &connect_property);
    esp_mqtt5_client_delete_user_property(connect_property.user_property);

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt5_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void app_main(void) {
    ESP_LOGI(TAG, "[APP] Iniciando...");
    ESP_LOGI(TAG, "[APP] Memória livre: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] Versão IDF: %s", esp_get_idf_version());

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    // Inicialização do LED
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    mqtt5_app_start();
}
