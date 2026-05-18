# Changelog

所有重要变更都将记录在此文件中。

格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.0.0/)，
项目遵循 [语义化版本](https://semver.org/lang/zh-CN/)。

## [Unreleased]

### Added
- 自动化测试脚本 `scripts/test_modbus.py`（待实现）
- GitHub Actions CI 配置（待实现）

### Changed
- 优化任务栈配置：UART 从 128→96 words，Monitor 从 240→160 words，节省 448 bytes RAM
- 调整栈水位检查间隔单位为秒（更直观）
- 更新硬件实物图与接线示意图（RS485 模块 / 舵机 PWM / I2C 传感器）

### Fixed
- 修正 README 中栈水位描述错误（剩余量 vs 使用量）
- 修复文档中 Modbus 异常码返回机制的表述不准确问题
- **P0-003** Monitor 任务停止标志缺失：添加 `volatile uint8_t s_monitor_running` 标志位，修复任务退出逻辑
- **P2-007/P2-010** 阻塞延时替换：`HAL_Delay()` → `osDelay()`，释放 CPU 给其他任务，避免看门狗超时
- **P2-009** 配置校验增强：新增栈大小/优先级/看门狗参数校验，非法值触发 Error_Handler
- **P2-011** 修正测试指令 CRC16 校验位错误

## [1.5.0] - 2026-05-10

### Added
- UART 接收链背靠背请求帧丢失防护机制
- 独立看门狗（IWDG）故障自恢复机制
- 任务栈水位监控与低水位告警
- Modbus RTU 协议层入参校验（帧长度、从机地址、CRC）
- 寄存器访问边界保护与互斥锁机制
- 错误日志限流机制

### Changed
- 优化 ORE/FE/NE/PE 错误分级恢复策略
- 调整 LSI 时钟开启顺序与看门狗重载值计算
- 重构 Monitor_Task 栈占用，减少 snprintf 局部变量压力

### Fixed
- 修复 HAL 库 UART 状态机在奇偶交替帧时的同步问题
- 修正 CRC 校验字节序处理逻辑
- 修复 DMA 循环模式配置错误导致的接收中断问题

### Removed
- 移除 BSP 层硬编码的发送超时参数，统一由协议层管理

## [1.0.0] - 2026-04-XX

### Added
- 初始版本：基于 STM32F103C8T6 + FreeRTOS 的 Modbus RTU 从机网关
- 支持功能码 0x03（读保持寄存器）和 0x06（写单个寄存器）
- RS485 硬件接口（MAX485）驱动
- LED 状态指示任务
- 基础看门狗机制

---

