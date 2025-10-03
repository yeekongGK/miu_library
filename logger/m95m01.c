/******************************************************************************
 * File:        m95m01.c
 * Author:      Firmware Team
 * Created:     03-10-2025
 * Last Update: -
 *
 * Description:
 *   This file implements the driver for the M95M01 SPI EEPROM memory chip.
 *   It manages low-level SPI communication, including command transmission and
 *   data read/write operations. A transaction queue is used to handle
 *   asynchronous requests, and state machines manage the write and read
 *   processes, including write-enable, busy checking, and data validation.
 *   The driver also defines and manages memory partitions within the EEPROM.
 *
 * Notes:
 *   - This driver is built on top of a lower-level SPI driver (`spi1.c`).
 *   - It includes error handling and retry mechanisms for robust communication.
 *
 * To Do:
 *   - -
 *
 ******************************************************************************/

#include "common.h"
#include "m95m01.h"
#include "spi1.h"

static void M95M01_CSPin(uint8_t _level);
static void M95M01_SetTimeout(uint32_t _ms);
static bool M95M01_IsTimeout(void);
static ErrorStatus M95M01_TxCmd(uint8_t _cmd, uint16_t _timeout);
static ErrorStatus M95M01_TxRxCmd(uint8_t _cmd, uint16_t _timeout);
static ErrorStatus M95M01_WriteData(uint32_t _address, const uint8_t *_data, uint16_t _len, uint16_t _timeout);
static ErrorStatus M95M01_ReadData(uint32_t _address, uint16_t _len, uint16_t _timeout);
static void M95M01_Transaction_Write(void);
static void M95M01_Transaction_Read(void);

uint32_t ulEepromStartAddress[]= {
		M95M01_CFG_PARTITION_NONE_START_ADDR, 	M95M01_CFG_PARTITION_1_START_ADDR, 	M95M01_CFG_PARTITION_2_START_ADDR,
		M95M01_CFG_PARTITION_3_START_ADDR, 	M95M01_CFG_PARTITION_4_START_ADDR, 	M95M01_CFG_PARTITION_5_START_ADDR,
		M95M01_CFG_PARTITION_6_START_ADDR, 	M95M01_CFG_PARTITION_7_START_ADDR, 	M95M01_CFG_PARTITION_8_START_ADDR
};

uint32_t ulEepromLastAddress[]= {
		M95M01_CFG_PARTITION_NONE_LAST_ADDR, 	M95M01_CFG_PARTITION_1_LAST_ADDR, 	M95M01_CFG_PARTITION_2_LAST_ADDR,
		M95M01_CFG_PARTITION_3_LAST_ADDR, 	M95M01_CFG_PARTITION_4_LAST_ADDR, 	M95M01_CFG_PARTITION_5_LAST_ADDR,
		M95M01_CFG_PARTITION_6_LAST_ADDR, 	M95M01_CFG_PARTITION_7_LAST_ADDR, 	M95M01_CFG_PARTITION_8_LAST_ADDR
};

uint32_t ulEepromSize[]= {
		M95M01_CFG_PARTITION_NONE_SIZE,		M95M01_CFG_PARTITION_1_SIZE, 		M95M01_CFG_PARTITION_2_SIZE,
		M95M01_CFG_PARTITION_3_SIZE,		M95M01_CFG_PARTITION_4_SIZE, 		M95M01_CFG_PARTITION_5_SIZE,
		M95M01_CFG_PARTITION_6_SIZE,		M95M01_CFG_PARTITION_7_SIZE, 		M95M01_CFG_PARTITION_8_SIZE
};

__IO uint8_t pucTxBytes[M95M01_CFG_TXBYTES_SIZE];
__IO uint8_t pucRxBytes[M95M01_CFG_RXBYTES_SIZE];/*TODO: the size of this buffer give me the infamous "out of order log transmission" bug!!*/
__IO uint8_t *pucWriteData= &pucTxBytes[4];//pucTxBytes[0]: cmd, pucTxBytes[1-3]: address
__IO uint8_t *pucCmdReply= &pucRxBytes[1];//pucRxBytes[0] is rubbish data from spi clock
__IO uint8_t *pucReadData= &pucRxBytes[M95M01_CFG_READ_OFFSET];//pucRxBytes[0-4] are rubbish data from spi clock

SPI1_Transaction_t eEepromSpiTrx;

static M95M01_Transaction_t		eNoTrx= {
								.type=  M95M01_TYPE_WRITE,
								.state.write= M95M01_WRITE_STATE_IDLE,
								.partition= M95M01_PARTITION_NONE,
								.dataPtr= NULL,
								.dataLen= 0,
								.dataAddress= 0,
								.timeout= 0,
								.retryCounter= 0,
								.status= M95M01_STATUS_SUCCESS,
								.transactionInQueue= false,
								.transactionInProgress= false,
								.transactionCompleted= false,
								.TransactionCompletedCb= NULL,
							};
M95M01_Transaction_t 		*eM95m01TrxQueue[M95M01_CFG_MAX_TRX_QUEUE];
M95M01_Transaction_t 		*eCurrentM95m01Trx= &eNoTrx;
uint8_t 			ucM95m01TrxQueueDepth= 0;
uint8_t 			ucM95m01TrxDequeueIndex= 0;

static bool bM95M01AccessErrorFlag= false;

uint8_t M95M01_QueueDepth(void)
{
	return ucM95m01TrxQueueDepth;
}

ErrorStatus M95M01_Enqueue(M95M01_Transaction_t *_task)
{
	/*TODO: boundary check for logs - so far logs task has no error checking :(*/
	if(M95M01_CFG_MAX_TRX_QUEUE!= ucM95m01TrxQueueDepth)
	{
		_task->transactionInQueue= true;
		_task->transactionInProgress= false;
		_task->transactionCompleted= false;
    	if(M95M01_TYPE_WRITE== _task->type)
    	{
    		_task->state.write= M95M01_WRITE_STATE_INITIALIZE;
    	}
    	else
    	{
    		_task->state.read= M95M01_READ_STATE_READ_DATA_1;
    	}

		eM95m01TrxQueue[(ucM95m01TrxDequeueIndex+ ucM95m01TrxQueueDepth)% M95M01_CFG_MAX_TRX_QUEUE]= _task;
		ucM95m01TrxQueueDepth++;
		return SUCCESS;
	}

	return ERROR;
}

M95M01_Transaction_t *M95M01_Dequeue(void)
{
	M95M01_Transaction_t *_task= NULL;
	if(0!= ucM95m01TrxQueueDepth)
	{
		_task= eM95m01TrxQueue[ucM95m01TrxDequeueIndex];
		_task->transactionInQueue= false;
		_task->transactionInProgress= true;
		_task->transactionCompleted= false;
		ucM95m01TrxQueueDepth--;
		ucM95m01TrxDequeueIndex= (ucM95m01TrxDequeueIndex+ 1)% M95M01_CFG_MAX_TRX_QUEUE;
	}
	return _task;
}

static void M95M01_CSPin(uint8_t _level)
{
	if(0== _level)
	{
		LL_GPIO_ResetOutputPin(SPI1_NSS_GPIO_Port, SPI1_NSS_Pin);
	}
	else
	{
		LL_GPIO_SetOutputPin(SPI1_NSS_GPIO_Port, SPI1_NSS_Pin);
	}
}

void M95M01_Init(void)
{
	static bool _isInitialized= false;

	if(true== _isInitialized)
	{
		return;
	}
	_isInitialized= true;

	/* CS pin */
	LL_GPIO_InitTypeDef GPIO_InitStruct;

	SYS_EnablePortClock(SPI1_NSS_GPIO_Port);
	LL_GPIO_SetOutputPin(SPI1_NSS_GPIO_Port, SPI1_NSS_Pin);/*to set default value*/
	GPIO_InitStruct.Pin = SPI1_NSS_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
	LL_GPIO_Init(SPI1_NSS_GPIO_Port, &GPIO_InitStruct);

	eEepromSpiTrx.CSPin= M95M01_CSPin;
	eEepromSpiTrx.TransactionCompletedCb= NULL;
	SPI1_Init();

//	for(int i= 0; i< 9; i++)
//	{
//		DBG_Print("eeprom[%d] size: %d\r\n", i, ulEepromSize[i]);
//	}
}

static void M95M01_SetTimeout(uint32_t _ms)
{
	if(0!= _ms)
	{
		SYS_Sleep(M95M01_TaskId, _ms);
	}
}

static bool M95M01_IsTimeout(void)
{
	if(true== SYS_IsAwake(M95M01_TaskId))
	{
		DBG_Print("M95M01_IsTimeout. \r\n");
		return true;
	}
	return false;
}

static ErrorStatus M95M01_TxCmd(uint8_t _cmd, uint16_t _timeout)
{
    pucTxBytes[0]= _cmd;

    eEepromSpiTrx.txBuffer= pucTxBytes;
    eEepromSpiTrx.rxBuffer= pucTxBytes;
    eEepromSpiTrx.bufferLen= 1;
    if(SUCCESS== SPI1_Enqueue(&eEepromSpiTrx))
    {
        M95M01_SetTimeout(_timeout);
        return SUCCESS;
    }

	return ERROR;
}

static ErrorStatus M95M01_TxRxCmd(uint8_t _cmd, uint16_t _timeout)
{
	pucTxBytes[0]= _cmd;//1st byte as cmd
    pucTxBytes[1]= 0xFF;//2nd byte to fetch reply

    eEepromSpiTrx.txBuffer= pucTxBytes;
    eEepromSpiTrx.rxBuffer= pucRxBytes;
    eEepromSpiTrx.bufferLen= 2;
    if(SUCCESS== SPI1_Enqueue(&eEepromSpiTrx))
    {
        M95M01_SetTimeout(_timeout);
        return SUCCESS;
    }

	return ERROR;
}

static ErrorStatus M95M01_WriteData(uint32_t _address, const uint8_t *_data, uint16_t _len, uint16_t _timeout)
{
	pucTxBytes[0]= M95M01_CMD_WRITE;
    pucTxBytes[1]= (uint8_t)(_address>> 16);
    pucTxBytes[2]= (uint8_t)(_address>> 8);
    pucTxBytes[3]= (uint8_t)(_address);
    memcpy((uint8_t *)pucTxBytes+ 4, _data, _len);

    eEepromSpiTrx.txBuffer= pucTxBytes;
    eEepromSpiTrx.rxBuffer= pucTxBytes;
    eEepromSpiTrx.bufferLen= 4+ _len;
    if(SUCCESS== SPI1_Enqueue(&eEepromSpiTrx))
    {
        M95M01_SetTimeout(_timeout);
        return SUCCESS;
    }

	return ERROR;
}

static ErrorStatus M95M01_ReadData(uint32_t _address, uint16_t _len, uint16_t _timeout)
{
	pucTxBytes[0]= M95M01_CMD_READ;
    pucTxBytes[1]= (uint8_t)(_address>> 16);
    pucTxBytes[2]= (uint8_t)(_address>> 8);
    pucTxBytes[3]= (uint8_t)(_address);

    eEepromSpiTrx.txBuffer= pucTxBytes;
    eEepromSpiTrx.rxBuffer= pucRxBytes;
    eEepromSpiTrx.bufferLen= M95M01_CFG_READ_OFFSET+ _len;
    if(SUCCESS== SPI1_Enqueue(&eEepromSpiTrx))
    {
        M95M01_SetTimeout(_timeout);
        return SUCCESS;
    }

	return ERROR;
}

void M95M01_Transaction(void)
{
	if(true== eCurrentM95m01Trx->transactionInProgress)
	{
		switch(eCurrentM95m01Trx->type)
		{
			case M95M01_TYPE_WRITE:
				M95M01_Transaction_Write();
				break;

			case M95M01_TYPE_READ:
				M95M01_Transaction_Read();
				break;

			default:
				SYS_FailureHandler();
				break;
		}

		//return;
	}

	if(true== eCurrentM95m01Trx->transactionCompleted)
	{
		if(NULL!= eCurrentM95m01Trx->TransactionCompletedCb)
		{
			eCurrentM95m01Trx->TransactionCompletedCb();
		}
		eCurrentM95m01Trx= &eNoTrx;
	}

	if((0!= M95M01_QueueDepth())
	    	&&(false== eCurrentM95m01Trx->transactionInProgress))
	{
		eCurrentM95m01Trx= M95M01_Dequeue();
//    	char *_hexstring[128];
//    	DBG_Print("M95M01_Writing data:%s.\r\n", UTILI_BytesToHexString(eCurrentM95m01Trx->dataPtr, eCurrentM95m01Trx->dataLen, _hexstring));
	}
}

static void M95M01_SetWriteErrorDCODE(uint32_t _value)
{
	DIAG_Code(WRITE_ERROR_M95M01DCode, _value);
	bM95M01AccessErrorFlag= true;
}

static void M95M01_SetValidateErrorDCODE(uint32_t _value)
{
	DIAG_Code(VALIDATE_ERROR_M95M01DCode, _value);
	bM95M01AccessErrorFlag= true;
}

static void M95M01_SetReadErrorDCODE(uint32_t _value)
{
	DIAG_Code(READ_ERROR_M95M01DCode, _value);
	bM95M01AccessErrorFlag= true;
}

static void M95M01_Transaction_Write(void)
{
	static uint32_t ulDataAddress;
	static uint16_t uwDataLen;
	static uint16_t uwDataLenToWrite;
	static uint8_t *pucDataPtr;

    switch(eCurrentM95m01Trx->state.write)
    {
        case M95M01_WRITE_STATE_IDLE:
            break;

        case M95M01_WRITE_STATE_INITIALIZE:
    		pucDataPtr= eCurrentM95m01Trx->dataPtr;/*this will be incremented if there is iteration*/
    		ulDataAddress= eCurrentM95m01Trx->dataAddress;
    		uwDataLen= eCurrentM95m01Trx->dataLen;
    		ulDataAddress+= ulEepromStartAddress[eCurrentM95m01Trx->partition];/*plus address offset*/

        case M95M01_WRITE_STATE_READ_RDSR_REGISTER:
        	eCurrentM95m01Trx->status= M95M01_STATUS_BUSY;

            if(SUCCESS== M95M01_TxRxCmd(M95M01_CMD_RDSR, eCurrentM95m01Trx->timeout))
            {
            	eCurrentM95m01Trx->state.write= M95M01_WRITE_STATE_CHECK_WRITE_BUSY;
            }
            break;

        case M95M01_WRITE_STATE_CHECK_WRITE_BUSY:
            if((false== eEepromSpiTrx.transactionCompleted)&& (false== M95M01_IsTimeout()))
            {
                break;
            }
            else if(true== M95M01_IsTimeout())
            {
                if(0!= eCurrentM95m01Trx->retryCounter)
                {
                	eCurrentM95m01Trx->retryCounter--;
                    eCurrentM95m01Trx->state.write= M95M01_WRITE_STATE_READ_RDSR_REGISTER;
                }
                else
                {
                    eCurrentM95m01Trx->status= M95M01_STATUS_TXRX_TIMEOUT;
                    eCurrentM95m01Trx->state.write= M95M01_WRITE_STATE_IDLE;
                	eCurrentM95m01Trx->transactionInProgress= false;
                	eCurrentM95m01Trx->transactionCompleted= true;
					M95M01_SetWriteErrorDCODE(M95M01_WRITE_STATE_CHECK_WRITE_BUSY);
                }
                break;
            }

            if(1== (pucCmdReply[0]& M95M01_STATUS_WIP))//write busy
            {
                eCurrentM95m01Trx->state.write= M95M01_WRITE_STATE_READ_RDSR_REGISTER;
            }
            else
            {
                eCurrentM95m01Trx->state.write= M95M01_WRITE_STATE_SET_WRITE_ENABLE;
            }
            break;

        case M95M01_WRITE_STATE_SET_WRITE_ENABLE:
            if((false== eEepromSpiTrx.transactionCompleted)&& (false== M95M01_IsTimeout()))
            {
                break;
            }
            else if(true== M95M01_IsTimeout())
            {
                if(0!= eCurrentM95m01Trx->retryCounter)
                {
                	eCurrentM95m01Trx->retryCounter--;
                    eCurrentM95m01Trx->state.write= M95M01_WRITE_STATE_READ_RDSR_REGISTER;
                }
                else
                {
                    eCurrentM95m01Trx->status= M95M01_STATUS_TXRX_TIMEOUT;
                    eCurrentM95m01Trx->state.write= M95M01_WRITE_STATE_IDLE;
                	eCurrentM95m01Trx->transactionInProgress= false;
                	eCurrentM95m01Trx->transactionCompleted= true;
					M95M01_SetWriteErrorDCODE(M95M01_WRITE_STATE_SET_WRITE_ENABLE);
                }
                break;
            }

            if(SUCCESS== M95M01_TxCmd(M95M01_CMD_WREN, eCurrentM95m01Trx->timeout))
            {
            	eCurrentM95m01Trx->state.write= M95M01_WRITE_STATE_WRITE_DATA_1;
            }
            break;

        case M95M01_WRITE_STATE_WRITE_DATA_1:
            if((false== eEepromSpiTrx.transactionCompleted)&& (false== M95M01_IsTimeout()))
            {
                break;
            }
            else if(true== M95M01_IsTimeout())
            {
                if(0!= eCurrentM95m01Trx->retryCounter)
                {
                	eCurrentM95m01Trx->retryCounter--;
                    eCurrentM95m01Trx->state.write= M95M01_WRITE_STATE_READ_RDSR_REGISTER;
                }
                else
                {
                    eCurrentM95m01Trx->status= M95M01_STATUS_TXRX_TIMEOUT;
                    eCurrentM95m01Trx->state.write= M95M01_WRITE_STATE_IDLE;
                	eCurrentM95m01Trx->transactionInProgress= false;
                	eCurrentM95m01Trx->transactionCompleted= true;
					M95M01_SetWriteErrorDCODE(M95M01_WRITE_STATE_WRITE_DATA_1);
                }
                break;
            }

        case M95M01_WRITE_STATE_WRITE_DATA_2:
        	/*if address not in range we roll over*/
        	while(ulDataAddress> ulEepromLastAddress[eCurrentM95m01Trx->partition])
        	{
        		ulDataAddress-= ulEepromSize[eCurrentM95m01Trx->partition];
        	}

        	if((ulDataAddress+ uwDataLen)<= (ulEepromLastAddress[eCurrentM95m01Trx->partition]+ 1))
        	{
        		/*data to write is within start and last address*/
        		uwDataLenToWrite= uwDataLen;
        	}
        	else
        	{
        		/*data to write exceeds start and last address, so we fetch in-range first*/
        		uwDataLenToWrite= (ulEepromLastAddress[eCurrentM95m01Trx->partition]- ulDataAddress)+ 1;
        	}

        	/*check for page write overflow*/
        	{
        		uint32_t _byteOffset= ulDataAddress% M95M01_CFG_PAGE_SIZE;
            	if((_byteOffset+ uwDataLenToWrite)> M95M01_CFG_PAGE_SIZE)
            	{
            		uwDataLenToWrite= M95M01_CFG_PAGE_SIZE- _byteOffset;
            	}
        	}

        	eCurrentM95m01Trx->state.write= M95M01_WRITE_STATE_WRITE_DATA_3;

        case M95M01_WRITE_STATE_WRITE_DATA_3:
            if(SUCCESS== M95M01_WriteData(ulDataAddress, pucDataPtr, uwDataLenToWrite, eCurrentM95m01Trx->timeout))
            {
            	eCurrentM95m01Trx->state.write= M95M01_WRITE_STATE_READ_RDSR_REGISTER_2;
            }
            break;

        case M95M01_WRITE_STATE_READ_RDSR_REGISTER_2:
            if((false== eEepromSpiTrx.transactionCompleted)&& (false== M95M01_IsTimeout()))
            {
                break;
            }
            else if(true== M95M01_IsTimeout())
            {
                if(0!= eCurrentM95m01Trx->retryCounter)
                {
                	eCurrentM95m01Trx->retryCounter--;
                    eCurrentM95m01Trx->state.write= M95M01_WRITE_STATE_READ_RDSR_REGISTER;
                }
                else
                {
                    eCurrentM95m01Trx->status= M95M01_STATUS_TXRX_TIMEOUT;
                    eCurrentM95m01Trx->state.write= M95M01_WRITE_STATE_IDLE;
                	eCurrentM95m01Trx->transactionInProgress= false;
                	eCurrentM95m01Trx->transactionCompleted= true;
					M95M01_SetWriteErrorDCODE(M95M01_WRITE_STATE_READ_RDSR_REGISTER_2);
                }
                break;
            }

            if(SUCCESS== M95M01_TxRxCmd(M95M01_CMD_RDSR, eCurrentM95m01Trx->timeout))
            {
            	eCurrentM95m01Trx->state.write= M95M01_WRITE_STATE_CHECK_WRITE_BUSY_2;
            }
            break;

        case M95M01_WRITE_STATE_CHECK_WRITE_BUSY_2:
            if((false== eEepromSpiTrx.transactionCompleted)&& (false== M95M01_IsTimeout()))
            {
                break;
            }
            else if(true== M95M01_IsTimeout())
            {
                if(0!= eCurrentM95m01Trx->retryCounter)
                {
                	eCurrentM95m01Trx->retryCounter--;
                    eCurrentM95m01Trx->state.write= M95M01_WRITE_STATE_READ_RDSR_REGISTER;
                }
                else
                {
                    eCurrentM95m01Trx->status= M95M01_STATUS_TXRX_TIMEOUT;
                    eCurrentM95m01Trx->state.write= M95M01_WRITE_STATE_IDLE;
                	eCurrentM95m01Trx->transactionInProgress= false;
                	eCurrentM95m01Trx->transactionCompleted= true;
					M95M01_SetWriteErrorDCODE(M95M01_WRITE_STATE_CHECK_WRITE_BUSY_2);
                }
                break;
            }

            if(1== (pucCmdReply[0]& M95M01_STATUS_WIP))//write busy
            {
                eCurrentM95m01Trx->state.write= M95M01_WRITE_STATE_READ_RDSR_REGISTER_2;
            }
            else
            {
                eCurrentM95m01Trx->state.write= M95M01_WRITE_STATE_VALIDATE_DATA_1;
            }

            break;

        case M95M01_WRITE_STATE_VALIDATE_DATA_1:
            if((false== eEepromSpiTrx.transactionCompleted)&& (false== M95M01_IsTimeout()))
            {
                break;
            }
            else if(true== M95M01_IsTimeout())
            {
                if(0!= eCurrentM95m01Trx->retryCounter)
                {
                	eCurrentM95m01Trx->retryCounter--;
                    eCurrentM95m01Trx->state.write= M95M01_WRITE_STATE_READ_RDSR_REGISTER;
                }
                else
                {
                    eCurrentM95m01Trx->status= M95M01_STATUS_TXRX_TIMEOUT;
                    eCurrentM95m01Trx->state.write= M95M01_WRITE_STATE_IDLE;
                	eCurrentM95m01Trx->transactionInProgress= false;
                	eCurrentM95m01Trx->transactionCompleted= true;
					M95M01_SetWriteErrorDCODE(M95M01_WRITE_STATE_VALIDATE_DATA_1);
                }
                break;
            }

        	memset((uint8_t *)pucReadData, 0xff, uwDataLenToWrite);
            if(SUCCESS== M95M01_ReadData(ulDataAddress, uwDataLenToWrite, eCurrentM95m01Trx->timeout))
            {
            	eCurrentM95m01Trx->state.write= M95M01_WRITE_STATE_VALIDATE_DATA_2;
            }
            break;

        case M95M01_WRITE_STATE_VALIDATE_DATA_2:
            if((false== eEepromSpiTrx.transactionCompleted)&& (false== M95M01_IsTimeout()))
            {
                break;
            }
            else if(true== M95M01_IsTimeout())
            {
                if(0!= eCurrentM95m01Trx->retryCounter)
                {
                	eCurrentM95m01Trx->retryCounter--;
                    eCurrentM95m01Trx->state.write= M95M01_WRITE_STATE_READ_RDSR_REGISTER;
                }
                else
                {
                    eCurrentM95m01Trx->status= M95M01_STATUS_TXRX_TIMEOUT;
                    eCurrentM95m01Trx->state.write= M95M01_WRITE_STATE_IDLE;
                	eCurrentM95m01Trx->transactionInProgress= false;
                	eCurrentM95m01Trx->transactionCompleted= true;
					M95M01_SetWriteErrorDCODE(M95M01_WRITE_STATE_VALIDATE_DATA_2);
                }
                break;
            }
            if(0!= memcmp((uint8_t *)pucReadData, pucDataPtr, uwDataLenToWrite))
            {
            	char *_hexstring[128];
            	DBG_Print("M95M01_WriteData addr:%d len:%d data:%s.\r\n", ulDataAddress, uwDataLenToWrite, UTILI_BytesToHexString(pucDataPtr, uwDataLenToWrite, _hexstring));
            	DBG_Print("M95M01_ReadData  addr:%d len:%d data:%s.\r\n", ulDataAddress, uwDataLenToWrite, UTILI_BytesToHexString(pucReadData, uwDataLenToWrite, _hexstring));
                if(0!= eCurrentM95m01Trx->retryCounter)
                {
                	eCurrentM95m01Trx->retryCounter--;
                    eCurrentM95m01Trx->state.write= M95M01_WRITE_STATE_READ_RDSR_REGISTER;
                }
                else
                {
                    eCurrentM95m01Trx->status= M95M01_STATUS_VALIDATE_ERROR;
                    eCurrentM95m01Trx->state.write= M95M01_WRITE_STATE_IDLE;
                	eCurrentM95m01Trx->transactionInProgress= false;
                	eCurrentM95m01Trx->transactionCompleted= true;

                    DBG_Print("M95M01_WRITE_VALIDATE_ERROR.\r\n");
					M95M01_SetValidateErrorDCODE(M95M01_WRITE_STATE_VALIDATE_DATA_2);
                }
                break;
            }
//            else
//            {
//            	char *_hexstring[128];
//            	DBG_Print("M95M01_WriteData addr:%d len:%d data:%s.\r\n", ulDataAddress, uwDataLenToWrite, UTILI_BytesToHexString(pucDataPtr, uwDataLenToWrite, _hexstring));
//            	DBG_Print("M95M01_ReadData  addr:%d len:%d data:%s.\r\n", ulDataAddress, uwDataLenToWrite, UTILI_BytesToHexString(pucReadData, uwDataLenToWrite, _hexstring));
//            }

            ulDataAddress+= uwDataLenToWrite;
            uwDataLen-= uwDataLenToWrite;
            pucDataPtr+= uwDataLenToWrite;

        	/*we decrement len every iteration, if len is 0 meaning all data fetched.*/
        	if(0== uwDataLen)
        	{
                eCurrentM95m01Trx->status= M95M01_STATUS_SUCCESS;
                eCurrentM95m01Trx->state.write= M95M01_WRITE_STATE_IDLE;
            	eCurrentM95m01Trx->transactionInProgress= false;
            	eCurrentM95m01Trx->transactionCompleted= true;

                //DBG_Print("M95M01_WRITE_SUCCESS. \r\n");
        	}
        	else
        	{
                eCurrentM95m01Trx->state.write= M95M01_WRITE_STATE_READ_RDSR_REGISTER;/*next iteration*/
        	}
            break;

        default:
			SYS_FailureHandler();
            break;
    }
}

static void M95M01_Transaction_Read(void)
{
	static uint32_t ulDataAddress;
	static uint16_t uwDataLen;
	static uint16_t uwDataLenToFetch;
	static uint8_t *pucDataPtr;

    switch(eCurrentM95m01Trx->state.read)
    {
        case M95M01_READ_STATE_IDLE:
            break;

        case M95M01_READ_STATE_READ_DATA_1:
        	eCurrentM95m01Trx->status= M95M01_STATUS_BUSY;

    		pucDataPtr= eCurrentM95m01Trx->dataPtr;/*this will be incremented if there is iteration*/
    		ulDataAddress= eCurrentM95m01Trx->dataAddress;
    		uwDataLen= eCurrentM95m01Trx->dataLen;
    		ulDataAddress+= ulEepromStartAddress[eCurrentM95m01Trx->partition];/*plus address offset*/

        case M95M01_READ_STATE_READ_DATA_2:
        	/*if address not in range we roll over*/
        	while(ulDataAddress> ulEepromLastAddress[eCurrentM95m01Trx->partition])
        	{
        		ulDataAddress-= ulEepromSize[eCurrentM95m01Trx->partition];
        	}

        	if((ulDataAddress+ uwDataLen)<= (ulEepromLastAddress[eCurrentM95m01Trx->partition]+ 1))
        	{
        		/*data to fetch is within start and last address*/
        		uwDataLenToFetch= uwDataLen;
        	}
        	else
        	{
        		/*data to fetch exceeds start and last address, so we fetch in-range first*/
        		uwDataLenToFetch= (ulEepromLastAddress[eCurrentM95m01Trx->partition]- ulDataAddress)+ 1;
        	}

        	if(uwDataLenToFetch> M95M01_CFG_RXBYTES_SIZE)
        	{
        		uwDataLenToFetch= M95M01_CFG_RXBYTES_SIZE;
        	}
        	eCurrentM95m01Trx->state.read= M95M01_READ_STATE_READ_DATA_3;

        case M95M01_READ_STATE_READ_DATA_3:
            if(SUCCESS== M95M01_ReadData(ulDataAddress, uwDataLenToFetch, eCurrentM95m01Trx->timeout))
            {
            	eCurrentM95m01Trx->state.read= M95M01_READ_STATE_READ_DATA_4;
            }
            break;

        case M95M01_READ_STATE_READ_DATA_4:
            if((false== eEepromSpiTrx.transactionCompleted)&& (false== M95M01_IsTimeout()))
            {
                break;
            }
            else if(true== M95M01_IsTimeout())
            {
                if(0!= eCurrentM95m01Trx->retryCounter)
                {
                	eCurrentM95m01Trx->retryCounter--;
                    eCurrentM95m01Trx->state.read= M95M01_READ_STATE_READ_DATA_1;
                }
                else
                {
                    eCurrentM95m01Trx->status= M95M01_STATUS_TXRX_TIMEOUT;
                    eCurrentM95m01Trx->state.read= M95M01_READ_STATE_IDLE;
                	eCurrentM95m01Trx->transactionInProgress= false;
                	eCurrentM95m01Trx->transactionCompleted= true;
                	M95M01_SetReadErrorDCODE(0);
                }
                break;
            }

            memcpy(pucDataPtr, (uint8_t *)pucReadData, uwDataLenToFetch);

            ulDataAddress+= uwDataLenToFetch;
            uwDataLen-= uwDataLenToFetch;
            pucDataPtr+= uwDataLenToFetch;

        	/*we decrement len every iteration, if len is 0 meaning all data fetched.*/
        	if(0== uwDataLen)
        	{
                eCurrentM95m01Trx->status= M95M01_STATUS_SUCCESS;
                eCurrentM95m01Trx->state.read= M95M01_READ_STATE_IDLE;
            	eCurrentM95m01Trx->transactionInProgress= false;
            	eCurrentM95m01Trx->transactionCompleted= true;

                //DBG_Print("M95M01_READ_SUCCESS. \r\n");
        	}
        	else
        	{
                eCurrentM95m01Trx->state.read= M95M01_READ_STATE_READ_DATA_2;/*next iteration*/
        	}
            break;

        default:
			SYS_FailureHandler();
            break;
    }
}

M95M01_Transaction_Status M95M01_GetTrxStatus(void)
{
    return eCurrentM95m01Trx->status;
}

bool M95M01_GetM95M01AccessErrorFlag(void)
{
	return bM95M01AccessErrorFlag;
}

void M95M01_ClearM95M01AccessErrorFlag(void)
{
	bM95M01AccessErrorFlag= false;
}

uint8_t M95M01_TransactionState(void)
{
	switch(eCurrentM95m01Trx->type)
	{
		case M95M01_TYPE_WRITE:
			if(M95M01_WRITE_STATE_IDLE== eCurrentM95m01Trx->state.write)
		    {
		    	return SLEEP_TaskState;
		    }
			break;

		case M95M01_TYPE_READ:
			if(M95M01_READ_STATE_IDLE== eCurrentM95m01Trx->state.read)
		    {
		    	return SLEEP_TaskState;
		    }
			break;
	}

    return RUN_TaskState;
}

