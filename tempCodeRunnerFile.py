import serial
import struct
import os
import sys
import glob
from time import sleep

''' Bootloader Commands '''
CBL_FLASH_ERASE_CMD          = 0x13

INVALID_PAGE_NUMBER          = 0x00
VALID_PAGE_NUMBER            = 0x01
UNSUCCESSFUL_ERASE           = 0x02
SUCCESSFUL_ERASE             = 0x03


verbose_mode = 1
Memory_Write_Active = 0

def Check_Serial_Ports():
    Serial_Ports = []
    
    if sys.platform.startswith('win'):
        Ports = ['COM%s' % (i + 1) for i in range(256)]
    else:
        raise EnvironmentError("Error !! Unsupported Platform \n")
    
    for Serial_Port in Ports:
        try:
            test = serial.Serial(Serial_Port)
            test.close()
            Serial_Ports.append(Serial_Port)
        except (OSError, serial.SerialException):
            pass
    
    return Serial_Ports

def Serial_Port_Configuration(Port_Number):
    global Serial_Port_Obj
    try:
        Serial_Port_Obj = serial.Serial(Port_Number, 115200, timeout = 2)
    except:
        print("\nError !! That was not a valid port")
    
        Port_Number = Check_Serial_Ports()
        if(not Port_Number):
            print("\nError !! No ports Detected")
        else:
            print("\nHere are some available ports on your PC. Try Again !!")
            print("\n   ", Port_Number)
        return -1
    
    if Serial_Port_Obj.is_open:
        print("Port Open Success \n")
    else:
        print("Port Open Failed \n")

def Write_Data_To_Serial_Port(Value, Length):
    _data = struct.pack('>B', Value)
    if(verbose_mode):
        Value = bytearray(_data)
        print("   "+"0x{:02x}".format(Value[0]), end = ' ')
        if(Memory_Write_Active and (not verbose_mode)):
            print("#", end = ' ')
        Serial_Port_Obj.write(_data)

def Read_Serial_Port(Data_Len):
    
    Serial_Value = Serial_Port_Obj.read(Data_Len)
    Serial_Value_len = len(Serial_Value)
    while Serial_Value_len <= 0:
        Serial_Value = Serial_Port_Obj.read(Data_Len)
        Serial_Value_len = len(Serial_Value)
        print("Waiting Replay from the Bootloader")
    return Serial_Value
    
    '''
    Serial_Value = Serial_Port_Obj.read(Data_Len)
    return Serial_Value
    '''

def Read_Data_From_Serial_Port(Command_Code):
    Length_To_Follow = 0
    
    BL_ACK = Read_Serial_Port(2)
    if(len(BL_ACK)):
        BL_ACK_Array = bytearray(BL_ACK)
        if(BL_ACK_Array[0] == 0xCD):
            print ("\n   Received Acknowledgement from Bootloader")
            Length_To_Follow = BL_ACK_Array[1]
            print("   Preparing to receive (", int(Length_To_Follow), ") bytes from the bootloader")
            if (Command_Code == CBL_FLASH_ERASE_CMD):
                Process_CBL_FLASH_ERASE_CMD(Length_To_Follow)

        else:
            print ("\n   Received Not-Acknowledgement from Bootloader")
            sys.exit()
        

def Process_CBL_FLASH_ERASE_CMD(Data_Len):
    BL_Erase_Status = 0
    Serial_Data = Read_Serial_Port(Data_Len)
    if(len(Serial_Data)):
        BL_Erase_Status = bytearray(Serial_Data)
        if(BL_Erase_Status[0] == INVALID_PAGE_NUMBER):
            print("\n   Erase Status -> Invalid Sector Number ")
        elif (BL_Erase_Status[0] == UNSUCCESSFUL_ERASE):
            print("\n   Erase Status -> Unsuccessfule Erase ")
        elif (BL_Erase_Status[0] == SUCCESSFUL_ERASE):
            print("\n   Erase Status -> Successfule Erase ")
        else:
            print("\n   Erase Status -> Unknown Error")
    else:
        print("Timeout !!, Bootloader is not responding")

        
def Calculate_CRC32(Buffer, Buffer_Length):
    CRC_Value = 0xFFFFFFFF
    for DataElem in Buffer[0:Buffer_Length]:
        CRC_Value = CRC_Value ^ DataElem
        for DataElemBitLen in range(32):
            if(CRC_Value & 0x80000000):
                CRC_Value = (CRC_Value << 1) ^ 0x04C11DB7
            else:
                CRC_Value = (CRC_Value << 1)
    return CRC_Value
    
def Word_Value_To_Byte_Value(Word_Value, Byte_Index, Byte_Lower_First):
    Byte_Value = (Word_Value >> (8 * (Byte_Index - 1)) & 0x000000FF)
    return Byte_Value

def CalulateBinFileLength():
    BinFileLength = os.path.getsize("Application.bin")
    return BinFileLength

def OpenBinFile():
    global BinFile
    BinFile = open('Application.bin', 'rb')

def Decode_CBL_Command(Command):
    BL_Host_Buffer = []
    BL_Return_Value = 0
    
    ''' Clear the bootloader host buffer '''
    for counter in range(255):
        BL_Host_Buffer.append(0)
    
    
    if (Command == 4):
        print("Mass erase or sector erase of the user flash command")
        CBL_FLASH_ERASE_CMD_Len = 11
        SectorNumber = 0
        NumberOfSectors = 0
        BL_Host_Buffer[0] = CBL_FLASH_ERASE_CMD_Len - 1
        BL_Host_Buffer[1] = CBL_FLASH_ERASE_CMD
        SectorNumber = input("\n   Please enter start Adress          : ")
        SectorNumber = int(SectorNumber, 16)
        if(SectorNumber != 0xFF):
            NumberOfSectors = int(input("\n   Please enter number of pages to erase (12 Max): "), 16)

        BL_Host_Buffer[2] = Word_Value_To_Byte_Value(SectorNumber, 1, 1)
        BL_Host_Buffer[3] = Word_Value_To_Byte_Value(SectorNumber, 2, 1)
        BL_Host_Buffer[4] = Word_Value_To_Byte_Value(SectorNumber, 3, 1)
        BL_Host_Buffer[5] = Word_Value_To_Byte_Value(SectorNumber, 4, 1)

        BL_Host_Buffer[6] = NumberOfSectors
        CRC32_Value = Calculate_CRC32(BL_Host_Buffer, CBL_FLASH_ERASE_CMD_Len - 4) 
        CRC32_Value = CRC32_Value & 0xFFFFFFFF
        BL_Host_Buffer[7] = Word_Value_To_Byte_Value(CRC32_Value, 1, 1)
        BL_Host_Buffer[8] = Word_Value_To_Byte_Value(CRC32_Value, 2, 1)
        BL_Host_Buffer[9] = Word_Value_To_Byte_Value(CRC32_Value, 3, 1)
        BL_Host_Buffer[10] = Word_Value_To_Byte_Value(CRC32_Value, 4, 1)
        Write_Data_To_Serial_Port(BL_Host_Buffer[0], 1)
        for Data in BL_Host_Buffer[1 : CBL_FLASH_ERASE_CMD_Len]:
            Write_Data_To_Serial_Port(Data, CBL_FLASH_ERASE_CMD_Len - 1)
        Read_Data_From_Serial_Port(CBL_FLASH_ERASE_CMD)
        

SerialPortName = input("Enter the Port Name of your device(Ex: COM3):")
Serial_Port_Configuration(SerialPortName)
        
while True:
    print("\nSTM32F103 Custome BootLoader")
    print("==============================")
    print("Which command you need to send to the bootLoader :");
    print("   CBL_FLASH_ERASE_CMD          --> 4")
    
    CBL_Command = input("\nEnter the command code : ")
    
    if(not CBL_Command.isdigit()):
        print("   Error !!, Please enter a valid command !! \n")
    else:
        Decode_CBL_Command(int(CBL_Command))
    
    input("\nPlease press any key to continue ...")
    Serial_Port_Obj.reset_input_buffer()