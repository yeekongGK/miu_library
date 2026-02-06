/**
 ******************************************************************************
 * @file           : dbg.h
 * @brief          : Header for debug task
 ******************************************************************************
 * @attention
 * @PIC: muhammad.ahmad@georgekent.net
 * @Created: 18 Dec 2020
 * @Copyright: George Kent (Malaysia) Berhad.
 *
 ******************************************************************************
 **/
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef INC_DBG_H_
#define INC_DBG_H_

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Exported types ------------------------------------------------------------*/
typedef enum
{
  CHANNEL_COM = 0,
  CHANNEL_SNF = 1,
  CHANNEL_DBG = 2
} DBG_Channel_t;

/* Exported constants --------------------------------------------------------*/
#define DBG_CFG_NO_OF_CHANNEL 3
#define DBG_CFG_BUF_SIZE 512 // 768 512
#define DBG_CHANNEL_COM 0
#define DBG_CHANNEL_SNF 1
#define DBG_CHANNEL_DBG 2
#define DBG_CHANNEL_COM_IND '0'
#define DBG_CHANNEL_SNF_IND '1'
#define DBG_CHANNEL_DBG_IND '2'

/* Exported macros -----------------------------------------------------------*/
#define DBG_Print(f_, ...) printf((f_), ##__VA_ARGS__)
#define DBG_PrintLine(f_, ...)   \
  {                              \
    printf((f_), ##__VA_ARGS__); \
    printf("\r\n");              \
  }

#define DBG_DBG_INT(x) ((int)(x))
#define DBG_DBG_FLOAT(x) ((x > 0) ? ((int)(((x) - DBG_DBG_INT(x)) * 1000)) : (-1 * (((int)(((x) - DBG_DBG_INT(x)) * 1000)))))

/* Exported functions --------------------------------------------------------*/
void DBG_Init(UART_HandleTypeDef *uart_instance);
void DBG_Task(void);

void DBG_COM_PutByte(uint8_t _byte);
void DBG_SNF_PutByte(uint8_t _byte);
void DBG_DBG_PutByte(uint8_t _byte);
void DBG_DBG_PutString(char *_string);
void DBG_DBG_PutNumber(uint32_t _uint32);

void DBG_COM_PutString(char *_string);
void DBG_SNF_PutString(char *_string);
void DBG_PrintNow(uint8_t *bytes, uint8_t len);

void DBG_Channel_Printf(DBG_Channel_t channel, const char *format, ...);

#endif /* INC_DBG_H_ */
