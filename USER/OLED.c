#include "OLED.h"
#include "OLED_FONT.h"
#include "main.h" // 关键：引入 HAL 库的系统头文件

/* 【核心引脚对应】利用 HAL 库控制 PB6(SCL) 和 PB7(SDA) */
#define OLED_W_SCL(x)		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, (GPIO_PinState)(x))
#define OLED_W_SDA(x)		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, (GPIO_PinState)(x))

static void OLED_I2C_Delay(void)
{
	uint8_t i = 10;
	while (i--) { __NOP(); }
}

void OLED_I2C_Start(void)
{
	OLED_W_SDA(1);
	OLED_W_SCL(1);
	OLED_I2C_Delay();
	OLED_W_SDA(0);
	OLED_I2C_Delay();
	OLED_W_SCL(0);
	OLED_I2C_Delay();
}

void OLED_I2C_Stop(void)
{
	OLED_W_SDA(0);
	OLED_I2C_Delay();
	OLED_W_SCL(1);
	OLED_I2C_Delay();
	OLED_W_SDA(1);
	OLED_I2C_Delay();
}

void OLED_I2C_SendByte(uint8_t Byte)
{
	uint8_t i;
	for (i = 0; i < 8; i++)
	{
		OLED_W_SDA(!!(Byte & (0x80 >> i)));
		OLED_I2C_Delay();
		OLED_W_SCL(1);
		OLED_I2C_Delay();
		OLED_W_SCL(0);
		OLED_I2C_Delay();
	}
	OLED_W_SCL(1);
	OLED_I2C_Delay();
	OLED_W_SCL(0);
	OLED_I2C_Delay();
}

void OLED_WriteCommand(uint8_t Command)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(0x78);		// 从机地址
	OLED_I2C_SendByte(0x00);		// 写命令
	OLED_I2C_SendByte(Command); 
	OLED_I2C_Stop();
}

void OLED_WriteData(uint8_t Data)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(0x78);		// 从机地址
	OLED_I2C_SendByte(0x40);		// 写数据
	OLED_I2C_SendByte(Data);
	OLED_I2C_Stop();
}

void OLED_SetCursor(uint8_t Y, uint8_t X)
{
	OLED_WriteCommand(0xB0 | Y);					
	OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));	
	OLED_WriteCommand(0x00 | (X & 0x0F));			
}

void OLED_Clear(void)
{  
	uint8_t i, j;
	for (j = 0; j < 8; j++)
	{
		OLED_SetCursor(j, 0);
		for(i = 0; i < 128; i++)
		{
			OLED_WriteData(0x00);
		}
	}
}

void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
	uint8_t i;

	if (Line < 1 || Line > 4 || Column < 1 || Column > 16) { return; }
	if (Char < ' ' || Char > '~') { Char = '?'; }

	OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);		
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i]);			
	}
	OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);	
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);		
	}
}

void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i++)
	{
		OLED_ShowChar(Line, Column + i, String[i]);
	}
}

/* 【已修正】这里补上了 missing 的 return 声明 */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;
	while (Y--) { Result *= X; }
	return Result;
}

void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
	uint8_t i; uint32_t Number1;
	if (Number >= 0) { OLED_ShowChar(Line, Column, '+'); Number1 = Number; }
	else { OLED_ShowChar(Line, Column, '-'); Number1 = -Number; }
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i, SingleNumber;
	for (i = 0; i < Length; i++)							
	{
		SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
		if (SingleNumber < 10) { OLED_ShowChar(Line, Column + i, SingleNumber + '0'); }
		else { OLED_ShowChar(Line, Column + i, SingleNumber - 10 + 'A'); }
	}
}

void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
	}
}

void OLED_Init(void)
{
    // 换成 HAL 库高精度延时，防止上电过快导致屏幕来不及复位
    HAL_Delay(200); 
	
	OLED_W_SCL(1);
	OLED_W_SDA(1);
	
	OLED_WriteCommand(0xAE);	// 关闭显示
	OLED_WriteCommand(0xD5);	// 设置显示时钟分频比
	OLED_WriteCommand(0x80);
	OLED_WriteCommand(0xA8);	// 设置多路复用率
	OLED_WriteCommand(0x3F);
	OLED_WriteCommand(0xD3);	// 设置显示偏移
	OLED_WriteCommand(0x00);
	OLED_WriteCommand(0x40);	// 设置显示开始行
	OLED_WriteCommand(0xA1);	// 设置左右方向
	OLED_WriteCommand(0xC8);	// 设置上下方向
	OLED_WriteCommand(0xDA);	// 设置COM引脚硬件配置
	OLED_WriteCommand(0x12);
	OLED_WriteCommand(0x81);	// 设置对比度控制
	OLED_WriteCommand(0xCF);
	OLED_WriteCommand(0xD9);	// 设置预充电周期
	OLED_WriteCommand(0xF1);
	OLED_WriteCommand(0xDB);	// 设置VCOMH取消选择级别
	OLED_WriteCommand(0x30);
	OLED_WriteCommand(0xA4);	// 设置整个显示打开/关闭
	OLED_WriteCommand(0xA6);	// 设置正常/倒转显示
	OLED_WriteCommand(0x8D);	// 设置充电泵
	OLED_WriteCommand(0x14);
	OLED_WriteCommand(0xAF);	// 开启显示
		
	OLED_Clear();				// 清屏
}
