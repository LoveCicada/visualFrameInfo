# 发布流程

本文件描述如何将 Release 版本的安装包打包并上传到 GitHub Releases。

## 前置条件

| 工具 | 说明 |
| ------ | ------ |
| Visual Studio 2017 (x64 toolchain) | 必须在 VS 开发者命令提示符中构建 |
| Qt 5.12.12 msvc2017\_64 | 路径配置在 `CMakePresets.json` |
| Inno Setup 6 | 安装后 `ISCC.exe` 在 PATH 中，或设置 `INNO_SETUP_HOME` 环境变量 |
| ffmpeg / ffprobe / ffplay | 将三个 `.exe` 放入 `ffmpeg/` 目录（不入仓库，见下文） |

## 一键构建 + 部署 + 打包

在 VS Code 中运行以下任务（支持顺序依赖，一键完成三步）：

```text
Tasks: Run Task → Build Deploy And Package Installer
```

该任务依次执行：

1. **CMake Build Release** — `cmake --build --preset default-release-build`，产物：`build/cmake-msvc-release/Release/visualFrameInfo.exe`
2. **Deploy Release Package** — 执行 `scripts/deploy_release.ps1`，调用 `windeployqt` 复制 Qt 运行时与 `ffmpeg/` 目录，产物：`install/Release/`
3. **Package Inno Setup Installer** — 执行 `scripts/package_inno.ps1`，读取 `CMakeLists.txt` 中的版本号，产物：`install/installer/visualFrameInfo_setup_<version>.exe`

> `install/` 目录已被 `.gitignore` 排除，所有构建产物均不进入源码仓库。

## 单独执行各步骤（可选）

```powershell
# 1. 仅构建 Release
cmake --build --preset default-release-build

# 2. 仅部署 Qt 运行时
powershell -File scripts/deploy_release.ps1

# 3. 仅打包安装程序
powershell -File scripts/package_inno.ps1
```

## 确认安装包版本

版本号由 `CMakeLists.txt` 中的 `project(visualFrameInfo VERSION x.y.z)` 决定，脚本自动读取，无需手动维护。安装包输出路径：

```text
install/installer/visualFrameInfo_setup_<version>.exe
```

## 上传到 GitHub Release

推荐直接使用脚本自动发布：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/publish_github_release.ps1 -Tag 0.1.1
```

发布前 dry-run 检查：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/publish_github_release.ps1 -Tag 0.1.1 -DryRun
```

脚本会自动完成以下动作：

1. 检查 `install/installer/visualFrameInfo_setup_<version>.exe` 是否存在
2. 检查 `gh` 登录状态
3. 创建并推送对应 Tag（若本地不存在）
4. 创建 GitHub Release 或更新已有 Release 的安装包资产

如需手动发布，按以下步骤执行：

1. 在 `master` 分支打好标签：

   ```bash
   git tag v<version>
   git push origin v<version>
   ```

2. 打开 GitHub 仓库页面 → **Releases** → **Draft a new release**。
3. 选择刚刚推送的标签（如 `v0.1.0`）。
4. 填写 Release 标题和版本说明（变更日志可参考 `docs/changes.md`）。
5. 在 **Assets** 区域拖入或点击上传：

   ```text
   install/installer/visualFrameInfo_setup_<version>.exe
   ```

6. 点击 **Publish release**。

## 每次发版只看这一段

1. 修改 `CMakeLists.txt` 中的版本号为目标版本
2. 提交并推送代码到远程 `master`
3. 运行任务 `Build Package And Publish Release`
4. 如果不用任务，就执行：

   ```powershell
   powershell -NoProfile -ExecutionPolicy Bypass -File scripts/publish_github_release.ps1 -Tag <version>
   ```

5. 打开 GitHub Releases 页面，确认 Tag、正文和安装包附件无误

注意：安装包文件名来自 `CMakeLists.txt` 版本号，因此版本号必须先改，再打包，再发布。

## v0.1.1 升级示例

1. 修改版本号：`CMakeLists.txt` 中改为 `project(visualFrameInfo VERSION 0.1.1 LANGUAGES CXX)`
2. 提交并推送：

   ```bash
   git add CMakeLists.txt
   git commit -m "chore: bump version to 0.1.1"
   git push origin master
   ```

3. 执行发布前检查（不改远程状态）：

   ```powershell
   powershell -NoProfile -ExecutionPolicy Bypass -File scripts/publish_github_release.ps1 -Tag 0.1.1 -DryRun
   ```

4. 运行一键链路发布：

   ```text
   Tasks: Run Task → Build Package And Publish Release
   ```

## ffmpeg 二进制说明

`ffmpeg/*.exe` 单文件体积约 110 MB，超过 GitHub 普通 Git 单文件 100 MB 限制，因此不入源码仓库。  
用户可从 [https://www.gyan.dev/ffmpeg/builds/](https://www.gyan.dev/ffmpeg/builds/) 下载 Windows 静态构建（`ffmpeg-release-essentials.zip`），解压后将 `ffmpeg.exe`、`ffprobe.exe`、`ffplay.exe` 放入仓库根目录的 `ffmpeg/` 文件夹，再执行构建流程。
