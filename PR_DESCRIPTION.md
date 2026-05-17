# PR #4: refactor(config): 消除魔法数字与配置规范化重构

## 变更概述
完成第一步"基石层"改造，消除 Modbus 协议层硬编码索引（魔法数字），实现配置文件职责分离，提升代码可读性和可维护性。

## 核心变更

### 1. Modbus 协议层 - 消除硬编码索引
- 在 [modbus.h](file://d:\STM32\myproject\ModbusRTU_Gateway\App\Protocol\Inc\modbus.h) 中新增 22 个帧字段索引宏
- 替换 [modbus.c](file://d:\STM32\myproject\ModbusRTU_Gateway\App\Protocol\Src\modbus.c) 中所有 `frame[2]`, `modbus_tx_buffer[0]` 等硬编码索引
- 验证结果：`grep "frame\[\d+\]" modbus.c` → 0 matches

### 2. 配置文件职责分离
- **[uart.h](file://d:\STM32\myproject\ModbusRTU_Gateway\Bsp\Inc\uart.h)**: UART BSP 层配置（硬件常量 + 驱动业务参数）
- **[driver_servo.h](file://d:\STM32\myproject\ModbusRTU_Gateway\App\Driver\Inc\driver_servo.h)**: 舵机驱动配置（角度/速度范围）
- **[error_handler.h](file://d:\STM32\myproject\ModbusRTU_Gateway\App\System\Inc\error_handler.h)**: 错误处理配置（LED 闪烁、日志超时）
- **[system_config.h](file://d:\STM32\myproject\ModbusRTU_Gateway\App\System\Inc\system_config.h)**: 精简至 40 行，仅保留系统级配置（任务栈、优先级、看门狗）
- 删除未使用的测试宏（`TEST_LED_*` 系列）

### 3. 架构规范优化
- 遵循分层依赖规则：BSP 层不依赖 Driver/App 层
- 引脚命名规范化：`BSP_UART_TX_PIN` → `BSP_UART1_TX_PIN`
- 预留扩展接口：注释 USART2 配置（PA2/PA3）

## 测试验证

| 验证项 | 状态 | 结论 |
|--------|------|------|
| 编译验证 | ✅ | 无错误，无警告 |
| 烧录验证 | ✅ | ST-Link 烧录成功 |
| 功能测试 | ✅ | Modbus Poll 读写正常，舵机控制正常 |
| 宏定义完整性 | ✅ | 硬编码索引清零 |

## 注意事项
1. 初期尝试将 UART 配置移至 [driver_uart.h](file://d:\STM32\myproject\ModbusRTU_Gateway\App\Driver\Inc\driver_uart.h)，违反分层依赖规则，已纠正
2. 文档同步更新：[日志记录.md](file://d:\STM32\myproject\ModbusRTU_Gateway\docs\日志记录.md)、[问题记录.md](file://d:\STM32\myproject\ModbusRTU_Gateway\docs\问题记录.md)、[协议说明.md](file://d:\STM32\myproject\ModbusRTU_Gateway\docs\协议说明.md)

## 关联文档
- [日志记录.md](file://d:\STM32\myproject\ModbusRTU_Gateway\docs\日志记录.md): 2026-05-14 条目
- [问题记录.md](file://d:\STM32\myproject\ModbusRTU_Gateway\docs\问题记录.md): P2-010, P2-011, P2-012
- [协议说明.md](file://d:\STM32\myproject\ModbusRTU_Gateway\docs\协议说明.md): 第 10 章 配置管理规范

## 更改统计
- 修改文件: 7 个
- 新增行数: ~150 行
- 删除行数: ~100 行
- 净变化: +50 行
