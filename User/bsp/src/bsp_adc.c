/*
*********************************************************************************************************
*
*	ģ������ : ADC����
*	�ļ����� : bsp_adc.c
*	��    �� : V1.0
*	˵    �� : ADC��ͨ������
*              1. ����Ĭ���õ�PLLʱ�ӹ�ADCʹ�ã���ҿ���ͨ��bsp_adc.c�ļ���ͷ�궨���л���AHBʱ�ӡ�
*              2������DMA��ʽ���ж�ͨ���������ɼ���PC0, Vbat/4, VrefInt���¶ȡ�
*
*	�޸ļ�¼ :
*		�汾��  ����        ����     ˵��
*		V1.0    2018-12-12 armfly  ��ʽ����
*
*	Copyright (C), 2018-2030, ���������� www.armfly.com
*
*********************************************************************************************************
*/
#include "bsp.h"



/* ѡ��ADC��ʱ��Դ */
//#define ADC_CLOCK_SOURCE_AHB     /* ѡ��AHBʱ��Դ */
#define ADC_CLOCK_SOURCE_PLL     /* ѡ��PLLʱ��Դ */

/* ����Cache���API��������32�ֽڶ��� */
#if defined ( __ICCARM__ )
#pragma location = 0x38000000
uint16_t ADCxValues[4];
#elif defined ( __CC_ARM )
ALIGN_32BYTES(__attribute__((section (".RAM_D3"))) uint16_t ADCxValues[4]);
#endif


/*
*********************************************************************************************************
*	�� �� ��: bsp_InitADC
*	����˵��: ��ʼ��ADC������DMA��ʽ���ж�ͨ���������ɼ���PC0, Vbat/4, VrefInt���¶�
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void bsp_InitADC(void)
{
	ADC_HandleTypeDef   AdcHandle = {0};
	DMA_HandleTypeDef   DMA_Handle = {0};
	ADC_ChannelConfTypeDef   sConfig = {0};
	GPIO_InitTypeDef          GPIO_InitStruct;

  /* ## - 1 - ����ADC������ʱ�� ####################################### */
#if defined (ADC_CLOCK_SOURCE_PLL)
	/* ����PLL2ʱ��Ϊ��72MHz�������Ƶ����ADC���ʱ��36MHz */
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
  
  /* ʹ��AHBʱ�ӵĻ����������ã�Ĭ��ѡ��*/
  
#endif

	/* ## - 2 - ����ADC����ʹ�õ�ʱ�� ####################################### */
	__HAL_RCC_GPIOC_CLK_ENABLE();

	GPIO_InitStruct.Pin = GPIO_PIN_0;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  
	/* ## - 3 - ����ADC����ʹ�õ�ʱ�� ####################################### */
	__HAL_RCC_DMA1_CLK_ENABLE();
	DMA_Handle.Instance                 = DMA1_Stream1;            /* ʹ�õ�DMA1 Stream1 */
	DMA_Handle.Init.Request             = DMA_REQUEST_ADC3;  	   /* �������Ͳ���DMA_REQUEST_ADC3 */  
	DMA_Handle.Init.Direction           = DMA_PERIPH_TO_MEMORY;    /* ���䷽���ǴӴ洢�������� */  
	DMA_Handle.Init.PeriphInc           = DMA_PINC_DISABLE;        /* �����ַ������ֹ */ 
	DMA_Handle.Init.MemInc              = DMA_MINC_ENABLE;         /* �洢����ַ����ʹ�� */  
	DMA_Handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;   /* �������ݴ���λ��ѡ����֣���16bit */     
	DMA_Handle.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;   /* �洢�����ݴ���λ��ѡ����֣���16bit */    
	DMA_Handle.Init.Mode                = DMA_CIRCULAR;            /* ѭ��ģʽ */   
	DMA_Handle.Init.Priority            = DMA_PRIORITY_LOW;        /* ���ȼ��� */  
	DMA_Handle.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;    /* ��ֹFIFO*/
	DMA_Handle.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL; /* ��ֹFIFO��λ�������ã��������÷�ֵ */
	DMA_Handle.Init.MemBurst            = DMA_MBURST_SINGLE;       /* ��ֹFIFO��λ�������ã����ڴ洢��ͻ�� */
	DMA_Handle.Init.PeriphBurst         = DMA_PBURST_SINGLE;       /* ��ֹFIFO��λ�������ã���������ͻ�� */

	/* ��ʼ��DMA */
	if(HAL_DMA_Init(&DMA_Handle) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);     
	}
    
	/* ����ADC�����DMA��� */
	__HAL_LINKDMA(&AdcHandle, DMA_Handle, DMA_Handle);
	
    
	/* ## - 4 - ����ADC ########################################################### */
	__HAL_RCC_ADC3_CLK_ENABLE();
	AdcHandle.Instance = ADC3;

#if defined (ADC_CLOCK_SOURCE_PLL)
	AdcHandle.Init.ClockPrescaler        = ADC_CLOCK_ASYNC_DIV8;          /* ����PLL�첽ʱ�ӣ�8��Ƶ����72MHz/8 = 36MHz */
#elif defined (ADC_CLOCK_SOURCE_AHB)
	AdcHandle.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;      /* ����AHBͬ��ʱ�ӣ�4��Ƶ����200MHz/4 = 50MHz */
#endif
	
	AdcHandle.Init.Resolution            = ADC_RESOLUTION_16B;        /* 16λ�ֱ��� */
	AdcHandle.Init.ScanConvMode          = ADC_SCAN_ENABLE;           /* ��ֹɨ�裬��Ϊ������һ��ͨ�� */
	AdcHandle.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;       /* EOCת��������־ */
	AdcHandle.Init.LowPowerAutoWait      = DISABLE;                   /* ��ֹ�͹����Զ��ӳ����� */
	AdcHandle.Init.ContinuousConvMode    = ENABLE;                    /* ��ֹ�Զ�ת�������õ�������� */
	AdcHandle.Init.NbrOfConversion       = 4;                         /* ʹ����4��ת��ͨ�� */
	AdcHandle.Init.DiscontinuousConvMode = DISABLE;                   /* ��ֹ������ģʽ */
	AdcHandle.Init.NbrOfDiscConversion   = 1;                         /* ��ֹ������ģʽ�󣬴˲������ԣ���λ���������ò�����������ͨ���� */
	AdcHandle.Init.ExternalTrigConv      = ADC_SOFTWARE_START;        /* ����������� */
	AdcHandle.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_RISING;    /* ������������Ļ�����λ���� */
	AdcHandle.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR; /* DMAѭ��ģʽ����ADCת�������� */
	AdcHandle.Init.BoostMode             = DISABLE;                            /* ADCʱ�ӵ���20MHz�Ļ������Խ�ֹboost */
	AdcHandle.Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;     	   /* ADCת������Ļ�������ADC�����ݼĴ��� */
	AdcHandle.Init.OversamplingMode      = DISABLE;                            /* ��ֹ������ */

    /* ��ʼ��ADC */
	if (HAL_ADC_Init(&AdcHandle) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}
  
  
	/* У׼ADC������ƫ��У׼ */
	if (HAL_ADCEx_Calibration_Start(&AdcHandle, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}
  
	/* ����ADCͨ��������1������PC0���� */
	/*
		����PLL2ʱ�ӵĻ���ADCCLK = 72MHz / 8 = 9MHz
	    ADC�����ٶȣ���ת��ʱ�� = ����ʱ�� + ��αƽ�ʱ��
	                            = 810.5 + 8.5(16bit)
	                            = 820��ADCʱ������
	    ��ôת���ٶȾ���9MHz / 820 = 10975Hz
	*/
	sConfig.Channel      = ADC_CHANNEL_10;              /* ����ʹ�õ�ADCͨ�� */
	sConfig.Rank         = ADC_REGULAR_RANK_1;          /* ����������ĵ�1�� */
	sConfig.SamplingTime = ADC_SAMPLETIME_810CYCLES_5;  /* �������� */
	sConfig.SingleDiff   = ADC_SINGLE_ENDED;            /* �������� */
	sConfig.OffsetNumber = ADC_OFFSET_NONE;             /* ��ƫ�� */ 
	sConfig.Offset = 0;                                 /* ��ƫ�Ƶ�����£��˲������� */
	sConfig.OffsetRightShift       = DISABLE;           /* ��ֹ���� */
	sConfig.OffsetSignedSaturation = DISABLE;           /* ��ֹ�з��ű��� */
	
	if (HAL_ADC_ConfigChannel(&AdcHandle, &sConfig) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}
	
	/* ����ADCͨ��������2������Vbat/4 */
	sConfig.Channel      = ADC_CHANNEL_VBAT_DIV4;       /* ����ʹ�õ�ADCͨ�� */
	sConfig.Rank         = ADC_REGULAR_RANK_2;          /* ����������ĵ�1�� */
	sConfig.SamplingTime = ADC_SAMPLETIME_810CYCLES_5;  /* �������� */
	sConfig.SingleDiff   = ADC_SINGLE_ENDED;            /* �������� */
	sConfig.OffsetNumber = ADC_OFFSET_NONE;             /* ��ƫ�� */ 
	sConfig.Offset = 0;                                 /* ��ƫ�Ƶ�����£��˲������� */
	sConfig.OffsetRightShift       = DISABLE;           /* ��ֹ���� */
	sConfig.OffsetSignedSaturation = DISABLE;           /* ��ֹ�з��ű��� */
	
	if (HAL_ADC_ConfigChannel(&AdcHandle, &sConfig) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}
  
	/* ����ADCͨ��������3������VrefInt */
	sConfig.Channel      = ADC_CHANNEL_VREFINT;         /* ����ʹ�õ�ADCͨ�� */
	sConfig.Rank         = ADC_REGULAR_RANK_3;          /* ����������ĵ�1�� */
	sConfig.SamplingTime = ADC_SAMPLETIME_810CYCLES_5;  /* �������� */
	sConfig.SingleDiff   = ADC_SINGLE_ENDED;            /* �������� */
	sConfig.OffsetNumber = ADC_OFFSET_NONE;             /* ��ƫ�� */ 
	sConfig.Offset = 0;                                 /* ��ƫ�Ƶ�����£��˲������� */
	sConfig.OffsetRightShift       = DISABLE;           /* ��ֹ���� */
	sConfig.OffsetSignedSaturation = DISABLE;           /* ��ֹ�з��ű��� */
	
	if (HAL_ADC_ConfigChannel(&AdcHandle, &sConfig) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}

	/* ����ADCͨ��������4�������¶� */
	sConfig.Channel      = ADC_CHANNEL_TEMPSENSOR;      /* ����ʹ�õ�ADCͨ�� */
	sConfig.Rank         = ADC_REGULAR_RANK_4;          /* ����������ĵ�1�� */
	sConfig.SamplingTime = ADC_SAMPLETIME_810CYCLES_5;  /* �������� */
	sConfig.SingleDiff   = ADC_SINGLE_ENDED;            /* �������� */
	sConfig.OffsetNumber = ADC_OFFSET_NONE;             /* ��ƫ�� */ 
	sConfig.Offset = 0;                                 /* ��ƫ�Ƶ�����£��˲������� */
	sConfig.OffsetRightShift       = DISABLE;           /* ��ֹ���� */
	sConfig.OffsetSignedSaturation = DISABLE;           /* ��ֹ�з��ű��� */
	
	if (HAL_ADC_ConfigChannel(&AdcHandle, &sConfig) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}	
  

	/* ## - 6 - ����ADC��DMA��ʽ���� ####################################### */
	if (HAL_ADC_Start_DMA(&AdcHandle, (uint32_t *)ADCxValues, 4) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}
}

/*
*********************************************************************************************************
*	�� �� ��: bsp_GetAdcValues
*	����˵��: ��ȡADC�����ݲ���ӡ
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void bsp_GetAdcValues(void)
{
    float AdcValues[5];
    uint16_t TS_CAL1;
    uint16_t TS_CAL2;
    
    /*
       ʹ�ô˺���Ҫ�ر�ע�⣬��1��������ַҪ32�ֽڶ��룬��2������Ҫ��32�ֽڵ�������
    */
    SCB_InvalidateDCache_by_Addr((uint32_t *)ADCxValues,  sizeof(ADCxValues));
    AdcValues[0] = ADCxValues[0] * 3.3 / 65536;
    AdcValues[1] = ADCxValues[1] * 3.3 / 65536; 
    AdcValues[2] = ADCxValues[2] * 3.3 / 65536;     
 
    /* ���ݲο��ֲ���Ĺ�ʽ�����¶�ֵ */
    TS_CAL1 = *(__IO uint16_t *)(0x1FF1E820);
    TS_CAL2 = *(__IO uint16_t *)(0x1FF1E840);
    
    AdcValues[3] = (110.0 - 30.0) * (ADCxValues[3] - TS_CAL1)/ (TS_CAL2 - TS_CAL1) + 30;  
    
    printf("PC0 = %5.3fV, Vbat/4 = %5.3fV, VrefInt = %5.3fV�� TempSensor = %5.3f��\r\n", 
            AdcValues[0],  AdcValues[1], AdcValues[2], AdcValues[3]);

}

/***************************** ���������� www.armfly.com (END OF FILE) *********************************/
