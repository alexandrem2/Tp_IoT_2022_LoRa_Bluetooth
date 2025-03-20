#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "sx127x.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "mqtt_client.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"

#define WIFI_SSID "Portable Alex"
#define WIFI_PASS "101214(@!)"
#define MQTT_BROKER_URI "mqtt://test.mosquitto.org"

// LoRa Configuration
#define LORA_FREQ 868E6   // 868 MHz
#define LORA_SF 7         // Spreading Factor 7
#define LORA_BW 125E3     // 125 kHz Bandwidth
#define LORA_CR 5         // Coding Rate 4/5

static const char *TAG = "ESP32_MQTT_LoRa";
static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_BIT = BIT0;

sx127x_t lora_dev;
esp_mqtt_client_handle_t mqtt_client;  // Stocke le client MQTT

/* Gestion des événements Wi-Fi */
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Wi-Fi déconnecté, tentative de reconnexion...");
        esp_wifi_disconnect();
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "Wi-Fi connecté avec IP !");
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/* Initialisation du Wi-Fi */
void wifi_init(void) {
    wifi_event_group = xEventGroupCreate();

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}

/* Gestion des événements MQTT */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Connecté au broker MQTT !");
        esp_mqtt_client_subscribe(client, "tp/alban", 0);
        esp_mqtt_client_subscribe(client, "SALAD", 0);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "Message reçu sur %.*s: %.*s", 
                 event->topic_len, event->topic, 
                 event->data_len, event->data);
        
        // Transmission du message reçu via LoRa
        char lora_message[256];
        snprintf(lora_message, sizeof(lora_message), "%.*s", event->data_len, event->data);
        sx127x_transmit(&lora_dev, (uint8_t *)lora_message, strlen(lora_message), 1000);
        ESP_LOGI(TAG, "Message envoyé en LoRa : %s", lora_message);
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "Erreur MQTT !");
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Déconnecté du broker MQTT.");
        break;

    default:
        break;
    }
}

/* Démarrage du client MQTT */
void mqtt_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

/* Initialisation de LoRa */
void lora_init(void) {
    ESP_LOGI(TAG, "Initialisation de LoRa...");

    sx127x_config_t config = {
        .spi_host = SPI2_HOST,
        .spi_dma = SPI_DMA_CH_AUTO,
        .miso_io_num = 19,
        .mosi_io_num = 23,
        .sck_io_num = 18,
        .nss_io_num = 15,
        .reset_io_num = 16,
        .dio0_io_num = -1,
        .frequency = LORA_FREQ,
        .spreading_factor = LORA_SF,
        .bandwidth = LORA_BW,
        .coding_rate = LORA_CR
    };

    if (sx127x_init(&lora_dev, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Erreur d'initialisation LoRa !");
        return;
    }

    ESP_LOGI(TAG, "LoRa initialisé avec succès !");
}

void app_main(void) {
    ESP_LOGI(TAG, "Démarrage...");
    nvs_flash_init();

    wifi_init();

    // Attente de la connexion Wi-Fi avant de démarrer MQTT
    ESP_LOGI(TAG, "Attente de la connexion Wi-Fi...");
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "Wi-Fi connecté, lancement de MQTT...");

    mqtt_app_start();
    lora_init();
}
