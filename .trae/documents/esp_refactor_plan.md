# ESP32智能桌面环境控制系统 - 重构计划

## 1. 任务核心分配优化

### [x] 任务1.1: 将Blynk任务从核心0迁移到核心1
- **Priority**: P0
- **Depends On**: None
- **Description**: 
  - 将Blynk任务从核心0迁移到核心1
  - 调整优先级为2-3（按照最佳实践）
  - 确保核心0只运行系统级任务和实时性高的任务
- **Success Criteria**:
  - Blynk任务在核心1运行稳定
  - WiFi连接稳定，无断连现象
- **Test Requirements**:
  - `programmatic` TR-1.1: 系统运行24小时无WiFi断连
  - `programmatic` TR-1.2: 任务运行在核心1，优先级正确
- **Notes**: 核心0应保留给系统任务和实时性高的任务，避免抢占系统资源

## 2. 代码简化与任务重构

### [x] 任务2.1: 使用定时器替代更多FreeRTOS任务
- **Priority**: P1
- **Depends On**: Task 1.1
- **Description**: 
  - 评估当前FreeRTOS任务，将适合的任务转换为BlynkTimer定时任务
  - 保留必要的FreeRTOS任务（如需要实时响应的任务）
  - 简化任务管理，减少系统开销
- **Success Criteria**:
  - 减少FreeRTOS任务数量
  - 系统响应速度不降低
  - 代码结构更清晰
- **Test Requirements**:
  - `programmatic` TR-2.1: 系统运行稳定，功能正常
  - `human-judgement` TR-2.2: 代码结构更清晰，易于维护
- **Notes**: 定时器适合周期性执行的任务，FreeRTOS任务适合需要实时响应的任务

### [x] 任务2.2: 优化Blynk数据发送策略
- **Priority**: P1
- **Depends On**: Task 1.1
- **Description**: 
  - 根据Blynk最佳实践，合理设置数据发送间隔
  - 避免频繁发送数据导致服务器过载
  - 确保数据实时性的同时减少网络流量
- **Success Criteria**:
  - 数据发送间隔合理（传感器数据2-5秒，状态更新5-10秒）
  - Blynk服务器无过载警告
  - 数据显示实时准确
- **Test Requirements**:
  - `programmatic` TR-3.1: 数据发送间隔符合最佳实践
  - `programmatic` TR-3.2: 系统运行24小时无Blynk连接问题
- **Notes**: 参考Blynk文档，避免在loop()中无限制发送数据

## 3. 天气功能彻底删除

### [x] 任务3.1: 删除weather相关头文件和源文件
- **Priority**: P1
- **Depends On**: None
- **Description**:
  - 删除weather目录下的所有文件
  - 移除代码中对weather功能的引用
  - 确保编译无错误
- **Success Criteria**:
  - weather目录及文件被彻底删除
  - 代码中无weather相关引用
  - 编译通过无错误
- **Test Requirements**:
  - `programmatic` TR-4.1: 编译无错误
  - `programmatic` TR-4.2: weather目录不存在
- **Notes**: 确保所有weather相关的函数调用和头文件包含都被删除

### [x] 任务3.2: 清理代码中的天气功能痕迹
- **Priority**: P2
- **Depends On**: Task 3.1
- **Description**:
  - 检查并删除代码中对weather功能的任何引用
  - 确保没有残留的天气相关变量或函数调用
- **Success Criteria**:
  - 代码中无weather相关的变量、函数或注释
  - 编译通过无错误
- **Test Requirements**:
  - `programmatic` TR-5.1: 编译无错误
  - `human-judgement` TR-5.2: 代码中无weather相关内容

## 4. 代码可读性增强

### [x] 任务4.1: 代码结构优化与注释整理
- **Priority**: P2
- **Depends On**: All previous tasks
- **Description**:
  - 优化代码结构，提高可读性
  - 整理注释，确保代码易于理解
  - 统一代码风格
- **Success Criteria**:
  - 代码结构清晰，逻辑分明
  - 注释完整，易于理解
  - 代码风格统一
- **Test Requirements**:
  - `human-judgement` TR-6.1: 代码结构清晰，易于理解
  - `human-judgement` TR-6.2: 注释完整，说明充分

### [x] 任务4.2: 保留秒级时间功能
- **Priority**: P2
- **Depends On**: All previous tasks
- **Description**:
  - 确保时间功能保留秒级精度
  - 优化时间更新逻辑，确保准确性
- **Success Criteria**:
  - 时间显示精确到秒
  - 时间更新稳定，无跳变
- **Test Requirements**:
  - `programmatic` TR-7.1: 时间显示精确到秒
  - `programmatic` TR-7.2: 时间更新稳定，无错误

## 5. 验证与测试

### [ ] 任务5.1: 系统功能测试
- **Priority**: P0
- **Depends On**: All previous tasks
- **Description**:
  - 测试系统各项功能是否正常
  - 验证Blynk通信是否稳定
  - 测试传感器数据采集是否准确
  - 测试智能控制逻辑是否正确
- **Success Criteria**:
  - 所有功能正常运行
  - Blynk通信稳定
  - 传感器数据准确
  - 智能控制逻辑正确
- **Test Requirements**:
  - `programmatic` TR-8.1: 所有功能测试通过
  - `programmatic` TR-8.2: 系统运行24小时无异常

### [ ] 任务5.2: 性能测试
- **Priority**: P2
- **Depends On**: Task 5.1
- **Description**:
  - 测试系统性能，包括响应速度、内存使用等
  - 确保系统运行流畅，无卡顿
- **Success Criteria**:
  - 系统响应速度快
  - 内存使用合理
  - 无系统卡顿现象
- **Test Requirements**:
  - `programmatic` TR-9.1: 系统响应时间<100ms
  - `programmatic` TR-9.2: 内存使用稳定，无泄漏

## 核心分配最佳实践验证

根据提供的核心分配最佳实践表格，我们将采用以下分配策略：

| 任务类型 | 推荐核心 | 推荐优先级 | 注意事项 |
|---------|---------|-----------|----------|
| Blynk通信任务 | 核心1 | 2-3 | 高频调用Blynk.run()，短延迟 |
| 普通业务任务（传感器读取） | 核心1 | 1-2 | 优先级低于Blynk任务 |
| 实时性高的任务（GPIO中断处理） | 核心0 | 3-4 | 仅短逻辑，快速返回 |
| 系统级任务（WiFi/蓝牙） | 核心0 | 1-2 | 不要修改，由系统管理 |

**结论**：当前Blynk任务运行在核心0的做法不符合最佳实践，应迁移到核心1。

## 执行计划

1. 首先执行任务1.1，将Blynk任务迁移到核心1
2. 执行任务3.1和3.2，彻底删除天气功能
3. 执行任务2.1和2.2，优化任务结构和Blynk数据发送策略
4. 执行任务4.1和4.2，增强代码可读性和保留秒级时间功能
5. 执行任务5.1和5.2，验证系统功能和性能

通过以上重构，系统将更加稳定、高效，代码更加清晰易维护。