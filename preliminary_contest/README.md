# HUAWEICHALLENGE2022合作指南
## 关于代码
(大家可以把建议更新在这里)
- 请大家务必将功能模块化，写成一个个函数
- 希望大家在该注释的地方写好注释
- 变量命名建议（下划线方式，注意语义谢谢！），例如：qos_constraint, site_bandwidth_limit...
- 函数命名建议（驼峰式），例如：getSiteBandwidth(), getClientDemand()...

## 关于提交
1. 提交前检查代码中文件路径是否正确

**数据读取在目录`/data`下**

**数据保存在文件`/output/solution.txt`中**

2. 生成压缩文件[CodeCraft-2022.zip](SDK_python/CodeCraft-2022.zip)：
```
bash SDK_python/CodeCraft_zip.sh
```
## 目录管理
大家可以把不同的方案写成不同的py文件，放在目录SDK_python/CodeCraft-2022/solutions下。要测试的话就把相应内容copy到SDK_python/CodeCraft-2022/src/CodeCraft-2022.py。或者是在SDK_python/CodeCraft-2022/src/CodeCraft-2022.py中进行import的调用。

## 方案说明
请大家把解题思路更新在这里

思路：（以下均为某一单独时刻的情况）
如果按某种顺序考虑用户节点的需求，并将带宽需求以某种方式分配到满足Qos要求的边缘节点上。不可否认可能会出现无法满足用户节点需求的情况（不可行解），举例如下：
按上述顺序，最后一个用户节点带宽需求为100，满足其Qos要求的边缘节点只有一个，该边缘节点的带宽上限为120，但是在迭代到该用户节点之前已经分配40带宽给其他用户节点，故当前用户节点带宽需求无法被满足80＜100，出现矛盾。
当出现矛盾时如何调整其他用户节点与边缘节点之间的分配关系，我认为是一件较为复杂的事情，暂时没有好的解决方案，所以我认为应该考虑将 如何避免不可行解 作为出发点（这里我想问问大家这样的思路是否合理，或者有没有其他的思路可以一起讨论）。
若以此作为解题思路的话，接下来需要考虑的是上述顺序及分配方案如何定义，即以哪种顺序考虑用户节点，和以哪种方式分配多少带宽至各满足QoS要求的边缘节点。
这里我暂时想了几种优先级方案，有效性暂时不确定，需要编程实践验证效果：
用户节点顺序：
1. 总带宽需求 升序or降序
2. 满足Qos要求的边缘节点数 升序or降序
3. 总带宽需求/满足Qos要求的边缘节点数（姑且称之为压力吧） 升序or降序

边缘节点带宽分配方案：
1. 平均分配，Too Naive，极有可能出现不可行解
2. 加权分配：
1. 以各自连接的满足Qos需求的用户节点数作为权重（这里我之后补充公式），连接的用户节点数越多，分配给该边缘节点的带宽越少
2. 以当前空闲带宽（带宽上限减去已分配出去的带宽）作为权重（补充公式），空闲带宽越多，分配给该边缘节点的带宽越多

大家集思广益一下，有不同的思路，或者有新的合理的优先级方案都可以讨论和补充！


### 方案一
代码文件：xxx.py
主要思路：......
调用说明：......

## 其他有用的库
1. 测试指标生成：[CodeCraft2022-benchmark](https://github.com/diphosphane/CodeCraft2022-benchmark)
2. 压测数据生成：[CodeCraft2022-PressureGenerator](https://github.com/diphosphane/CodeCraft2022-PressureGenerator)
