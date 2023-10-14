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

uint8_t recVal = NULL_REC;
uint8_t newVersion = 0;

void setup() 
{
    Serial.begin(9600);
    UART_Serial.begin(9600, SERIAL_8N1, 16, 17);

    newVersion = 1;
}

void loop() 
{
    while(newVersion)
    {
        // Once breaking the loop, the target recieves the reqiured Command
        recVal = NULL_REC;
        while(1)
        {
            UART_Serial.write(CBL_FLASH_ERASE_CMD); 
            delay(100);
            recVal = UART_Serial.read();

            if(recVal == CBL_FLASH_ERASE_CMD)
              break;
            else 
              recVal = NULL_REC;
        }

        recVal = NULL_REC;
        while(1)
        {
            if (UART_Serial.available() > 0) 
            {
              // If there is data available to read
              recVal = UART_Serial.read();
            }
            if(recVal == CBL_FLASH_ERASE_CMD)
              break;
        }
    }
    
    
}
