#include "Arduino.h"
#include "ESP8266WiFi.h"
#include "PubSubClient.h"
#include "NTPClient.h"
#include "WiFiUdp.h"
#include "../include/secrets.h"
#include "ArduinoJson.h"
#include "cstring"

// Since I'm using AWS IoT Core's MQTT as the remote MQTT broker, I need to use TLS.
// The certificates are stored in the secrets.h file, which is not included in the repository.
// The secrets.h file should contain the following:
// #define SECRETS_WIFI_SSID           "your_wifi_ssid"
// #define SECRETS_WIFI_PWD            "your_wifi_password"
// #define SECRETS_MQTT_SERVER         "your_mqtt_server"
// #define SECRETS_MQTT_SERVER_PORT    TLS port: 8883
// const char ca_cert[] PROGMEM = "your_ca_certificate";
// const char client_cert[] PROGMEM = "your_client_certificate";
// const char client_key[] PROGMEM = "your_client_private_key";

#define MQTT_TLS 
#define MQTT_TLS_VERIFY
#define MQTT_RSA_VERIFY
// #define DESKTOP_SERIAL  // For debugging. Comment out when running on device

const char* ssid = SECRETS_WIFI_SSID;
const char* password = SECRETS_WIFI_PWD;
const char* mqtt_server = SECRETS_MQTT_SERVER;
const uint16_t mqtt_server_port = SECRETS_MQTT_SERVER_PORT;     //TLS: 8883
// const char* mqttUser = "user";                               // Not needed in this case
// const char* mqttPassword = "pass";                           // Not needed in this case
// const char* mqtt_topic_in = "test/esp8266/in";
const char* mqtt_topic_out_prefix = "weather/sensors/";
const char* mqtt_topic_error_out = "weather/sensors/error_out";

boolean connected_serial = false;
boolean connected_to_sensor_device = false;

#ifdef MQTT_TLS 
    WiFiClientSecure wifiClient;
#else
    WiFiClient wifiClient;
#endif

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
PubSubClient mqttClient(wifiClient);

void setup_wifi() {   
    delay(10); 
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    timeClient.begin();
    timeClient.update();

    #ifdef MQTT_TLS
        wifiClient.setSSLVersion(BR_TLS12);
        #ifdef MQTT_TLS_VERIFY
            time_t current_time = timeClient.getEpochTime();
            wifiClient.setX509Time(current_time);
            wifiClient.setTrustAnchors(new X509List(ca_cert));
        #else
            wifiClient.setInsecure();
        #endif
        #ifdef MQTT_RSA_VERIFY
            wifiClient.setClientRSACert(new X509List(client_cert), new PrivateKey(client_key));
        #endif
    #endif
    Serial.println("WiFi connected");
}

void connect() {
    char err_buf[256];
    while (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection...");
        String mqtt_client_id = "esp-01s-esp8266-weather-transmitter";
        // if (mqttClient.connect(mqttClientId.c_str(), mqttUser, mqttPassword))
        if (mqttClient.connect(mqtt_client_id.c_str())) { 
            Serial.println("connected");
            // mqttClient.subscribe(mqtt_topic_in);
        } 
        else {
            Serial.print("failed, rc=");
            Serial.println(mqttClient.state());
            wifiClient.getLastSSLError(err_buf, sizeof(err_buf));
            Serial.print("SSL error: ");
            Serial.println(err_buf);
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

// void callback(char* topic, byte* payload, unsigned int length) {
//     Serial.print("Message arrived on topic: '");
//     Serial.print(topic);
//     Serial.print("' with payload: ");
//     for (unsigned int i = 0; i < length; i++) {
//         Serial.print((char)payload[i]);
//     }
//     Serial.println();
//     time_t current_time = timeClient.getEpochTime();
//     mqttClient.publish(mqtt_topic_out,("Current time: " + (String)ctime(&current_time)).c_str());
// }


void send_request() {
    time_t current_time = timeClient.getEpochTime();
    JsonDocument doc;
    doc["time"] = current_time;
    serializeJson(doc, Serial);
    Serial.println();
}

void publish_response(String _response){
    JsonDocument response_doc;
    DeserializationError error = deserializeJson(response_doc, _response);
    if (error) {
        mqttClient.publish(mqtt_topic_error_out, (error.c_str() + (String)" ---> " + _response).c_str());
      }
    else{
        char* deviceID = strdup(response_doc["device"]);
        char mqtt_topic_out[strlen(deviceID) + strlen(mqtt_topic_out_prefix)];
        strcpy(mqtt_topic_out, mqtt_topic_out_prefix);
        strcat(mqtt_topic_out, deviceID);
        mqttClient.publish(mqtt_topic_out, _response.c_str());
    }
}

void setup() {
    // To know it's ON
    pinMode(2, OUTPUT);
    digitalWrite(2, LOW);

    #ifdef DESKTOP_SERIAL
        Serial.begin(115200);
    #endif
    setup_wifi();
    mqttClient.setServer(mqtt_server, mqtt_server_port);
    // mqttClient.setCallback(callback);
    #ifndef DESKTOP_SERIAL
        Serial.begin(115200);
        Serial.setTimeout(2000);
    #endif
}

void loop() {
    unsigned long start_time = millis();
    while(millis() - start_time < 5000){
        if (!mqttClient.connected()) {
            #ifdef DESKTOP_SERIAL
                connect();
            #else
                Serial.end();
                connect();
                Serial.begin(115200);
            #endif
        }
        mqttClient.loop();
        timeClient.update();
        delay(100);
    }
    send_request();
    delay(100);
    String response = Serial.readStringUntil('\n');
    #ifdef DESKTOP_SERIAL
        Serial.print("Response: '");
        Serial.println(response);
    #endif
    publish_response(response);
}

