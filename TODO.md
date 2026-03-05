

## TODO

- [ ] 映射生成模块
  - [x] 检查实现状态 整体已经实现
  - [ ] 测试映射生成
  
- [ ] 实时推理模块
  - [ ] 输入数据获取与转换
    - [x] component 调用 adapter 的 api 需要对齐
    - [x] Instance 使用 Map 处理数据转换
    - [x] 只能处理单一动态维度 [#注意](#注意)
    - [ ] 确认四元数坐标系转换对齐
    - [ ] 测试实时推理
  - [x] 模型输出隐藏层处理
  
- [ ] 应用偏移模块\
  
  - [ ] 参考文献https://zhuanlan.zhihu.com/p/632849525, UE5 Plugin ML Deformer Vertex Delta Model
  
  - [ ] 能否将映射部分`Applymapping`转移到GPU?
  
  - [x] ~~先尝试实现在 CPU 上修改顶点~~ **失败, 服装顶点存储在显存中**
  
  - [ ] 需要实现 `Update` 函数 使用 `ComputeShader` 将偏移应用到 `USkeletalMeshComponent`
  
    - [x] 在 `Optimus` 框架下实现相关类
    - [ ] 修复 `ClothDataInterface.cpp`
  
  - [ ] 优化单精度双精度转换
  
    
  