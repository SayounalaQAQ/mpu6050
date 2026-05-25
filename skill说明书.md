Skill 是智能体的可复用能力扩展包，其编写规范包括固定目录结构、SKILL.md 文件标准、YAML 元数据、渐进式披露、工作流设计、脚本集成及测试迭代等。
1. Skill 的定义与核心理念
Skill 是一个封装特定任务或工作流指令集的能力模块，不是一次性 Prompt，也不是独立应用程序。它通过自然语言和脚本指导智能体完成多步骤任务，实现可复用、可维护、可扩展的智能工作流 
掘金
掘金
+2
。
核心设计原则：

渐进式披露：元数据常驻内存，指令和资源按需加载，减少上下文消耗 
掘金
掘金
+1
。
单一职责：每个 Skill 只解决一个明确问题，避免复杂度过高 
知乎
知乎
+1
。
可组合性与可移植性：Skill 可与其他 Skill 协作，并在不同环境中一致运行 
掘金
掘金
。
2. 目录结构与命名规范
标准 Skill 目录结构：

<skill-name>/          # Skill 根目录
├── SKILL.md           # 必需：核心指令文件
├── scripts/           # 可选：可执行脚本（Python/Bash）
├── references/        # 可选：参考文档
└── assets/            # 可选：模板、图标等资源
命名规范：

目录名使用小写字母和连字符（kebab-case），禁止空格、下划线或大写字母。
SKILL.md 文件必须区分大小写，禁止使用 README.md 替代。
打包文件名为 <skill-name>.skill，仅包含必要文件 
CSDN
CSDN
+1
。
3. SKILL.md 文件规范
YAML Frontmatter
位于文件顶部，定义 Skill 的元数据：

---
name: skill-name
description: 描述 Skill 核心能力及触发场景
dependency:
  python:
    - package-name==version
  system:
    - mkdir -p some-path
---
name：小写连字符格式。
description：清晰说明 Skill 做什么、何时触发、关键能力。
dependency：可选，列出 Python 包或系统命令，不执行安装操作 
CSDN
CSDN
+1
。
Markdown Body
包含指令、示例和故障排除：

指令（Instructions）：具体可执行步骤，使用祈使语气。
示例（Examples）：输入输出示例，帮助模型理解。
故障排除（Troubleshooting）：定义失败路径和处理方式。
HARD GATE：明确禁止行为和触发条件，防止误操作 
知乎
知乎
+1
。
4. 工作流与 Checklist
使用 Checklist + 流程图 明确每一步操作。
每条任务以动词开头，包含条件和完成标准。
对复杂任务，建立“验证 → 修正 → 再验证”的反馈闭环 
知乎
知乎
+1
。
5. 脚本与资源集成
scripts/：实现复杂操作或 API 调用，遵循纯函数式、参数化设计。
references/：存放长文档、示例或格式规范，按需引用。
assets/：存放模板、图标等可直接引用的资源。
脚本必须输出可理解结果，显式处理错误，避免模型猜测 
CSDN
CSDN
+1
。
6. 测试与迭代
触发测试：确保 Skill 在正确场景被加载。
功能测试：验证工作流、API 调用及边缘情况。
性能对比：量化 Skill 提升效率和稳定性。
遵循“评测驱动、失败优先”，持续迭代 Skill 并回归验证 
知乎
知乎
+2
。
7. 安全与权限
最小权限原则：Skill 仅访问必要工具。
高风险操作需确认环节。
YAML 元数据禁止使用尖括号 < > 防止注入攻击。
明确失败安全和降级路径 
掘金
掘金
+1
。
8. 常见误区
Skill ≠ Prompt：Skill 是可复用的能力模块，而非一次性提示。
description 不明确：必须包含触发条件和使用场景。
过度复杂：职责单一、边界清晰的 Skill 更稳定。
忽略测试：Skill 必须通过触发和功能测试验证。
无版本管理：Skill 应纳入团队 Git 流程，像代码一样维护