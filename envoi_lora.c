 #include <stdio.h>
 #include <inttypes.h>
 #include <string.h>
 
 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"
 #include "freertos/event_groups.h"
 
 #include "esp_log.h"
 #include "nvs_flash.h"
 #include "esp_event.h"
 #include "esp_netif.h"
 #include "mqtt_client.h"
 #include "esp_wifi.h"
 
 #include "lora.h"  // Fournit lora_init(), lora_set_frequency(), lora_send_packet(), etc.
 
 #define WIFI_SSID       "Portable Alex"
 #define WIFI_PASS       "101214(@!)"
 #define MQTT_BROKER_URI "mqtt://test.mosquitto.org"
 
 // Les topics auxquels s'abonner
 #define TOPIC_1 "tp/alban"
 #define TOPIC_2 "SALAD"
 
 static const char *TAG = "MQTT_LoRa";
 
 // Pour la gestion des événements Wi-Fi
 static EventGroupHandle_t wifi_event_group;
 const int WIFI_CONNECTED_BIT = BIT0;
 
 /* Gestion des événements Wi-Fi */
 static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
 {
     if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
         esp_wifi_connect();
     }
     else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
         ESP_LOGI(TAG, "Wi-Fi déconnecté, tentative de reconnexion...");
         esp_wifi_disconnect();
         esp_wifi_connect();
         xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
     }
     else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
         ESP_LOGI(TAG, "Wi-Fi connecté avec IP !");
         xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
     }
 }
 
 /* Initialisation du Wi-Fi */
 static void wifi_init(void)
 {
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
 static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
 {
     esp_mqtt_event_handle_t event = event_data;
     switch (event->event_id) {
     case MQTT_EVENT_CONNECTED:
         ESP_LOGI(TAG, "Connecté au broker MQTT");
         esp_mqtt_client_subscribe(event->client, TOPIC_1, 0);
         esp_mqtt_client_subscribe(event->client, TOPIC_2, 0);
         break;
 
     case MQTT_EVENT_DATA:
         ESP_LOGI(TAG, "MQTT -> Topic: %.*s, Message: %.*s",
                  event->topic_len, event->topic,
                  event->data_len, event->data);
         // Transmettre le message reçu en LoRa
         // Envoi du message reçu via MQTT en LoRa
         // lora_send_packet((uint8_t *)event->data, event->data_len);
         // Envoi du mot de passe choisi en LoRa
         char custom_message[] = "DODO JTM";
         lora_send_packet((uint8_t *)custom_message, strlen(custom_message));
         ESP_LOGI(TAG, "Message personnalisé envoyé en LoRa : %s", custom_message);
         break;
 
     case MQTT_EVENT_ERROR:
         ESP_LOGE(TAG, "Erreur MQTT");
         break;
 
     case MQTT_EVENT_DISCONNECTED:
         ESP_LOGI(TAG, "Déconnecté du broker MQTT");
         break;
 
     default:
         break;
     }
 }
 
 /* Démarrage du client MQTT */
 static void mqtt_app_start(void)
 {
     esp_mqtt_client_config_t mqtt_cfg = {
         .broker.address.uri = MQTT_BROKER_URI,
     };
     esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
     esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
     esp_mqtt_client_start(client);
 }
 
 void app_main(void)
 {
     ESP_LOGI(TAG, "Démarrage de l'application");
 
     // Initialisation de la NVS, Wi-Fi et MQTT
     nvs_flash_init();
     wifi_init();
 
     ESP_LOGI(TAG, "En attente de la connexion Wi-Fi...");
     xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
     ESP_LOGI(TAG, "Wi-Fi connecté, lancement de MQTT...");
     mqtt_app_start();
 
     // Initialisation de LoRa
     if (lora_init() == 0) {
         ESP_LOGE(TAG, "Module LoRa non reconnu !");
         while (1) {
             vTaskDelay(1);
         }
     }
     ESP_LOGI(TAG, "Module LoRa initialisé");
 
     // Configuration LoRa
     lora_set_frequency(868e6);  // 868 MHz
     lora_enable_crc();          // Activer la vérification CRC
     lora_set_coding_rate(1);    // Coding Rate 4/5 (1 = 4/5)
     lora_set_bandwidth(7);      // Bande passante 125 kHz (7 = 125kHz)
     lora_set_spreading_factor(7);  // Spreading Factor 7
 
     ESP_LOGI(TAG, "Configuration LoRa : 868MHz, CR=4/5, BW=125kHz, SF=7");
 
 }
 
