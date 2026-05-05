# visualFrameInfo

视频帧信息分析工具，基于 Qt 5 + CMake + ffprobe，支持 GOP 段预览、帧级别统计、平均比特率显示与 CSV 导出。

## 下载安装

前往 [Releases](https://github.com/LoveCicada/visualFrameInfo/releases) 下载最新版 `visualFrameInfo_setup_<version>.exe` 安装即可，无需自行构建。

## 本地构建

### 环境要求

| 依赖 | 版本 |
| ------ | ------ |
| Windows | 10 / 11 x64 |
| Visual Studio | 2017（需含 MSVC x64 工具链） |
| Qt | 5.12.12 msvc2017\_64 |
| CMake | ≥ 3.16 |
| Inno Setup | 6（打包安装程序时需要） |

### 放置 ffmpeg 二进制

本仓库不包含 ffmpeg 可执行文件（超过 GitHub 单文件 100 MB 限制）。  
从 [https://www.gyan.dev/ffmpeg/builds/](https://www.gyan.dev/ffmpeg/builds/) 下载 `ffmpeg-release-essentials.zip`，将其中的以下三个文件复制到仓库根目录的 `ffmpeg/` 文件夹：

```text
ffmpeg/
  ffmpeg.exe
  ffprobe.exe
  ffplay.exe
```

### 构建步骤

在 **Visual Studio 开发者命令提示符（x64）** 中操作：

```bash
# 1. 配置
cmake --preset msvc-qt5-release

# 2. 构建 Debug
cmake --build --preset default-debug-build

# 3. 构建 Release
cmake --build --preset default-release-build
```

或在 VS Code 中直接使用 CMake Tools 插件一键构建。

### 生成安装包

在 VS Code 中运行任务：

```text
Tasks: Run Task → Build Deploy And Package Installer
```

产物位于 `install/installer/visualFrameInfo_setup_<version>.exe`。  
完整流程说明见 [docs/release.md](docs/release.md)。

### 发布到 GitHub Release

最短命令模板：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/publish_github_release.ps1 -Tag 0.1.1
```

发布前干跑检查：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/publish_github_release.ps1 -Tag 0.1.1 -DryRun
```

超短操作说明：

1. 先把 `CMakeLists.txt` 里的版本号改成目标版本，例如 `0.1.1`
2. 提交并推送代码到远程 `master`
3. 运行 VS Code 任务 `Build Package And Publish Release`
4. 或手动执行上面的发布命令
5. 到 GitHub Releases 页面确认 Tag、正文和安装包附件都正确

`v0.1.1` 升级示例：

1. 修改 `CMakeLists.txt`：`project(visualFrameInfo VERSION 0.1.1 LANGUAGES CXX)`
2. 提交版本更新：`git commit -am "chore: bump version to 0.1.1"`
3. 推送代码：`git push origin master`
4. 打包并发布：`Tasks: Run Task → Build Package And Publish Release`

## 许可

本项目仅供个人学习使用。ffmpeg 遵循其自身 [许可协议](https://ffmpeg.org/legal.html)。
