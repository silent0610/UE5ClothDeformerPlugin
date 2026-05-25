## TODO

- [x] 检查实现状态 — 整体已经实现
- [x] ~~先尝试实现在 CPU 上修改顶点~~ **失败, 服装顶点存储在显存中**
- [x] [Optimus 框架下实现相关类](https://zhuanlan.zhihu.com/p/632849525)
- [x] Component 调用 Adapter 的 API 需对齐
- [x] Instance 使用 Map 处理数据转换
- [x] 模型输出隐藏层处理
- [x] 映射生成测试 — `Cloth.SparseMatrix.*` 4 项单元测试已覆盖
- [x] 实时推理测试 — `Cloth.OnnxInstance.*` 1 项 + `Cloth.Adapter.*` 2 项
- [x] Adapter 自动初始化 — `BeginPlay` 中创建 `FSnugInputAdapter`

---

### P0

- [ ] GPU 变形闭环 — ComputeShader 消费 OffsetBufferSRV 应用到顶点 **[应用偏移模块]**

### P1

- [ ] 确认四元数坐标系转换对齐 — 打开 `ConvertBoneRotation` 调用 **[实时推理模块]**
- [ ] 修复 `ClothDataInterface.cpp` **[应用偏移模块]**

### P2

- [ ] 异步 Bake + 进度条 — `FAsyncTask` 封装耗时计算 **[编辑器工具]**
- [ ] Bake 后自动保存资产 — 调用 `UEditorLoadingAndSavingUtils::SavePackages` **[编辑器工具]**
- [ ] ApplyMapping 迁移 GPU **[应用偏移模块]**
- [ ] 优化单精度双精度转换 — 移除 `FVector` → `FVector3f` 每帧拷贝 **[应用偏移模块]**
- [ ] 多动态维度支持 — 扩展 `CalculateInputTensorDimensions` **[实时推理模块]**
- [ ] FSurfaceProjection::GenerateMapping 补全 **[映射生成模块]**

### P3

- [ ] 资产自动导入工具 — 实现 Asset Action **[资产系统]**
- [ ] 跨平台支持 — Android/iOS **[扩展]**
- [ ] 多 LOD 支持 **[扩展]**
- [ ] 性能 Dashboard **[扩展]**
