# MiniSQL-IndexManager

###ISSUE
API.cpp has two variables:
bool displayComments
bool displayRecordContents
we can use them to control if the CMD displays comments.

1. `CommonHeader.hpp`中`Value`类的`type`定义修改为：

  - `-1`：`int`

  - `0`：`float`

  - `>0`：`string`，值为字符串长度n

  为了与`CatalogManager`和`BPlusTree`中的`type`对应。
  
