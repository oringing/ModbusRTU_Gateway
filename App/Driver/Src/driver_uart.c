// App/Src/driver/driver_uart.c
#include "driver_uart.h"
#include "uart.h"
#include "cmsis_os.h"
#include "task.h"

static osMutexId s_uart_mutex = NULL;

static void UART_Driver_EnsureMutex(void)
{
    if (s_uart_mutex != NULL) {
        return;
    }

    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        osMutexDef(DriverUartMutex);
        s_uart_mutex = osMutexCreate(osMutex(DriverUartMutex));
    }
}

bool UART_Driver_Init(void)
{
    UART_Driver_EnsureMutex();
    return true;
}

bool UART_Driver_Send(const uint8_t *data, uint16_t len, uint32_t timeout)
{
    if (data == NULL || len == 0U) {
        return false;
    }

    UART_Driver_EnsureMutex();

    /* Scheduler not started yet: send directly. */
    if (s_uart_mutex == NULL) {
        return BSP_UART_Send(data, len, timeout);
    }

    if (osMutexWait(s_uart_mutex, timeout) != osOK) {
        return false;
    }

    bool ok = BSP_UART_Send(data, len, timeout);
    osMutexRelease(s_uart_mutex);
    return ok;
}

uint16_t UART_Driver_Receive(uint8_t *buffer, uint16_t max_len, uint32_t timeout)
{
    if (buffer == NULL || max_len == 0U) {
        return 0;
    }

    uint32_t start = HAL_GetTick();
    if (timeout == 0U) {
        if (BSP_UART_IsFrameReady()) {
            return BSP_UART_ReadFrame(buffer, max_len);
        }
        return 0U;
    }

    while ((HAL_GetTick() - start) <= timeout) {
        if (BSP_UART_IsFrameReady()) {
            return BSP_UART_ReadFrame(buffer, max_len);
        }
        osDelay(1);
    }

    return 0;
}

