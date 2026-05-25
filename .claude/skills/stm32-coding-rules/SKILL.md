---
name: stm32-coding-rules
description: STM32 项目编码规范。在生成或修改 C 代码时自动应用，涵盖文件组织、低耦合设计、宏定义规范、注释规范、错误处理、命名风格及 CubeMX 兼容性。
---

# STM32 编码规范

## 指令

在为本项目编写或修改 C 代码时，必须遵守以下规则：

### 1. 文件组织
- 新外设驱动文件统一放在 `USER/` 目录下，同时在 `USER/` 下创建对应的 `.c` 和 `.h` 文件。不要将用户驱动放入 `Core/Src/` 或 `Core/Inc/`。
- 文件命名使用 `模块名.h` / `模块名.c` 格式，例如 `MPU6050.h`、`OLED.c`。
- 每个 `.h` 文件必须使用 `#ifndef` / `#define` / `#endif` 宏保护防止重复包含。

### 2. 低耦合设计
- 每个外设驱动独立成模块，模块间通过函数接口通信，禁止跨模块直接访问全局变量。
- 驱动层只封装硬件操作和协议逻辑，业务逻辑放在上层调用处。
- 避免在头文件中定义全局变量，优先使用 `extern` 声明 + `.c` 文件中定义。

### 3. 善用宏定义
- 禁止在代码中直接出现 Magic Number（裸数字、裸地址、裸长度等），所有常量必须用 `#define` 定义为有意义的宏名称。
- 外设地址、引脚、超时时间、缓冲区大小、寄存器地址等常量必须用宏定义。
- 示例：
  ```c
  /* 正确 */
  #define MPU6050_ADDR     0x68
  #define I2C_TIMEOUT      100
  #define ACCEL_SCALE      16384
  uint8_t data = HAL_I2C_Mem_Read(&hi2c2, MPU6050_ADDR << 1, MPU6050_REG_ACCEL_X, 1, buf, 14, I2C_TIMEOUT);

  /* 错误 */
  uint8_t data = HAL_I2C_Mem_Read(&hi2c2, 0x68<<1, 0x3B, 1, buf, 14, 100);
  ```

### 4. 错误处理
- I2C、USART 等通信外设的函数必须返回错误状态或超时判断，禁止在驱动层死等。
- 使用 HAL 库返回值（`HAL_OK` / `HAL_ERROR` / `HAL_TIMEOUT`）进行状态判断。

### 5. 注释规范
每个函数必须包含 Doxygen 风格注释，说明功能、参数和返回值：

```c
/**
  * @brief   函数功能描述
  * @param   参数名 参数说明
  * @retval  返回值说明
  */
```

### 6. 命名风格
- 外设驱动函数使用统一模块前缀：`MPU6050_Init()`、`MPU6050_ReadAccel()`、`OLED_Display()` 等。
- 宏定义和枚举使用全大写 + 下划线：`#define I2C_TIMEOUT 100`。
- 局部变量使用小驼峰或下划线分隔。

### 7. CubeMX 兼容性
- 禁止删除或修改 CubeMX 自动生成的代码，即 `/* USER CODE BEGIN */` / `/* USER CODE END */` 标记区域外的内容。
- 用户代码一律写在 `USER CODE` 标记区域内。

## 示例

新增一个 BMP280 气压传感器驱动时：

```
USER/
├── BMP280.h      # 宏定义（地址、寄存器、超时）、函数声明
├── BMP280.c      # 初始化、读写函数实现（带错误返回、Doxygen 注释）
```

```c
/* BMP280.h */
#ifndef __BMP280_H__
#define __BMP280_H__

#define BMP280_ADDR         0x76
#define BMP280_REG_TEMP     0xFA
#define BMP280_REG_PRESS    0xF7
#define I2C_TIMEOUT         100

uint8_t BMP280_Init(void);
float   BMP280_ReadTemperature(void);

#endif
```

## 故障排除

- 如果 CubeMX 重新生成代码导致 USER 目录文件丢失：用户驱动在 `USER/` 下不受影响，只需重新添加 `#include` 即可。
- 如果编译报错未找到头文件：检查 MDK-ARM 项目配置中是否已将 `USER/` 目录添加到头文件搜索路径。
