/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : Franticware
 * Date               : 2024-11-03
 * Description        : Main program body.
 *******************************************************************************/

#include "debug.h"
#include "queue.h"

/* Global define */

#define SN01 -3
#define SN00 -2
#define SN10 -1
#define S11 0
#define SP01 1
#define SP00 2
#define SP10 3

#define OUT_PERIOD_MS (24)

/* Global Variable */

volatile uint8_t tick = 0;

/*********************************************************************
 * @fn      GPIO_Toggle_INIT
 *
 * @brief   Initializes GPIOA.0
 *
 * @return  none
 */
void GPIO_Toggle_INIT(void) {
	{
		GPIO_InitTypeDef GPIO_InitStructure = { 0 };
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
		GPIO_Init(GPIOD, &GPIO_InitStructure);
		GPIO_WriteBit(GPIOD, GPIO_Pin_4, (Bit_SET));
		GPIO_WriteBit(GPIOD, GPIO_Pin_5, (Bit_SET));
		GPIO_WriteBit(GPIOD, GPIO_Pin_6, (Bit_SET));
	}
	{
		GPIO_InitTypeDef GPIO_InitStructure = { 0 };
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
		GPIO_Init(GPIOC, &GPIO_InitStructure);
	}
}

void TIM_INT_Init(u16 arr, u16 psc) {
	TIM_TimeBaseInitTypeDef TIMBase_InitStruct;
	NVIC_InitTypeDef NVIC_InitStruct;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	TIMBase_InitStruct.TIM_Period = arr;
	TIMBase_InitStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TIMBase_InitStruct.TIM_Prescaler = psc;
	TIMBase_InitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInit(TIM2, &TIMBase_InitStruct);

	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

	NVIC_InitStruct.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 5;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 5;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);
	TIM_Cmd(TIM2, ENABLE);
}

void TIM2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

void TIM2_IRQHandler(void) {
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET) {
		tick = 1;
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}

/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main(void) {
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
	SystemCoreClockUpdate();
	Delay_Init();
	int16_t state = S11;
	uint8_t swState = 1;
	Queue queue;
	QueueInit(&queue);
	GPIO_Toggle_INIT();
	TIM_INT_Init(OUT_PERIOD_MS - 1, 8000 - 1);
	Delay_Ms(OUT_PERIOD_MS);

	while (1) {
		uint16_t inputData = GPIO_ReadInputData(GPIOC) & 7;
		uint16_t inputRot = inputData & 3;
		switch (state) {
		case SN01:
			if (inputRot == 0b00) {
				state = SN00;
			} else if (inputRot == 0b11) {
				state = S11;
				// insert - output signal sequence
				QueuePut(&queue, GPIO_Pin_4 | 0);
				QueuePut(&queue, GPIO_Pin_5 | 0);
				QueuePut(&queue, GPIO_Pin_4 | 1);
				QueuePut(&queue, GPIO_Pin_5 | 1);
			}
			break;
		case SN00:
			if (inputRot == 0b01) {
				state = SN01;
			} else if (inputRot == 0b10) {
				state = SN10;
			}
			break;
		case SN10:
			if (inputRot == 0b11) {
				state = S11;
			} else if (inputRot == 0b00) {
				state = SN00;
			}
			break;
		case S11:
			if (inputRot == 0b01) {
				state = SP01;
			} else if (inputRot == 0b10) {
				state = SN10;
			}
			break;
		case SP01:
			if (inputRot == 0b11) {
				state = S11;
			} else if (inputRot == 0b00) {
				state = SP00;
			}
			break;
		case SP00:
			if (inputRot == 0b01) {
				state = SP01;
			} else if (inputRot == 0b10) {
				state = SP10;
			}
			break;
		case SP10:
			if (inputRot == 0b00) {
				state = SP00;
			} else if (inputRot == 0b11) {
				state = S11;
				// insert + output signal sequence
				QueuePut(&queue, GPIO_Pin_5 | 0);
				QueuePut(&queue, GPIO_Pin_4 | 0);
				QueuePut(&queue, GPIO_Pin_5 | 1);
				QueuePut(&queue, GPIO_Pin_4 | 1);
			}
			break;
		}

		if (tick) {
			tick = 0;
			uint8_t inputSw = !!(inputData & 4);
			if (inputSw != swState) {
				QueuePut(&queue, GPIO_Pin_6 | inputSw);
				swState = inputSw;
			}
			if (QueueNotEmpty(&queue)) {
				uint8_t item = QueueGet(&queue);
				GPIO_WriteBit(GPIOD,
						item & (GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6),
						((item & 1) == 0) ? (Bit_RESET) : (Bit_SET));
			}
		}
	}
}
