//App/Protocol/Inc/modbus.h
#ifndef __MODBUS_H__
#define __MODBUS_H__

#include "cmsis_os.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MODBUS_SLAVE_ADDR                 1
#define MODBUS_REG_MAX_COUNT             10
#define MODBUS_FUNC_READ_HOLDING_REGS    0x03
#define MODBUS_FUNC_WRITE_SINGLE_REG     0x06
#define MODBUS_MAX_READ_REGS             125
#define MODBUS_BUFFER_SIZE               256
#define MODBUS_RESPONSE_BUFFER_SIZE      256
#define MODBUS_EXCEPTION_RESPONSE_SIZE   5
#define MODBUS_POLL_INTERVAL             10

void Modbus_Init(void);
void Modbus_Process(void);

#ifdef __cplusplus
}
#endif

#endif /* __MODBUS_H__ */
