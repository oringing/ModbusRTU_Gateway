# 贡献指南

欢迎为 Modbus RTU Gateway 项目做出贡献！

## 📋 如何报告 Bug

1. **搜索现有 Issue**：确保问题尚未被报告
2. **创建新 Issue**：包含以下信息
   - 硬件环境（STM32 型号、RS485 模块型号）
   - 软件版本（固件版本、FreeRTOS 版本）
   - 复现步骤（详细的操作流程）
   - 预期行为 vs 实际行为
   - 串口日志或示波器截图（若有）

## 🔧 开发流程

1. **Fork 本项目**到你的 GitHub 账号
2. **克隆仓库**：
   ```bash
   git clone https://github.com/<your-username>/ModbusRTU_Gateway.git
   cd ModbusRTU_Gateway
   ```
3. **创建功能分支**：
   ```bash
   git checkout -b feat/your-feature-name
   ```
4. **提交修改**：
   ```bash
   git commit -am 'feat: add your feature'
   ```
5. **推送到分支**：
   ```bash
   git push origin feat/your-feature-name
   ```
6. **发起 Pull Request**

## 🌿 分支命名规范

- `feat/xxx` - 新功能
- `fix/xxx` - Bug 修复
- `refactor/xxx` - 代码重构
- `docs/xxx` - 文档更新
- `test/xxx` - 测试相关

## 📝 提交信息格式

遵循约定式提交规范（Conventional Commits）：

```text
<type>(<scope>): <subject>

<body>
```

**Type 类型**:
- `feat`: 新功能
- `fix`: Bug 修复
- `docs`: 文档修改
- `style`: 代码格式修改（不影响代码运行的变动）
- `refactor`: 重构（即不是新增功能，也不是修改 bug 的代码变动）
- `test`: 测试相关
- `chore`: 构建过程或辅助工具的变动

**示例**:
```text
fix(modbus): 修复 0x06 功能码响应格式错误
```

## 💻 代码规范

- **格式化工具**：提交前请使用项目根目录的 `.clang-format` 配置文件进行格式化。
- **编码风格**：严格遵循 [CODING_STYLE.md](CODING_STYLE.md)。
  - 函数命名使用 `PascalCase`
  - 变量使用 `snake_case`
  - 所有公共 API 必须添加 Doxygen 注释
- **编译要求**：代码需通过 Keil MDK v5 编译，且无警告。
- **兼容性**：确保代码在 STM32F103C8T6 + MAX485 环境下运行正常。

## 🧪 测试要求

- 所有修复需提供测试验证记录（如串口日志、逻辑分析仪截图等）。
- 回归测试通过后方可合并。
- 新增功能建议提供简单的测试用例或验证步骤。

## 📞 联系方式

如有问题，请通过 GitHub Issue 联系，或通过邮箱：your-email@example.com

---

**注意**：本项目采用 MIT 许可证，贡献代码即表示你同意以该许可证发布你的贡献。
```
