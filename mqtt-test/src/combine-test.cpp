#include <Arduino.h>
#include <DHT.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Ethernet.h>
#include <SSLClient.h>
#include "certificates.h"

// To generate certificates header file:
// 1. go to https://openslab-osu.github.io/bearssl-certificate-utility/
// 2. Enter www.emqx.com & submit (for EMQX Broker)
// 3. Create new header file on src and include on main program.

// Modified SSLCLient.h line 469 due to memory errors "Discarded unread data to favor write operation"
// from: unsigned char m_iobuf[2048];
// to: unsigned char m_iobuf[BR_SSL_BUFSIZE_BIDI];

#define LED1                            21
#define LED2                            22
#define DHT_PIN                         4
#define ETHERNET_PIN                    5
#define ETH_ANALOG_PIN                  2
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 0, 177);

//EMQX Custom SSL Broker
const char* mqtt_server = "r7a48bba.ala.asia-southeast1.emqxsl.com";
const char* mqtt_username = "esp32demo";
const char* mqtt_password = "esp32demo";
const int mqtt_port = 8883;

//EMQX Public Broker
// const char* mqtt_server = "broker.emqx.io";
// const char* mqtt_username = "eqmx";
// const char* mqtt_password = "public";
// const int mqtt_port = 1883;

// //HiveMQ
// const char* mqtt_server = "broker.hivemq.com";
// const char* mqtt_username = "";
// const char* mqtt_password = "";
// const int mqtt_port = 1883;


DHT dht(DHT_PIN, DHTTYPE);

EthernetClient ethClient;
SSLClient ethClientSSL(ethClient, TAs, (size_t)TAs_NUM, ETH_ANALOG_PIN);
PubSubClient client(ethClientSSL);


byte willQoS = 2;
const char* willTopic = "b3l2/status";
const char* willMessage = "offline";
boolean willRetain = true;

unsigned long prev;
float dht_temp, dht_hum;

#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];



void reconnect()
{
    // Loop until we're reconnected
    while (!client.connected())
    {
        Serial.print("Attempting MQTT connection...");
        String clientId = "EthernetClient-"; // Create a random client ID
        clientId += String(random(0xffff), HEX);
        // Attempt to connect
        if (client.connect(clientId.c_str(), mqtt_username, mqtt_password, willTopic, willQoS, willRetain, willMessage))
        {
            Serial.println("connected");

            client.subscribe("b3l2/led1_state"); // subscribe the topics here
            client.subscribe("b3l2/led2_state"); // subscribe the topics here
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds"); // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void callback(char* topic, byte* payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    // Switch on the LED if an 1 was received as first character
    if (strcmp(topic, "b3l2/led1_state") == 0)
    {
        if ((char)payload[0] == '1')
        {
            digitalWrite(LED1, HIGH); // Turn the LED on (Note that LOW is the voltage level
            // but actually the LED is on; this is because
            // it is active low on the ESP-01)
        }
        else
        {
            digitalWrite(LED1, LOW); // Turn the LED off by making the voltage HIGH
        }
    }

    else if (strcmp(topic, "b3l2/led2_state") == 0)
    {
        if ((char)payload[0] == '1')
        {
            digitalWrite(LED2, HIGH); // Turn the LED on (Note that LOW is the voltage level
            // but actually the LED is on; this is because
            // it is active low on the ESP-01)
        }
        else
        {
            digitalWrite(LED2, LOW); // Turn the LED off by making the voltage HIGH
        }
    }
    
}

void setup()
{
    Serial.begin(9600);
    dht.begin();
    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);

    //espClient.setCACert(ca_cert);
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);

    Ethernet.init(ETHERNET_PIN);
    Serial.println("Initialize Ethernet with DHCP:");
    if (Ethernet.begin(mac) == 0)
    {
        Serial.println("Failed to configure Ethernet using DHCP");
        // Check for Ethernet hardware present
        if (Ethernet.hardwareStatus() == EthernetNoHardware)
        {
            Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
            while (true)
            {
                delay(1); // do nothing, no point running without Ethernet hardware
            }
        }
        if (Ethernet.linkStatus() == LinkOFF)
        {
            Serial.println("Ethernet cable is not connected.");
        }
        // try to configure using IP address instead of DHCP:
        Ethernet.begin(mac, ip);
    }
    else
    {
        Serial.print("  DHCP assigned IP ");
        Serial.println(Ethernet.localIP());
    }
    // give the Ethernet shield a second to initialize:
    delay(1000);
    reconnect();
    delay(1000);
    
}

void loop()
{
    if (!client.connected())
    {
        reconnect();
    }
    client.loop();

    if (millis() - prev >= 1000 || prev == 0)
    {
        dht_temp = dht.readTemperature();
        dht_hum = dht.readHumidity();
        printf("Temp: %.2f, Hum: %.2f\n", dht_temp, dht_hum);
        snprintf(msg, MSG_BUFFER_SIZE, "%.2f", dht_temp);
        client.publish("b3l2/temperature", msg);
        snprintf(msg, MSG_BUFFER_SIZE, "%.2f", dht_hum);
        client.publish("b3l2/humidity", msg);
        client.publish(willTopic, "online");
        prev = millis();
    }
}
