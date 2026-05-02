# 第二批性能优化 — 基准对比数据

测试日期：2026-05-02  
测试环境：Windows 11, MSVC2017_64, ffmpeg/ffprobe bundled build  
基准参照：第一批优化后结果（`-threads 0` + `showinfo=checksum=0`）  
素材：
- `car.mp4`（11280 帧，H.264，高复杂度，文件较大）
- `10min_noAudio_60fps.mp4`（36626 帧，H.264，10 分钟 60fps）

---

## 改动内容（第二批）

| 文件 | 改动 |
|------|------|
| `src/services/FfmpegRunner.cpp` | 主路径 ffprobe 输出格式：`-of default=noprint_wrappers=0` → `-of csv` |
| `src/parsers/ShowInfoParser.cpp` | `parseFile()`：新增 CSV 帧行解析分支（`frame,{key_frame},{pts},{pts_time},{dur},{dur_time},{pict_type}`）；保留 default 格式兼容 |
| `src/parsers/ShowInfoParser.cpp` | `buildSummary()`：新增 CSV stream 行解析分支（`stream,{codec},{w},{h},{avg_fps},{r_fps},...`）；保留 default 格式兼容 |

**CSV 格式特性：**
- 每帧输出 1 行（default 格式为 8 行：`[FRAME]` + 6 字段 + `[/FRAME]`）
- 不含字段名前缀（`key_frame=` 等）→ 日志文件体积减小约 73%
- 解析器维持双格式兼容（CSV 和 default 均可解析）

---

## 耗时对比（与第一批基线对比）

| 素材 | 批次一（default，-threads 0）| 批次二（csv，-threads 0）| 节省 |
|------|--------------------------|------------------------|------|
| car.mp4 | 6.84 s | **6.59 s** | **−3.7%** |
| 10min_noAudio_60fps.mp4 | 7.41 s | **1.87 s** | **−74.8%** |

---

## 日志体积对比

| 素材 | 批次一（default）| 批次二（csv）| 减少 |
|------|----------------|-------------|------|
| car.mp4 | 3.48 MB（3,643,270 B）| **0.93 MB（980,088 B）**| **−73.1%** |
| 10min_noAudio_60fps.mp4 | 11.36 MB（11,906,236 B）| **3.11 MB（3,261,336 B）**| **−72.6%** |

---

## 帧数一致性验证

| 素材 | 批次一帧数 | 批次二帧数 | 字段抽样验证 |
|------|-----------|-----------|------------|
| car.mp4 | 11280 | 11280 | ✅ 前 3 帧 key/pts/type 完全一致 |
| 10min_noAudio_60fps.mp4 | 36626 | 36626 | ✅ |

---

## 两批叠加后的总收益（与原始基线对比）

| 素材 | 原始基线 | 两批叠加后 | 总节省 |
|------|---------|-----------|--------|
| car.mp4（ffprobe）| 47.22 s | **6.59 s** | **−86.0%** |
| 10min 60fps（ffprobe）| 7.09 s | **1.87 s** | **−73.6%** |
| car.mp4 日志体积 | 3.48 MB | **0.93 MB** | **−73.3%** |
| 10min 日志体积 | 11.36 MB | **3.11 MB** | **−72.6%** |

---

## 结论

- **10min 60fps 视频提速最显著**（7.41s → 1.87s，-74.8%）：该视频为 I/O 密集型，CSV 格式将日志从 11.9 MB 压缩到 3.1 MB，I/O 开销大幅下降。
- **car.mp4 提速有限**（6.84s → 6.59s，-3.7%）：该视频为 CPU/解码密集型，时间主要耗在帧解码上，I/O 收益有限但日志体积仍减小 73%。
- 日志体积减小约 **73%**，有助于降低磁盘占用和后续解析内存开销。
- **向后兼容**：ShowInfoParser 同时支持 CSV 和 default 格式，旧日志文件仍可正常解析。
