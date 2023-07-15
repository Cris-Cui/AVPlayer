# 遇到的问题

##### 1. QT程序异常结束

分为两种情况:

- 启动异常结束
  **与动态链接库有关**
  1.  直接访问外部库文件

  ```cpp
  //-L+文件路径+\
  //这个是直接通过路径访问外部库里面的dll和lib文件
  LIBS += -LE:/opcv/opencv/build/x64/vc14/lib \
          -lopencv_world344
  ```

  2.  引用外部库文件
  **需要在构建文件release/debug中加入手动加入lib文件对应的dll**

  ```cpp
  LIBS += E:/opcv/opencv/build/x64/vc14/lib/opencv_world344.lib
  ```

- 运行中异常结束

##### 2. QLabel标签截断内容

*   [x] scaledContents勾选，**但不保证等比例缩放**




