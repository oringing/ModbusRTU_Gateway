/**
 * @file test_validation.c
 * @brief Modbus 参数校验单元测试
 * @note 本测试文件仅用于验证 P1-001 参数校验修复，不应提交到主干分支
 */

#include "modbus.h"
#include "system_config.h"
#include "led.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>


// 测试专用忙等待延时，关中断安全；72MHz下每毫秒约68000次循环（实测校准值）
// @warning 100%占用CPU，仅用于测试/调试场景
static void Test_BusyDelay(uint32_t ms) {
    volatile uint32_t loops = ms * 68000U;
    while (loops-- != 0U) {
        __NOP();
    }
}

// 测试失败处理：LED常亮并停机
static void Test_OnFail(void) {
    BSP_LED_On();
    while(1);  /* 停机便于调试 */
}

// 测试用LED闪烁，count为次数，on_ms/off_ms为亮灭时间
static void Test_LED_Blink(uint8_t count, uint16_t on_ms, uint16_t off_ms) {
    for(uint8_t i = 0; i < count; i++) {
        BSP_LED_On();
        Test_BusyDelay(on_ms);
        BSP_LED_Off();
        Test_BusyDelay(off_ms);
    }
    Test_BusyDelay(200);  /* 测试间间隔 */
}

// 测试通过指示：3次快闪
static void Test_IndicatePass(void) {
    for(int i = 0; i < 3; i++) {
        BSP_LED_On();
        Test_BusyDelay(50);
        BSP_LED_Off();
        Test_BusyDelay(50);
    }
    Test_BusyDelay(500);
}

/* ==================== 断言宏 ==================== */

#define TEST_ASSERT_EQUAL(expected, actual) do { \
    if((expected) != (actual)) { Test_OnFail(); } \
} while(0)

#define TEST_ASSERT_TRUE(condition) do { \
    if(!(condition)) { Test_OnFail(); } \
} while(0)

#define TEST_ASSERT_FALSE(condition) do { \
    if(condition) { Test_OnFail(); } \
} while(0)

/* ==================== 测试用例 ==================== */

static void test_modbus_read_null_pointer(void) {
    bool result = Modbus_ReadHoldingRegister(0, NULL);
    TEST_ASSERT_FALSE(result);
}

static void test_modbus_read_address_out_of_bounds(void) {
    uint16_t value;
    bool result;
    
    result = Modbus_ReadHoldingRegister(MODBUS_REG_MAX_COUNT, &value);
    TEST_ASSERT_FALSE(result);
    
    result = Modbus_ReadHoldingRegister(0xFFFF, &value);
    TEST_ASSERT_FALSE(result);
}

static void test_modbus_read_success(void) {
    uint16_t value;
    bool result = Modbus_ReadHoldingRegister(0, &value);
    TEST_ASSERT_TRUE(result);
}

static void test_modbus_write_address_out_of_bounds(void) {
    bool result;
    
    result = Modbus_WriteHoldingRegister(MODBUS_REG_MAX_COUNT, 0x1234);
    TEST_ASSERT_FALSE(result);
    
    result = Modbus_WriteHoldingRegister(0xFFFF, 0x1234);
    TEST_ASSERT_FALSE(result);
}

static void test_modbus_write_readonly_register(void) {
    /* TODO: 添加 Modbus_SetReadOnlyFlag 接口后再启用 */
    /* 当前跳过此测试 */
}

static void test_modbus_write_success(void) {
    uint16_t read_value;
    bool result;
    
    result = Modbus_WriteHoldingRegister(4, 90);
    TEST_ASSERT_TRUE(result);
    
    result = Modbus_ReadHoldingRegister(4, &read_value);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(90, read_value);
}

static void test_modbus_register_callback_out_of_bounds(void) {
    bool result = Modbus_RegisterOnChange(MODBUS_REG_MAX_COUNT, NULL);
    TEST_ASSERT_FALSE(result);
}

/* ==================== 测试用例表 ==================== */

typedef struct {
    const char* name;
    void (*func)(void);
    uint8_t blink_count;
} TestCase_t;

static const TestCase_t g_test_cases[] = {
    {"null_pointer",        test_modbus_read_null_pointer,          1},
    {"addr_out_of_bounds",  test_modbus_read_address_out_of_bounds, 2},
    {"read_success",        test_modbus_read_success,               3},
    {"write_addr_invalid",  test_modbus_write_address_out_of_bounds,4},
    {"write_readonly",      test_modbus_write_readonly_register,    5},
    {"write_success",       test_modbus_write_success,              6},
    {"callback_out_of_bounds", test_modbus_register_callback_out_of_bounds, 7},
};

#define TEST_CASE_COUNT (sizeof(g_test_cases) / sizeof(g_test_cases[0]))

/* ==================== 测试入口 ==================== */
void Run_Param_Validation_Test(void) {
    for(uint8_t i = 0; i < TEST_CASE_COUNT; i++) {
        Test_LED_Blink(g_test_cases[i].blink_count, 50, 100);
        g_test_cases[i].func();
    }
    
    Test_IndicatePass();
}