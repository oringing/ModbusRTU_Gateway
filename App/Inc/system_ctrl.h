// App/Inc/sysctrl.h
#ifndef __SYSCRTL_H
#define __SYSCRTL_H

// 系统状态枚举
typedef enum {
    SYS_STATE_INIT,
    SYS_STATE_RUNNING,
    SYS_STATE_ERROR
} SysState_t;

// 函数声明
void System_Ctrl_Init(void);
void System_Ctrl_Process(void);
SysState_t System_GetState(void);



#endif /* __SYSCRTL_H */

