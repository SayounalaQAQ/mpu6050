#include "MPU6050.h"
#include "main.h"
#include "i2c.h"
#include <math.h>

/* ---------- 采样率分频（陀螺仪输出率 1kHz / (1 + divider)） ---------- */
#define MPU6050_SMPLRT_DIV     19

/* ---------- 俯仰角低通滤波系数（越小越平滑） ---------- */
#define PITCH_LP_ALPHA          0.4f

/* ---------- 私有变量 ---------- */
static uint8_t  MPU_Data[MPU6050_DATA_LEN];
static int16_t  Accel_X, Accel_Y, Accel_Z;
static int16_t  Gyro_X, Gyro_Y, Gyro_Z;
static float    Accel_X_f, Accel_Y_f, Accel_Z_f;
static float    Gyro_X_f, Gyro_Y_f, Gyro_Z_f;
static float    Temp_f;
static float    Pitch_Filtered = 0.0f;   /* 一阶低通滤波后的俯仰角 */

/**
  * @brief   写 MPU6050 寄存器
  * @param   reg  寄存器地址
  * @param   val  写入值
  * @retval  HAL_OK / HAL_ERROR / HAL_TIMEOUT
  */
static HAL_StatusTypeDef MPU6050_WriteReg(uint8_t reg, uint8_t val)
{
    return HAL_I2C_Mem_Write(&hi2c2, MPU6050_I2C_ADDR, reg,
                             I2C_MEMADD_SIZE_8BIT, &val, 1,
                             MPU6050_I2C_TIMEOUT);
}

/**
  * @brief   读 MPU6050 多字节寄存器
  * @param   reg   起始寄存器地址
  * @param   data  输出缓冲区
  * @param   len   读取长度
  * @retval  HAL_OK / HAL_ERROR / HAL_TIMEOUT
  */
static HAL_StatusTypeDef MPU6050_ReadRegs(uint8_t reg, uint8_t *data, uint16_t len)
{
    return HAL_I2C_Mem_Read(&hi2c2, MPU6050_I2C_ADDR, reg,
                            I2C_MEMADD_SIZE_8BIT, data, len,
                            MPU6050_I2C_TIMEOUT);
}

/* ---------- 公有函数 ---------- */

/**
  * @brief   初始化 MPU6050：唤醒传感器并配置量程 / 滤波器 / 采样率
  * @retval  0-成功，1-失败
  */
uint8_t MPU6050_Init(void)
{
    /* 唤醒传感器：清除 SLEEP 位 */
    if (MPU6050_WriteReg(MPU6050_REG_PWR_MGMT_1, MPU6050_PWR_WAKE) != HAL_OK)
        return 1;

    /* 陀螺仪量程 ±250°/s（最高精度） */
    if (MPU6050_WriteReg(MPU6050_REG_GYRO_CONFIG, MPU6050_GYRO_RANGE_250) != HAL_OK)
        return 1;

    /* 加速度量程 ±2g（最高精度） */
    if (MPU6050_WriteReg(MPU6050_REG_ACCEL_CONFIG, MPU6050_ACCEL_RANGE_2G) != HAL_OK)
        return 1;

    /* 数字低通滤波器 44Hz（滤除高频振动噪声） */
    if (MPU6050_WriteReg(MPU6050_REG_CONFIG, MPU6050_DLPF_44HZ) != HAL_OK)
        return 1;

    /* 采样率 50Hz */
    if (MPU6050_WriteReg(MPU6050_REG_SMPLRT_DIV, MPU6050_SMPLRT_DIV) != HAL_OK)
        return 1;

    return 0;
}

/**
  * @brief   读取所有传感器原始数据并换算为物理单位
  * @retval  0-成功，1-失败
  */
uint8_t MPU6050_ReadAll(void)
{
    if (MPU6050_ReadRegs(MPU6050_REG_ACCEL_XOUT_H, MPU_Data, MPU6050_DATA_LEN) != HAL_OK)
        return 1;

    /* 合并高 / 低字节（大端格式） */
    Accel_X = (MPU_Data[0]  << 8) | MPU_Data[1];
    Accel_Y = (MPU_Data[2]  << 8) | MPU_Data[3];
    Accel_Z = (MPU_Data[4]  << 8) | MPU_Data[5];
    int16_t raw_temp = (MPU_Data[6]  << 8) | MPU_Data[7];
    Gyro_X  = (MPU_Data[8]  << 8) | MPU_Data[9];
    Gyro_Y  = (MPU_Data[10] << 8) | MPU_Data[11];
    Gyro_Z  = (MPU_Data[12] << 8) | MPU_Data[13];

    /* 换算成物理单位 */
    Accel_X_f = Accel_X / MPU6050_ACCEL_SCALE;
    Accel_Y_f = Accel_Y / MPU6050_ACCEL_SCALE;
    Accel_Z_f = Accel_Z / MPU6050_ACCEL_SCALE;
    Gyro_X_f  = Gyro_X  / MPU6050_GYRO_SCALE;
    Gyro_Y_f  = Gyro_Y  / MPU6050_GYRO_SCALE;
    Gyro_Z_f  = Gyro_Z  / MPU6050_GYRO_SCALE;
    Temp_f    = (raw_temp / MPU6050_TEMP_SCALE) + MPU6050_TEMP_OFFSET;

    return 0;
}

/**
  * @brief  获取 X 轴加速度（g）
  */
float MPU6050_GetAccelX(void)
{
    return Accel_X_f;
}

/**
  * @brief  获取 Y 轴加速度（g）
  */
float MPU6050_GetAccelY(void)
{
    return Accel_Y_f;
}

/**
  * @brief  获取 Z 轴加速度（g）
  */
float MPU6050_GetAccelZ(void)
{
    return Accel_Z_f;
}

/**
  * @brief  获取 X 轴角速度（°/s）
  */
float MPU6050_GetGyroX(void)
{
    return Gyro_X_f;
}

/**
  * @brief  获取 Y 轴角速度（°/s）
  */
float MPU6050_GetGyroY(void)
{
    return Gyro_Y_f;
}

/**
  * @brief  获取 Z 轴角速度（°/s）
  */
float MPU6050_GetGyroZ(void)
{
    return Gyro_Z_f;
}

/**
  * @brief  获取温度（°C）
  */
float MPU6050_GetTemperature(void)
{
    return Temp_f;
}

/**
  * @brief  计算俯仰角（°），基于加速度计数据（含一阶低通滤波）
  * @retval 角度值，正号表示前倾，负号表示后仰
  */
float MPU6050_GetPitch(void)
{
    float pitch = atan2f(-Accel_X_f,
                         sqrtf(Accel_Y_f * Accel_Y_f + Accel_Z_f * Accel_Z_f));
    pitch = pitch * 180.0f / 3.14159265f;

    /* 一阶低通滤波：平滑跳变 */
    Pitch_Filtered = PITCH_LP_ALPHA * pitch + (1.0f - PITCH_LP_ALPHA) * Pitch_Filtered;

    return Pitch_Filtered;
}
