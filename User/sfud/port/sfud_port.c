/*
 * This file is part of the Serial Flash Universal Driver Library.
 *
 * Copyright (c) 2018, zylx, <qgyhd1234@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Portable interface for each platform.
 * Created on: 2018-11-23
 */
#include "bsp.h"

#include <sfud.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* 串行Flsh的片选GPIO端口， PD13  */
#define SF_CS_CLK_ENABLE() 			__HAL_RCC_GPIOD_CLK_ENABLE()
#define SF_CS_GPIO					GPIOD
#define SF_CS_PIN					GPIO_PIN_13
//#define SF_CS_0()					SF_CS_GPIO->BSRRH = SF_CS_PIN
//#define SF_CS_1()					SF_CS_GPIO->BSRRL = SF_CS_PIN

extern SPI_HandleTypeDef hspi;

typedef struct
{
	SPI_HandleTypeDef* spix;
	GPIO_TypeDef* cs_gpiox;
	uint16_t cs_gpio_pin;
} spi_user_data, * spi_user_data_t;

static spi_user_data spi1 = { .spix = &hspi, .cs_gpiox = SF_CS_GPIO, .cs_gpio_pin = SF_CS_PIN };

static char log_buf[256];
void sfud_log_info(const char* format, ...);
void sfud_log_debug(const char* file, const long line, const char* format, ...);

//static void spi_lock(const sfud_spi* spi)
//{
//    __disable_irq();
//}
//
//static void spi_unlock(const sfud_spi* spi)
//{
//    __enable_irq();
//}

/**
 * SPI write data then read data
 */
static sfud_err spi_write_read(const sfud_spi* spi, const uint8_t* write_buf, size_t write_size, uint8_t* read_buf,
    size_t read_size)
{
    sfud_err result = SFUD_SUCCESS;

    spi_user_data_t spi_dev = (spi_user_data_t)spi->user_data;

    if (write_size)
    {
        SFUD_ASSERT(write_buf);
    }
    if (read_size)
    {
        SFUD_ASSERT(read_buf);
    }

	bsp_SpiBusEnter();
	bsp_InitSPIParam(SPI_BAUDRATEPRESCALER_8, SPI_PHASE_1EDGE, SPI_POLARITY_LOW);
    /* reset cs pin */
    if (spi_dev->cs_gpiox != NULL)
        HAL_GPIO_WritePin(spi_dev->cs_gpiox, spi_dev->cs_gpio_pin, GPIO_PIN_RESET);

    if (write_size && read_size)
    {
        /* read data */
        //qspi_send_then_recv(write_buf, write_size, read_buf, read_size);
        HAL_SPI_Transmit(spi_dev->spix, (uint8_t*)write_buf, write_size, 1000); //SPI发送data中的数据
        HAL_SPI_Receive(spi_dev->spix, (uint8_t*)read_buf, read_size, 1000);    //SPI读取data中的数据
    }
    else if (write_size)
    {
        /* send data */
		//qspi_send_then_recv(write_buf, write_size, NULL, NULL);
		HAL_SPI_Transmit(spi_dev->spix, (uint8_t*)write_buf, write_size, 1000); //SPI发送data中的数据
    }

    /* set cs pin */
    if (spi_dev->cs_gpiox != NULL)
        HAL_GPIO_WritePin(spi_dev->cs_gpiox, spi_dev->cs_gpio_pin, GPIO_PIN_SET);

	bsp_SpiBusExit();

    return result;
}

/**
 * QSPI fast read data
 */
#ifdef SFUD_USING_QSPI
static sfud_err qspi_read(const struct __sfud_spi* spi, uint32_t addr, sfud_qspi_read_cmd_format* qspi_read_cmd_format, uint8_t* read_buf, size_t read_size)
{

    sfud_err result = SFUD_SUCCESS;
    QSPI_CommandTypeDef Cmdhandler;
    extern QSPI_HandleTypeDef hqspi;

    /* set cmd struct */
    Cmdhandler.Instruction = qspi_read_cmd_format->instruction;
    if (qspi_read_cmd_format->instruction_lines == 0)
    {
        Cmdhandler.InstructionMode = QSPI_INSTRUCTION_NONE;
    }
    else if (qspi_read_cmd_format->instruction_lines == 1)
    {
        Cmdhandler.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    }
    else if (qspi_read_cmd_format->instruction_lines == 2)
    {
        Cmdhandler.InstructionMode = QSPI_INSTRUCTION_2_LINES;
    }
    else if (qspi_read_cmd_format->instruction_lines == 4)
    {
        Cmdhandler.InstructionMode = QSPI_INSTRUCTION_4_LINES;
    }

    Cmdhandler.Address = addr;
    Cmdhandler.AddressSize = QSPI_ADDRESS_24_BITS;
    if (qspi_read_cmd_format->address_lines == 0)
    {
        Cmdhandler.AddressMode = QSPI_ADDRESS_NONE;
    }
    else if (qspi_read_cmd_format->address_lines == 1)
    {
        Cmdhandler.AddressMode = QSPI_ADDRESS_1_LINE;
    }
    else if (qspi_read_cmd_format->address_lines == 2)
    {
        Cmdhandler.AddressMode = QSPI_ADDRESS_2_LINES;
    }
    else if (qspi_read_cmd_format->address_lines == 4)
    {
        Cmdhandler.AddressMode = QSPI_ADDRESS_4_LINES;
    }

    Cmdhandler.AlternateBytes = 0;
    Cmdhandler.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    Cmdhandler.AlternateBytesSize = 0;

    Cmdhandler.DummyCycles = qspi_read_cmd_format->dummy_cycles;

    Cmdhandler.NbData = read_size;
    if (qspi_read_cmd_format->data_lines == 0)
    {
        Cmdhandler.DataMode = QSPI_DATA_NONE;
    }
    else if (qspi_read_cmd_format->data_lines == 1)
    {
        Cmdhandler.DataMode = QSPI_DATA_1_LINE;
    }
    else if (qspi_read_cmd_format->data_lines == 2)
    {
        Cmdhandler.DataMode = QSPI_DATA_2_LINES;
    }
    else if (qspi_read_cmd_format->data_lines == 4)
    {
        Cmdhandler.DataMode = QSPI_DATA_4_LINES;
    }

    Cmdhandler.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
    Cmdhandler.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    Cmdhandler.DdrMode = QSPI_DDR_MODE_DISABLE;
    Cmdhandler.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    HAL_QSPI_Command(&hqspi, &Cmdhandler, 5000);

    if (HAL_QSPI_Receive(&hqspi, read_buf, 5000) != HAL_OK)
    {
        sfud_log_info("qspi recv data failed(%d)!", hqspi.ErrorCode);
        hqspi.State = HAL_QSPI_STATE_READY;
        result = SFUD_ERR_READ;
    }

    return result;
}
#endif
/* about 100 microsecond delay */
static void retry_delay_100us(void)
{
    uint32_t delay = 2400;
    while (delay--);
}

sfud_err sfud_spi_port_init(sfud_flash* flash)
{
    sfud_err result = SFUD_SUCCESS;
	
	switch (flash->index)
	{
		case SFUD_W25_DEVICE_INDEX:
		{
			GPIO_InitTypeDef gpio_init;
			/* 打开GPIO时钟 */
			SF_CS_CLK_ENABLE();
			gpio_init.Mode = GPIO_MODE_OUTPUT_PP;		/* 设置推挽输出 */
			gpio_init.Pull = GPIO_NOPULL;				/* 上下拉电阻不使能 */
			gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;  	/* GPIO速度等级 */
			gpio_init.Pin = SF_CS_PIN;
			HAL_GPIO_Init(SF_CS_GPIO, &gpio_init);

			/* set the interfaces and data */
			flash->spi.wr = spi_write_read;
			//flash->spi.qspi_read = qspi_read;
			//flash->spi.lock = spi_lock;
			//flash->spi.unlock = spi_unlock;
			flash->spi.user_data = &spi1;
			/* about 100 microsecond delay */
			flash->retry.delay = retry_delay_100us;
			/* adout 60 seconds timeout */
			flash->retry.times = 60 * 10000;

			break;
		}
	}

    return result;
}

/**
 * This function is print debug info.
 *
 * @param file the file which has call this function
 * @param line the line number which has call this function
 * @param format output format
 * @param ... args
 */
void sfud_log_debug(const char* file, const long line, const char* format, ...)
{
    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);
    printf("[SFUD](%s:%ld) ", file, line);
    /* must use vprintf to print */
    vsnprintf(log_buf, sizeof(log_buf), format, args);
    printf("%s\r\n", log_buf);
    va_end(args);
}

/**
 * This function is print routine info.
 *
 * @param format output format
 * @param ... args
 */
void sfud_log_info(const char* format, ...)
{
    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);
    printf("[SFUD]");
    /* must use vprintf to print */
    vsnprintf(log_buf, sizeof(log_buf), format, args);
    printf("%s\r\n", log_buf);
    va_end(args);
}

/**
 * This function can send or send then receive QSPI data.
 */
#ifdef SFUD_USING_QSPI
sfud_err qspi_send_then_recv(const void* send_buf, size_t send_length, void* recv_buf, size_t recv_length)
{
    assert_param(send_buf);
    assert_param(recv_buf);
    assert_param(send_length != 0);

    QSPI_CommandTypeDef Cmdhandler;
    unsigned char* ptr = (unsigned char*)send_buf;
    size_t count = 0;
    sfud_err result = SFUD_SUCCESS;

    /* get instruction */
    Cmdhandler.Instruction = ptr[0];
    Cmdhandler.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    count++;

    /* get address */
    if (send_length > 1)
    {
        if (send_length >= 4)
        {
            /* address size is 3 Byte */
            Cmdhandler.Address = (ptr[1] << 16) | (ptr[2] << 8) | (ptr[3]);
            Cmdhandler.AddressSize = QSPI_ADDRESS_24_BITS;
            count += 3;
        }
        else
        {
            return SFUD_ERR_READ;
        }
        Cmdhandler.AddressMode = QSPI_ADDRESS_1_LINE;
    }
    else
    {
        /* no address stage */
        Cmdhandler.Address = 0;
        Cmdhandler.AddressMode = QSPI_ADDRESS_NONE;
        Cmdhandler.AddressSize = 0;
    }

    Cmdhandler.AlternateBytes = 0;
    Cmdhandler.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    Cmdhandler.AlternateBytesSize = 0;

    Cmdhandler.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;
    Cmdhandler.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    Cmdhandler.DdrMode = QSPI_DDR_MODE_DISABLE;
    Cmdhandler.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;

    if (send_buf && recv_buf)
    {
        /* recv data */
        /* set dummy cycles */
        if (count != send_length)
        {
            Cmdhandler.DummyCycles = (send_length - count) * 8;
        }
        else
        {
            Cmdhandler.DummyCycles = 0;
        }

        /* set recv size */
        Cmdhandler.DataMode = QSPI_DATA_1_LINE;
        Cmdhandler.NbData = recv_length;
        HAL_QSPI_Command(&hqspi, &Cmdhandler, 5000);

        if (recv_length != 0)
        {
            if (HAL_QSPI_Receive(&hqspi, recv_buf, 5000) != HAL_OK)
            {
                sfud_log_info("qspi recv data failed(%d)!", hqspi.ErrorCode);
                hqspi.State = HAL_QSPI_STATE_READY;
                result = SFUD_ERR_READ;
            }
        }

        return result;
    }
    else
    {
        /* send data */
        /* set dummy cycles */
        Cmdhandler.DummyCycles = 0;

        /* determine if there is data to send */
        if (send_length - count > 0)
        {
            Cmdhandler.DataMode = QSPI_DATA_1_LINE;
        }
        else
        {
            Cmdhandler.DataMode = QSPI_DATA_NONE;
        }

        /* set send buf and send size */
        Cmdhandler.NbData = send_length - count;
        HAL_QSPI_Command(&hqspi, &Cmdhandler, 5000);

        if (send_length - count > 0)
        {
            if (HAL_QSPI_Transmit(&hqspi, (uint8_t*)(ptr + count), 5000) != HAL_OK)
            {
                sfud_log_info("qspi send data failed(%d)!", hqspi.ErrorCode);
                hqspi.State = HAL_QSPI_STATE_READY;
                result = SFUD_ERR_WRITE;
            }
        }

        return result;
    }
}
#endif
