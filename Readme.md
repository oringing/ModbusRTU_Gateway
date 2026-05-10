# Modbus RTU 网关（STM32F103C8T6）

> **版本**: v1.2.0 | **许可证**: MIT  
> 基于 STM32F1 + FreeRTOS 的工业级 Modbus RTU 协议从机网关，具备高稳定性 UART 接收链和故障自恢复机制。

## 🔍 技术亮点

### ✅  UART 接收链防护

- **背靠背请求帧丢失防护**：通过空闲中断（IDLE）+ 双缓冲 + HAL 状态机同步重绑，消除奇偶交替响应故障
- **错误分级恢复机制**：ORE 立即恢复、FE/NE 连续 3 次触发恢复、PE 连续 5 次触发恢复，避免误判导致频繁重启

### ✅ 故障预警处理

- **独立看门狗（IWDG）**：2 秒超时复位，修正 LSI 时钟开启与重载值计算陷阱
- **栈水位监控**：周期性检测任务栈高水位，低水位告警与恢复通知机制
- **安全模式停机**：配置校验 fail-fast，LED 编码指示 HardFault/MemManage/BusFault 等严重故障

### ✅ 协议层防护

- **协议层入参校验**：在 `Modbus_Process()` 入口处校验帧长度、从机地址、CRC，非法帧直接丢弃
- **寄存器访问边界保护**：读/写请求中校验起始地址与数量组合，防止数组越界和整数溢出
- **寄存器互斥保护**：单次加锁快照读取，写回调锁外执行，避免并发一致性风险
- **日志限流机制**：错误日志按频率控制输出，避免阻塞主流程

## 🛠️ 技术栈

- **MCU**: STM32F103C8T6 (ARM Cortex-M3, 72MHz)
- **RTOS**: FreeRTOS v10.3.1 (CMSIS-RTOS v1 接口)
- **通信协议**: Modbus RTU over RS485 (MAX485)
- **固件库**: STM32 HAL (F1)
- **工具链**: STM32CubeMX + Keil MDK v5 + VS Code

## 📊 性能指标

| 指标     | 数值                                 |
| ------ | ---------------------------------- |
| 支持波特率  | 115200 bps                         |
| 最大帧长度  | 256 字节                             |
| 平均响应延迟 | < 1ms（典型值）                         |
| 看门狗超时  | 2 秒（LSI 40kHz / 预分频 256 / 重载值 312） |
| 内存占用   | \~10KB RAM (FreeRTOS heap)         |

## 📁 项目结构

```
ModbusRTU_Gateway/
├── Core/                 # 启动、时钟、中断、main
├── Bsp/                  # 硬件抽象层（UART/LED/KEY/EXTI）
├── App/                  # 应用层（Modbus 从机）
│   ├── Driver/           # OS 相关驱动封装（UART 互斥发送）
│   ├── Protocol/         # Modbus 协议处理（0x03/0x06）
│   ├── System/           # 系统控制、错误处理、配置校验
│   └── Task/             # LED/UART/Monitor 任务
├── docs/                 # 技术文档与记录
├── Drivers/              # HAL 库文件
├── MDK-ARM/              # Keil 工程文件
├── Middlewares/          # FreeRTOS 内核文件
└── Release/              # 当前版本预编译hex文件
```

***

## 🔌 硬件接线与图片

### 1. 系统整体与主从硬件

![系统整体图](docs/images/整体图V1.0.jpg)
![MAX485从机端图解](docs/images/max485从机端图解.jpg)
![MAX485主机硬件图](docs/images/max485主机硬件图.jpg)

### 2. TTL 转 RS485 模块（自动流向控制型）接线说明

| 模块引脚   | 连接 STM32或USB-TTL  | 说明                |
| ------ | ----------------- | ----------------- |
| VCC    | 3.3V              | 模块电源（3.3V/5V兼容）   |
| GND    | GND               | 共地                |
| TXD    | PA9 (USART1\_TX)  | 模块发送接MCU发送        |
| RXD    | PA10 (USART1\_RX) | 模块接收接MCU接收        |
| A      | A+ (主机侧)          | 485总线正极           |
| B      | B- (主机侧)          | 485总线负极           |
| 大地/屏蔽地 | -                 | 接大地端子，用于屏蔽层接地，抗干扰 |

```
STM32F103从机     MAX485模块（从机端）    MAX485模块（主机端）         PC USB转TTL    
┌─────────┐        ┌──────────┐           ┌──────────┐             ┌──────────┐ 
│ PA9  TX ├───────►│ TXD   A+ │◄─────────►│ A+   TXD │◄────────────│ TXD      │
│ PA10 RX │◄───────│ RXD      │           │      RXD │────────────►│ RXD      │
│ 3.3V    ├────────│ VCC   B- │◄─────────►│ B-   VCC │ 无隔离器不接  │ VCC      │
│ GND     ├────────│ GND      │           │      GND │─────────────│ GND      │
└─────────┘        └─────────┬┘           └┬─────────┘             └──────────┘
                           屏蔽地（悬空） 屏蔽地（悬空）

```

- 本模块为自动流向控制型，无需MCU控制RE/DE引脚，操作与普通串口一致。
- MCU的UART串口 或 PC的USB转TTL串口 的 TX 连接 本模块串口侧 的 TXD 引脚，RX同理，否则无法正常通信。

![TTL转RS485模块外形与接线](docs/images/TTL转RS485模块外形与接线.jpg)

### 3. 关键接线要点（务必检查）

- **USART1**：PA9 (TX)、PA10 (RX)
- **RS485**：A/B 极性必须与主机一致
- **电源地**：主从设备与 USB 转串口必须共地
- **抗干扰**：建议增加板级外部上拉/终端电阻，`GPIO_PULLUP` 仅作辅助

***

## ⚙️ 编译与烧录

### 开发环境要求

- **IDE**: Keil MDK v5（需安装 STM32F1 系列支持包）
- **调试器**: ST-LINK V2
- **固件库**: STM32 HAL (F1)
- **可选工具**: VS Code（用于文档编辑与代码浏览）

### 编译步骤

1. 打开工程：`MDK-ARM/ModbusRTU_Gateway.uvprojx`
2. 选择目标并编译（确保无警告和错误）
3. 连接 ST-LINK V2，下载并调试

### 烧录验证

- 可直接使用 [当前版本预编译 hex 文件](Release/ModbusRTU_Gateway.hex) 烧录验证功能
- 或通过编译生成的 hex 文件进行烧录

### 注意事项

- 若编译报错，检查是否已安装 STM32F1 系列 Device Family Pack
- 确保 `docs/images/` 下存在对应图片文件以正常显示文档

***

### 核心模块说明

- **Core/**: MCU 启动入口、时钟配置、中断向量表、FreeRTOS 初始化
- **Bsp/**: 硬件抽象层，封装 GPIO、UART、按键等底层驱动
- **App/Driver/**: 操作系统相关驱动封装（如 UART 互斥发送）
- **App/Protocol/**: Modbus RTU 协议解析与寄存器访问逻辑
- **App/System/**: 系统控制、错误处理、配置校验、看门狗管理
- **App/Task/**: FreeRTOS 任务实现（LED 闪烁、UART 轮询、栈监控）

***

## 🚀 启动流程

1. `main()` 完成 `HAL_Init()` 与时钟初始化
2. 初始化 BSP 与协议：`BSP_LED_Init()`、`BSP_UART_Init()`、`Modbus_Init()`
3. `System_Ctrl_Init()` 做配置校验，失败直接 `Error_Handler()`
4. `System_StartTasks()` 创建 LED/UART/Monitor 任务
5. `osKernelStart()` 启动调度器

***

## 📋 任务与配置

### 当前运行任务

- **LED\_Task**：500ms 翻转 LED（优先级：Low）
- **UART\_Task**：周期调用 `Modbus_Process()`（优先级：Normal）
- **Monitor\_Task**：周期性输出任务栈高水位 + 喂狗（优先级：BelowNormal）

### 主要配置参数

配置文件：[App/System/Inc/system_config.h](App/System/Inc/system_config.h)

| 参数                               | 值          | 说明                                 |
| -------------------------------- | ---------- | ---------------------------------- |
| `LED_TASK_STACK_SIZE`            | 64 words   | 实测峰值 28 words（空闲/高压一致），预留 1.3 倍剩余量 |
| `UART_TASK_STACK_SIZE`           | 125 words  | 实测峰值 54 words（100ms/帧高压），预留 1.3 倍剩余量 |
| `MONITOR_TASK_STACK_SIZE`        | 165 words  | 实测峰值 74 words（空闲/高压一致），预留 1.3 倍剩余量 |
| `STACK_WATERMARK_CHECK_INTERVAL` | 10        | 栈水位检查间隔（单位：秒，实际 = 10 × 1000ms） |
| `SYSTEM_USE_IWDG`                | 1         | 启用硬件看门狗                            |
| `IWDG_RELOAD_VALUE`              | 312       | 看门狗重载值（约 2 秒超时，312 * 256 / 40000 ≈ 2.0）                    |
| `SYSTEM_UART_TEXT_LOG_ENABLE`    | 1         | 开启 USART1 ASCII 日志（用于调试阶段）          |

***

## 📄 技术文档索引

### 核心文档

- [docs/问题记录.md](docs/问题记录.md)：当前已知问题清单与改进计划
- [docs/日志记录.md](docs/日志记录.md)：开发里程碑记录与技术决策


***

## 🧪 快速测试

### 测试环境

- **串口助手**：SSCOM/Putty，HEX 模式，波特率 115200，8N1
- **从机地址**：`1`
- **寄存器映射**：10 个保持寄存器（仅地址 `0x0004` 可写）

### 读保持寄存器（0x03）

```
发送: 01 03 00 00 00 02 C4 0B
响应:（返回寄存器地址 0x00 和 0x01 中的值）
```

### 写单寄存器（0x06）

```
发送: 01 06 00 04 00 5A 8A 8C  （写入地址 4，值 90）
响应: 01 06 00 04 00 5A 8A 8C  （原样回显）
验证: 01 03 00 04 00 01 C5 CB  （读取确认）
```

### 异常情况测试

- **CRC 错帧**：无任何响应（直接丢弃）
- **越界地址**：返回 `01 86 02 XX XX`（异常码 0x02）
- **非法值**：返回 `01 86 03 XX XX`（异常码 0x03）
- **只读寄存器写**：返回 `01 86 03 XX XX`（异常码 0x03）

***

## 🔍 调试与排障要点

### 错误日志格式

```
ERR[类型] 文件:行号
```

类型编码：

- `ERR[1]`: ERROR\_HAL（HAL 调用或底层初始化错误）
- `ERR[2]`: ERROR\_UART（UART 接收链 / 恢复链异常）
- `ERR[4]`: ERROR\_STACK\_OVERFLOW（FreeRTOS 栈溢出）
- `ERR[5]`: ERROR\_SYSTEM（系统控制层错误）
- `ERR[6-9]`: 硬件故障（HardFault/MemManage/BusFault/UsageFault）

### 常见问题排查

1. **LED 常亮且无任务输出**：优先检查是否进入 `System_EnterSafeMode()`
2. **ERR\[2] 高频刷屏**：检查 UART 恢复路径是否误判 `HAL_BUSY`
3. **Modbus 无响应**：
   - 确认 HEX 模式、关闭自动换行
   - 检查主从波特率一致
   - 验证 CRC 校验码正确性
4. **图片不显示**：确认文件已放入 `docs/images/` 且文件名与文档中一致

***

## ⚠️ 已知约束与局限性

### 功能限制

- 当前仅实现 `0x03`（读保持寄存器）和 `0x06`（写单寄存器）功能码
- 寄存器总数：10 个，仅地址 `0x0004` 可写（范围 0\~180），其余为只读占位
- 未实现写多个寄存器（0x10）、线圈/离散输入等功能码

### 测试局限性

- **UART 硬件错误触发**：实验室环境无法可靠触发 ORE/FE/NE 硬件错误标志位，真实工业干扰场景需进一步验证
- **长稳测试**：建议进行 24h+ 连续运行测试，验证任务栈稳定性与内存泄漏情况

### 硬件建议

- 工业现场建议增加板级外部上拉/终端电阻，`GPIO_PULLUP` 仅作辅助
- RS485 A/B 线建议使用双绞线，降低电磁干扰影响

***

## 🎯 下一步演进方向

- 按实际运行数据持续回归任务栈与优先级策略
- 补全协议异常码覆盖、边界帧测试与长稳测试（24h+）
- 接入 DHT11 温湿度传感器与 SG90 舵机，实现外设闭环控制
- 扩展 Modbus 主站功能，实现双 MCU 主从架构

