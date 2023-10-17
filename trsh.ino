#include <SoftwareSerial.h>
#include <Arduino.h>

SoftwareSerial SerialPort(2, 3); // Define the SoftwareSerial object for UART communication

#define CBL_MEM_WRITE_CMD 0x14
#define FLASH_PAYLOAD_WRITE_FAILED 0x00
#define FLASH_PAYLOAD_WRITE_PASSED 0x01

byte BL_Host_Buffer[255];

bool verbose_mode = true;
bool Memory_Write_Active = false;
bool Memory_Write_Is_Active = false;
bool Memory_Write_All = false;

void Check_Serial_Ports() {
  String Serial_Ports;
  Serial_Ports = "Available ports on your PC: ";
  
  for (int i = 0; i <= 255; i++) {
    String portName = "COM" + String(i);
    SerialPort = SoftwareSerial(i, 0); // Try to create a SoftwareSerial object on the port
    if (SerialPort.begin(115200)) {
      SerialPort.end();
      Serial_Ports += portName + " ";
    }
  }

  if (Serial_Ports == "Available ports on your PC: ") {
    Serial.println("\nError!! No ports detected.");
  } else {
    Serial.println(Serial_Ports);
  }
}

void Serial_Port_Configuration(int Port_Number) {
  if (SerialPort.begin(115200)) {
    Serial.println("Port Open Success");
  } else {
    Serial.println("Port Open Failed");
    Port_Number = -1;
  }
}

void Write_Data_To_Serial_Port(uint8_t Value, int Length) {
  if (verbose_mode) {
    Serial.print("0x");
    Serial.print(Value, HEX);
    Serial.print(" ");
  }
  if (Memory_Write_Active && !verbose_mode) {
    Serial.print("#");
  }
  SerialPort.write(Value);
}

String Read_Serial_Port(int Data_Len) {
  String Serial_Value = "";
  while (Serial_Value.length() <= 0) {
    while (SerialPort.available() > 0) {
      Serial_Value = SerialPort.readStringUntil('\n');
    }
  }
  return Serial_Value;
}

void Read_Data_From_Serial_Port(uint8_t Command_Code) {
  uint8_t Length_To_Follow = 0;
  String BL_ACK = Read_Serial_Port(2);
  if (BL_ACK.length() > 0) {
    byte BL_ACK_Array[2];
    BL_ACK.getBytes(BL_ACK_Array, 2);
    if (BL_ACK_Array[0] == 0xCD) {
      Serial.println("\nReceived Acknowledgment from Bootloader");
      Length_To_Follow = BL_ACK_Array[1];
      Serial.print("Preparing to receive (");
      Serial.print(Length_To_Follow);
      Serial.println(") bytes from the bootloader");

      if (Command_Code == CBL_MEM_WRITE_CMD) {
        Process_CBL_MEM_WRITE_CMD(Length_To_Follow);
      }
    } else {
      Serial.println("\nReceived Not-Acknowledgment from Bootloader");
      // sys.exit(); // This won't work in Arduino
    }
  }
}

void Process_CBL_FLASH_ERASE_CMD(int Data_Len) {
  int BL_Erase_Status = 0;
  String Serial_Data = Read_Serial_Port(Data_Len);
  if (Serial_Data.length() > 0) {
    BL_Erase_Status = Serial_Data.toInt();
    if (BL_Erase_Status == INVALID_PAGE_NUMBER) {
      Serial.println("\nErase Status -> Invalid Sector Number ");
    } else if (BL_Erase_Status == UNSUCCESSFUL_ERASE) {
      Serial.println("\nErase Status -> Unsuccessful Erase ");
    } else if (BL_Erase_Status == SUCCESSFUL_ERASE) {
      Serial.println("\nErase Status -> Successful Erase ");
    } else {
      Serial.println("\nErase Status -> Unknown Error");
    }
  } else {
    Serial.println("Timeout!! Bootloader is not responding");
  }
}

void Process_CBL_MEM_WRITE_CMD(int Data_Len) {
  byte BL_Write_Status = 0;
  String Serial_Data = Read_Serial_Port(Data_Len);
  BL_Write_Status = Serial_Data.toInt();
  if (BL_Write_Status == FLASH_PAYLOAD_WRITE_FAILED) {
    Serial.println("\nWrite Status -> Write Failed or Invalid Address ");
  } else if (BL_Write_Status == FLASH_PAYLOAD_WRITE_PASSED) {
    Serial.println("\nWrite Status -> Write Successful ");
    Memory_Write_All = Memory_Write_All && FLASH_PAYLOAD_WRITE_PASSED;
  } else {
    Serial.println("Timeout!! Bootloader is not responding");
  }
}

void Calculate_CRC32(byte Buffer[], int Buffer_Length, uint32_t &CRC_Value) {
  CRC_Value = 0xFFFFFFFF;
  for (int i = 0; i < Buffer_Length; i++) {
    CRC_Value = CRC_Value ^ Buffer[i];
    for (int j = 0; j < 32; j++) {
      if (CRC_Value & 0x80000000) {
        CRC_Value = (CRC_Value << 1) ^ 0x04C11DB7;
      } else {
        CRC_Value = (CRC_Value << 1);
      }
    }
  }
}

byte Word_Value_To_Byte_Value(uint32_t Word_Value, int Byte_Index, bool Byte_Lower_First) {
  return (Word_Value >> (8 * (Byte_Index - 1))) & 0x000000FF;
}

unsigned long CalculateBinFileLength() {
  File BinFile = SD.open("Application.bin", FILE_READ);
  unsigned long length = BinFile.size();
  BinFile.close();
  return length;
}

bool OpenBinFile(File &BinFile) {
  BinFile = SD.open("Application.bin", FILE_READ);
  return BinFile;
}

void Decode_CBL_Command(int Command) {
  switch (Command) {
    case 5:
      Serial.println("Write data into different memories of the MCU command");
      uint32_t File_Total_Len = 0;
      uint32_t BinFileRemainingBytes = 0;
      uint32_t BinFileSentBytes = 0;
      uint32_t BaseMemoryAddress = 0;
      uint32_t BinFileReadLength = 0;
      Memory_Write_All = true;

      File_Total_Len = CalculateBinFileLength();
      Serial.print("Preparing to write a binary file with length (");
      Serial.print(File_Total_Len);
      Serial.println(") Bytes");
      File BinFile;
      if (OpenBinFile(BinFile)) {
        Serial.print("Enter the start address: ");
        BaseMemoryAddress = Serial.parseInt();

        while (BinFile.available()) {
          Memory_Write_Is_Active = true;
          if (BinFile.available() >= 64) {
            BinFileReadLength = 64;
          } else {
            BinFileReadLength = BinFile.available();
          }

          for (int i = 7; i < 7 + BinFileReadLength; i++) {
            BL_Host_Buffer[i] = BinFile.read();
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
          BL_Host_Buffer[7 + BinFileReadLength] = Word_Value_To_Byte_Value(CRC32_Value, 1, 1);
          BL_Host_Buffer[8 + BinFileReadLength] = Word_Value_To_Byte_Value(CRC32_Value, 2, 1);
          BL_Host_Buffer[9 + BinFileReadLength] = Word_Value_To_Byte_Value(CRC32_Value, 3, 1);
          BL_Host_Buffer[10 + BinFileReadLength] = Word_Value_To_Byte_Value(CRC32_Value, 4, 1);
          BaseMemoryAddress += BinFileReadLength;
          Write_Data_To_Serial_Port(BL_Host_Buffer[0], 1);
          delay(100);
          for (int i = 1; i < CBL_MEM_WRITE_CMD_Len; i++) {
            Write_Data_To_Serial_Port(BL_Host_Buffer[i], CBL_MEM_WRITE_CMD_Len - 1);
          }
          BinFileSentBytes += BinFileReadLength;
          BinFileRemainingBytes = File_Total_Len - BinFileSentBytes;
          Serial.print("\nBytes sent to the bootloader: ");
          Serial.println(BinFileSentBytes);
          Read_Data_From_Serial_Port(CBL_MEM_WRITE_CMD);
          delay(100);
        }
        Memory_Write_Is_Active = false;
        if (Memory_Write_All) {
          Serial.println("\nPayload Written Successfully");
        }
        BinFile.close();
      } else {
        Serial.println("Error opening binary file.");
      }
      break;
    default:
      Serial.println("Error! Please enter a valid command.");
  }
  Serial.println("\nPlease press any key to continue ...");
  while (Serial.available() <= 0) {
    // Wait for user input to continue
  }
  Serial.read(); // Read the user input
  SerialPort.flush();
}

void setup() {
  Serial.begin(115200); // Initialize the hardware serial for debugging
  Check_Serial_Ports();
  Serial.print("\nEnter the Port Name of your device (Ex: COM3): ");
  String SerialPortName;
  while (SerialPortName.length() == 0) {
    SerialPortName = Serial.readStringUntil('\n');
  }
  Serial_Port_Configuration(SerialPortName.toInt());
}

void loop() {
  Serial.println("\nSTM32F103 Custom Bootloader");
  Serial.println("==============================");
  Serial.println("Which command do you need to send to the bootloader:");
  Serial.println("CBL_MEM_WRITE_CMD -> 5");

  Serial.print("\nEnter the command code: ");
  String CBL_Command;
  while (CBL_Command.length() == 0) {
    CBL_Command = Serial.readStringUntil('\n');
  }

  if (!CBL_Command.toInt()) {
    Serial.println("Error! Please enter a valid command.");
  } else {
    Decode_CBL_Command(CBL_Command.toInt());
  }

  Serial.println("\nPlease press any key to continue ...");
  while (Serial.available() <= 0) {
    // Wait for user input to continue
  }
  Serial.read(); // Read the user input
  SerialPort.flush();
}
