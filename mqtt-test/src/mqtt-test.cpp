#include <Arduino.h>
#include <DHT.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

#define LED1                        18
#define LED2                        19
#define DHT_PIN                     4
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321


DHT dht(DHT_PIN, DHTTYPE);
WiFiManager wifiManager;
//WiFiClient espClient;
WiFiClientSecure espClient;
PubSubClient client(espClient);

const char* mqtt_server = "r7a48bba.ala.asia-southeast1.emqxsl.com";
const char* mqtt_username = "esp32demo";
const char* mqtt_password = "esp32demo";
const int mqtt_port = 8883;
byte willQoS = 2;
const char* willTopic = "status";
const char* willMessage = "offline";
boolean willRetain = true;

unsigned long prev;
float dht_temp, dht_hum;

#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];

const char* ca_cert= \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n" \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n" \
"QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n" \
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n" \
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n" \
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n" \
"CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n" \
"nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n" \
"43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n" \
"T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n" \
"gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n" \
"BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n" \
"TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n" \
"DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n" \
"hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n" \
"06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n" \
"PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n" \
"YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n" \
"CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=" \
"-----END CERTIFICATE-----\n";

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";   // Create a random client ID
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password, willTopic, willQoS, willRetain, willMessage)) {
      client.publish(willTopic, "online");
      Serial.println("connected");

      client.subscribe("led1_state");   // subscribe the topics here
      client.subscribe("led2_state");   // subscribe the topics here

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");   // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  //Switch on the LED if an 1 was received as first character
  if(strcmp(topic, "led1_state") == 0){
    if ((char)payload[0] == '1') {
      digitalWrite(LED1, HIGH);   // Turn the LED on (Note that LOW is the voltage level
      // but actually the LED is on; this is because
      // it is active low on the ESP-01)
    } else {
      digitalWrite(LED1, LOW);  // Turn the LED off by making the voltage HIGH
    }
  }

  else if(strcmp(topic, "led2_state") == 0){
    if ((char)payload[0] == '1') {
      digitalWrite(LED2, HIGH);   // Turn the LED on (Note that LOW is the voltage level
      // but actually the LED is on; this is because
      // it is active low on the ESP-01)
    } else {
      digitalWrite(LED2, LOW);  // Turn the LED off by making the voltage HIGH
    }
  }
}

void setup() {
  Serial.begin(9600);
  dht.begin();
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  wifiManager.autoConnect("mqtt-client");
  
  espClient.setCACert(ca_cert);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  if(millis() - prev >= 1000 || prev == 0){
    dht_temp = dht.readTemperature();
    dht_hum = dht.readHumidity();
    printf("Temp: %.2f, Hum: %.2f\n", dht_temp, dht_hum);
    snprintf(msg, MSG_BUFFER_SIZE, "%.2f", dht_temp);
    client.publish("temperature", msg);
    snprintf(msg, MSG_BUFFER_SIZE, "%.2f", dht_hum);
    client.publish("humidity", msg);
    prev = millis();
  }
}

