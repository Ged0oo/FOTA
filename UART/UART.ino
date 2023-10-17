#include <HardwareSerial.h>
#include <Arduino.h>
#include <algorithm>

// UART configurations
HardwareSerial SerialPort(1);

#define     CBL_MEM_WRITE_CMD                       0x14
#define     FLASH_PAYLOAD_WRITE_FAILED              0x00
#define     FLASH_PAYLOAD_WRITE_PASSED              0x01

#define 	SEND_NACK        						0xAB
#define 	SEND_ACK         						0xCD

uint8_t data = 0x55;
uint8_t recVal = SEND_NACK;


bool verbose_mode = true;
bool Memory_Write_Active = false;
bool Memory_Write_Is_Active = false;
bool Memory_Write_All = false;


void setup() 
{
    Serial.begin(115200); // Initialize the hardware serial for debugging
    SerialPort.begin(115200, SERIAL_8N1, 16, 17);
    Serial.println("\nSTM32F103 Custom Bootloader");
}

uint8_t dataToSend[4] = {0x01, 0x02, 0x03, 0x04}; // Your data frame.
uint8_t framSize = sizeof(dataToSend);

void loop() 
{   
    waitAck();
    Write_Fram_To_Serial_Port(dataToSend, framSize);
}

void Write_Fram_To_Serial_Port(uint8_t Value[], uint8_t frameLength) 
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
            Serial.println("\nReceived ACK\n\n");  
    }
    
}

void Write_Message_To_Serial_Port(uint8_t Value) 
{    
    // Once breaking the loop, the target recieves the reqiured Command
    recVal = SEND_NACK;
    while(recVal == SEND_NACK)
    {
        // Send the data message
        SerialPort.write(Value);

        if(SerialPort.available()) 
            recVal = SerialPort.read();

        if(SEND_ACK == recVal)
            Serial.println("\nReceived ACK\n\n");   
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