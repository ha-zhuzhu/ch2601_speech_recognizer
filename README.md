# 基于 RVB2601 的云语音识别

本项目旨在实现基于 T-HEAD RVB2601 的云语音识别，主要工作在于拓展官方网路库 HTTPClient，将音频数据上传至 HTTP 服务器。

项目文章：https://hazhuzhu.com/mcu/rvb2601-speech_recognizer.html

演示视频：https://occ.t-head.cn/community/post/detail?spm=a2cl5.14300636.0.0.429d180fdX5uEb&id=4018742358014234624

参考资料：[平头哥 YOC 文档](https://yoc.docs.t-head.cn/yocbook/)；[YOC github 源码](https://github.com/T-head-Semi/open-yoc)

程序流程与关键组件：

1. gpio_pin：设置按键中断。按下按钮后设置“开始录音标志位”为 1。
2. rhino：注册监控任务。监控“开始录音标志位”，若为 1 则：
   1. codec：进行录音。
   2. HTTPClient ：上传录音数据，接收并返回识别结果。
   3. 清空标志位。

目录结构：

```
├─app                   // 工程源码
│  ├─include
│  └─src
├─configs               // 配置与链接文件
├─Packages              // 在 webplayer_demo 基础上添加的组件
│  └─components
│      ├─httpclient     // HTTPClient 组件，有修改
│      └─transport
├─script
├─web_code              // 服务器端代码
...
```