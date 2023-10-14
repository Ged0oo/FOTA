#include <HardwareSerial.h>

HardwareSerial UART_Serial(2);

#define 	NULL_REC								          0xFF
#define 	NOT_ACK								            0x0
#define 	CBL_FLASH_ERASE_CMD          			0x1
#define 	CBL_MEM_WRITE_CMD            			0x2
#define 	CBL_JMP_USER_APP_CMD            	0x3
#define   FLASH_ERASED                      0x4
#define   MEMORY_WRITTEN                    0x5
#define   JUMPED_TO_APPLICATION             0x6

void setup() 
{
    Serial.begin(9600);
    UART_Serial.begin(9600, SERIAL_8N1, 16, 17);
}

void loop() 
{
    UART_Serial.println(CBL_FLASH_ERASE_CMD);  
    delay(1000);
}
