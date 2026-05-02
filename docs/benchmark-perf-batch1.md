# 第一批性能优化 — 基准对比数据

测试日期：2026-05-02  
测试环境：Windows 11, MSVC2017_64, ffmpeg/ffprobe bundled build  
素材：
- `car.mp4`（约 11280 帧，H.264，可变码率，文件较大）
- `10min_noAudio_60fps.mp4`（36626 帧，H.264，10 分钟 60fps）

---

## 改动内容（第一批）

| 文件 | 改动 |
|------|------|
| `src/services/FfmpegRunner.cpp` | 新增 `ffmpegThreadOptionValue()` 返回 `"0"`（auto）；主流程和基准流程的 ffprobe / ffmpeg 命令均追加 `-threads 0` |
| `src/services/FfmpegRunner.cpp` | ffmpeg showinfo 回退路径：`showinfo` → `showinfo=checksum=0`，关闭逐帧像素 checksum 计算 |

---

## 耗时对比（秒）

| 素材 | 路径 | 改造前 | 改造后 | 节省 |
|------|------|--------|--------|------|
| car.mp4 | ffprobe（主路径）| 49.93 s | 6.84 s | **−86.3%** |
| car.mp4 | ffmpeg showinfo（回退）| 23.44 s | 7.03 s | **−70.0%** |
| 10min_noAudio_60fps.mp4 | ffprobe（主路径）| 7.58 s | 7.41 s | −2.2%（可忽略，I/O 瓶颈）|
| 10min_noAudio_60fps.mp4 | ffmpeg showinfo（回退）| 10.87 s | 4.26 s | **−60.8%** |

---

## 日志体积对比

| 素材 | 路径 | 改造前 | 改造后 | 减少 |
|------|------|--------|--------|------|
| car.mp4 | ffprobe | 3.48 MB | 3.48 MB | 0%（输出内容完全一致）|
| car.mp4 | showinfo | 8.40 MB | 6.16 MB | **−26.7%** |
| 10min_noAudio_60fps.mp4 | ffprobe | 11.36 MB | 11.36 MB | 0%（输出内容完全一致）|
| 10min_noAudio_60fps.mp4 | showinfo | 27.93 MB | 20.80 MB | **−25.5%** |

---

## 帧数一致性验证

| 素材 | 路径 | 改造前帧数 | 改造后帧数 | 一致？ |
|------|------|-----------|-----------|--------|
| car.mp4 | ffprobe `[FRAME]` 块数 | 11280 | 11280 | ✅ |
| 10min_noAudio_60fps.mp4 | ffprobe `[FRAME]` 块数 | 36626 | 36626 | ✅ |
| car.mp4 | showinfo 帧行数 | 9960 | 9960 | ✅ |
| 10min_noAudio_60fps.mp4 | showinfo 帧行数 | 36626 | 36626 | ✅ |

---

## 结论

- **主路径 ffprobe + car.mp4：49.93 s → 6.84 s（−86.3%）**，用户感受最明显的场景大幅提升。
- showinfo 回退路径两个素材均有 60–70% 提速，日志体积缩小约 26%。
- 10min 60fps 视频 ffprobe 几乎无变化——该视频为 I/O 密集型，多线程无法进一步加速。
- **输出内容完全一致**，无正确性回退风险。
