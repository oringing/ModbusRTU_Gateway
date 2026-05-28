// Bsp/Src/soft_i2c.c
#include "soft_i2c.h"
#include "stm32f1xx_hal.h"

// 基于逻辑分析仪实测校准：72MHz 主频下，该循环约 5μs
// 满足 I2C 100kHz 标准要求（tLOW≥4.7μs，tHIGH≥4.0μs）
static void Soft_I2C_Delay(void) {
    for (volatile uint32_t i = 0; i < 8; i++) {
        __NOP(); // 防止编译器优化，确保精确延时
    }
}

// 全局重试间隔（单位：ms）。值为 0 表示不重试；可通过 Set_I2C_Retry 修改
// 访问规则：BSP 初始化或任务上下文可修改，ISR 中请勿修改
static unsigned short RETRY_IN_MLSEC = I2C_DEFAULT_RETRY_MS; // 默认重试间隔 55ms

void Set_I2C_Retry(unsigned short ml_sec) {
    RETRY_IN_MLSEC = ml_sec;
}

unsigned short Get_I2C_Retry(void) {
    return RETRY_IN_MLSEC;
}

// 将 SDA/SCL 配置为开漏输出，总线空闲时外部 4.7kΩ 电阻上拉至高电平
void I2C_Bus_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = SOFT_I2C_SCL | SOFT_I2C_SDA;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SOFT_I2C_PORT, &GPIO_InitStruct);

    SOFT_I2C_SCL_1;
    SOFT_I2C_SDA_1;
    Soft_I2C_Delay();
}

// 生成 I2C 起始条件（SDA 在 SCL 为高时从高->低）
// 检测总线忙态，避免在总线被占用时强行拉低导致冲突
static uint8_t Soft_I2C_Start(void) {
    SOFT_I2C_SDA_1;
    Soft_I2C_Delay();

    SOFT_I2C_SCL_1;
    Soft_I2C_Delay();

    // 检查总线是否被其他设备占用（SDA 应为高电平）
    if (!SOFT_I2C_SDA_READ) {
        return 1; // 总线忙
    }

    SOFT_I2C_SDA_0;
    Soft_I2C_Delay();

    SOFT_I2C_SCL_0;
    Soft_I2C_Delay();

    return 0;
}

// 生成 I2C 停止条件（SDA 在 SCL 高时从低->高）
static void Soft_I2C_Stop(void) {
    SOFT_I2C_SDA_0;
    Soft_I2C_Delay();

    SOFT_I2C_SCL_1;
    Soft_I2C_Delay();

    SOFT_I2C_SDA_1;
    Soft_I2C_Delay();
}

// 主机发送 ACK（拉低 SDA），在多字节读取中确认继续读取下一个字节
static void Soft_I2C_SendAck(void) {
    SOFT_I2C_SDA_0;
    Soft_I2C_Delay();
    SOFT_I2C_SCL_1;
    Soft_I2C_Delay();
    SOFT_I2C_SCL_0;
    Soft_I2C_Delay();
}

// 主机发送 NACK（释放 SDA，由外部上拉至高电平）
// 读取最后一个字节后通知从机停止发送
static void Soft_I2C_SendNack(void) {
    SOFT_I2C_SDA_1;
    Soft_I2C_Delay();
    SOFT_I2C_SCL_1;
    Soft_I2C_Delay();
    SOFT_I2C_SCL_0;
    Soft_I2C_Delay();
}

// 等待从机应答（第 9 个时钟周期读取 SDA）
// 通过轮询检测 ACK，超时由 I2C_WAIT_ACK_MAX_RETRY 控制
static uint8_t Soft_I2C_WaitAck(void) {
    uint8_t timeout = 0;

    SOFT_I2C_SDA_1;
    Soft_I2C_Delay();
    SOFT_I2C_SCL_1;
    Soft_I2C_Delay();

    while (SOFT_I2C_SDA_READ) {
        timeout++;
        if (timeout > I2C_WAIT_ACK_MAX_RETRY) {
            Soft_I2C_Stop();
            return 2; // 无应答超时
        }
    }

    SOFT_I2C_SCL_0;
    Soft_I2C_Delay();
    return 0;
}

// 发送一个字节（MSB 优先），返回从机应答状态（0=ACK，非0=错误/超时）
static uint8_t Soft_I2C_SendByte(uint8_t data) {
    uint8_t i;

    SOFT_I2C_SCL_0;
    for (i = 0; i < 8; i++) {
        if (data & 0x80) {
            SOFT_I2C_SDA_1;
        } else {
            SOFT_I2C_SDA_0;
        }
        data <<= 1;
        Soft_I2C_Delay();

        SOFT_I2C_SCL_1;
        Soft_I2C_Delay();
        SOFT_I2C_SCL_0;
        Soft_I2C_Delay();
    }
    return Soft_I2C_WaitAck();
}

// 接收一个字节并发送 NACK（用于读取最后一个字节）
// 逐位采样 SDA，并在结束后发送 NACK
static uint8_t Soft_I2C_ReceiveByte(void) {
    uint8_t i, data = 0;

    SOFT_I2C_SDA_1;
    SOFT_I2C_SCL_0;

    for (i = 0; i < 8; i++) {
        SOFT_I2C_SCL_1;
        Soft_I2C_Delay();
        data <<= 1;
        if (SOFT_I2C_SDA_READ) {
            data |= 0x01;
        }
        SOFT_I2C_SCL_0;
        Soft_I2C_Delay();
    }
    Soft_I2C_SendNack();
    return data;
}

// 接收一个字节并发送 ACK（用于读取非最后一个字节）
static uint8_t Soft_I2C_ReceiveByteWithAck(void) {
    uint8_t i, data = 0;

    SOFT_I2C_SDA_1;
    SOFT_I2C_SCL_0;

    for (i = 0; i < 8; i++) {
        SOFT_I2C_SCL_1;
        Soft_I2C_Delay();
        data <<= 1;
        if (SOFT_I2C_SDA_READ) {
            data |= 0x01;
        }
        SOFT_I2C_SCL_0;
        Soft_I2C_Delay();
    }
    Soft_I2C_SendAck();
    return data;
}

// ========== 寄存器式传感器接口 ==========

// 写寄存器：START + 写地址 + 寄存器地址 + 数据 + STOP
static uint8_t Soft_I2C_WriteReg(uint8_t dev_addr, uint8_t reg_addr, uint16_t len,
                                 const uint8_t* data) {
    uint8_t i, result;

    result = Soft_I2C_Start();
    if (result)
        return result;

    result = Soft_I2C_SendByte((dev_addr << 1) | 0);
    if (result) {
        Soft_I2C_Stop();
        return result;
    }

    result = Soft_I2C_SendByte(reg_addr);
    if (result) {
        Soft_I2C_Stop();
        return result;
    }

    for (i = 0; i < len; i++) {
        result = Soft_I2C_SendByte(data[i]);
        if (result) {
            Soft_I2C_Stop();
            return result;
        }
    }

    Soft_I2C_Stop();
    return 0;
}

// 读寄存器：START + 写地址 + 寄存器地址 + RESTART + 读地址 + 数据 + STOP
static uint8_t Soft_I2C_ReadReg(uint8_t dev_addr, uint8_t reg_addr, uint16_t len, uint8_t* data) {
    uint8_t result;

    result = Soft_I2C_Start();
    if (result)
        return result;

    result = Soft_I2C_SendByte((dev_addr << 1) | 0);
    if (result) {
        Soft_I2C_Stop();
        return result;
    }

    result = Soft_I2C_SendByte(reg_addr);
    if (result) {
        Soft_I2C_Stop();
        return result;
    }

    result = Soft_I2C_Start();
    if (result) {
        Soft_I2C_Stop();
        return result;
    }

    result = Soft_I2C_SendByte((dev_addr << 1) | 1);
    if (result) {
        Soft_I2C_Stop();
        return result;
    }

    while (len) {
        if (len == 1) {
            *data = Soft_I2C_ReceiveByte();
        } else {
            *data = Soft_I2C_ReceiveByteWithAck();
        }
        data++;
        len--;
    }

    Soft_I2C_Stop();
    return 0;
}

bool Sensors_I2C_WriteRegister(unsigned char slave_addr, unsigned char reg_addr, unsigned short len,
                               const unsigned char* data_ptr) {
    bool           ret;
    unsigned short retry = Get_I2C_Retry();

    do {
        ret = (Soft_I2C_WriteReg(slave_addr, reg_addr, len, data_ptr) == 0);
        if (ret)
            break;
        HAL_Delay(retry);
    } while (retry);

    return ret;
}

bool Sensors_I2C_ReadRegister(unsigned char slave_addr, unsigned char reg_addr, unsigned short len,
                              unsigned char* data_ptr) {
    bool           ret;
    unsigned short retry = Get_I2C_Retry();

    do {
        ret = (Soft_I2C_ReadReg(slave_addr, reg_addr, len, data_ptr) == 0);
        if (ret)
            break;
        HAL_Delay(retry);
    } while (retry);

    return ret;
}

// ========== 命令式传感器接口 ==========

// 只写命令：START + 写地址 + 命令字节 + STOP（无寄存器地址）
static uint8_t Soft_I2C_WriteCmd(uint8_t dev_addr, uint16_t len, const uint8_t* data) {
    uint8_t i, result;

    result = Soft_I2C_Start();
    if (result)
        return result;

    result = Soft_I2C_SendByte((dev_addr << 1) | 0);
    if (result) {
        Soft_I2C_Stop();
        return result;
    }

    for (i = 0; i < len; i++) {
        result = Soft_I2C_SendByte(data[i]);
        if (result) {
            Soft_I2C_Stop();
            return result;
        }
    }

    Soft_I2C_Stop();
    return 0;
}

// 写命令字节到指定从机地址
static uint8_t Soft_I2C_WriteAddrAndCmd(uint8_t dev_addr, uint8_t cmd) {
    uint8_t result;

    result = Soft_I2C_Start();
    if (result)
        return result;

    result = Soft_I2C_SendByte((dev_addr << 1) | 0);
    if (result) {
        Soft_I2C_Stop();
        return result;
    }

    result = Soft_I2C_SendByte(cmd);
    if (result) {
        Soft_I2C_Stop();
        return result;
    }

    return 0;
}

// 重启总线并读取数据：RESTART + 读地址 + 数据 + STOP
static uint8_t Soft_I2C_RestartAndRead(uint8_t dev_addr, uint16_t len, uint8_t* data) {
    uint8_t result;

    result = Soft_I2C_Start();
    if (result) {
        Soft_I2C_Stop();
        return result;
    }

    result = Soft_I2C_SendByte((dev_addr << 1) | 1);
    if (result) {
        Soft_I2C_Stop();
        return result;
    }

    while (len) {
        if (len == 1) {
            *data = Soft_I2C_ReceiveByte();
        } else {
            *data = Soft_I2C_ReceiveByteWithAck();
        }
        data++;
        len--;
    }

    Soft_I2C_Stop();
    return 0;
}

// 组合操作：写命令后立即重启读取数据（AHT20 专用流程）
static uint8_t Soft_I2C_ReadCmdData(uint8_t dev_addr, uint8_t cmd, uint16_t len, uint8_t* data) {
    uint8_t result;

    result = Soft_I2C_WriteAddrAndCmd(dev_addr, cmd);
    if (result)
        return result;

    result = Soft_I2C_RestartAndRead(dev_addr, len, data);
    return result;
}

bool Sensors_I2C_WriteCommand(unsigned char slave_addr, const unsigned char* data,
                              unsigned short len) {
    bool           ret;
    unsigned short retry = Get_I2C_Retry();

    if (data == NULL || len == 0)
        return false;

    do {
        ret = (Soft_I2C_WriteCmd(slave_addr, len, data) == 0);
        if (ret)
            break;
        HAL_Delay(retry);
    } while (retry);

    return ret;
}

bool Sensors_I2C_ReadCommandData(unsigned char slave_addr, unsigned char cmd, unsigned char* data,
                                 unsigned short len) {
    bool           ret;
    unsigned short retry = Get_I2C_Retry();

    do {
        ret = (Soft_I2C_ReadCmdData(slave_addr, cmd, len, data) == 0);
        if (ret)
            break;
        HAL_Delay(retry);
    } while (retry);

    return ret;
}
