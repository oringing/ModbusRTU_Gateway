# 依赖清单

本项目使用的第三方库、工具链及硬件平台版本信息。

## 🛠️ 开发工具链

| 工具 | 版本 | 说明 |
|------|------|------|
| **Keil MDK** | v5.37+ | ARM 编译器 v6 (ARMCLANG) |
| **STM32CubeMX** | v6.9.0+ | HAL 库代码生成器 |
| **STM32F1 Device Family Pack** | v1.8.4 | STM32F1 系列支持包 |
| **ST-LINK Utility** | v4.6.0 | 固件烧录工具 |

## 📦 核心依赖

### HAL 库
- **STM32 HAL Library**: v1.1.8（通过 CubeMX 生成）
  - 路径：`Drivers/STM32F1xx_HAL_Driver/`
  - 关键模块：UART、DMA、IWDG、GPIO

### RTOS
- **FreeRTOS**: v10.3.1
  - 路径：`Middlewares/Third_Party/FreeRTOS/`
  - 接口：CMSIS-RTOS v1
  - 配置文件：`FreeRTOSConfig.h`

### 中间件
- **CMSIS**: v5.9.0
  - 路径：`Drivers/CMSIS/`
  - 用途：ARM Cortex-M 内核抽象层

## 🔌 硬件平台

| 组件 | 型号 | 规格 |
|------|------|------|
| **MCU** | STM32F103C8T6 | ARM Cortex-M3, 72MHz, 64KB Flash, 20KB RAM |
| **RS485 收发器** | MAX485ESA | 半双工，5V 供电 |
| **调试器** | ST-LINK V2 | SWD 接口 |
| **晶振** | 8MHz HSE | 外部高速时钟源 |

## 🧪 测试工具

| 工具 | 用途 |
|------|------|
| **Modbus Poll** | Modbus 主机模拟器（Windows） |
| **Modbus Slave** | 从机响应验证工具 |
| **串口助手** | SSCOM / XCOM / Hercules |
| **逻辑分析仪** | Saleae Logic Pro 8（可选，用于时序分析） |

## 📋 兼容性说明

### 已验证环境
- ✅ Windows 11 + Keil MDK v5.37
- ✅ STM32F103C8T6 最小系统板（蓝 pill）
- ✅ MAX485ESA + 5V 供电
- ✅ 波特率 115200 bps

### 未测试环境
- ⚠️ Linux/Mac 下的交叉编译（需 gcc-arm-none-eabi）
- ⚠️ 其他 STM32F1 变种型号（如 F103RCT6）
- ⚠️ 不同厂商的 RS485 收发器（如 SP3485）

## 🔄 更新历史

| 日期 | 版本 | 变更说明 |
|------|------|---------|
| 2026-05-10 | v1.0.0 | 初始依赖清单，记录当前项目配置 |

---

**注意**：如需在其他平台编译，可能需要调整 Makefile 或 CMakeLists.txt（当前仅提供 Keil 工程）。
