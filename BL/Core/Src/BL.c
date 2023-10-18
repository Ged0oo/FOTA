/*
 * BL.c
 *
 *  Created on: Oct 18, 2023
 *      Author: Mohamed Nagy
 */

#include "BL.h"

uint8_t BL_Host_Buffer[300];

static uint32_t StartAddress = 0x08008000;
uint16_t newAppSize = 0;

void MemoryWrite()
{
	uint8_t len = 0;
	ReciveMessageBL(CBL_MEM_WRITE_CMD, 1);
	RecieveLengthBl(&len);
	ReciveFramBL(len);
	PerformMemoryWrite(len);
}

uint32_t swapByteOrder(uint32_t value)
{
    return ((value & 0xFF) << 24) | (((value >> 8) & 0xFF) << 16) | (((value >> 16) & 0xFF) << 8) | ((value >> 24) & 0xFF);
}


void PerformMemoryWrite(uint8_t Framelength)
{
	HAL_StatusTypeDef HAL_Status = HAL_ERROR;
	uint8_t Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_FAILED;
	uint16_t Payload_Counter = 0;

	uint32_t Address=0;
	uint8_t UpdataAdress=0;


	/* Unlock the FLASH control register access */
	HAL_Status = HAL_FLASH_Unlock();
	if(HAL_Status != HAL_OK)
	{
		Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_FAILED;
	}
	else
	{
		for(Payload_Counter=0 , UpdataAdress=0 ; Payload_Counter < Framelength/2 ; Payload_Counter+=4 , UpdataAdress+=4)
		{
			Address = StartAddress + UpdataAdress;
			uint32_t swappedData = swapByteOrder(*(uint32_t*)(BL_Host_Buffer + Payload_Counter));

			/* Program a byte at a specified address */
			HAL_Status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address, swappedData);

			if(HAL_Status != HAL_OK)
			{
				Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_FAILED;
				break;
			}
			else
			{
				Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_PASSED;
			}
		}
	}
	StartAddress = Address + 4;
}


void RecieveLengthBl(uint8_t *length)
{
	uint8_t recVal = 0xff;
	HAL_StatusTypeDef Hal_State = HAL_ERROR;

	sendACK();
	Hal_State = HAL_UART_Receive(&huart1, &recVal, 1, HAL_MAX_DELAY);

	if(Hal_State == HAL_OK)
	{
		*length = recVal;
		recVal = 0xcd;
	}
	else
		recVal = 0xab;

	Hal_State = HAL_UART_Transmit(&huart1, &recVal, 1, HAL_MAX_DELAY);
}


uint16_t RecieveNewAppSize(uint16_t length)
{
	uint16_t recVal = 0xff;
	HAL_StatusTypeDef Hal_State = HAL_ERROR;

	sendACK();
	Hal_State = HAL_UART_Receive(&huart1, &recVal, 1, HAL_MAX_DELAY);

	if(Hal_State == HAL_OK)
	{
		length = recVal;
		recVal = 0xcd;
	}
	else
		recVal = 0xab;

	Hal_State = HAL_UART_Transmit(&huart1, &recVal, 1, HAL_MAX_DELAY);
	return length;
}


void ReciveMessageBL(uint8_t message, uint8_t length)
{
	uint8_t recVal = 0xff;
	HAL_StatusTypeDef Hal_State = HAL_ERROR;

	sendACK();
	Hal_State = HAL_UART_Receive(&huart1, &recVal, length, HAL_MAX_DELAY);

	if(Hal_State == HAL_OK)
	{
		if(recVal == message)
		{
			recVal = SEND_ACK;
		}
	}
	else
	{
		recVal = SEND_NACK;
	}

	Hal_State = HAL_UART_Transmit(&huart1, &recVal, 1, HAL_MAX_DELAY);
}


void ReciveFramBL(uint8_t length)
{
	/* Intialize the data Buffer by Zeros */
	memset(BL_Host_Buffer, 0, 200);

	uint8_t recVal = 0xff;
	HAL_StatusTypeDef Hal_State = HAL_ERROR;

	sendACK();
	Hal_State = HAL_UART_Receive(&huart1, &BL_Host_Buffer[0], length, HAL_MAX_DELAY);

	if(Hal_State == HAL_OK)
		recVal = 0xcd;
	else
		recVal = 0xab;

	Hal_State = HAL_UART_Transmit(&huart1, &recVal, 1, HAL_MAX_DELAY);
}


void sendACK()
{
	uint8_t ACK = 0xcd;
	HAL_StatusTypeDef Hal_State = HAL_ERROR;
	Hal_State = HAL_UART_Transmit(&huart1, &ACK, 1, HAL_MAX_DELAY);
}


void MemoryErase()
{
	uint8_t RecErase = SEND_NACK;
	uint8_t EraseStatus = UNSUCCESSFUL_ERASE;
	ReciveMessageBL(CBL_MEM_Erasing_CMD, 1);
	PerformFlashErase();
}


void PerformFlashErase()
{
	FLASH_EraseInitTypeDef pEraseInit;
	HAL_StatusTypeDef Hal_status = HAL_ERROR;
	uint32_t PageError = 0;
	uint8_t PageStatus = INVALID_PAGE_NUMBER;

	pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
	pEraseInit.Banks = FLASH_BANK_1;
	pEraseInit.PageAddress = 0x08008000;
	pEraseInit.NbPages = 12;

	HAL_FLASH_Unlock();
	Hal_status = HAL_FLASHEx_Erase(&pEraseInit,&PageError);
	HAL_FLASH_Lock();

	if(PageError == HAL_SUCCESSFUL_ERASE)
	{
		PageStatus = SUCCESSFUL_ERASE;
	}
	else
	{
		PageStatus = UNSUCCESSFUL_ERASE;
	}
	return PageStatus;
}


void JumpToApplication()
{
	//just a function pointer to hold the address of the reset handler of the user app.
	void (*app_reset_handler)(void);

	//disbale interuppts
    __set_PRIMASK(1);
    __disable_irq();

    SCB->VTOR = FLASH_SECTOR2_BASE_ADDRESS;

    // 1. configure the MSP by reading the value from the base address of the sector 2
    uint32_t msp_value = *(__IO uint32_t *)FLASH_SECTOR2_BASE_ADDRESS;

    __set_MSP(msp_value);

    uint32_t resethandler_address = *(__IO uint32_t *) (FLASH_SECTOR2_BASE_ADDRESS + 4);

    app_reset_handler = (void*) resethandler_address;

    //3. jump to reset handler of the user application
    app_reset_handler();
}

