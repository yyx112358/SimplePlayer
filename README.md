# SimplePlayer
简易跨平台播放器


# 2023年5月30日

目标：通过实现一个自己的播放器，系统性学习音视频相关知识。近期路线图：
- [ ] 实现一个预览器，可将FBO通过OpenGL预览上屏。
- [ ] 实现一个解码器，使用FFMpeg解码音视频。
- [ ] 将解码的帧上屏
- [ ] 音画同步
- [ ] 支持暂停、播放、停止、重播
- [ ] 支持多段视频
- [ ] 支持图文混排
- [ ] 实现合成导出
- [ ] 使用cmake建立工程。拆分独立音视频SDK，UI各端实现。暂定实现Mac、iOS、Win，实现无缝开发衔接。
- [ ] 回顾总结，以待重构 

这一轮主要以快速实现框架为主，不必太在乎代码质量。后续可以在TT中间页切换时候重写

综合ChatGPT、[CMake 良心教程，教你从入门到入魂](https://zhuanlan.zhihu.com/p/500002865)、[全网最细的CMake教程！(强烈建议收藏)](https://zhuanlan.zhihu.com/p/534439206) 编写cmakelist，构造了基本的工程。cmake入门还是很简单的。

# [日记](./DevelopDiary/SimplePlayer/SimplePlayer.md)



