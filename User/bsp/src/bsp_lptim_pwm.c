/*
*********************************************************************************************************
*
*	模块名称 : 低功耗定时器驱动
*	文件名称 : bsp_lptim_pwm.c
*	版    本 : V1.0
*	说    明 : STM32H7低功耗定时器LPTIM超时模式的停机唤醒
*	修改记录 :
*		版本号  日期        作者     说明
*		V1.0    2018-12-12 armfly  正式发布
*
*	Copyright (C), 2018-2030, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/

#include "bsp.h"


/*
	LPTIM输入输出所复用的GPIO:
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

/* 选择LPTIM的时钟源 */
#define LPTIM_CLOCK_SOURCE_LSE     /* LSE 时钟32768Hz */
//#define LPTIM_CLOCK_SOURCE_LSI   /* LSI 时钟32768Hz */ 
//#define LPTIM_CLOCK_SOURCE_PCLK  /* PCLK 时钟100MHz */ 

LPTIM_HandleTypeDef     LptimHandle = {0};

/*
*********************************************************************************************************
*	函 数 名: bsp_InitLPTIM
*	功能说明: 初始化LPTIM
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_InitLPTIM(void)
{
	RCC_PeriphCLKInitTypeDef   RCC_PeriphCLKInitStruct = {0};
	

	/* ## - 1 - 使能LPTIM时钟和GPIO时钟 ####################################### */
	__HAL_RCC_LPTIM1_CLK_ENABLE();

	/* ## - 2 - 配置LPTIM时钟，可以选择LSE，LSI或者PCLK ######################## */		
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
		bsp.c 文件中 void SystemClock_Config(void) 函数对时钟的配置如下: 

        System Clock source       = PLL (HSE)
        SYSCLK(Hz)                = 400000000 (CPU Clock)
        HCLK(Hz)                  = 200000000 (AXI and AHBs Clock)
        AHB Prescaler             = 2
        D1 APB3 Prescaler         = 2 (APB3 Clock  100MHz)
        D2 APB1 Prescaler         = 2 (APB1 Clock  100MHz)
        D2 APB2 Prescaler         = 2 (APB2 Clock  100MHz)
        D3 APB4 Prescaler         = 2 (APB4 Clock  100MHz)

        因为APB1 prescaler != 1, 所以 APB1上的TIMxCLK = APB1 x 2 = 200MHz;
        因为APB2 prescaler != 1, 所以 APB2上的TIMxCLK = APB2 x 2 = 200MHz;
        APB4上面的TIMxCLK没有分频，所以就是100MHz;

        APB1 定时器有 TIM2, TIM3 ,TIM4, TIM5, TIM6, TIM7, TIM12, TIM13, TIM14，LPTIM1
        APB2 定时器有 TIM1, TIM8 , TIM15, TIM16，TIM17

        APB4 定时器有 LPTIM2，LPTIM3，LPTIM4，LPTIM5
	----------------------------------------------------------------------- */
	RCC_PeriphCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LPTIM1;
	RCC_PeriphCLKInitStruct.Lptim1ClockSelection = RCC_LPTIM1CLKSOURCE_LSE;
	HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct);
#else
	#error Please select the LPTIM Clock source inside the bsp_lptim_pwm.c file
#endif

	/* ## - 3 - 配置LPTIM ######################################################## */		
	LptimHandle.Instance = LPTIM1;

	LptimHandle.Init.Clock.Source    = LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC; /* 对应寄存器CKSEL，选择内部时钟源 */
	LptimHandle.Init.Clock.Prescaler = LPTIM_PRESCALER_DIV8;        /* 设置LPTIM时钟分频 */
	LptimHandle.Init.CounterSource   = LPTIM_COUNTERSOURCE_INTERNAL;/* LPTIM计数器对内部时钟源计数 */
	LptimHandle.Init.Trigger.Source  = LPTIM_TRIGSOURCE_SOFTWARE;   /* 软件触发 */ 
	LptimHandle.Init.OutputPolarity  = LPTIM_OUTPUTPOLARITY_HIGH;   /* 超时模式用不到这个配置 */
	LptimHandle.Init.UpdateMode      = LPTIM_UPDATE_IMMEDIATE;      /* 比较寄存器和ARR自动重载寄存器选择更改后立即更新 */ 
	LptimHandle.Init.Input1Source    = LPTIM_INPUT1SOURCE_GPIO;     /* 外部输入1，本配置未使用 */
	LptimHandle.Init.Input2Source    = LPTIM_INPUT2SOURCE_GPIO;     /* 外部输入2，本配置未使用 */

	if (HAL_LPTIM_Init(&LptimHandle) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}

	/* ## - 4 - 配置LPTIM ######################################################## */		
	/* 配置中断优先级并使能中断 */
	HAL_NVIC_SetPriority(LPTIM1_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(LPTIM1_IRQn);
}

/*
*********************************************************************************************************
*	函 数 名: bsp_StartLPTIM
*	功能说明: 启动LPTIM
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_StartLPTIM(void)
{
	/*
	   ARR是自动重装寄存器，对应函数HAL_LPTIM_TimeOut_Start_IT的第2个参数
	   Compare是比较寄存器，对应函数HAL_LPTIM_TimeOut_Start_IT的第3个参数

	   ---------------------
	   LSE = 32768Hz
	   分频设置为LPTIM_PRESCALER_DIV8，即8分频（函数bsp_InitLPTIM里面做的初始化配置）
	   ARR自动重载寄存器 = 32768
	   实际测试发现溢出中断与ARR寄存器无关，全部由第3个参数，Compare寄存器决定
	
	   LPTIM的计数器计数1次的时间是 1 / (32768 / 8) = 8 /32768。
	   第三个参数配置的是32767，那么计数到32767就是 (32767 + 1)*（8 /32768） = 8秒，计算的时候要加1。
	*/
	if (HAL_LPTIM_TimeOut_Start_IT(&LptimHandle, 0, 32767) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}
}

/*
*********************************************************************************************************
*	函 数 名: LPTIM1_IRQHandler
*	功能说明: LPTIM1中断服务程序
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void LPTIM1_IRQHandler(void)
{
	if((LPTIM1->ISR & LPTIM_FLAG_CMPM) != RESET)
	{
		/* 清除比较匹配中断 */
		LPTIM1->ICR = LPTIM_FLAG_CMPM;
		
		/* 关闭溢出中断 */
		HAL_LPTIM_TimeOut_Stop_IT(&LptimHandle);
		
		bsp_LedToggle(4);
	}
}

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
