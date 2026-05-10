/**
 * @file test_validation.c
 * @brief Modbus 参数校验单元测试
 * //测试代码 begin：本测试文件仅用于验证 P1-001 参数校验修复，不应提交到主干分支
 */


#include "modbus.h"
#include "system_config.h"
#include "led.h"
#include "stm32f1xx_hal.h" // 包含 HAL_Delay

/* 自定义断言宏：测试失败时 LED 常亮停机 */
#ifndef TEST_ASSERT_EQUAL
#define TEST_ASSERT_EQUAL(expected, actual) do { \
    if((expected) != (actual)) { \
        BSP_LED_On(); /* 测试失败：LED 常亮 */ \
        while(1);     /* 停机以便调试 */ \
    } \
} while(0)
#endif

#ifndef TEST_ASSERT_TRUE
#define TEST_ASSERT_TRUE(condition) do { \
    if(!(condition)) { \
        BSP_LED_On(); \
        while(1); \
    } \
} while(0)
#endif

#ifndef TEST_ASSERT_FALSE
#define TEST_ASSERT_FALSE(condition) do { \
    if(condition) { \
        BSP_LED_On(); \
        while(1); \
    } \
} while(0)
#endif

/**
 * @brief 测试 Modbus_ReadHoldingRegister 空指针防护
 */
void test_modbus_read_null_pointer(void) {
    /* 传入 NULL 指针，应返回 false */
    bool result = Modbus_ReadHoldingRegister(0, NULL);
    TEST_ASSERT_FALSE(result);
}

/**
 * @brief 测试 Modbus_ReadHoldingRegister 地址越界
 */
void test_modbus_read_address_out_of_bounds(void) {
    uint16_t value;
    
    /* 地址等于 MODBUS_REG_MAX_COUNT，应返回 false */
    bool result = Modbus_ReadHoldingRegister(MODBUS_REG_MAX_COUNT, &value);
    TEST_ASSERT_FALSE(result);
    
    /* 地址超出范围，应返回 false */
    result = Modbus_ReadHoldingRegister(0xFFFF, &value);
    TEST_ASSERT_FALSE(result);
}

/**
 * @brief 测试 Modbus_ReadHoldingRegister 正常读取
 */
void test_modbus_read_success(void) {
    uint16_t value;
    
    /* 合法地址 0，应返回 true */
    bool result = Modbus_ReadHoldingRegister(0, &value);
    TEST_ASSERT_TRUE(result);
}

/**
 * @brief 测试 Modbus_WriteHoldingRegister 地址越界
 */
void test_modbus_write_address_out_of_bounds(void) {
    /* 地址越界，应返回 false */
    bool result = Modbus_WriteHoldingRegister(MODBUS_REG_MAX_COUNT, 0x1234);
    TEST_ASSERT_FALSE(result);
    
    result = Modbus_WriteHoldingRegister(0xFFFF, 0x1234);
    TEST_ASSERT_FALSE(result);
}

/**
 * @brief 测试 Modbus_WriteHoldingRegister 只读寄存器保护
 * @note 当前实现中，所有寄存器默认都是可写的，需要手动设置 read_only 标志
 */
void test_modbus_write_readonly_register(void) {
    /* 先写入一个值作为基准 */
    Modbus_WriteHoldingRegister(0, 0x1234);
    
    /* 当前代码未实现 read_only 标志的设置接口，此测试暂时跳过 */
    /* TODO: 添加 Modbus_SetReadOnlyFlag 接口后再启用此测试 */
}

/**
 * @brief 测试 Modbus_WriteHoldingRegister 正常写入
 */
void test_modbus_write_success(void) {
    uint16_t read_value;
    
    /* 写入合法值到地址 4（舵机目标角度） */
    bool result = Modbus_WriteHoldingRegister(4, 90);
    TEST_ASSERT_TRUE(result);
    
    /* 读取验证 */
    result = Modbus_ReadHoldingRegister(4, &read_value);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(90, read_value);
}

/**
 * @brief 测试 Modbus_RegisterOnChange 地址越界
 */
void test_modbus_register_callback_out_of_bounds(void) {
    /* 地址越界，应返回 false */
    bool result = Modbus_RegisterOnChange(MODBUS_REG_MAX_COUNT, NULL);
    TEST_ASSERT_FALSE(result);
}

/**
 * @brief 运行所有参数校验测试
 * //测试代码 begin：此函数必须在 FreeRTOS 调度器启动后调用（即在某个任务中执行）
 * @note 测试通过时 LED 快速闪烁 3 次，失败则常亮停机
 * @note Debug 模式：每个测试用例前 LED 闪烁次数不同，便于定位失败点
 */
void Run_Param_Validation_Test(void) {
    /* Debug: 第1个测试前闪1次 */
    BSP_LED_On(); HAL_Delay(50); BSP_LED_Off(); HAL_Delay(200);
    
    test_modbus_read_null_pointer();
    
    /* Debug: 第2个测试前闪2次 */
    BSP_LED_On(); HAL_Delay(50); BSP_LED_Off(); HAL_Delay(100);
    BSP_LED_On(); HAL_Delay(50); BSP_LED_Off(); HAL_Delay(200);
    
    test_modbus_read_address_out_of_bounds();
    
    /* Debug: 第3个测试前闪3次 */
    for(int i = 0; i < 3; i++) {
        BSP_LED_On(); HAL_Delay(50); BSP_LED_Off(); HAL_Delay(100);
    }
    HAL_Delay(200);
    
    test_modbus_read_success();
    
    /* Debug: 第4个测试前闪4次 */
    for(int i = 0; i < 4; i++) {
        BSP_LED_On(); HAL_Delay(50); BSP_LED_Off(); HAL_Delay(100);
    }
    HAL_Delay(200);
    
    test_modbus_write_address_out_of_bounds();
    
    /* Debug: 第5个测试前闪5次 */
    for(int i = 0; i < 5; i++) {
        BSP_LED_On(); HAL_Delay(50); BSP_LED_Off(); HAL_Delay(100);
    }
    HAL_Delay(200);
    
    test_modbus_write_readonly_register();
    
    /* Debug: 第6个测试前闪6次 */
    for(int i = 0; i < 6; i++) {
        BSP_LED_On(); HAL_Delay(50); BSP_LED_Off(); HAL_Delay(100);
    }
    HAL_Delay(200);
    
    test_modbus_write_success();
    
    /* Debug: 第7个测试前闪7次 */
    for(int i = 0; i < 7; i++) {
        BSP_LED_On(); HAL_Delay(50); BSP_LED_Off(); HAL_Delay(100);
    }
    HAL_Delay(200);
    
    test_modbus_register_callback_out_of_bounds();
    
    /* 所有测试通过：LED 快速闪烁 3 次（50ms 间隔，明显区别于正常 500ms） */
    for(int i = 0; i < 3; i++) {
        BSP_LED_On();   // 亮
        HAL_Delay(50);  // 50ms
        BSP_LED_Off();  // 灭
        HAL_Delay(50);  // 50ms
    }
    // 额外停顿 500ms，让用户看清结果
    HAL_Delay(500);
}
