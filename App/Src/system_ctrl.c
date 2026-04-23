//App/Src/sysctrl.c
#include "system_ctrl.h"
#include "led.h" // 假设你有 LED 驱动用于报警

static SysState_t current_state = SYS_STATE_INIT;

void System_Ctrl_Init(void) {
    current_state = SYS_STATE_RUNNING;
}

void System_Ctrl_Process(void) {
    // 这里可以放一些全局性的轮询逻辑
    // 比如：如果系统处于 ERROR 状态，每秒闪烁一次红灯
}

SysState_t System_GetState(void) {
    return current_state;
}
