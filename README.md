# MiniSQL-IndexManager

###ISSUE

1. `CommonHeader.hpp`中`Value`类的`type`定义修改为：

  - `-1`：`int`

  - `0`：`float`

  - `>0`：`string`，值为字符串长度

  为了与`CatalogManager`和`BPlusTree`中的`type`对应。
  
  
2. `RecordManager`中调用了`BufferManager`未提供的函数`getBlockByOffsets`
