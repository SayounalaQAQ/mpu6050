#include "OLED.h"
#include "OLED_FONT.h"
#include "main.h"

/* ---------- GPIO 引脚宏 ---------- */
#define OLED_SCL_PIN             GPIO_PIN_6
#define OLED_SDA_PIN             GPIO_PIN_7
#define OLED_GPIO_PORT           GPIOB

#define OLED_W_SCL(x)            HAL_GPIO_WritePin(OLED_GPIO_PORT, OLED_SCL_PIN, (GPIO_PinState)(x))
#define OLED_W_SDA(x)            HAL_GPIO_WritePin(OLED_GPIO_PORT, OLED_SDA_PIN, (GPIO_PinState)(x))

/* ---------- I2C 地址 ---------- */
#define OLED_ADDR                0x78
#define OLED_CMD_MODE            0x00
#define OLED_DATA_MODE           0x40

/* ---------- 显示参数 ---------- */
#define OLED_PAGE_COUNT          8
#define OLED_WIDTH               128
#define OLED_LINE_MAX            4
#define OLED_COL_MAX             16
#define OLED_CHAR_WIDTH          8
#define OLED_FONT_HEIGHT         16

/* ---------- 时序参数 ---------- */
#define OLED_I2C_DELAY_LOOP      10

/* ---------- 命令宏 ---------- */
#define OLED_CMD_DISPLAY_OFF     0xAE
#define OLED_CMD_DISPLAY_ON      0xAF
#define OLED_CMD_CLK_DIV         0xD5
#define OLED_CMD_MUX_RATIO       0xA8
#define OLED_CMD_DISPLAY_OFFSET  0xD3
#define OLED_CMD_START_LINE      0x40
#define OLED_CMD_SEG_REMAP       0xA1
#define OLED_CMD_COM_SCAN_DIR    0xC8
#define OLED_CMD_COM_PIN_CFG     0xDA
#define OLED_CMD_CONTRAST        0x81
#define OLED_CMD_PRECHARGE       0xD9
#define OLED_CMD_VCOMH           0xDB
#define OLED_CMD_DISPLAY_ALL_ON  0xA4
#define OLED_CMD_NORMAL_DISPLAY  0xA6
#define OLED_CMD_CHARGE_PUMP     0x8D

/* ---------- 命令参数 ---------- */
#define OLED_CLK_DIV_VAL         0x80
#define OLED_MUX_RATIO_VAL       0x3F
#define OLED_DISPLAY_OFFSET_VAL  0x00
#define OLED_COM_PIN_CFG_VAL     0x12
#define OLED_CONTRAST_VAL        0xCF
#define OLED_PRECHARGE_VAL       0xF1
#define OLED_VCOMH_VAL           0x30
#define OLED_CHARGE_PUMP_VAL     0x14

/* ---------- 初始化延时 ---------- */
#define OLED_INIT_DELAY_MS       200

/**
  * @brief   软件 I2C 延时
  */
static void OLED_I2C_Delay(void)
{
    uint8_t i = OLED_I2C_DELAY_LOOP;
    while (i--) { __NOP(); }
}

/**
  * @brief   I2C 起始信号
  */
static void OLED_I2C_Start(void)
{
    OLED_W_SDA(1);
    OLED_W_SCL(1);
    OLED_I2C_Delay();
    OLED_W_SDA(0);
    OLED_I2C_Delay();
    OLED_W_SCL(0);
    OLED_I2C_Delay();
}

/**
  * @brief   I2C 停止信号
  */
static void OLED_I2C_Stop(void)
{
    OLED_W_SDA(0);
    OLED_I2C_Delay();
    OLED_W_SCL(1);
    OLED_I2C_Delay();
    OLED_W_SDA(1);
    OLED_I2C_Delay();
}

/**
  * @brief   I2C 发送一个字节
  * @param   Byte  要发送的数据
  */
static void OLED_I2C_SendByte(uint8_t Byte)
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

/**
  * @brief   向 OLED 写命令
  * @param   Command  命令字节
  */
static void OLED_WriteCommand(uint8_t Command)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(OLED_ADDR);
    OLED_I2C_SendByte(OLED_CMD_MODE);
    OLED_I2C_SendByte(Command);
    OLED_I2C_Stop();
}

/**
  * @brief   向 OLED 写数据
  * @param   Data  数据字节
  */
static void OLED_WriteData(uint8_t Data)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(OLED_ADDR);
    OLED_I2C_SendByte(OLED_DATA_MODE);
    OLED_I2C_SendByte(Data);
    OLED_I2C_Stop();
}

/**
  * @brief   设置 OLED 光标位置
  * @param   Y  页地址（0-7）
  * @param   X  列地址（0-127）
  */
static void OLED_SetCursor(uint8_t Y, uint8_t X)
{
    OLED_WriteCommand(0xB0 | Y);
    OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));
    OLED_WriteCommand(0x00 | (X & 0x0F));
}

/**
  * @brief   清屏
  */
void OLED_Clear(void)
{
    uint8_t i, j;
    for (j = 0; j < OLED_PAGE_COUNT; j++)
    {
        OLED_SetCursor(j, 0);
        for (i = 0; i < OLED_WIDTH; i++)
        {
            OLED_WriteData(0x00);
        }
    }
}

/**
  * @brief   在指定位置显示一个字符
  * @param   Line    行号（1-4）
  * @param   Column  列号（1-16）
  * @param   Char    要显示的字符
  */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
    uint8_t i;

    if (Line < 1 || Line > OLED_LINE_MAX || Column < 1 || Column > OLED_COL_MAX)
        return;
    if (Char < ' ' || Char > '~')
        Char = '?';

    OLED_SetCursor((Line - 1) * 2, (Column - 1) * OLED_CHAR_WIDTH);
    for (i = 0; i < OLED_CHAR_WIDTH; i++)
    {
        OLED_WriteData(OLED_F8x16[Char - ' '][i]);
    }
    OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * OLED_CHAR_WIDTH);
    for (i = 0; i < OLED_CHAR_WIDTH; i++)
    {
        OLED_WriteData(OLED_F8x16[Char - ' '][i + OLED_CHAR_WIDTH]);
    }
}

/**
  * @brief   在指定位置显示字符串
  * @param   Line    行号（1-4）
  * @param   Column  列号（1-16）
  * @param   String  要显示的字符串
  */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i++)
    {
        OLED_ShowChar(Line, Column + i, String[i]);
    }
}

/**
  * @brief   计算 X 的 Y 次幂
  * @param   X  底数
  * @param   Y  指数
  * @retval  计算结果
  */
static uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--) { Result *= X; }
    return Result;
}

/**
  * @brief   在指定位置显示无符号整数
  * @param   Line    行号（1-4）
  * @param   Column  列号（1-16）
  * @param   Number  要显示的数字
  * @param   Length  数字位数
  */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
    }
}

/**
  * @brief   在指定位置显示有符号整数
  * @param   Line    行号（1-4）
  * @param   Column  列号（1-16）
  * @param   Number  要显示的数字
  * @param   Length  数字位数（不含符号位）
  */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
    uint8_t i;
    uint32_t Number1;

    if (Number >= 0)
    {
        OLED_ShowChar(Line, Column, '+');
        Number1 = Number;
    }
    else
    {
        OLED_ShowChar(Line, Column, '-');
        Number1 = -Number;
    }

    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
    }
}

/**
  * @brief   在指定位置显示十六进制数
  * @param   Line    行号（1-4）
  * @param   Column  列号（1-16）
  * @param   Number  要显示的数字
  * @param   Length  数字位数
  */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i, SingleNumber;
    for (i = 0; i < Length; i++)
    {
        SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
        if (SingleNumber < 10)
            OLED_ShowChar(Line, Column + i, SingleNumber + '0');
        else
            OLED_ShowChar(Line, Column + i, SingleNumber - 10 + 'A');
    }
}

/**
  * @brief   在指定位置显示二进制数
  * @param   Line    行号（1-4）
  * @param   Column  列号（1-16）
  * @param   Number  要显示的数字
  * @param   Length  数字位数
  */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++)
    {
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
    }
}

/**
  * @brief   初始化 OLED 屏幕
  */
void OLED_Init(void)
{
    HAL_Delay(OLED_INIT_DELAY_MS);

    OLED_W_SCL(1);
    OLED_W_SDA(1);

    OLED_WriteCommand(OLED_CMD_DISPLAY_OFF);
    OLED_WriteCommand(OLED_CMD_CLK_DIV);
    OLED_WriteCommand(OLED_CLK_DIV_VAL);
    OLED_WriteCommand(OLED_CMD_MUX_RATIO);
    OLED_WriteCommand(OLED_MUX_RATIO_VAL);
    OLED_WriteCommand(OLED_CMD_DISPLAY_OFFSET);
    OLED_WriteCommand(OLED_DISPLAY_OFFSET_VAL);
    OLED_WriteCommand(OLED_CMD_START_LINE);
    OLED_WriteCommand(OLED_CMD_SEG_REMAP);
    OLED_WriteCommand(OLED_CMD_COM_SCAN_DIR);
    OLED_WriteCommand(OLED_CMD_COM_PIN_CFG);
    OLED_WriteCommand(OLED_COM_PIN_CFG_VAL);
    OLED_WriteCommand(OLED_CMD_CONTRAST);
    OLED_WriteCommand(OLED_CONTRAST_VAL);
    OLED_WriteCommand(OLED_CMD_PRECHARGE);
    OLED_WriteCommand(OLED_PRECHARGE_VAL);
    OLED_WriteCommand(OLED_CMD_VCOMH);
    OLED_WriteCommand(OLED_VCOMH_VAL);
    OLED_WriteCommand(OLED_CMD_DISPLAY_ALL_ON);
    OLED_WriteCommand(OLED_CMD_NORMAL_DISPLAY);
    OLED_WriteCommand(OLED_CMD_CHARGE_PUMP);
    OLED_WriteCommand(OLED_CHARGE_PUMP_VAL);
    OLED_WriteCommand(OLED_CMD_DISPLAY_ON);

    OLED_Clear();
}
