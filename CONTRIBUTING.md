# 贡献指南

感谢你对 Modbus RTU Gateway 项目的关注！欢迎各种形式的贡献。

## 📋 如何报告 Bug

1. **搜索现有 Issue**：确保问题尚未被报告
2. **创建新 Issue**：包含以下信息
   - 硬件环境（STM32 型号、RS485 模块型号）
   - 软件版本（固件版本、FreeRTOS 版本）
   - 复现步骤（详细的操作流程）
   - 预期行为 vs 实际行为
   - 串口日志或示波器截图（若有截图）

## 🔧 如何提交代码

### 1. Fork 并克隆
```bash
git fork https://github.com/oringing/ModbusRTU_Gateway.git
git clone https://github.com/oringing/ModbusRTU_Gateway.git
cd ModbusRTU_Gateway
```

### 2. 创建分支
```bash
git checkout -b feature/your-feature-name
# 或
git checkout -b fix/issue-description
```

### 3. 编码规范
- 代码格式：项目根目录有 `.clang-format` 配置文件，提交前请用 clang-format 格式化
- 注释规范：**必须**遵循 [CODING_STYLE.md](CODING_STYLE.md) 中的注释规范，提交前对照自查清单逐项检查
- 函数命名使用 `PascalCase`，变量使用 `snake_case`
- 所有公共 API 必须添加 Doxygen 注释
- 新增功能需更新相关文档

### 4. 测试要求
- 确保代码在 STM32F103C8T6 + MAX485 环境下编译通过
- 提供测试用例或验证步骤

### 5. 提交规范
```bash
git commit -m "feat: 添加 UART 错误分级恢复机制"
# 或
git commit -m "fix: 修复 CRC 字节序处理错误"
```

**提交类型**：
- `feat`: 新功能
- `fix`: Bug 修复
- `docs`: 文档更新
- `style`: 代码格式调整
- `refactor`: 重构
- `test`: 测试相关
- `chore`: 构建/工具链变更

### 6. 推送并创建 PR
```bash
git push origin feature/your-feature-name
```

然后在 GitHub 上创建 Pull Request，描述中需包含：
- 变更目的
- 测试验证结果
- 是否破坏兼容性
- 关联的 Issue 编号（如有）

## 📝 文档贡献

- 修正拼写错误或语法问题
- 补充缺失的使用示例
- 翻译文档到其他语言
- 优化架构图或流程图

## ❓ 提问之前

1. 阅读 [Readme.md](Readme.md)
2. 查阅 [docs/](docs/) 目录下的相关文档
3. 搜索已关闭的 Issue

## 🎯 优先处理的 Issue

查看带有以下标签的 Issue：
- `good first issue`：适合新手
- `help wanted`：急需帮助
- `bug`：高优先级修复

## 📞 联系方式

如有问题，请通过 GitHub Issue 联系，或通过邮箱：your-email@example.com

---

**注意**：本项目采用 MIT 许可证，贡献代码即表示你同意以该许可证发布你的贡献。
```
