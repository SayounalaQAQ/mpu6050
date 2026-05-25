#include "MPU6050.h"
#include "main.h"
#include "i2c.h"

/* ---------- 私有变量 ---------- */
static uint8_t  MPU_Data[14];   // 原始寄存器数据
static int16_t  Accel_X, Accel_Y, Accel_Z;  // 加速度原始值
static int16_t  Gyro_X, Gyro_Y, Gyro_Z;     // 陀螺仪原始值

/* ---------- 公有变量（换算后） ---------- */
float Ax, Ay, Az;
float Gx, Gy, Gz;
float Temperature;

/* ---------- 内部函数 ---------- */

/* 读 MPU6050 寄存器 */
static HAL_StatusTypeDef MPU6050_WriteReg(uint8_t reg, uint8_t val)
{
	return HAL_I2C_Mem_Write(&hi2c2, 0xD0, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, HAL_MAX_DELAY);
}

/* ---------- 公有函数 ---------- */

void MPU6050_Init(void)
{
	// 1. 唤醒传感器：清除 SLEEP 位
	MPU6050_WriteReg(0x6B, 0x00);

	// 2. 陀螺仪量程 ±250°/s → 131 LSB/°/s（最高精度）
	MPU6050_WriteReg(0x1B, 0x00);

	// 3. 加速度量程 ±2g → 16384 LSB/g（最高精度）
	MPU6050_WriteReg(0x1C, 0x00);

	// 4. 数字低通滤波器 44Hz（滤除高频振动噪声）
	MPU6050_WriteReg(0x1A, 0x03);

	// 5. 采样率分频：50Hz（陀螺仪输出率 1kHz / (1+19) = 50Hz）
	MPU6050_WriteReg(0x19, 19);
}

void MPU6050_Read_All(void)
{
	if (HAL_I2C_Mem_Read(&hi2c2, 0xD0, 0x3B, I2C_MEMADD_SIZE_8BIT, MPU_Data, 14, HAL_MAX_DELAY) != HAL_OK)
		return;

	// 合并高/低字节（大端格式）
	Accel_X = (MPU_Data[0]  << 8) | MPU_Data[1];
	Accel_Y = (MPU_Data[2]  << 8) | MPU_Data[3];
	Accel_Z = (MPU_Data[4]  << 8) | MPU_Data[5];
	int16_t raw_temp = (MPU_Data[6]  << 8) | MPU_Data[7];
	Gyro_X  = (MPU_Data[8]  << 8) | MPU_Data[9];
	Gyro_Y  = (MPU_Data[10] << 8) | MPU_Data[11];
	Gyro_Z  = (MPU_Data[12] << 8) | MPU_Data[13];

	// 换算成物理单位
	Ax = Accel_X / 16384.0f;       // g
	Ay = Accel_Y / 16384.0f;
	Az = Accel_Z / 16384.0f;
	Gx = Gyro_X  / 131.0f;         // °/s
	Gy = Gyro_Y  / 131.0f;
	Gz = Gyro_Z  / 131.0f;
	Temperature = (raw_temp / 340.0f) + 36.53f;  // °C
}
