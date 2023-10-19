#include <WiFi.h>
#include <PubSubClient.h>
#include <HardwareSerial.h>
#include <Arduino.h>
#include <algorithm>

// UART configurations
HardwareSerial SerialPort(1);

#define 	SEND_NACK        						0xAB
#define 	SEND_ACK         						0xCD
#define     CBL_MEM_WRITE_CMD                       0x14
#define     CBL_MEM_Erasing_CMD                     0x15
#define     FLASH_PAYLOAD_WRITE_FAILED              0x00
#define     FLASH_PAYLOAD_WRITE_PASSED              0x01


const char* ssid = "MN";
const char* password = "1562001mn";
const char* mqtt_server = "public.mqtthq.com";
const char* mqtt_topic = "mqttHQ-client-test";


uint8_t recVal = SEND_NACK;
bool verbose_mode = true;
bool Memory_Write_Active = false;
bool Memory_Write_Is_Active = false;
bool Memory_Write_All = false;


uint8_t dataBuffer[20480];  // Define a buffer with sufficient space (adjust the size as needed)
uint8_t PursedData[20480];

int dataLength = 0;     // Keep track of the data length
uint8_t appFlag = 0;
uint16_t newAppLength = 0;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() 
{
    Serial.begin(115200);
    SerialPort.begin(115200, SERIAL_8N1, 16, 17);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(1000);
        Serial.println("Connecting to WiFi ... ");
    }
    Serial.println("Connected to WiFi");
    appFlag = 0;

    // Set the maximum MQTT packet size
    client.setBufferSize(20480); // Change the buffer size to your desired value

    // Set the MQTT broker host by the host name (DNS)
    client.setServer(mqtt_server, 1883);  // Use the port number provided by your MQTT broker
    client.setCallback(callback);
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
        removeSpaces(dataBuffer);
        newAppLength = parseHexData(dataBuffer, PursedData);

        Serial.print("\nNew Application Available with length : ");
        Serial.println(newAppLength); 

        Serial.println("\nSTM32F103 Custom Bootloader");

        PayloadWrite();
        waitAck();

        Serial.println("\nNew App Installed");
        appFlag = 0;
    }
}


/* MQTT APIs */
void callback(char* topic, byte* payload, unsigned int length) 
{
    Serial.print("Message arrived in topic : ");
    Serial.println(topic);
    Serial.print("Message : ");

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
        newAppLength = dataLength;
    }
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


/* Purser APIs */
void removeSpaces(uint8_t* input) 
{
    size_t j = 0;
    for (size_t i = 0; input[i] != '\0'; i++) 
    {
        if (input[i] != ' ') 
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


/* Bootloader APIs */
void Word_Value_To_Bytes(uint8_t* input, uint8_t* output, int numWords, bool byteLowerFirst) 
{
    for (int i = 0; i < numWords; i++) 
    {
        uint16_t wordValue = (input[i * 2] << 8) | input[i * 2 + 1];
        output[i * 4] = (byteLowerFirst ? (wordValue & 0xFF) : (wordValue >> 8) & 0xFF);
        output[i * 4 + 1] = (byteLowerFirst ? (wordValue >> 8) & 0xFF : (wordValue & 0xFF));
    }
}

void PayloadWrite()
{
    uint16_t dataSize = newAppLength;
    uint16_t chunkSize = 16;
    uint16_t currentIndex = 0;

    waitAck();
    Write_Message_To_Serial_Port(dataSize/chunkSize);

    // Send data in 10-byte chunks
    while (currentIndex < dataSize) 
    {
        waitAck();
        Write_Message_To_Serial_Port(CBL_MEM_WRITE_CMD);

        uint16_t remaining = dataSize - currentIndex;
        uint16_t chunkLength = min(chunkSize, remaining);

        waitAck();
        Write_Message_To_Serial_Port(chunkLength);

        waitAck();
        Write_Fram_To_Serial_Port(PursedData + currentIndex, chunkLength);
        currentIndex += chunkLength;
    }
}


void Erasing_Command()
{
    waitAck();
    Write_Message_To_Serial_Port(CBL_MEM_Erasing_CMD);
}


void Write_Fram_To_Serial_Port(uint8_t *Value, uint16_t frameLength) 
{
    for (int i = 0 ; i < frameLength ; i++)
    {
        if (verbose_mode) 
        {
            Serial.print("0x");
            Serial.print(Value[i], HEX);
            Serial.print(" ");
        }
        if (Memory_Write_Active && !verbose_mode) 
        {
            Serial.print("#");
        }
    }

    // Once breaking the loop, the target recieves the reqiured Command
    recVal = SEND_NACK;
    // Send the frame data
    SerialPort.write(Value, frameLength);
    while(recVal == SEND_NACK)
    {
        if(SerialPort.available()) 
            recVal = SerialPort.read();

        if(SEND_ACK == recVal) 
            Serial.println("\nReceived ACK\n");  
    }
}


void Write_Message_To_Serial_Port(uint16_t Value) 
{    
    // Once breaking the loop, the target recieves the reqiured Command
    recVal = SEND_NACK;
    // Send the data message
    SerialPort.write(Value);
    while(recVal == SEND_NACK)
    {
        if(SerialPort.available()) 
            recVal = SerialPort.read();

        if(SEND_ACK == recVal)
            Serial.println("\nReceived ACK\n");   
    }
}


void waitAck()
{
    // Once breaking the loop, the target recieves the reqiured Command
    recVal = SEND_NACK;
    while(recVal == SEND_NACK)
    {
        if(SerialPort.available()) 
            recVal = SerialPort.read();

        if(SEND_ACK == recVal)
            Serial.println("\nReceived ACK\n\n");   
    }
}


int minFun (int x, int y)
{
    if(x>y) return x;
    else return y;
}

void Write_Data_To_Serial_Port(uint8_t Value, int frameLength) 
{
    if (verbose_mode) 
    {
        Serial.print("0x");
        Serial.print(Value, HEX);
        Serial.print(" ");
    }
    if (Memory_Write_Active && !verbose_mode) 
    {
        Serial.print("#");
    }

    // Once breaking the loop, the target recieves the reqiured Command
    recVal = SEND_NACK;
    while(recVal == SEND_NACK)
    {
        // Send the frame data
        for (int i = 0; i < frameLength; i++)  SerialPort.write(Value);

        if(SerialPort.available()) 
            recVal = SerialPort.read();

        switch(recVal)
        {
            case SEND_ACK :
                Serial.println("\nReceived ACK\n\n");
                break;

            case SEND_NACK :
                Serial.println("\nReceived NACK\n\n");
                break;
        }   
    }
}

void Read_Serial_Port() 
{
    // Once breaking the loop, the target recieves the reqiured Command
    recVal = SEND_ACK;
    while(recVal == SEND_NACK)
    {
        if(SerialPort.available())
            recVal = SerialPort.read();

        switch(recVal)
        {
            case SEND_ACK :
                Serial.println("\nReceived ACK\n\n");
                recVal = SEND_ACK;
                break;

            case SEND_NACK :
                Serial.println("\nReceived NACK\n\n");
                recVal = SEND_NACK;
                break;
        }   
    }
}