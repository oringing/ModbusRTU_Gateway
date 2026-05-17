refactor(config): 消除魔法数字与配置规范化重构

- Modbus 协议层：新增 22 个帧字段索引宏，替换所有硬编码索引（grep → 0 matches）
- 配置职责分离：UART/舵机/错误处理/监控任务配置移至对应头文件
- system_config.h 从 80+ 行精简至 24 行，仅保留系统级配置
- 业务参数规范化：提取 LED 翻转周期、8 个栈/缓冲区校验阈值宏定义
- 架构优化：校验阈值宏移至 system_ctrl.h（高内聚原则），引脚命名规范化
- 文档同步更新：协议说明增加宏定义列，问题记录更新 P1-002 状态
- 验证结果：编译无警告，烧录成功，循环混合指令测试通过

关联问题：P1-002
修改文件：11 个（modbus.h/c, uart.h/c, driver_servo.h, error_handler.h, monitor_task.h, system_config.h, system_ctrl.h/c, led_task.c）