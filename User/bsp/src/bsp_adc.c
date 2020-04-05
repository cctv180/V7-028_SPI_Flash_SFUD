/*
*********************************************************************************************************
*
*	模块名称 : ADC驱动
*	文件名称 : bsp_adc.c
*	版    本 : V1.0
*	说    明 : ADC多通道采样
*              1. 例子默认用的PLL时钟供ADC使用，大家可以通过bsp_adc.c文件开头宏定义切换到AHB时钟。
*              2、采用DMA方式进行多通道采样，采集了PC0, Vbat/4, VrefInt和温度。
*
*	修改记录 :
*		版本号  日期        作者     说明
*		V1.0    2018-12-12 armfly  正式发布
*
*	Copyright (C), 2018-2030, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/
#include "bsp.h"



/* 选择ADC的时钟源 */
//#define ADC_CLOCK_SOURCE_AHB     /* 选择AHB时钟源 */
#define ADC_CLOCK_SOURCE_PLL     /* 选择PLL时钟源 */

/* 方便Cache类的API操作，做32字节对齐 */
#if defined ( __ICCARM__ )
#pragma location = 0x38000000
uint16_t ADCxValues[4];
#elif defined ( __CC_ARM )
ALIGN_32BYTES(__attribute__((section (".RAM_D3"))) uint16_t ADCxValues[4]);
#endif


/*
*********************************************************************************************************
*	函 数 名: bsp_InitADC
*	功能说明: 初始化ADC，采用DMA方式进行多通道采样，采集了PC0, Vbat/4, VrefInt和温度
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_InitADC(void)
{
	ADC_HandleTypeDef   AdcHandle = {0};
	DMA_HandleTypeDef   DMA_Handle = {0};
	ADC_ChannelConfTypeDef   sConfig = {0};
	GPIO_InitTypeDef          GPIO_InitStruct;

  /* ## - 1 - 配置ADC采样的时钟 ####################################### */
#if defined (ADC_CLOCK_SOURCE_PLL)
	/* 配置PLL2时钟为的72MHz，方便分频产生ADC最高时钟36MHz */
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_ADC;
	PeriphClkInitStruct.PLL2.PLL2M = 25;
	PeriphClkInitStruct.PLL2.PLL2N = 504;
	PeriphClkInitStruct.PLL2.PLL2P = 7;
	PeriphClkInitStruct.PLL2.PLL2Q = 7;
	PeriphClkInitStruct.PLL2.PLL2R = 7;
	PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_0;
	PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
	PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
	PeriphClkInitStruct.AdcClockSelection = RCC_ADCCLKSOURCE_PLL2;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);  
	}
#elif defined (ADC_CLOCK_SOURCE_AHB)
  
  /* 使用AHB时钟的话，无需配置，默认选择*/
  
#endif

	/* ## - 2 - 配置ADC采样使用的时钟 ####################################### */
	__HAL_RCC_GPIOC_CLK_ENABLE();

	GPIO_InitStruct.Pin = GPIO_PIN_0;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  
	/* ## - 3 - 配置ADC采样使用的时钟 ####################################### */
	__HAL_RCC_DMA1_CLK_ENABLE();
	DMA_Handle.Instance                 = DMA1_Stream1;            /* 使用的DMA1 Stream1 */
	DMA_Handle.Init.Request             = DMA_REQUEST_ADC3;  	   /* 请求类型采用DMA_REQUEST_ADC3 */  
	DMA_Handle.Init.Direction           = DMA_PERIPH_TO_MEMORY;    /* 传输方向是从存储器到外设 */  
	DMA_Handle.Init.PeriphInc           = DMA_PINC_DISABLE;        /* 外设地址自增禁止 */ 
	DMA_Handle.Init.MemInc              = DMA_MINC_ENABLE;         /* 存储器地址自增使能 */  
	DMA_Handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;   /* 外设数据传输位宽选择半字，即16bit */     
	DMA_Handle.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;   /* 存储器数据传输位宽选择半字，即16bit */    
	DMA_Handle.Init.Mode                = DMA_CIRCULAR;            /* 循环模式 */   
	DMA_Handle.Init.Priority            = DMA_PRIORITY_LOW;        /* 优先级低 */  
	DMA_Handle.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;    /* 禁止FIFO*/
	DMA_Handle.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL; /* 禁止FIFO此位不起作用，用于设置阀值 */
	DMA_Handle.Init.MemBurst            = DMA_MBURST_SINGLE;       /* 禁止FIFO此位不起作用，用于存储器突发 */
	DMA_Handle.Init.PeriphBurst         = DMA_PBURST_SINGLE;       /* 禁止FIFO此位不起作用，用于外设突发 */

	/* 初始化DMA */
	if(HAL_DMA_Init(&DMA_Handle) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);     
	}
    
	/* 关联ADC句柄和DMA句柄 */
	__HAL_LINKDMA(&AdcHandle, DMA_Handle, DMA_Handle);
	
    
	/* ## - 4 - 配置ADC ########################################################### */
	__HAL_RCC_ADC3_CLK_ENABLE();
	AdcHandle.Instance = ADC3;

#if defined (ADC_CLOCK_SOURCE_PLL)
	AdcHandle.Init.ClockPrescaler        = ADC_CLOCK_ASYNC_DIV8;          /* 采用PLL异步时钟，8分频，即72MHz/8 = 36MHz */
#elif defined (ADC_CLOCK_SOURCE_AHB)
	AdcHandle.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;      /* 采用AHB同步时钟，4分频，即200MHz/4 = 50MHz */
#endif
	
	AdcHandle.Init.Resolution            = ADC_RESOLUTION_16B;        /* 16位分辨率 */
	AdcHandle.Init.ScanConvMode          = ADC_SCAN_ENABLE;           /* 禁止扫描，因为仅开了一个通道 */
	AdcHandle.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;       /* EOC转换结束标志 */
	AdcHandle.Init.LowPowerAutoWait      = DISABLE;                   /* 禁止低功耗自动延迟特性 */
	AdcHandle.Init.ContinuousConvMode    = ENABLE;                    /* 禁止自动转换，采用的软件触发 */
	AdcHandle.Init.NbrOfConversion       = 4;                         /* 使用了4个转换通道 */
	AdcHandle.Init.DiscontinuousConvMode = DISABLE;                   /* 禁止不连续模式 */
	AdcHandle.Init.NbrOfDiscConversion   = 1;                         /* 禁止不连续模式后，此参数忽略，此位是用来配置不连续子组中通道数 */
	AdcHandle.Init.ExternalTrigConv      = ADC_SOFTWARE_START;        /* 采用软件触发 */
	AdcHandle.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_RISING;    /* 采用软件触发的话，此位忽略 */
	AdcHandle.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR; /* DMA循环模式接收ADC转换的数据 */
	AdcHandle.Init.BoostMode             = DISABLE;                            /* ADC时钟低于20MHz的话，可以禁止boost */
	AdcHandle.Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;     	   /* ADC转换溢出的话，覆盖ADC的数据寄存器 */
	AdcHandle.Init.OversamplingMode      = DISABLE;                            /* 禁止过采样 */

    /* 初始化ADC */
	if (HAL_ADC_Init(&AdcHandle) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}
  
  
	/* 校准ADC，采用偏移校准 */
	if (HAL_ADCEx_Calibration_Start(&AdcHandle, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}
  
	/* 配置ADC通道，序列1，采样PC0引脚 */
	/*
		采用PLL2时钟的话，ADCCLK = 72MHz / 8 = 9MHz
	    ADC采样速度，即转换时间 = 采样时间 + 逐次逼近时间
	                            = 810.5 + 8.5(16bit)
	                            = 820个ADC时钟周期
	    那么转换速度就是9MHz / 820 = 10975Hz
	*/
	sConfig.Channel      = ADC_CHANNEL_10;              /* 配置使用的ADC通道 */
	sConfig.Rank         = ADC_REGULAR_RANK_1;          /* 采样序列里的第1个 */
	sConfig.SamplingTime = ADC_SAMPLETIME_810CYCLES_5;  /* 采样周期 */
	sConfig.SingleDiff   = ADC_SINGLE_ENDED;            /* 单端输入 */
	sConfig.OffsetNumber = ADC_OFFSET_NONE;             /* 无偏移 */ 
	sConfig.Offset = 0;                                 /* 无偏移的情况下，此参数忽略 */
	sConfig.OffsetRightShift       = DISABLE;           /* 禁止右移 */
	sConfig.OffsetSignedSaturation = DISABLE;           /* 禁止有符号饱和 */
	
	if (HAL_ADC_ConfigChannel(&AdcHandle, &sConfig) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}
	
	/* 配置ADC通道，序列2，采样Vbat/4 */
	sConfig.Channel      = ADC_CHANNEL_VBAT_DIV4;       /* 配置使用的ADC通道 */
	sConfig.Rank         = ADC_REGULAR_RANK_2;          /* 采样序列里的第1个 */
	sConfig.SamplingTime = ADC_SAMPLETIME_810CYCLES_5;  /* 采样周期 */
	sConfig.SingleDiff   = ADC_SINGLE_ENDED;            /* 单端输入 */
	sConfig.OffsetNumber = ADC_OFFSET_NONE;             /* 无偏移 */ 
	sConfig.Offset = 0;                                 /* 无偏移的情况下，此参数忽略 */
	sConfig.OffsetRightShift       = DISABLE;           /* 禁止右移 */
	sConfig.OffsetSignedSaturation = DISABLE;           /* 禁止有符号饱和 */
	
	if (HAL_ADC_ConfigChannel(&AdcHandle, &sConfig) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}
  
	/* 配置ADC通道，序列3，采样VrefInt */
	sConfig.Channel      = ADC_CHANNEL_VREFINT;         /* 配置使用的ADC通道 */
	sConfig.Rank         = ADC_REGULAR_RANK_3;          /* 采样序列里的第1个 */
	sConfig.SamplingTime = ADC_SAMPLETIME_810CYCLES_5;  /* 采样周期 */
	sConfig.SingleDiff   = ADC_SINGLE_ENDED;            /* 单端输入 */
	sConfig.OffsetNumber = ADC_OFFSET_NONE;             /* 无偏移 */ 
	sConfig.Offset = 0;                                 /* 无偏移的情况下，此参数忽略 */
	sConfig.OffsetRightShift       = DISABLE;           /* 禁止右移 */
	sConfig.OffsetSignedSaturation = DISABLE;           /* 禁止有符号饱和 */
	
	if (HAL_ADC_ConfigChannel(&AdcHandle, &sConfig) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}

	/* 配置ADC通道，序列4，采样温度 */
	sConfig.Channel      = ADC_CHANNEL_TEMPSENSOR;      /* 配置使用的ADC通道 */
	sConfig.Rank         = ADC_REGULAR_RANK_4;          /* 采样序列里的第1个 */
	sConfig.SamplingTime = ADC_SAMPLETIME_810CYCLES_5;  /* 采样周期 */
	sConfig.SingleDiff   = ADC_SINGLE_ENDED;            /* 单端输入 */
	sConfig.OffsetNumber = ADC_OFFSET_NONE;             /* 无偏移 */ 
	sConfig.Offset = 0;                                 /* 无偏移的情况下，此参数忽略 */
	sConfig.OffsetRightShift       = DISABLE;           /* 禁止右移 */
	sConfig.OffsetSignedSaturation = DISABLE;           /* 禁止有符号饱和 */
	
	if (HAL_ADC_ConfigChannel(&AdcHandle, &sConfig) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}	
  

	/* ## - 6 - 启动ADC的DMA方式传输 ####################################### */
	if (HAL_ADC_Start_DMA(&AdcHandle, (uint32_t *)ADCxValues, 4) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}
}

/*
*********************************************************************************************************
*	函 数 名: bsp_GetAdcValues
*	功能说明: 获取ADC的数据并打印
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_GetAdcValues(void)
{
    float AdcValues[5];
    uint16_t TS_CAL1;
    uint16_t TS_CAL2;
    
    /*
       使用此函数要特别注意，第1个参数地址要32字节对齐，第2个参数要是32字节的整数倍
    */
    SCB_InvalidateDCache_by_Addr((uint32_t *)ADCxValues,  sizeof(ADCxValues));
    AdcValues[0] = ADCxValues[0] * 3.3 / 65536;
    AdcValues[1] = ADCxValues[1] * 3.3 / 65536; 
    AdcValues[2] = ADCxValues[2] * 3.3 / 65536;     
 
    /* 根据参考手册给的公式计算温度值 */
    TS_CAL1 = *(__IO uint16_t *)(0x1FF1E820);
    TS_CAL2 = *(__IO uint16_t *)(0x1FF1E840);
    
    AdcValues[3] = (110.0 - 30.0) * (ADCxValues[3] - TS_CAL1)/ (TS_CAL2 - TS_CAL1) + 30;  
    
    printf("PC0 = %5.3fV, Vbat/4 = %5.3fV, VrefInt = %5.3fV， TempSensor = %5.3f℃\r\n", 
            AdcValues[0],  AdcValues[1], AdcValues[2], AdcValues[3]);

}

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
