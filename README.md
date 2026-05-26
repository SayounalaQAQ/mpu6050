# MPU6050 + OLED 显示项目详解

## 项目概述

基于 STM32F103C8T6（Blue Pill），通过 I2C 总线读取 MPU6050 六轴传感器（加速度 + 陀螺仪）数据，并在 0.96 寸 OLED 屏幕（SSD1306）上实时显示。

---

## 一、硬件接线

### 引脚分配

| 外设 | 接口方式 | 引脚 | 说明 |
|:---|:---|:---|:---|
| **OLED (0.96")** | 软件模拟 I2C | PB6 (SCL), PB7 (SDA) | GPIO 引脚直接翻转电平 |
| **MPU6050** | 硬件 I2C2 外设 | PB10 (SCL), PB11 (SDA) | 复用功能开漏输出 |
| **USART1（调试）** | 串口 | PA9 (TX), PA10 (RX) | 可用于 printf 输出 |
| **直流电机** | TB717A3 驱动 | PA1 (PWM), PA4 (IN1), PA5 (IN2) | TIM2_CH2 输出 20kHz PWM |

### MPU6050 接线明细

| MPU6050 引脚 | 接 STM32 | 备注 |
|:---|:---|:---|
| VCC | 3.3V | 不可接 5V，否则损坏 |
| GND | GND | 共地 |
| SCL | PB10 | I2C2 时钟线 |
| SDA | PB11 | I2C2 数据线 |
| AD0 | GND 或悬空 | 决定 I2C 地址为 0x68 |

> **重要：** I2C 总线 SCL/SDA 需要 **外部 4.7kΩ 上拉电阻** 到 3.3V。部分 GY-521 模块已自带，若没有则必须外接。

### OLED 接线明细

OLED 使用软件模拟 I2C（bit-bang），接 PB6(SCL)、PB7(SDA)，供电 3.3V。

### 直流电机接线明细

电机通过 **TB717A3** 驱动芯片控制，该芯片用于放大 PWM 信号电压以驱动直流电机：

| TB717A3 引脚 | 接 STM32 | 功能 |
|:---|:---|:---|
| PWM | PA1 (TIM2_CH2) | 20kHz 调速信号 |
| IN1 | PA4 | 转向控制 1 |
| IN2 | PA5 | 转向控制 2 |

**转向逻辑：**

| IN1 | IN2 | 电机状态 |
|:---:|:---:|:---|
| 1 | 0 | 正转 (CW) |
| 0 | 1 | 反转 (CCW) |
| 0 | 0 | 停止 (STOP) |
| 1 | 1 | 刹车 |

**PWM 参数：**

- 频率：**20kHz**（人耳不可闻，适合直流电机驱动）
- 分辨率：**450 级**（0 ~ 449）
- 有效占空比范围：22% ~ 100%

---

## 二、OLED 驱动实现

### OLED 规格

- 驱动芯片：SSD1306
- 分辨率：128×64 像素
- 接口：I2C（从机地址 0x78）
- 显示方式：页寻址模式，分 8 页，每页 128 字节

### 软件 I2C 实现

由于项目使用了 STM32CubeMX 生成框架，OLED 的 I2C 是**用 GPIO 软件模拟**的，没有使用硬件 I2C 外设。这样可以避免与 MPU6050 的硬件 I2C2 冲突，也方便移植。

实现代码在 `USER/OLED.c` 中，核心时序如下：

#### 起始条件 (Start)
```
SCL = 1, SDA = 1  →  延时  →  SDA = 0  →  延时  →  SCL = 0
```
在 SCL 高电平时 SDA 从高变低，表示起始条件。

#### 发送一个字节
```
for (8 个 bit): {
    设置 SDA = 当前 bit
    延时
    SCL = 1  (SCL 高电平时从机采样数据)
    延时
    SCL = 0
}
SCL = 1 → 延时 → 读取 ACK → SCL = 0
```

#### 停止条件 (Stop)
```
SDA = 0  →  SCL = 1  →  延时  →  SDA = 1
```
在 SCL 高电平时 SDA 从低变高，表示停止条件。

### OLED 指令流程

初始化序列：
```
1. 0xAE - 关闭显示
2. 0xD5, 0x80 - 设置显示时钟分频
3. 0xA8, 0x3F - 设置多路复用率（64行）
4. 0xD3, 0x00 - 设置显示偏移
5. 0x40      - 设置显示开始行
6. 0xA1      - 左右方向（镜像）
7. 0xC8      - 上下方向（镜像）
8. 0xDA, 0x12 - COM引脚配置
9. 0x81, 0xCF - 设置对比度
10. 0xD9, 0xF1 - 预充电周期
11. 0xDB, 0x30 - VCOMH取消选择
12. 0xA4      - 全局显示开启
13. 0xA6      - 正常显示（非反色）
14. 0x8D, 0x14 - 开启充电泵（必须，否则无显示）
15. 0xAF      - 开启显示
```

### 显示原理

- 字符使用 **8×16 像素** 的字体（在 `OLED_FONT.h` 中定义）
- 每行对应 16 像素高度（2 页），所以 128×64 共 **4 行 × 16 列**
- 显示字符时，先写上半部分（8 像素），再写下半部分（8 像素）
- 写入命令/数据通过 `OLED_WriteCommand()` / `OLED_WriteData()` 实现

写命令格式：
```
Start → 0x78 (地址+写) → 0x00 (命令标志) → 命令字节 → Stop
```

写数据格式：
```
Start → 0x78 (地址+写) → 0x40 (数据标志) → 数据字节 → Stop
```

---

## 三、MPU6050 驱动实现

### MPU6050 简介

MPU6050 是全球首款集成 6 轴 MotionTracking 设备，包含：
- **3 轴加速度计**（±2g/±4g/±8g/±16g 可配置）
- **3 轴陀螺仪**（±250/±500/±1000/±2000 °/s 可配置）
- **内置温度传感器**

### I2C 地址

- AD0 = GND：7 位地址 **0x68**，写地址 **0xD0**，读地址 **0xD1**
- AD0 = VCC：7 位地址 **0x69**，写地址 **0xD2**，读地址 **0xD3**

> 本项目 AD0 接地，地址为 0xD0（HAL 函数需传入左移后的地址）。

### 硬件 I2C 配置

使用 STM32 的 I2C2 外设，CubeMX 配置如下：
- 时钟速度：100kHz（标准模式）
- 地址长度：7 位
- 引脚：PB10 (SCL) 复用开漏，PB11 (SDA) 复用开漏

### 寄存器配置

MPU6050 上电后默认处于**休眠模式**（Sleep Mode），且各量程为默认值。本项目在初始化时显式配置了以下寄存器：

#### 唤醒（0x6B - PWR_MGMT_1）

寄存器 **0x6B**（Power Management 1）：
| 位 | 名称 | 说明 |
|:---|:---|:---|
| bit 7 | DEVICE_RESET | 1=复位所有寄存器 |
| bit 6 | SLEEP | 1=进入休眠（默认值） |
| bit 5 | CYCLE | 1=循环模式 |
| bit 4~3 | - | 保留 |
| bit 2 | STANDBY | 待机标志 |
| bit 1~0 | CLKSEL | 时钟源选择 |

唤醒操作：
```c
uint8_t wake_cmd = 0x00;
HAL_I2C_Mem_Write(&hi2c2, 0xD0, 0x6B, I2C_MEMADD_SIZE_8BIT, &wake_cmd, 1, HAL_MAX_DELAY);
```
将寄存器 0x6B 写为 0x00，清除 SLEEP 位，选择内部振荡器作为时钟源。

#### 陀螺仪量程（0x1B - GYRO_CONFIG）

| 设置值 | 量程 | 灵敏度 |
|:---|:---|:---:|
| 0x00 | ±250 °/s | **131 LSB/°/s** ← 本项目选此 |
| 0x08 | ±500 °/s | 65.5 LSB/°/s |
| 0x10 | ±1000 °/s | 32.8 LSB/°/s |
| 0x18 | ±2000 °/s | 16.4 LSB/°/s |

选 ±250°/s，灵敏度最高，输出角速度更精细。

#### 加速度量程（0x1C - ACCEL_CONFIG）

| 设置值 | 量程 | 灵敏度 |
|:---|:---|:---:|
| 0x00 | ±2g | **16384 LSB/g** ← 本项目选此 |
| 0x08 | ±4g | 8192 LSB/g |
| 0x10 | ±8g | 4096 LSB/g |
| 0x18 | ±16g | 2048 LSB/g |

选 ±2g，灵敏度最高，适合检测微小倾角变化。

#### 数字低通滤波器（0x1A - CONFIG）

DLPF 设置用于滤除传感器的高频噪声：

| DLPF_CFG | 加速度带宽 | 陀螺仪带宽 | 延时 |
|:---:|:---:|:---:|:---:|
| 0 | 260 Hz | 256 Hz | 0 ms |
| 1 | 184 Hz | 188 Hz | 2 ms |
| 2 | 94 Hz | 98 Hz | 3 ms |
| 3 | **44 Hz** | **42 Hz** | **4.9 ms** ← 本项目选此 |
| 4 | 21 Hz | 20 Hz | 8.5 ms |
| 5 | 10 Hz | 10 Hz | 13.8 ms |
| 6 | 5 Hz | 5 Hz | 19 ms |

选 **44Hz**，在噪声抑制和响应速度之间取得平衡。

#### 采样率（0x19 - SMPLRT_DIV）

```
采样率 = 陀螺仪输出率 / (1 + SMPLRT_DIV)
```

当 DLPF 为 44Hz 时，陀螺仪输出率为 1kHz。设置 `SMPLRT_DIV = 19`：
```
采样率 = 1000 / (1 + 19) = 50 Hz
```

### 数据读取

MPU6050 的数据寄存器从地址 0x3B 开始连续排列：

| 寄存器地址 | 内容 | 长度 |
|:---|:---|:---:|
| 0x3B ~ 0x3C | ACCEL_XOUT[15:8] / [7:0] | 2 字节 |
| 0x3D ~ 0x3E | ACCEL_YOUT[15:8] / [7:0] | 2 字节 |
| 0x3F ~ 0x40 | ACCEL_ZOUT[15:8] / [7:0] | 2 字节 |
| 0x41 ~ 0x42 | TEMP_OUT[15:8] / [7:0] | 2 字节 |
| 0x43 ~ 0x44 | GYRO_XOUT[15:8] / [7:0] | 2 字节 |
| 0x45 ~ 0x46 | GYRO_YOUT[15:8] / [7:0] | 2 字节 |
| 0x47 ~ 0x48 | GYRO_ZOUT[15:8] / [7:0] | 2 字节 |

使用一次连续读取 14 字节：
```c
HAL_I2C_Mem_Read(&hi2c2, 0xD0, 0x3B, I2C_MEMADD_SIZE_8BIT, MPU_Data, 14, HAL_MAX_DELAY);
```

然后合并高低字节：
```c
Accel_X = (MPU_Data[0] << 8) | MPU_Data[1];  // 高8位在前，低8位在后
```

> **注意：** 数据是大端格式（Big Endian），高字节在前，低字节在后。

### 单位换算

#### 加速度（默认 ±2g 量程）

灵敏度：**16384 LSB/g**

```c
float accel_g = raw_value / 16384.0f;
```

| 加速度 | 原始值 | 转换后 |
|:---|:---:|:---:|
| 静止 Z 轴 (1g) | 16384 | 1.00 g |
| 静止 X/Y 轴 (0g) | ~0 | ~0.00 g |
| 最大量程 | ±32768 | ±2.00 g |

#### 陀螺仪（±250 °/s 量程，分辨率 131 LSB/°/s）

```c
float gyro_dps = raw_value / 131.0f;
```

| 状态 | 原始值 | 转换后 |
|:---|:---:|:---:|
| 静止（无转动） | ~0 | ~0.00 °/s |
| 缓慢转动 | ~131 | 1.00 °/s |

#### 温度

温度传感器公式：
```
Temp (°C) = (raw_value / 340.0) + 36.53
```

| 条件 | 原始值 | 温度 |
|:---|:---:|:---:|
| 室温 25°C | ~-3915 | ~25.0°C |
| 35°C | ~-521 | ~35.0°C |

> raw_value 为负值时温度低于 36.53°C。

### 俯仰角计算

使用加速度计数据计算俯仰角（Pitch），公式如下：

```
pitch = atan2(-Accel_X, sqrt(Accel_Y² + Accel_Z²)) × 180 / π
```

- **正值**表示前倾，**负值**表示后仰
- 基于加速度计的重力分量计算，静态精度高但动态响应有延迟

#### 软件低通滤波

原始加速度计噪声较大，导致俯仰角跳变。代码中实现了一阶低通滤波（EMA）：

```c
#define PITCH_LP_ALPHA  0.4f

Pitch_Filtered = ALPHA × pitch_raw + (1 - ALPHA) × Pitch_Filtered;
```

- `ALPHA = 0.4`：兼顾平滑与响应速度
- 有效滤除高频抖动，同时保证放平后约 100ms 内电机停转

---



## 五、直流电机驱动实现

### 驱动芯片 TB717A3

TB717A3 是一款直流电机驱动芯片，提供：
- **电压放大**：将 STM32 的 3.3V PWM 信号升压为电机所需电压
- **方向控制**：通过 IN1/IN2 逻辑电平控制正反转
- **使能控制**：通过 PWM 信号调速

### 电机驱动模块（DC_Motor）

驱动代码位于 `USER/DC_Motor.c` 和 `USER/DC_Motor.h`，封装了所有电机操作。

#### 初始化流程

```c
void DC_Motor_Init(void)
{
    /* 重配置 TIM2 为 20kHz PWM */
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
    TIM2->PSC = 7;       // 72MHz / 8 = 9MHz
    TIM2->ARR = 449;     // 9MHz / 450 = 20kHz
    TIM2->EGR = TIM_EGR_UG;  // 加载新配置
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);

    DC_Motor_SetDirection(STOP);
    DC_Motor_SetSpeed(0);
}
```

CubeMX 默认将 TIM2 配置为 1kHz 通用定时器，不适合电机驱动。初始化时在用户代码区实时重配为 **20kHz**（高于人耳听觉范围，避免电机啸叫）。

#### 转速控制

```c
void DC_Motor_SetSpeed(uint16_t pulse)
```

通过 `__HAL_TIM_SET_COMPARE()` 设置 TIM2_CH2 比较值，控制 PWM 占空比：

| 脉冲值 | 占空比 | 电机状态 |
|:---:|:---:|:---|
| 0 | 0% | 停止 |
| 100 | ~22% | 最小启动（克服静摩擦） |
| 300 | ~67% | 中速 |
| 449 | ~100% | 最快 |

> **注意：** 占空比过小时（<20%）电机只响不转，因为线圈产生的电磁力不足以克服转子静摩擦。`MOTOR_DUTY_MIN_START = 100` 保证了起步驱动力。

#### 转向控制

```c
void DC_Motor_SetDirection(uint8_t dir)
```

通过控制 PA4(IN1) 和 PA5(IN2) 的电平实现：

| dir | IN1 | IN2 | 效果 |
|:---|:---:|:---:|:---|
| `MOTOR_DIR_CW` | 1 | 0 | 正转 |
| `MOTOR_DIR_CCW` | 0 | 1 | 反转 |
| `MOTOR_DIR_STOP` | 0 | 0 | 停止 |

#### 俯仰角控制算法

```c
void DC_Motor_ControlByPitch(float pitch)
```

这是核心控制函数，根据 MPU6050 的俯仰角决定电机行为：

```
俯仰角 < 死区（5°） -> 电机停止（防抖动）
俯仰角 > 0（前倾）  -> 正转
俯仰角 < 0（后仰）  -> 反转

转速映射：
  pulse = MIN_START + (FULL - MIN_START) x (|角度| / 最大角度)

示例：
  角度 = 0°  ->  pulse =   0  -> 电机停止
  角度 = 5°  ->  pulse = 100  -> 最低速旋转（22% 占空比）
  角度 = 30° ->  pulse = 274  -> 中速（61%）
  角度 = 60° ->  pulse = 449  -> 最快（100%）
```

角度限幅在 **60°**，超出按最大速度处理。死区 **5°** 防止微小角度变化导致电机频繁启停。

## 六、程序主流程 (main.c)

```
HAL_Init()
   ↓
SystemClock_Config()   → HSE 8MHz → PLL x9 → SYSCLK 72MHz
   ↓
MX_GPIO_Init()         → PB6/PB7 开漏输出（OLED 软件 I2C）
                         PA4/PA5 推挽输出（电机方向）
MX_USART1_UART_Init()  → 串口 printf
MX_I2C2_Init()         → PB10/PB11 I2C2（MPU6050）
MX_TIM2_Init()         → PA1 TIM2_CH2 PWM（默认 1kHz）
   ↓
OLED_Init()            → SSD1306 初始化 + 清屏，显示 "OLED: OK"
MPU6050_Init()         → 唤醒、配置量程/滤波器/采样率，显示 "MPU6050: Ready"
DC_Motor_Init()        → 重配 TIM2 为 20kHz，启动 PWM，显示 "Motor: OK"
   ↓
while(1) {
    MPU6050_ReadAll()       → 读取 14 字节并换算物理单位
    pitch = GetPitch()      → 加速度计计算俯仰角（含低通滤波）
    DC_Motor_ControlByPitch → 根据俯仰角控制转向 + 转速

    OLED 显示 AX/AY/AZ/GX/GY/GZ/Temp/Pitch
    延时 50ms（约 20Hz）
}
```

### I2C 扫描函数

在初始化时调用 `I2C_Scan()`，遍历所有可能的 7 位 I2C 地址，用 `HAL_I2C_IsDeviceReady()` 检测哪些设备有应答，并在 OLED 上显示结果。这是排查硬件连接问题的有效手段。

---

## 七、常见问题排查

### OLED 不显示
- 检查 PB6/PB7 接线
- 确认 OLED 供电 3.3V
- 检查充电泵是否开启（0x8D, 0x14）

### MPU6050 显示 ERROR / 找不到设备
- I2C 总线缺少上拉电阻（4.7kΩ）
- AD0 电平与代码地址不匹配（0xD0 vs 0xD2）
- VCC 供电不足或接线错误
- SCL/SDA 接反

### 数据不动或异常
- MPU6050 未正确唤醒（检查 0x6B 寄存器）
- I2C 速率过高（建议 100kHz）
- 数据未正确合并高低字节

### 电机只响不转
- **占空比过低**：检查 `MOTOR_DUTY_MIN_START`，低于 22% 电机无法克服静摩擦
- **转向引脚悬空**：检查 PA4/PA5 接线
- **PWM 频率不合适**：在 `DC_Motor_Init()` 中确认 TIM2 已重配为 20kHz
- **驱动芯片供电不足**：检查 TB717A3 的电源电压

### 电机转向反了
- 交换 `DC_Motor.c` 中 `MOTOR_DIR_CW` 的 IN1/IN2 电平，或将电机两根线对调

---

## 八、项目文件结构

```
mpu6050/
├── Core/
│   ├── Inc/               # 头文件
│   │   ├── main.h
│   │   ├── gpio.h
│   │   ├── tim.h
│   │   ├── i2c.h
│   │   └── usart.h
│   └── Src/               # 源文件
│       ├── main.c         # 主程序（初始化 + 主循环）
│       ├── gpio.c         # GPIO 初始化（含 PA4/PA5 电机方向）
│       ├── tim.c          # TIM2 初始化（PA1 PWM）
│       ├── i2c.c          # I2C2 外设初始化
│       ├── usart.c        # 串口初始化
│       └── stm32f1xx_it.c # 中断服务函数
├── USER/
│   ├── DC_Motor.c         # 直流电机驱动（PWM + 方向 + 俯仰角控制）
│   ├── DC_Motor.h
│   ├── MPU6050.c          # MPU6050 传感器驱动（含俯仰角计算 + 滤波）
│   ├── MPU6050.h
│   ├── OLED.c             # OLED 驱动（软件 I2C）
│   ├── OLED.h
│   └── OLED_FONT.h        # 8×16 字库
├── Drivers/               # STM32 HAL 库
├── MDK-ARM/               # Keil 工程文件
└── mpu6050.ioc            # STM32CubeMX 配置文件
```
