/*
 * BL.c
 *
 *  Created on: Oct 18, 2023
 *      Author: Mohamed Nagy
 */

#include "BL.h"

uint8_t BL_Host_Buffer[200];


void MemoryWrite()
{
	uint8_t len = 0;
	ReciveMessageBL(CBL_MEM_WRITE_CMD, 1);
	RecieveLengthBl(&len);
	ReciveFramBL(len);
	PerformMemoryWrite(len, 0x08008000);
}


void PerformMemoryWrite(uint8_t Framelength, uint32_t StartAddress)
{
	HAL_StatusTypeDef HAL_Status = HAL_ERROR;
	uint8_t Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_FAILED;
	uint16_t Payload_Counter = 0;

	uint32_t Address=0;
	uint8_t UpdataAdress=0;


	PerformFlashErase();


	/* Unlock the FLASH control register access */
	HAL_Status = HAL_FLASH_Unlock();
	if(HAL_Status != HAL_OK)
	{
		Flash_Payload_Write_Status = FLASH_PAYLOAD_WRITE_FAILED;
	}
	else
	{
		for(Payload_Counter=0 , UpdataAdress=0 ; Payload_Counter < Framelength ; Payload_Counter++ , UpdataAdress+=2)
		{
			Address = StartAddress + UpdataAdress;

			/* Program a byte at a specified address */
			HAL_Status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, Address , BL_Host_Buffer[Payload_Counter]);

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


