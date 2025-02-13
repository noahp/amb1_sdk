/*
 *  Routines to access hardware
 *
 *  Copyright (c) 2013 Realtek Semiconductor Corp.
 *
 *  This module is a confidential and proprietary property of RealTek and
 *  possession or use of this module requires written permission of RealTek.
 */

#include "ameba_soc.h"
#include "main.h"

#define PWM_PERIOD	(XTAL_ClkGet()/32768)

void tim5_gen_pwm_32k()
{
	int pwm_chan = 0;
	RTIM_TimeBaseInitTypeDef TIM_InitStruct_temp;
	TIM_CCInitTypeDef TIM_CCInitStruct;

	RTIM_TimeBaseStructInit(&TIM_InitStruct_temp);
	TIM_InitStruct_temp.TIM_Idx = 5;
	TIM_InitStruct_temp.TIM_Period = PWM_PERIOD;
	RTIM_TimeBaseInit(TIM5, &TIM_InitStruct_temp, TIMER5_IRQ, NULL, 0);

	RTIM_CCStructInit(&TIM_CCInitStruct);
	RTIM_CCxInit(TIM5, &TIM_CCInitStruct, pwm_chan);
	RTIM_CCRxSet(TIM5, PWM_PERIOD / 2 + 1, pwm_chan);
	RTIM_CCxCmd(TIM5, pwm_chan, TIM_CCx_Enable);

	Pinmux_Config(_PA_23, PINMUX_FUNCTION_PWM);
	RTIM_Cmd(TIM5, ENABLE);
}

void tim4_capture_ISR(u32 data)
{	
	u32 value1,value2;
	u32 value = TIM4->CCMRx[0] & 0xFFFF;

	value1 = XTAL_ClkGet()/(PWM_PERIOD+1);
	value2 = XTAL_ClkGet()/(PWM_PERIOD+1)+1;
	DBG_8195A("Pulse number: %d, %s\n", value, (value == value1|| value == value2 ? "success" : "fail");
	RTIM_INTClear(TIMx[4]);
}

void tim4_capture_pulse_num()
{
	RTIM_TimeBaseInitTypeDef  TIM_InitStruct_temp;
	TIM_CCInitTypeDef TIM_CCInitStruct;
		
	RTIM_TimeBaseStructInit(&TIM_InitStruct_temp);
	TIM_InitStruct_temp.TIM_Idx = 4;
	TIM_InitStruct_temp.TIM_Prescaler = ((XTAL_ClkGet())/200000)-1;
	TIM_InitStruct_temp.TIM_Period = 19999; //interrupt every 100ms
	RTIM_TimeBaseInit(TIM4, &TIM_InitStruct_temp, TIMx_irq[4], (IRQ_FUN) tim4_capture_ISR, 0);

	RTIM_CCStructInit(&TIM_CCInitStruct);
	TIM_CCInitStruct.TIM_ICPulseMode = TIM_CCMode_PulseNumber;
	RTIM_CCxInit(TIM4, &TIM_CCInitStruct, 0);

	RTIM_INTConfig(TIM4, TIM_IT_CC0, ENABLE);
	
	RTIM_CCxCmd(TIM4, 0, TIM_CCx_Enable);
	RTIM_Cmd(TIM4, ENABLE);

	Pinmux_Config(_PA_18, PINMUX_FUNCTION_TIMINPUT);
	PAD_PullCtrl(_PA_18, GPIO_PuPd_UP);

}

void tim4_capture_pulse_number_32k(void)
{
	/* generate 32760Hz pwm wave*/
	tim5_gen_pwm_32k();

	/* capture pwm pulse width*/
	tim4_capture_pulse_num();

	vTaskDelete(NULL);
}

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
void main(void)
{
	if(xTaskCreate( (TaskFunction_t)tim4_capture_pulse_number_32k, "TIM4 CAPTURE PULSE NUM DEMO", (2048/4), NULL, (tskIDLE_PRIORITY + 1), NULL)!= pdPASS) {
			DBG_8195A("Cannot create tim4 capture pulse num demo task\n\r");
	}

	vTaskStartScheduler();

	while(1);
}


