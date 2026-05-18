# 代码注释规范

> 版本：v1.3 | 最后更新：2026-05-17

---

## 一、核心原则

### 三个"从不"和一个"总是"

1. **从不**翻译代码（严禁 `// 定义变量`、`// 循环`、`// 返回 true`）
2. **从不**中英混杂（全部中文，变量名和函数名保留原文）
3. **从不**在 `.c` 里重复 `.h` 的文档块
4. **总是**解释"为什么这么做"和"调用者需要注意什么"

### 三秒判断法

如果需要超过三秒才能理解这段代码的意图，就必须加注释。

---

## 二、注释写在哪里

| 内容 | 位置 | 说明 |
|------|------|------|
| 公共 API（非 static 函数） | `.h` 声明处 | 完整文档块，给调用者看 |
| static 函数 | `.c` 定义处 | 按复杂度分级注释，给维护者看 |
| 公共函数在 `.c` 的定义 | 不写或只写一行 | 不要重复 `.h` 的文档块 |

---

## 三、函数注释分级

### L1：公共 API（完整文档块）

**适用**：所有非 static 函数、中断服务函数。

**写在**：`.h` 文件声明处。

**格式**：

```c
/**
 * @brief   {动词+对象，如：写入保持寄存器}
 * @param   {参数名} {用途及合法范围。没有参数就删掉这一行}
 * @return  {返回值含义，必须列出所有可能情况}
 * @warning {调用限制。没有特殊限制就删掉这一行}
 */
```

**示例**：

```c
/**
 * @brief   写入保持寄存器，值变化时自动触发回调
 * @param   addr 寄存器地址 [0, MODBUS_REG_MAX_COUNT-1]
 * @param   value 待写入值
 * @return  true 写入成功，false 失败（地址越界、寄存器只读或锁超时）
 * @warning 非中断安全，必须在任务中调用
 */
bool Modbus_WriteHoldingRegister(uint16_t addr, uint16_t value);
```

---

### L2：复杂内部函数（设计意图）

**适用**：static 函数，包含状态机、协议解析、复杂算法、性能权衡、或非直观的边界处理。

**写在**：`.c` 文件定义处。

**格式**：

```c
// {为什么选择这个方案。有参考文档必须标注}
```

**示例**：

```c
// CRC16 采用直接计算法而非查表法，节省 Flash 512 字节，9600bps 下每帧耗时小于 1ms
// 参考：Modbus Application Protocol V1.1b 第 6 章
static uint16_t CalcCRC16(const uint8_t* data, uint16_t len)
{
    // ...
}
```

---

### L3：简单内部函数（一句话）

**适用**：static 函数，逻辑简单（getter、setter、初始化、参数检查、回调）。

**写在**：`.c` 文件定义处。

**格式**：

```c
// {一句话说明功能}
```

**示例**：

```c
// 确保互斥锁已创建，重复调用不会出错
static void Modbus_EnsureRegisterMutex(void)
{
    // ...
}
```

---

### L4：可以不写注释

**适用**：同时满足以下三个条件：

- 函数名已经能准确描述功能
- 函数体不超过 15 行
- 没有隐藏的副作用

---

## 四、宏定义注释

### 基本规则

- 每个宏必须写行尾注释，说明约束条件和取值原因
- 必须含单位（如有）：毫秒、字节、百分比等
- 严禁使用 `/**< */` 格式，统一用 `//`

### 四类分组

```c
// ---- 配置宏（可修改）----
#define MODBUS_SLAVE_ADDR              (1U)    // 从机地址，可通过拨码开关覆盖

// ---- 协议常量（不可修改）----
#define MODBUS_FUNC_READ_HOLDING_REGS  (0x03U) // 功能码：读保持寄存器

// ---- 可调参数（性能调优）----
#define MODBUS_POLL_INTERVAL           (10U)   // 轮询间隔(ms)，须匹配主机查询周期

// ---- 地址映射表（与硬件图同步维护）----
#define MODBUS_REG_ADDR_DEVICE_STATUS  (3U)    // 系统状态：bit0=心跳，bit1=故障
```

### 对齐要求

同类宏的行尾注释对齐到同一列。

---

## 五、全局变量注释

每个全局变量（包括 static 全局变量）必须说明三件事：

- 用途
- 哪些任务或中断会访问它
- 有没有锁保护

越是生僻的类型（如 `osMutexId`），越要写清楚它是什么、什么时候初始化。

**示例**：

```c
static uint8_t rx_buf[256];       // 接收缓冲区，UART 中断写入，任务中读取
static osMutexId s_mutex = NULL;  // 寄存器数组保护锁，Modbus_Init 中创建
```

---


## 六、结构体成员注释

每个成员行尾用简短说明：

```c
typedef struct {
    uint16_t value;      // 当前寄存器值
    bool     read_only;  // 只读标志，为 true 时拒绝写操作
    void    *callback;   // 值变化回调函数，NULL 表示无回调
} ModbusRegister_t;
```

---
## 七、函数内部注释

### 长函数划分段落

超过 15 行的函数，用编号注释分成逻辑块：

```c
void Modbus_Init(void)
{
    // 1. 初始化同步原语
    EnsureMutex();

    // 2. 初始化寄存器为安全值
    for (int i = 0; i < MODBUS_REG_MAX_COUNT; i++) {
        regs[i].value = 0;
    }

    // 3. 绑定外设回调
    Modbus_RegisterOnChange(ADDR_SERVO, OnServoChanged);

    // 4. 初始化硬件
    Servo_Driver_Init();
}
```

### 非直观操作加行尾注释

```c
tx_buf[2] = count * 2;               // 数据字节数 = 寄存器数 × 2
crc = CalcCRC16(tx_buf, data_len);   // 小端序追加到帧尾
```

---

## 八、提交前自查清单

- [ ] 每个宏有注释（含单位/约束条件）
- [ ] 每个全局变量有访问规则说明
- [ ] `.h` 公共 API 有完整文档块
- [ ] `.c` 无重复 `.h` 的文档块
- [ ] 无 `/**< */` 混用
- [ ] 无废话注释（不翻译代码）
- [ ] 无被注释掉的废弃代码

---

## 附录：示例对比

### 反面示例
```c
#define BAUD (115200U)                   
static osMutexId s_mutex = NULL;
#define MODBUS_SLAVE_ADDR (1U) /**< 地址 */

void Modbus_Process(void) {
    /**
     * @brief   周期性处理
     */
}
```

### 正面示例

```c
#define MODBUS_BAUD_RATE (115200U)   // 主机PLC固定波特率，从站必须一致，勿改
static osMutexId s_mutex = NULL;     // 寄存器数组保护锁，Modbus_Init中创建
#define MODBUS_SLAVE_ADDR (1U)       // 从机地址，可通过拨码开关覆盖

// 公共API .c 定义处不写注释，见 .h 声明
void Modbus_Process(void) { ... }

// CRC16采用直接计算法而非查表法，节省Flash 512B，9600bps下耗时<1ms
static uint16_t CalcCRC16(...) { ... }

// 确保互斥锁已创建，幂等操作
static void EnsureMutex(void) { ... }
```
