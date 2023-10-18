#include <HardwareSerial.h>
#include <Arduino.h>
#include <algorithm>

// UART configurations
HardwareSerial SerialPort(1);

#define     CBL_MEM_WRITE_CMD                       0x14
#define     CBL_MEM_Erasing_CMD                     0x15
#define     FLASH_PAYLOAD_WRITE_FAILED              0x00
#define     FLASH_PAYLOAD_WRITE_PASSED              0x01

#define 	SEND_NACK        						0xAB
#define 	SEND_ACK         						0xCD

uint8_t recVal = SEND_NACK;


bool verbose_mode = true;
bool Memory_Write_Active = false;
bool Memory_Write_Is_Active = false;
bool Memory_Write_All = false;


// Your data frame.
uint8_t dataToSend[256] = 
{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
    0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
    0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
}; 

byte BL_Host_Buffer[255];

uint16_t dataSize = sizeof(dataToSend);
uint16_t chunkSize = 16;
uint16_t currentIndex = 0;

void setup() 
{
    Serial.begin(115200); // Initialize the hardware serial for debugging
    SerialPort.begin(115200, SERIAL_8N1, 16, 17);
    Serial.println("\nSTM32F103 Custom Bootloader");
    
    // Sending the Erasing Command
    //Erasing_Command();

    PayloadWrite();


}

void loop() 
{   

}

void PayloadWrite()
{
    uint16_t dataSize = sizeof(dataToSend);
    uint16_t chunkSize = 16;
    uint16_t currentIndex = 0;
    

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
        Write_Fram_To_Serial_Port(dataToSend + currentIndex, chunkLength);
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
    uint32_t File_Total_Len = dataSize;
    uint32_t BinFileSentBytes = 0;
    uint32_t BaseMemoryAddress = 0x08008000;
    uint32_t BinFileReadLength = 0;
    Memory_Write_All = true;

    Serial.print("Preparing to write a binary file with length (");
    Serial.print(File_Total_Len);
    Serial.println(") Bytes");

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
  if (offset < dataSize) 
  {
    return dataToSend[offset];
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