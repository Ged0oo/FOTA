#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "MN";
const char* password = "1562001mn";
const char* mqtt_server = "public.mqtthq.com";
const char* mqtt_topic = "mqttHQ-client-test";

// Define a buffer with sufficient space (adjust the size as needed)
uint8_t dataBuffer[20480];

// Keep track of the data length  
int dataLength = 0; 

uint8_t appFlag = 0;

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
  appFlag = 0;

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
    } 
    else 
    {
        Serial.println("Buffer full. Flushing data.");
        flushBuffer(); // Call the function to flush the buffer
    }
    appFlag = 1;
  }
}


void loop() 
{
    if (!client.connected()) 
    {
        reconnect();
    }
    client.loop();

    if(appFlag == 1)
    {
        removeColons(dataBuffer);
        uint8_t dataToSend[dataLength];
        dataLength = parseHexData(dataBuffer, dataToSend);

        Serial.print("New Application Available with length : ");
        Serial.println(dataLength); 

        for (int i = 0; i < dataLength; i++) 
        {
            Serial.print("0x");
            Serial.print(dataToSend[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        appFlag = 0;
    }
}


void reconnect() 
{
  while (!client.connected()) 
  {
    Serial.print("Connecting to MQTT BROKER ... ");
    if (client.connect("ESP32Client")) 
    {
      Serial.println("connected");
      client.subscribe(mqtt_topic);
    } 
    else 
    {
      Serial.println("Failed, try again in seconds ... ");
      delay(2500);
    }
  }
}

void flushBuffer() 
{
  // Handle the data in the buffer as needed
  // For example, you can send it over MQTT or save it to a file
  // Then reset the buffer and dataLength
  Serial.println("Flushing data...");
  // Add your code to handle the data here
  // Example: client.publish(mqtt_topic, dataBuffer, dataLength);
  dataLength = 0;
  memset(dataBuffer, 0, sizeof(dataBuffer));
}


void removeColons(uint8_t* input) 
{
    size_t j = 0;
    for (size_t i = 0; input[i] != '\0'; i++) 
    {
        if((input[i] != ':') && (input[i] != ' ')) 
        {
            input[j++] = input[i];
        }
    }
    input[j] = '\0';
}

size_t parseHexData(uint8_t* input, uint8_t* output) 
{
    size_t dataLength = 0;
    for (int i = 0; input[i] != '\0' && input[i + 1] != '\0'; i += 2) 
    {
        char hexByte[3];
        hexByte[0] = (char)input[i];
        hexByte[1] = (char)input[i + 1];
        hexByte[2] = '\0';
        output[dataLength] = strtol(hexByte, NULL, 16);
        dataLength++;
    }
    return dataLength;
}
