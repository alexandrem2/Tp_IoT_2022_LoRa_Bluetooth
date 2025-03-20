# Tp_IoT_2022_LoRa_Bluetooth

Ce TP a été réalisé par Mathurin BERDER et Alexandre MERCERON.

## 1. Mise en place

### 1. Installer l'IDE VScode et les dépendances nécessaires

cf. [https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32/get-started/index.html](https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32/get-started/index.html)

Documentation de la board: [http://www.smartcomputerlab.org/](http://www.smartcomputerlab.org/)

### Connection sur un point d'accès Wifi et envoi d'un message mqtt sur le broker test.mosquitto.org au topic tp/alban
![image](https://github.com/user-attachments/assets/9150e34f-6da9-40a6-8395-5e106af972e6)


## 2. LoRa

### 1. Définir des valeurs communes (à faire au tableau)
Topic : SALAD
mdp : POTATOTO

### 2. Communiquer via mqtt des données

Groupe 2. Ecouter les messages mqtt en en déduire les valeurs pour un envoi de données via Lora.
![image](https://github.com/user-attachments/assets/c944ab2b-4c98-4a07-8836-1bbfb33f5685)


### 3. Communiquer via LoRa des données

- Connexion au broker MQTT.  
- Publication d’un message "POTATOTO" sur le topic "SALAD". 

Consigne : Envoyer un message LoRa contenant des données

Pour nous distinguer de l'autre groupe communiquant avec Dorian et Amandine on décide de choisir le mot de passe : "DODO JTM"

Envoi du mot de passe choisi :

![image](https://github.com/user-attachments/assets/07500f27-fe8a-42df-b89e-f1b7f051a166)

Réception de l'autre groupe (Dorian et Amandine) :
![image](https://github.com/user-attachments/assets/b11b743f-fc76-4e9d-9360-8cfc2abb4bf8)

