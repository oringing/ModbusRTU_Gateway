/**
 * @file test_validation.h
 * @brief Modbus 参数校验单元测试接口声明
 * //测试代码 begin：本头文件仅用于 P1-001 参数校验测试，不应提交到主干分支
 */

#ifndef __TEST_VALIDATION_H__
#define __TEST_VALIDATION_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 运行所有参数校验测试
 * @note 此函数必须在 FreeRTOS 调度器启动后调用（即在某个任务中执行）
 * @note 测试通过时 LED 快速闪烁 3 次，失败则常亮停机
 */
void Run_Param_Validation_Test(void);

#ifdef __cplusplus
}
#endif

#endif /* __TEST_VALIDATION_H__ */
