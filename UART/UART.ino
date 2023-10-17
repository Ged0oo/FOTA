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

uint8_t recVal = SEND_NACK;
uint8_t newVersion = 0;

byte BL_Host_Buffer[255];

bool verbose_mode = true;
bool Memory_Write_Active = false;
bool Memory_Write_Is_Active = false;
bool Memory_Write_All = false;

String userInput = ""; // Declare userInput globally
bool inputRequested = true;
bool inputComplete = false;
enum State { IDLE, WAIT_COMMAND, WAIT_ADDRESS };
State state = IDLE;

// Global array for binary data (replace this with your actual binary data)
byte binaryData[200] = 
{
  // Insert your binary data here
  // Example:
  0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
  0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
  0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
  0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
  
  0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
  0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
  0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
  0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa
};

unsigned long binaryDataLength = sizeof(binaryData);


uint8_t data_to_send = 0xcd;  // ACK message
uint8_t data_received;

void setup() 
{
    Serial.begin(9600); // Initialize the hardware serial for debugging
    SerialPort.begin(9600, SERIAL_8N1, 16, 17);
    Serial.println("\nSTM32F103 Custom Bootloader");
    //Read_Serial_Port(SEND_ACK);
    //Decode_CBL_Command(5);
}

void loop() 
{  
    recVal = SEND_ACK;
    Write_Data_To_Serial_Port(recVal);
    delay(500);
    
    /*
    SerialPort.write(data_to_send);
    if (SerialPort.available()) 
    {
        data_received = SerialPort.read();
        if (data_received == 0xcd) 
        {
            // Received ACK from the STM32
            Serial.println("Received ACK");
        }
    }

  delay(500);  // Delay before sending another ACK
  */

}

void Decode_CBL_Command(uint8_t Command) 
{
  switch (Command) 
  {
    case 5:
      Serial.println("Write data into different memories of the MCU command");
      MemWright();
      break;

    default:
      Serial.println("Error! Please enter a valid command.");
      break;
  }
}

void MemWright()
{
    uint32_t File_Total_Len = binaryDataLength;
    uint32_t BinFileSentBytes = 0;
    uint32_t BaseMemoryAddress = 0x08008000;
    uint32_t BinFileReadLength = 0;
    Memory_Write_All = true;

    Serial.print("Preparing to write a binary file with length (");
    Serial.print(File_Total_Len);
    Serial.println(") Bytes");
    userInput = ""; // Reset user input

    while (BinFileSentBytes < File_Total_Len) 
    {         
        Memory_Write_Is_Active = true;
        BinFileReadLength = minFun(64, File_Total_Len - BinFileSentBytes);

        for (int i = 7 ; i < 7 + BinFileReadLength ; i++) 
        {
            BL_Host_Buffer[i] = ReadFromGlobalArray(BinFileSentBytes + i - 7);
        }

        BL_Host_Buffer[1] = CBL_MEM_WRITE_CMD;
        BL_Host_Buffer[2] = Word_Value_To_Byte_Value(BaseMemoryAddress, 1, 1);
        BL_Host_Buffer[3] = Word_Value_To_Byte_Value(BaseMemoryAddress, 2, 1);
        BL_Host_Buffer[4] = Word_Value_To_Byte_Value(BaseMemoryAddress, 3, 1);
        BL_Host_Buffer[5] = Word_Value_To_Byte_Value(BaseMemoryAddress, 4, 1);
        BL_Host_Buffer[6] = BinFileReadLength;

        int CBL_MEM_WRITE_CMD_Len = (BinFileReadLength + 11);
        BL_Host_Buffer[0] = CBL_MEM_WRITE_CMD_Len - 1;

        uint32_t CRC32_Value;
        Calculate_CRC32(BL_Host_Buffer, CBL_MEM_WRITE_CMD_Len - 4, CRC32_Value);
        CRC32_Value &= 0xFFFFFFFF;

        BL_Host_Buffer[7 + BinFileReadLength]  = Word_Value_To_Byte_Value(CRC32_Value, 1, 1);
        BL_Host_Buffer[8 + BinFileReadLength]  = Word_Value_To_Byte_Value(CRC32_Value, 2, 1);
        BL_Host_Buffer[9 + BinFileReadLength]  = Word_Value_To_Byte_Value(CRC32_Value, 3, 1);
        BL_Host_Buffer[10 + BinFileReadLength] = Word_Value_To_Byte_Value(CRC32_Value, 4, 1);
       
        BaseMemoryAddress += BinFileReadLength;
        //Write_Data_To_Serial_Port(BL_Host_Buffer[0], 1);

        for (int i = 1; i < CBL_MEM_WRITE_CMD_Len; i++) 
        {
            //Write_Data_To_Serial_Port(BL_Host_Buffer[i], CBL_MEM_WRITE_CMD_Len - 1);
        }

        BinFileSentBytes += BinFileReadLength;
        Serial.print("\nBytes sent to the bootloader : ");
        Serial.println(BinFileSentBytes);
        Read_Serial_Port(SEND_ACK);
        delay(100);
    }

    Memory_Write_Is_Active = false;
    if(Memory_Write_All) 
    {
        Serial.println("\nPayload Written Successfully");
    }
}

byte ReadFromGlobalArray(uint32_t offset) 
{
  if (offset < binaryDataLength) 
  {
    return binaryData[offset];
  } 
  else 
  {
    return 0xFF; // Example error code
  }
}

byte Word_Value_To_Byte_Value(uint32_t Word_Value, int Byte_Index, bool Byte_Lower_First) 
{
  return (Word_Value >> (8 * (Byte_Index - 1))) & 0x000000FF;
}

void Calculate_CRC32(byte Buffer[], int Buffer_Length, uint32_t &CRC_Value) 
{
    CRC_Value = 0xFFFFFFFF;
    for (int i = 0; i < Buffer_Length; i++) 
    {
        CRC_Value = CRC_Value ^ Buffer[i];
        for (int j = 0; j < 32; j++) 
        {
            if (CRC_Value & 0x80000000) 
            {
                CRC_Value = (CRC_Value << 1) ^ 0x04C11DB7;
            } 
            else 
            {
                CRC_Value = (CRC_Value << 1);
            }
        }
    }
}

int minFun (int x, int y)
{
    if(x>y) return x;
    else return y;
}

void Write_Data_To_Serial_Port(uint8_t Value) 
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
        SerialPort.write(Value);
        if(SerialPort.available())  recVal = SerialPort.read();

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

void Read_Serial_Port(uint8_t Command_Code) 
{
    // Once breaking the loop, the target recieves the reqiured Command
    recVal = SEND_NACK;
    while(1)
    {
        recVal = SerialPort.read();
        if(recVal == SEND_ACK)
            break;
        else 
            recVal = SEND_NACK;
    }
}


void Read_Data_From_Serial_Port(uint8_t Command_Code) 
{
  uint8_t Length_To_Follow = 0;
  String BL_ACK = Read_Serial_Port(2);

  if (BL_ACK.length() > 0) 
  {
    byte BL_ACK_Array[2];
    BL_ACK.getBytes(BL_ACK_Array, 2);

    if (BL_ACK_Array[0] == 0xCD) 
    {
      Serial.println("\nReceived Acknowledgment from Bootloader");
      Length_To_Follow = BL_ACK_Array[1];

      Serial.print("Preparing to receive (");
      Serial.print(Length_To_Follow);
      Serial.println(") bytes from the bootloader");

      if (Command_Code == CBL_MEM_WRITE_CMD) 
      {
        Process_CBL_MEM_WRITE_CMD(Length_To_Follow);
      }
    } 
    else 
    {
      Serial.println("\nReceived Not-Acknowledgment from Bootloader");
    }
  }
}

String Read_Serial_Port(int Data_Len) 
{
  String Serial_Value = "";
  while (Serial_Value.length() <= 0) 
  {
    while (SerialPort.available() > 0) 
    {
      Serial_Value = SerialPort.readStringUntil('\n');
    }
  }
  return Serial_Value;
}

void Process_CBL_MEM_WRITE_CMD(int Data_Len) 
{
    byte BL_Write_Status = 0;
    String Serial_Data = Read_Serial_Port(Data_Len);
    BL_Write_Status = Serial_Data.toInt();

    if (BL_Write_Status == FLASH_PAYLOAD_WRITE_FAILED) 
    {
        Serial.println("\nWrite Status -> Write Failed or Invalid Address ");
    } 
    else if (BL_Write_Status == FLASH_PAYLOAD_WRITE_PASSED) 
    {
        Serial.println("\nWrite Status -> Write Successful ");
        Memory_Write_All = Memory_Write_All && FLASH_PAYLOAD_WRITE_PASSED;
    } 
    else 
    {
        Serial.println("Timeout!! Bootloader is not responding");
    }
}