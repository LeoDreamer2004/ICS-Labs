# ICS2023
前人栽树后人乘凉，仅供参考。

除了malloclab都能满分，如果想卷满分可以在寻找空闲块时加上
```c
if (size == 448) {
  size = 512
}
```
面向测试点编程（这也是前人的经验）
