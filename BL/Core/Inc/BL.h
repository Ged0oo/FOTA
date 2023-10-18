/*
 * BL.h
 *
 *  Created on: Oct 18, 2023
 *      Author: Mohamed Nagy
 */

#ifndef INC_BL_H_
#define INC_BL_H_


#include "crc.h"
#include "usart.h"
#include "gpio.h"
#include <string.h>


#define     CBL_MEM_WRITE_CMD                       0x14
#define     CBL_MEM_Erasing_CMD                     0x15
#define     FLASH_PAYLOAD_WRITE_FAILED              0x00
#define     FLASH_PAYLOAD_WRITE_PASSED              0x01

#define 	BL_HOST_COMMUNICATION_UART				&huart1
#define 	CRC_ENGINE             					&hcrc

#define 	INVALID_PAGE_NUMBER          			0x00
#define 	VALID_PAGE_NUMBER            			0x01
#define 	UNSUCCESSFUL_ERASE           			0x02
#define 	SUCCESSFUL_ERASE             			0x03

#define 	CBL_FLASH_MAX_PAGE_NUMBER    			0x12
#define 	CBL_FLASH_MASS_ERASE         			0xFF

#define 	HAL_SUCCESSFUL_ERASE         			0xFFFFFFFFU

#define 	ADDRESS_IS_INVALID           			0x00
#define 	ADDRESS_IS_VALID             			0x01

#define 	STM32F103_SRAM_SIZE         			(20 * 1024)
#define 	STM32F103_FLASH_SIZE         			(64 * 1024)
#define 	STM32F103_SRAM_END          			(SRAM_BASE + STM32F103_SRAM_SIZE)
#define 	STM32F103_FLASH_END          			(FLASH_BASE + STM32F103_FLASH_SIZE)

#define 	FLASH_PAYLOAD_WRITE_FAILED  			0x00
#define 	FLASH_PAYLOAD_WRITE_PASSED  			0x01

#define 	FLASH_SECTOR2_BASE_ADDRESS   			0x08008000U

#define 	SEND_NACK        						0xAB
#define 	SEND_ACK         						0xCD

#define 	CBL_VENDOR_ID                			76
#define 	CBL_SW_MAJOR_VERSION         			4
#define 	CBL_SW_MINOR_VERSION         			4
#define 	CBL_SW_PATCH_VERSION         			1

#define 	CRC_TYPE_SIZE_BYTE           			4


typedef enum
{
	CRC_FAIL = 0,
	CRC_PASS
}tCRC_VERIFY;


void MemoryWrite();
void PerformMemoryWrite(uint8_t Framelength, uint32_t StartAddress);

#endif /* INC_BL_H_ */
