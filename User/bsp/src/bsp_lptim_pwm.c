/*
*********************************************************************************************************
*
*	ģ������ : �͹��Ķ�ʱ������
*	�ļ����� : bsp_lptim_pwm.c
*	��    �� : V1.0
*	˵    �� : STM32H7�͹��Ķ�ʱ��LPTIM��ʱģʽ��ͣ������
*	�޸ļ�¼ :
*		�汾��  ����        ����     ˵��
*		V1.0    2018-12-12 armfly  ��ʽ����
*
*	Copyright (C), 2018-2030, ���������� www.armfly.com
*
*********************************************************************************************************
*/

#include "bsp.h"


/*
	LPTIM������������õ�GPIO:
	LPTIM1_IN1   PD12   PG12
	LPTIM1_IN2   PH2    PE1
	LPTIM1_OUT   PG13
	LPTIM1_OUT   PD13
	LPTIM1_ETR   PG14   PE0

	LPTIM2_IN1   PB10  PD12
	LPTIM2_IN2   PD11
	LPTIM2_OUT   PB13
	LPTIM2_ETR   PB11  PE0

	LPTIM3_OUT   PA1
	LPTIM4_OUT   PA2
	LPTIM5_OUT   PA3
*/

/* ѡ��LPTIM��ʱ��Դ */
#define LPTIM_CLOCK_SOURCE_LSE     /* LSE ʱ��32768Hz */
//#define LPTIM_CLOCK_SOURCE_LSI   /* LSI ʱ��32768Hz */ 
//#define LPTIM_CLOCK_SOURCE_PCLK  /* PCLK ʱ��100MHz */ 

LPTIM_HandleTypeDef     LptimHandle = {0};

/*
*********************************************************************************************************
*	�� �� ��: bsp_InitLPTIM
*	����˵��: ��ʼ��LPTIM
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void bsp_InitLPTIM(void)
{
	RCC_PeriphCLKInitTypeDef   RCC_PeriphCLKInitStruct = {0};
	

	/* ## - 1 - ʹ��LPTIMʱ�Ӻ�GPIOʱ�� ####################################### */
	__HAL_RCC_LPTIM1_CLK_ENABLE();

	/* ## - 2 - ����LPTIMʱ�ӣ�����ѡ��LSE��LSI����PCLK ######################## */		
#if defined (LPTIM_CLOCK_SOURCE_LSE)
	{
		RCC_OscInitTypeDef RCC_OscInitStruct = {0};

		RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE;
		RCC_OscInitStruct.LSEState = RCC_LSE_ON;
		RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

		if (HAL_RCC_OscConfig(&RCC_OscInitStruct)!= HAL_OK)
		{
			Error_Handler(__FILE__, __LINE__);		
		}
		
		RCC_PeriphCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LPTIM1;
		RCC_PeriphCLKInitStruct.Lptim1ClockSelection = RCC_LPTIM1CLKSOURCE_LSE;
		HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct);
	}
#elif defined (LPTIM_CLOCK_SOURCE_LSI)
	{
		RCC_OscInitTypeDef RCC_OscInitStruct = {0};

		RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI;
		RCC_OscInitStruct.LSIState = RCC_LSI_ON;
		RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;

		if (HAL_RCC_OscConfig(&RCC_OscInitStruct)!= HAL_OK)
		{
			Error_Handler(__FILE__, __LINE__);		
		}
		
		RCC_PeriphCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LPTIM1;
		RCC_PeriphCLKInitStruct.Lptim1ClockSelection = RCC_LPTIM1CLKSOURCE_LSI;
		HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct);
	}
#elif defined (LPTIM_CLOCK_SOURCE_PCLK)
	 /*-----------------------------------------------------------------------
		bsp.c �ļ��� void SystemClock_Config(void) ������ʱ�ӵ���������: 

        System Clock source       = PLL (HSE)
        SYSCLK(Hz)                = 400000000 (CPU Clock)
        HCLK(Hz)                  = 200000000 (AXI and AHBs Clock)
        AHB Prescaler             = 2
        D1 APB3 Prescaler         = 2 (APB3 Clock  100MHz)
        D2 APB1 Prescaler         = 2 (APB1 Clock  100MHz)
        D2 APB2 Prescaler         = 2 (APB2 Clock  100MHz)
        D3 APB4 Prescaler         = 2 (APB4 Clock  100MHz)

        ��ΪAPB1 prescaler != 1, ���� APB1�ϵ�TIMxCLK = APB1 x 2 = 200MHz;
        ��ΪAPB2 prescaler != 1, ���� APB2�ϵ�TIMxCLK = APB2 x 2 = 200MHz;
        APB4�����TIMxCLKû�з�Ƶ�����Ծ���100MHz;

        APB1 ��ʱ���� TIM2, TIM3 ,TIM4, TIM5, TIM6, TIM7, TIM12, TIM13, TIM14��LPTIM1
        APB2 ��ʱ���� TIM1, TIM8 , TIM15, TIM16��TIM17

        APB4 ��ʱ���� LPTIM2��LPTIM3��LPTIM4��LPTIM5
	----------------------------------------------------------------------- */
	RCC_PeriphCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LPTIM1;
	RCC_PeriphCLKInitStruct.Lptim1ClockSelection = RCC_LPTIM1CLKSOURCE_LSE;
	HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct);
#else
	#error Please select the LPTIM Clock source inside the bsp_lptim_pwm.c file
#endif

	/* ## - 3 - ����LPTIM ######################################################## */		
	LptimHandle.Instance = LPTIM1;

	LptimHandle.Init.Clock.Source    = LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC; /* ��Ӧ�Ĵ���CKSEL��ѡ���ڲ�ʱ��Դ */
	LptimHandle.Init.Clock.Prescaler = LPTIM_PRESCALER_DIV8;        /* ����LPTIMʱ�ӷ�Ƶ */
	LptimHandle.Init.CounterSource   = LPTIM_COUNTERSOURCE_INTERNAL;/* LPTIM���������ڲ�ʱ��Դ���� */
	LptimHandle.Init.Trigger.Source  = LPTIM_TRIGSOURCE_SOFTWARE;   /* ������� */ 
	LptimHandle.Init.OutputPolarity  = LPTIM_OUTPUTPOLARITY_HIGH;   /* ��ʱģʽ�ò���������� */
	LptimHandle.Init.UpdateMode      = LPTIM_UPDATE_IMMEDIATE;      /* �ȽϼĴ�����ARR�Զ����ؼĴ���ѡ����ĺ��������� */ 
	LptimHandle.Init.Input1Source    = LPTIM_INPUT1SOURCE_GPIO;     /* �ⲿ����1��������δʹ�� */
	LptimHandle.Init.Input2Source    = LPTIM_INPUT2SOURCE_GPIO;     /* �ⲿ����2��������δʹ�� */

	if (HAL_LPTIM_Init(&LptimHandle) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}

	/* ## - 4 - ����LPTIM ######################################################## */		
	/* �����ж����ȼ���ʹ���ж� */
	HAL_NVIC_SetPriority(LPTIM1_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(LPTIM1_IRQn);
}

/*
*********************************************************************************************************
*	�� �� ��: bsp_StartLPTIM
*	����˵��: ����LPTIM
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void bsp_StartLPTIM(void)
{
	/*
	   ARR���Զ���װ�Ĵ�������Ӧ����HAL_LPTIM_TimeOut_Start_IT�ĵ�2������
	   Compare�ǱȽϼĴ�������Ӧ����HAL_LPTIM_TimeOut_Start_IT�ĵ�3������

	   ---------------------
	   LSE = 32768Hz
	   ��Ƶ����ΪLPTIM_PRESCALER_DIV8����8��Ƶ������bsp_InitLPTIM�������ĳ�ʼ�����ã�
	   ARR�Զ����ؼĴ��� = 32768
	   ʵ�ʲ��Է�������ж���ARR�Ĵ����޹أ�ȫ���ɵ�3��������Compare�Ĵ�������
	
	   LPTIM�ļ���������1�ε�ʱ���� 1 / (32768 / 8) = 8 /32768��
	   �������������õ���32767����ô������32767���� (32767 + 1)*��8 /32768�� = 8�룬�����ʱ��Ҫ��1��
	*/
	if (HAL_LPTIM_TimeOut_Start_IT(&LptimHandle, 0, 32767) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}
}

/*
*********************************************************************************************************
*	�� �� ��: LPTIM1_IRQHandler
*	����˵��: LPTIM1�жϷ������
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void LPTIM1_IRQHandler(void)
{
	if((LPTIM1->ISR & LPTIM_FLAG_CMPM) != RESET)
	{
		/* ����Ƚ�ƥ���ж� */
		LPTIM1->ICR = LPTIM_FLAG_CMPM;
		
		/* �ر�����ж� */
		HAL_LPTIM_TimeOut_Stop_IT(&LptimHandle);
		
		bsp_LedToggle(4);
	}
}

/***************************** ���������� www.armfly.com (END OF FILE) *********************************/
