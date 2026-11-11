# Qt 影音播放器

> Qt Widgets + QMediaPlayer 实现的本地音视频播放器

## 项目简介

使用 Qt Widgets 编写的桌面影音播放器，基于 `QMediaPlayer` / `QVideoWidget` 实现媒体播放、播放列表与基础控制，界面由 Qt Designer 的 `.ui` 文件描述。

## 技术栈

- C++11
- Qt 5
- Qt Widgets
- Qt Multimedia
- qmake

## 目录结构

```
README.md
.gitignore
main.cpp
mainwindow.cpp
mainwindow.h
mainwindow.ui
qt-media-player.pro
```

## 快速开始

1. 使用 Qt Creator 打开 `qt-media-player.pro`
2. 配置 Qt 5 套件（需含 multimedia 模块）
3. 构建并运行；或直接 `qmake && make`

## 配置说明

`*.pro.user` 是 Qt Creator 的本机用户配置（含绝对路径），未纳入仓库。

## 文档

- `README.md`（本文件）

## 整理说明

- `OraOlaMediaPlay.pro.user` 未纳入（含本机路径）。

## 隐私与脱敏

本仓库在公开发布前已统一处理：移除 API Key / 私钥 / 数据库口令 / 手机号 / 个人邮箱 /
真实姓名 / 班级 / 学号 / 学校名称 / 本机绝对路径；排除构建产物、依赖缓存、扩展签名私钥、
演示视频与提交压缩包；原有 `.git` 历史未被复制。课程材料中的署名信息已替换为中性占位符。

> 本仓库为本地项目整理后的展示版本，不包含任何可直接用于生产环境的凭据。

---

*本 README 由项目整理流程自动生成。*
