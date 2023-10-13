#include <WiFi.h>
#include <PubSubClient.h>
#include <SPIFFS.h>
#include <FS.h>


const char* ssid = "Pablo";
const char* password = "010250@Maldini";
const char* mqtt_server = "public.mqtthq.com";
const char* mqtt_topic = "mqttHQ-client-test";

char dataBuffer[15360];  // Define a buffer with sufficient space (adjust the size as needed)
int dataLength = 0;     // Keep track of the data length


WiFiClient espClient;
PubSubClient client(espClient);

void setup() 
{
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Set the maximum MQTT packet size
  client.setBufferSize(20480); // Change the buffer size to your desired value

  // Set the MQTT broker host by the host name (DNS)
  client.setServer(mqtt_server, 1883);  // Use the port number provided by your MQTT broker
  client.setCallback(callback);
}


void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message: ");

  String payloadString = "";
  for (int i = 0; i < length; i++) 
  {
    payloadString += (char)payload[i];
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if(payloadString != "ttt")
  {
    if (dataLength + length < sizeof(dataBuffer)) 
    {
      memcpy(dataBuffer + dataLength, payload, length);
      dataLength += length; // Include the newline character
      Serial.println("\n\nData Buffer : ");
      Serial.println(dataBuffer);
    } 
    else 
    {
      Serial.println("Buffer full. Data is being lost.");
    }
  }
}


void loop() 
{
  if (!client.connected()) 
  {
    reconnect();
  }
  client.loop();
}


void reconnect() 
{
  while (!client.connected()) 
  {
    Serial.print("Connecting to MQTT broker...");
    if (client.connect("ESP32Client")) 
    {
      Serial.println("connected");
      client.subscribe(mqtt_topic);
    } 
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
