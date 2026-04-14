# MotorGateway 项目

## 项目简介

MotorGateway 是一个基于 STM32F103C8T6 的电机控制网关项目，主要功能包括：

- FreeRTOS 多任务调度
- LED 状态指示
- UART 通信
- 电位器控制舵机（计划功能）
- 电机驱动控制（计划功能）

## 硬件要求

- **主控板**：STM32F103C8T6 最小系统板
- **调试器**：ST-LINK V2
- **外设**：
  - LED（板载 PC13）
  - USART1 串口（PA9/PA10）
  - 电位器（计划）
  - 舵机（计划）
  - 电机驱动（计划）

## 软件环境

- **IDE**：Keil MDK v5.x
- **固件库**：STM32F1 HAL 库
- **RTOS**：FreeRTOS
- **工具**：STM32CubeMX

## 项目结构

```
MotorGateway/
├── Core/
│   ├── Inc/         # 头文件
│   └── Src/         # 源代码
├── Drivers/         # 驱动文件
├── docs/            # 文档
│   └── STM32F10xxx参考手册（中文）.pdf
├── MDK-ARM/         # Keil 项目文件
└── Readme.md        # 项目说明
```

## 主要功能模块

### 1. 系统初始化
- 时钟配置（72MHz）
- GPIO 初始化
- UART 初始化
- FreeRTOS 初始化

### 2. 任务管理
- **LED 任务**：500ms 闪烁一次，指示系统运行状态
- **UART 任务**：1s 发送一次状态信息，用于调试

### 3. 通信模块
- USART1 串口（115200bps）
- 支持 printf 重定向

### 4. 计划功能
- ADC 采样（电位器）
- PWM 生成（舵机控制）
- 电机驱动控制

## 编译与烧录

1. **打开项目**：使用 Keil MDK 打开 `MDK-ARM/MotorGateway.uvprojx`
2. **编译**：点击 Build 按钮编译项目
3. **烧录**：使用 ST-LINK V2 烧录到 STM32F103C8T6

## 调试说明

- **LED 状态**：PC13 闪烁表示系统运行正常
- **串口输出**：通过 PA9/PA10 连接 USB 转 TTL 模块，使用串口助手查看输出
- **调试工具**：可使用逻辑分析仪监控串口波形

## 常见问题

- **ST-LINK 连接失败**：尝试复位状态连接
- **串口无输出**：检查 PA9/PA10 连接
- **LED 不闪烁**：检查任务调度和 GPIO 配置

## 学习资源

- **参考手册**：STM32F103 Reference Manual (RM0008)
- **HAL 库文档**：STM32F1 HAL 库用户手册
- **FreeRTOS 文档**：FreeRTOS 官方文档

## 版本历史

- **v1.0.0**：基础框架搭建，实现 LED 闪烁和 UART 通信

## 许可证

本项目采用 MIT 许可证，详见 LICENSE 文件。

## 联系方式

- **作者**：[您的姓名]
- **邮箱**：[您的邮箱]
- **日期**：2026-04-14