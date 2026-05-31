// App/Driver/Src/driver_led.c
#include "driver_led.h"
#include "cmsis_os.h"
#include "led.h"

static LED_Mode_t s_current_mode = LED_MODE_OFF;
static LED_Mode_t s_requested_mode = LED_MODE_OFF;
static uint32_t   s_last_toggle_tick = 0;
static bool       s_current_state = false;
static osMutexId  s_led_mutex = NULL;

static void LED_Driver_EnsureMutex(void) {
    if (s_led_mutex != NULL)
        return;
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        osMutexDef(LedMutex);
        s_led_mutex = osMutexCreate(osMutex(LedMutex));
    }
}

void LED_Driver_Init(void) {
    BSP_LED_Init();
    s_current_mode = LED_MODE_OFF;
    s_requested_mode = LED_MODE_OFF;
    BSP_LED_Off();
}

void LED_Driver_SetMode(LED_Mode_t mode) {
    LED_Driver_EnsureMutex();

    if (s_led_mutex == NULL) {
        // 调度器未启动，直接设置
        if (mode == LED_MODE_FAULT || s_current_mode != LED_MODE_FAULT) {
            s_current_mode = mode;
            s_requested_mode = mode;
        }
        return;
    }

    if (osMutexWait(s_led_mutex, 100) == osOK) {
        // 故障模式不能被覆盖
        if (mode == LED_MODE_FAULT || s_current_mode != LED_MODE_FAULT) {
            s_requested_mode = mode;
        }
        osMutexRelease(s_led_mutex);
    }
}

LED_Mode_t LED_Driver_GetMode(void) {
    return s_current_mode;
}

void LED_Driver_Update(void) {
    LED_Driver_EnsureMutex();

    // 获取最新请求的模式
    if (s_led_mutex != NULL) {
        if (osMutexWait(s_led_mutex, 10) == osOK) {
            if (s_requested_mode != s_current_mode) {
                s_current_mode = s_requested_mode;
                s_last_toggle_tick = HAL_GetTick();
                s_current_state = false;
                BSP_LED_Off();
            }
            osMutexRelease(s_led_mutex);
        }
    }

    uint32_t now = HAL_GetTick();
    uint32_t interval = 0;
    bool     should_toggle = false;

    switch (s_current_mode) {
    case LED_MODE_OFF:
        return;
    case LED_MODE_ON:
        BSP_LED_On();
        return;
    case LED_MODE_FAULT:
        BSP_LED_On(); // 常亮，不可清除
        return;
    case LED_MODE_HEARTBEAT:
        interval = 1000; // 1Hz 翻转
        should_toggle = true;
        break;
    case LED_MODE_ERROR_1HZ:
        interval = 500;
        should_toggle = true;
        break;
    case LED_MODE_ERROR_2HZ:
        interval = 250;
        should_toggle = true;
        break;
    case LED_MODE_ERROR_4HZ:
        interval = 125;
        should_toggle = true;
        break;
    default:
        return;
    }

    if (should_toggle && (now - s_last_toggle_tick) >= interval) {
        s_current_state = !s_current_state;
        if (s_current_state) {
            BSP_LED_On();
        } else {
            BSP_LED_Off();
        }
        s_last_toggle_tick = now;
    }
}
