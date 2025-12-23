/*
 * user_com.c
 *
 *  Created on: Nov 2, 2021
 *      Author: trion
 */
#include <ctype.h>
#include <stdbool.h>

#include "stdio.h"
#include "stdarg.h"
#include "string.h"
#include "math.h"

#include "main.h"
#include "ring_buffer.h"
#include "user_com.h"
#include "user_def.h"
#include "buffers.h"
#include "string.h"
#include  <errno.h>
#include  <sys/unistd.h>

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart4;

/* UART DMA 버퍼들은 buffers.c에서 RAM_D2 영역에 정의됨 */



// UART1 queue
Queue rx_UART1_queue;
Queue rx_UART1_line_queue;
Queue tx_UART1_queue;

// UART2 queue
Queue rx_UART2_queue;
Queue rx_UART2_line_queue;
Queue tx_UART2_queue;

// UART4 queue
Queue rx_UART4_queue;
Queue rx_UART4_line_queue;
Queue tx_UART4_queue;

// USB queue
Queue rx_USB_queue;
Queue rx_USB_line_queue;
Queue tx_USB_queue;


/* UART/USB 상태 구조체는 buffers.c에서 RAM_D2 (Non-cache) 영역에 정의됨 */

extern struct usr_ST usr_val_ST;
extern union flash_UT ufl_UT;

uint8_t atoh(char in_ascii)
{
    uint8_t rtn_val;

    if ( (in_ascii >= '0') && (in_ascii <= '9') )
    {
        rtn_val = in_ascii - '0';
    }
    else if ( (in_ascii >= 'a') && (in_ascii <= 'f') )
    {
        rtn_val = in_ascii - 'a' + 0x0A;
    }
    else if ( (in_ascii >= 'A') && (in_ascii <= 'F') )
    {
        rtn_val = in_ascii - 'A' + 0x0A;
    }
    else
    {
        rtn_val = 0xFF;
    }

    return rtn_val;
}

uint8_t atod(char in_ascii)
{
    uint8_t rtn_val;

    if ( (in_ascii >= '0') || (in_ascii <= '9') )
    {
        rtn_val = in_ascii - '0';
    }
    else
    {
        rtn_val = 0xFF;
    }

    return rtn_val;
}

uint32_t atoh_str(char *str, uint8_t len)
{
	uint32_t val = 0;
	for (uint8_t i = 0; i < len; i++)
	{
		char ch = str[i];
		uint8_t digit = atoh(ch);  // 안전하게 처리돼야 함
		val = (val << 4) | (digit & 0xF);  // 한 자리씩 shift 후 추가
	}
	return val;
}

// 안전한 문자열-숫자 변환 함수 예시
uint16_t safe_atoi(const char *str, uint16_t len)
{
    if (!str || len == 0) {
        return 0;
    }

    uint16_t result = 0;
    uint16_t i = 0;

    // 선행 공백 건너뛰기
    while (i < len && (str[i] == ' ' || str[i] == '\t')) {
        i++;
    }

    // 숫자 변환
    while (i < len && str[i] >= '0' && str[i] <= '9') {
        result = result * 10 + (str[i] - '0');
        i++;
    }

    return result;
}

uint32_t atod_str(char *in_asc_str,uint8_t len)
{
	uint32_t rtn_val,i,j;

	rtn_val = 0;

	for(i=0,j=1;i<len;i++,j*=10)
	{
		rtn_val += (uint32_t)atod(in_asc_str[len-1-i])*j;
	}

	return rtn_val;
}

bool is_float(const char *str)
{
	bool dot_found = false;

	if (str == NULL || *str == '\0')
		return false;

	if (*str == '+' || *str == '-')
		str++;

	if (*str == '\0') return false;

	while (*str)
	{
		if (*str == '.')
		{
			if (dot_found) return false;
			dot_found = true;
		}
		else if (!isdigit((unsigned char)*str))
		{
			return false;
		}
		str++;
	}
	return true;
}

bool is_numeric(const char *str)
{
	if (str == NULL || *str == '\0')
		return false;

	while (*str)
	{
		if (!isdigit((unsigned char)*str))
			return false;
		str++;
	}
	return true;
}


void UART_baudrate_set(UART_HandleTypeDef *tm_hart,uint32_t baud_rate)
{
	tm_hart->Init.BaudRate = baud_rate;
  if (HAL_UART_Init(tm_hart) != HAL_OK)
  {
    Error_Handler();
  }
}


void init_UART_COM(void)
{

	UART_baudrate_set(&huart1,115200);
	UART_baudrate_set(&huart2,9600);
	UART_baudrate_set(&huart4,115200);

	// uart1
  InitQueue(&rx_UART1_line_queue,256);
  InitQueue(&rx_UART1_queue,512);
  InitQueue(&tx_UART1_queue,512);
	// uart2
  InitQueue(&rx_UART2_line_queue,512);
  InitQueue(&rx_UART2_queue,8192);
  InitQueue(&tx_UART2_queue,512);
//	// uart4
  InitQueue(&rx_UART4_line_queue,256);
  InitQueue(&rx_UART4_queue,512);
  InitQueue(&tx_UART4_queue,512);
//
	__HAL_UART_ENABLE_IT(&huart1,UART_IT_IDLE);
	HAL_UART_Receive_DMA(&huart1, uart1_stat.rx_buffer, U1_RX_BUF_SIZE);
  	__HAL_UART_ENABLE_IT(&huart2,UART_IT_IDLE);
  	HAL_UART_Receive_DMA(&huart2, uart2_stat.rx_buffer, U2_RX_BUF_SIZE);
	__HAL_UART_ENABLE_IT(&huart4,UART_IT_IDLE);
	HAL_UART_Receive_DMA(&huart4, uart4_stat.rx_buffer, U4_RX_BUF_SIZE);
}

void init_USB_COM(void)
{
	// USB
  InitQueue(&rx_USB_line_queue,256);
  InitQueue(&rx_USB_queue,512);
  InitQueue(&tx_USB_queue,512);
}

void printf_UARTC(UART_HandleTypeDef *h_tmUART,uint8_t color,const char *str_buf,...)
{
    char tx_bb[512];
    uint32_t len=0;

    if (color != PR_NOP)
    {
			// color set
			sprintf(tx_bb,"\033[%dm",color);
			len = strlen(tx_bb);
    }


    va_list ap;
    va_start(ap, str_buf);
    vsprintf(&tx_bb[len], str_buf, ap);
    va_end(ap);

    len = strlen(tx_bb);
    tx_bb[len] = 0;
    if (h_tmUART->Instance == USART1)		// ESP UART
    {
      Enqueue_bytes(&tx_UART1_queue,(uint8_t *)tx_bb,len);
      //HAL_UART_Transmit(&huart1,(uint8_t *)tx_bb,len,1000);
    }
    if (h_tmUART->Instance == USART2)		// 485 UART
    {
      //Enqueue_bytes(&tx_UART2_queue,(uint8_t *)tx_bb,len);
      HAL_UART_Transmit(&huart2,(uint8_t *)tx_bb,len,1000);
    }
    else if (h_tmUART->Instance == UART4)  // DEBUG Port
    {
    	//HAL_UART_Transmit(&huart4,(uint8_t *)tx_bb,len,1000);
      Enqueue_bytes(&tx_UART4_queue,(uint8_t *)tx_bb,len);
    }
}

void printf_UARTC_485(UART_HandleTypeDef *h_tmUART,uint8_t color,const char *str_buf,...)
{
  char tx_bb[512];
  uint32_t len=0;

  if (color != PR_NOP)
  {
		// color set
		sprintf(tx_bb,"\033[%dm",color);
		len = strlen(tx_bb);
  }


  va_list ap;
  va_start(ap, str_buf);
  vsprintf(&tx_bb[len], str_buf, ap);
  va_end(ap);

  len = strlen(tx_bb);
  tx_bb[len] = 0;
  if (h_tmUART->Instance == USART2)
  {
  	HAL_UART_Transmit(&huart2,(uint8_t *)tx_bb,len,1000);
    //Enqueue_bytes(&tx_UART2_queue,(uint8_t *)tx_bb,len);
  }
}

//
// printf_UARTC(&huart1,PR_GRN,"UART1 Test from STM32H723\r\n");
// ESP 연결은 UART1 을 사용한다.
void ESP_Printf(uint8_t color,const char *str_buf,...)
{
  char tx_bb[512];
  uint32_t len=0;

  if (color != PR_NOP)
  {
		// color set
		sprintf(tx_bb,"\033[%dm",color);
		len = strlen(tx_bb);
  }


  va_list ap;
  va_start(ap, str_buf);
  vsprintf(&tx_bb[len], str_buf, ap);
  va_end(ap);

  len = strlen(tx_bb);
  tx_bb[len] = 0;
  if (huart1.Instance == USART1)
  {
  	HAL_UART_Transmit(&huart1,(uint8_t *)tx_bb,len,1000);
    //Enqueue_bytes(&tx_UART1_queue,(uint8_t *)tx_bb,len);
  }
}

// ESP 연결은 UART1 을 사용한다.
COM_Idy_Typ ESP_GetLine(uint8_t *line_buf)
{
	uint16_t q_len;
	uint8_t rx_dat;
	COM_Idy_Typ rtn_val = NOT_LINE;

	q_len = Len_queue(&rx_UART1_queue);
	if (q_len)
	{
		for(uint16_t i=0;i<q_len;i++)
		{
			rx_dat = Dequeue(&rx_UART1_queue);
			Enqueue(&rx_UART1_line_queue,rx_dat);
			#ifdef UART1_ECHO
			printf_UARTC(&huart4,PR_NOP,"%c",rx_dat);
			#endif
			if (rx_dat == UART1_ETX)
			{
				#ifdef UART1_ECHO
				printf_UARTC(&huart4,PR_NOP,"\n");
				#endif
				flush_queue(&rx_UART1_queue);
				q_len = Len_queue(&rx_UART1_line_queue);
				Dequeue_bytes(&rx_UART1_line_queue,line_buf,q_len);
				line_buf[q_len-1] = 0;

		  	//printf_UARTC(&huart4,PR_GRN,"%s\033[%dm\r\n",line_buf,PR_INI);
				DBG_Printf(PR_GRN,"ESP Rcv data : %s\r\n",line_buf);
				DBG_Printf(PR_INI,"");

				if(
						(strncmp((char *)line_buf,"sdcd",4) == 0) ||
						(strncmp((char *)line_buf,"boot",4) == 0) ||
						(strncmp((char *)line_buf,"dir",3) == 0) ||
						(strncmp((char *)line_buf,"??",2) == 0) ||
						(strncmp((char *)line_buf,"c: ",3) == 0) ||
						(strncmp((char *)line_buf,"z: ",3) == 0) ||
						(strncmp((char *)line_buf,"f: ",3) == 0) ||
						(strncmp((char *)line_buf,"t: ",3) == 0) ||
						(strncmp((char *)line_buf,"b: ",3) == 0) ||
						(strncmp((char *)line_buf,"g: ",3) == 0) ||
						(strncmp((char *)line_buf,"rr: ",4) == 0) ||
						(strncmp((char *)line_buf,"gg: ",4) == 0) ||
						(strncmp((char *)line_buf,"bb: ",4) == 0) ||
						(strncmp((char *)line_buf,"SETG",4) == 0) ||
						(strncmp((char *)line_buf,"STRT",4) == 0) ||
						(strncmp((char *)line_buf,"STOP",4) == 0) ||
						(strncmp((char *)line_buf,"PAUZ",4) == 0) ||
						(strncmp((char *)line_buf,"RGRP",4) == 0) ||
						(strncmp((char *)line_buf,"INIT",4) == 0) ||
						(strncmp((char *)line_buf,"RESM",4) == 0)		)
				{
					rtn_val = RCV_LINE;
				}
				else
				{
					rtn_val = WNG_LINE;
				}
				break;	// escape from for()
			}
		}
	}

	return rtn_val;

}

// DBUG 포트는 UART4를 사용한다.
void DBG_Printf(uint8_t color,const char *str_buf,...)
{
  char tx_bb[256];
  uint32_t len=0;

  if (color != PR_NOP)
  {
		// color set
		sprintf(tx_bb,"\033[%dm",color);
		len = strlen(tx_bb);
  }


  va_list ap;
  va_start(ap, str_buf);
  vsprintf(&tx_bb[len], str_buf, ap);
  va_end(ap);

  len = strlen(tx_bb);
  tx_bb[len] = 0;
  if (huart4.Instance == UART4)
  {
  	HAL_UART_Transmit(&huart4,(uint8_t *)tx_bb,len,1000);
    //Enqueue_bytes(&tx_UART4_queue,(uint8_t *)tx_bb,len);
  }
}

// DBUG 포트는 UART4를 사용한다.
COM_Idy_Typ DBUG_GetLine(uint8_t *line_buf)
{
	uint16_t q_len;
	uint8_t rx_dat;
	COM_Idy_Typ rtn_val = NOT_LINE;

	q_len = Len_queue(&rx_UART4_queue);
	if (q_len)
	{
		for(uint16_t i=0;i<q_len;i++)
		{
			rx_dat = Dequeue(&rx_UART4_queue);
			Enqueue(&rx_UART4_line_queue,rx_dat);
//			#ifdef UART1_ECHO
			DBG_Printf(PR_WHT,"%c",rx_dat);
//			printf_UARTC(&huart4,PR_NOP,"%c",rx_dat);
//			#endif
			if (rx_dat == UART4_ETX)
			{
				#ifdef UART4_ECHO
				DBG_Printf(PR_WHT,"\n");
				//printf_UARTC(&huart4,PR_NOP,"\n");
				#endif
				flush_queue(&rx_UART4_queue);
				q_len = Len_queue(&rx_UART4_line_queue);
				Dequeue_bytes(&rx_UART4_line_queue,line_buf,q_len);
				line_buf[q_len-1] = 0;

		  	//printf_UARTC(&huart4,PR_YEL,"%s\033[%dm\r\n",line_buf,PR_INI);

				if(	(strncmp((char *)line_buf,"??",2) == 0) ||
						(strncmp((char *)line_buf,"dir",3) == 0) ||
						(strncmp((char *)line_buf,"cfg",3) == 0) ||
						(strncmp((char *)line_buf,"c: ",3) == 0) ||
						(strncmp((char *)line_buf,"f: ",3) == 0) ||
						(strncmp((char *)line_buf,"t: ",3) == 0) ||
						(strncmp((char *)line_buf,"b: ",3) == 0) ||
						(strncmp((char *)line_buf,"g: ",3) == 0) ||
						(strncmp((char *)line_buf,"rr: ",4) == 0) ||
						(strncmp((char *)line_buf,"gg: ",4) == 0) ||
						(strncmp((char *)line_buf,"bb: ",4) == 0) ||
						(strncmp((char *)line_buf,"SETG",4) == 0) ||
						(strncmp((char *)line_buf,"STRT",4) == 0) ||
						(strncmp((char *)line_buf,"STOP",4) == 0) ||
						(strncmp((char *)line_buf,"INIT",4) == 0) ||
						(strncmp((char *)line_buf,"PAUZ",4) == 0) ||
						(strncmp((char *)line_buf,"RESM",4) == 0)		)
				{
					rtn_val = RCV_LINE;
				}
				else
				{
					rtn_val = WNG_LINE;
				}
				break;	// escape from for()
			}
		}
	}

	return rtn_val;
}


// DBUG 포트는 UART4를 사용한다.
COM_Idy_Typ TF_DBUG_GetLine(uint8_t *line_buf)
{
#define UART4_ECHO
	uint16_t q_len;
	uint8_t rx_dat;
	COM_Idy_Typ rtn_val = NOT_LINE;

	q_len = Len_queue(&rx_UART4_queue);
	if (q_len)
	{
		for(uint16_t i=0;i<q_len;i++)
		{
			rx_dat = Dequeue(&rx_UART4_queue);
			Enqueue(&rx_UART4_line_queue,rx_dat);
			#ifdef UART4_ECHO
			DBG_Printf(PR_WHT,"%c",rx_dat);
			#endif
			if (rx_dat == UART4_ETX)
			{
				#ifdef UART4_ECHO
				DBG_Printf(PR_WHT,"\n");
				#endif
				flush_queue(&rx_UART4_queue);
				q_len = Len_queue(&rx_UART4_line_queue);
				Dequeue_bytes(&rx_UART4_line_queue,line_buf,q_len);
				line_buf[q_len-1] = 0;

				printf_UARTC(&huart1,PR_NOP,"%s",line_buf);

		  	//printf_UARTC(&huart4,PR_YEL,"%s\033[%dm\r\n",line_buf,PR_INI);

				if(	(strncmp((char *)line_buf,"sdcd",4) == 0) ||
						(strncmp((char *)line_buf,"mess",4) == 0)		)
				{
					rtn_val = RCV_LINE;
				}
				else
				{
					rtn_val = WNG_LINE;
				}
				break;	// escape from for()
			}
		}
	}

	return rtn_val;
}


//*************************************************************************
//               이하 사용금지 예정
//*************************************************************************
COM_Idy_Typ UART2_GetLine(uint8_t *line_buf)
{
	uint16_t q_len;
	uint8_t rx_dat;
	COM_Idy_Typ rtn_val = NOT_LINE;

	q_len = Len_queue(&rx_UART2_queue);
	if (q_len)
	{
		for(uint16_t i=0;i<q_len;i++)
		{
			rx_dat = Dequeue(&rx_UART2_queue);
			Enqueue(&rx_UART2_line_queue,rx_dat);
			#ifdef UART2_ECHO
			printf_UARTC(&huart2,PR_NOP,"%c",rx_dat);
			#endif
			if (rx_dat == UART2_ETX)
			{
				#ifdef UART2_ECHO
				printf_UARTC(&huart2,PR_NOP,"\n");
				#endif
				flush_queue(&rx_UART2_queue);
				q_len = Len_queue(&rx_UART2_line_queue);
				Dequeue_bytes(&rx_UART2_line_queue,line_buf,q_len);
				line_buf[q_len-1] = 0;

		  	printf_UARTC(&huart2,PR_YEL,"%s\033[%dm\r\n",line_buf,PR_INI);
		  	//CAN_str_trnsfer(line_buf,q_len);
				// "smpc" : Sampling Start
				// "clen" : Clean Start
				// "stop" : Stop
				// "rven" : Reserved sampling Enable/Disable
				// "rvtm" : Reserved sampling Time Setting
				// "aten" : Auto sampling Enable/Disable
				// "atlv" : Auto sampling Volt Setting
				if
				(
						(strncmp((char *)line_buf,"smpc",4) == 0) ||
						(strncmp((char *)line_buf,"clen",4) == 0) ||
						(strncmp((char *)line_buf,"stop",4) == 0) ||
						(strncmp((char *)line_buf,"rven",4) == 0) ||
						(strncmp((char *)line_buf,"rvtm",4) == 0) ||
						(strncmp((char *)line_buf,"aten",4) == 0) ||
						(strncmp((char *)line_buf,"atlv",4) == 0) ||
						(strncmp((char *)line_buf,"odor",4) == 0) ||
						(strncmp((char *)line_buf,"bzon",4) == 0) ||
						(strncmp((char *)line_buf,"qstr",4) == 0) ||
						(strncmp((char *)line_buf,"rdat",4) == 0)
				)
				{
					rtn_val = RCV_LINE;
				}
				else
				{
					rtn_val = WNG_LINE;
				}
				break;	// escape from for()
			}
		}
	}

	return rtn_val;
}

void printf_USBC(uint8_t color,const char *str_buf,...)
{
    char tx_bb[1024];
    uint32_t len=0;

    if (color != PR_NOP)
    {
			// color set
			sprintf(tx_bb,"\033[%dm",color);
			len = strlen(tx_bb);
    }


    va_list ap;
    va_start(ap, str_buf);
    vsprintf(&tx_bb[len], str_buf, ap);
    //vsnprintf(&tx_bb[len], sizeof(tx_bb) - len, str_buf, ap);
    va_end(ap);

    len = strlen(tx_bb);
    tx_bb[len] = 0;
    Enqueue_bytes(&tx_USB_queue,(uint8_t *)tx_bb,len);
}

COM_Idy_Typ USB_GetLine(uint8_t *line_buf)
{
	uint16_t q_len;
	uint8_t rx_dat;
	COM_Idy_Typ rtn_val = NOT_LINE;

	q_len = Len_queue(&rx_USB_queue);
	if (q_len)
	{
		for(uint16_t i=0;i<q_len;i++)
		{
			rx_dat = Dequeue(&rx_USB_queue);
			Enqueue(&rx_USB_line_queue,rx_dat);
			#ifdef USB_ECHO
			printf_USBC(PR_NOP,"%c",rx_dat);
			#endif
			if (rx_dat == USB_ETX)
			{
				#ifdef USB_ECHO
				printf_USBC(PR_NOP,"\n");
				#endif
				flush_queue(&rx_USB_queue);
				q_len = Len_queue(&rx_USB_line_queue);
				Dequeue_bytes(&rx_USB_line_queue,line_buf,q_len);
				line_buf[q_len-1] = 0;
				//printf_USBC(PR_YEL,(char *)line_buf);

		  	//printf_USBC(PR_YEL,"%s\033[%dm\r\n",line_buf,PR_INI);
		  	//CAN_str_trnsfer(line_buf,q_len);

				// "c:" : Contents 파일 선택 & 시작
				// "f:" : fps (5~20)
				// "t:" : rgb test (xxx ms)
				// "b:" : 휘도 밝기 (0 ~ 100% 단위)
				// "g:" : Gamma 값 (1 ~ 40 = 0.1 ~ 4.0이므로 나누기 10F 해야함)
				// "rr:" : xxx (0 ~ 255, RED Gain 값 조절, 255= RED 밝기 최대)
				// "gg:" : xxx (0 ~ 255, GREEN Gain 값 조절, 255= GREEN 밝기 최대)
				// "bb:" : xxx (0 ~ 255, BLUE Gain 값 조절, 255= BLUE 밝기 최대)
				// "STRT" : 최종 Play 했던 컨텐츠 다시 재생
				// "STOP" : 플레이중인 컨텐츠 중지
				// "PAUZ" : 플레이중인 컨텐츠 일시정지
				// "RESM" : 일시정지중인 컨텐츠 계속 Play
				if
				(
						(strncmp((char *)line_buf,"??",2) == 0) ||
						(strncmp((char *)line_buf,"c: ",3) == 0) ||
						(strncmp((char *)line_buf,"f: ",3) == 0) ||
						(strncmp((char *)line_buf,"t: ",3) == 0) ||
						(strncmp((char *)line_buf,"b: ",3) == 0) ||
						(strncmp((char *)line_buf,"g: ",3) == 0) ||
						(strncmp((char *)line_buf,"rr: ",4) == 0) ||
						(strncmp((char *)line_buf,"gg: ",4) == 0) ||
						(strncmp((char *)line_buf,"bb: ",4) == 0) ||
						(strncmp((char *)line_buf,"SETG",4) == 0) ||
						(strncmp((char *)line_buf,"STRT",4) == 0) ||
						(strncmp((char *)line_buf,"STOP",4) == 0) ||
						(strncmp((char *)line_buf,"PAUZ",4) == 0) ||
						(strncmp((char *)line_buf,"RESM",4) == 0)
				)
				{
					rtn_val = RCV_LINE;
				}
				else
				{
					rtn_val = WNG_LINE;
				}
				break;	// escape from for()
			}
		}
	}

	return rtn_val;
}

