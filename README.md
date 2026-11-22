# RK3506 语音对话 Demo

基于 C、ALSA 和云端语音服务的 RK3506 Linux 语音对话示例。程序通过麦克风录音，调用 ASR 转为文字，再将文字发送给大语言模型，最后通过 TTS 合成并播放回答。

项目用于验证端到端语音对话流程，采用同步、逐轮执行方式，保留当前进程内的对话历史。

## 工作流程

```text
等待语音 → 录音 → ASR 语音识别 → LLM 生成回答 → TTS 语音合成 → 播放
   ↑                                                               │
   └─────────────────────── 进入下一轮 ──────────────────────────────┘
```

1. 初始化日志、配置、ALSA 录音与播放设备，以及 HTTP 客户端。
2. 根据音量 RMS 阈值判断开始录音，在连续静音后结束录音。
3. 为录音添加 WAV 文件头，编码为 Base64，提交给 ASR 服务。
4. 将识别文本及历史消息发送给 LLM，记录本轮用户消息和模型回答。
5. 将回答提交给 TTS，下载返回的音频并播放。
6. 释放本轮临时数据，继续等待下一次语音。

代码当前使用的服务和模型如下，模型名称固定在对应源文件中：

| 功能 | 服务 | 模型 |
| --- | --- | --- |
| ASR | 阿里云百炼，北京地域、工作空间域名 | `qwen-audio-3.1-asr-flash` |
| LLM | DeepSeek，`https://api.deepseek.com/chat/completions` | `deepseek-flash` |
| TTS | 阿里云百炼，北京地域、工作空间域名 | `qwen-audio-3.1-tts-flash` |

## 目录结构

```text
.
├── CMakeLists.txt                 # 构建入口
├── cmake/
│   └── rk3506_toolchain.cmake     # ARM 交叉编译工具链与 sysroot 配置
├── config/
│   ├── config.json               # 本地服务配置，不纳入 Git
│   └── zlog.conf                 # 日志格式与输出规则
├── include/                      # 模块头文件
├── src/
│   ├── main.c                    # 初始化与语音对话主循环
│   ├── audio/
│   │   ├── audio_record.c        # ALSA 录音、音量检测、WAV 封装
│   │   └── audio_play.c          # ALSA 播放、单声道转双声道
│   ├── common/
│   │   ├── config.c              # JSON 配置与全局初始化
│   │   ├── http_client.c         # libcurl 请求与响应缓冲区
│   │   └── json_fun.c            # JSON 字段读取
│   └── services/
│       ├── asr_serivice.c        # 语音识别
│       ├── llm_service.c         # 对话请求与历史消息
│       └── tts_serivice.c        # 语音合成与音频下载
└── third_library/
    ├── include/                  # json-c、zlog 头文件
    └── lib/                      # json-c、zlog 静态库
```

本地还可包含 `build.sh`、`temp/` 构建目录和 `test/` 响应 JSON 示例；这些路径在当前 `.gitignore` 中被忽略。`test/` 中的 JSON 是响应样例，不是自动化测试。

## 构建环境

- Linux 主机，安装 CMake 3.10 或以上版本，以及相应构建工具。
- ARM hard-float 交叉编译工具链：`arm-none-linux-gnueabihf`。
- 与目标板匹配的 sysroot，其中需要包含 libcurl、ALSA 的开发头文件和链接库。
- 仓库附带的 json-c、zlog 静态库需要与工具链及目标 ABI 匹配。

默认路径由 `cmake/rk3506_toolchain.cmake` 定义：

```text
$HOME/RK3506/
├── arm-none-linux-gnueabihf/
│   └── bin/
│       ├── arm-none-linux-gnueabihf-gcc
│       └── arm-none-linux-gnueabihf-g++
└── sysroot/
```

如本机目录不同，请调整工具链文件中的 `DEVELOP_PREFIX`、`TOOLCHAIN_PREFIX` 和 `CMAKE_SYSROOT`。当前顶层 CMake 固定引用此工具链文件。

在仓库根目录执行：

```bash
cmake -S . -B temp
cmake --build temp
```

生成的目标板可执行文件为 `temp/aichat`。程序链接 libcurl、ALSA、json-c、zlog、libm 和 libresolv；运行时所需的动态库也应存在于目标板。

## 服务配置

在 `config/config.json` 中填写以下字段，示例值需要替换为自己的配置：

```json
{
  "aliyun_WorkspaceId": "YOUR_ALIYUN_WORKSPACE_ID",
  "aliyun_APIKEY": "YOUR_ALIYUN_API_KEY",
  "deepseek_APIKEY": "YOUR_DEEPSEEK_API_KEY",
  "llm_role": "你是一个语音助手，请用简洁的中文回答。",
  "tts_voice": "YOUR_TTS_VOICE"
}
```

| 字段 | 说明 |
| --- | --- |
| `aliyun_WorkspaceId` | 用于构造阿里云 ASR、TTS 请求域名的工作空间 ID |
| `aliyun_APIKEY` | 阿里云 ASR、TTS 请求使用的 API Key |
| `deepseek_APIKEY` | DeepSeek 对话请求使用的 API Key |
| `llm_role` | 对话历史中的初始 system 消息 |
| `tts_voice` | TTS 音色名称，需要与所用服务匹配 |

字段名称区分大小写，以上字段均需填写非空字符串。现有配置若包含 `tts_instruction`，当前代码不会读取或发送该字段。采样率、模型名称和服务地址目前在源码中设置。

`config/config.json` 已被 Git 忽略；请勿将真实 API Key 写入 README 或提交到仓库。

## 部署与运行

目标板需要具备可用的录音和播放设备、对应动态库，以及访问上述云端服务的网络环境。

以下以目标板地址 `192.168.31.6`、部署目录 `/root` 为例：

```bash
scp -r temp/aichat config root@192.168.31.6:/root/
ssh root@192.168.31.6
cd /root
./aichat
```

程序通过相对路径读取 `config/config.json` 和 `config/zlog.conf`，因此必须在包含 `config/` 的目录中启动。

如果本地存在 `build.sh`，执行 `bash build.sh` 会依次构建并将程序及配置复制到脚本中固定的目标板地址；使用前检查脚本中的地址和部署路径。

启动后直接对麦克风讲话，停顿后程序会识别并播放回答，然后进入下一轮。通过 `Ctrl+C` 结束进程。当前没有命令行参数、唤醒词或主动退出命令。

## 音频与日志

录音和播放均使用 ALSA 设备 `hw:0,0`，采样率为 16 kHz，格式为 `S16_LE`、双声道。录音时将右声道复制到左声道，封装为双声道 WAV；TTS 请求 PCM 音频，播放代码按 16 位单声道数据复制为双声道。

录音初始化还会访问 `hw:0` 上的 `Right Channel Capture Volume` 控件，并将增益设为 `8`。更换声卡或板卡时，需要检查设备编号、控件名称和音频参数。

录音每次读取约 20 ms 数据，RMS 大于 `200` 时开始录音，连续 18 次 RMS 小于 `50` 时结束录音，正常读取时对应约 360 ms 静音。相关阈值位于 `src/audio/audio_record.c`。

日志由 zlog 输出到标准错误，包含 `System`、`Record`、`Play`、`ASR`、`LLM`、`TTS` 分类。主循环会记录识别文本与 AI 回答。需要保存时可执行：

```bash
./aichat 2> run.log
```

## Demo 的边界

- 对话历史仅保存在内存中，重启后清空；当前没有历史长度限制，适合短时演示。
- 单次录音没有时长或容量上限，持续噪声可能导致录音无法结束并持续扩容。
- 录音、识别、生成、合成和播放顺序执行，不支持流式输出、播放打断或同时录放。
- 部分设备、网络或响应解析错误会直接结束进程，当前没有完整的重试与恢复机制。
- 主循环没有正常退出分支，循环后的清理函数不会在正常循环过程中执行。

排查启动问题时，先确认工作目录和配置字段，再检查 ALSA 设备及控件是否与目标板一致。若 CMake 找不到 CURL 或 ALSA，请检查 sysroot 内的开发文件；云端调用失败时，可结合日志中的 HTTP 状态及响应内容检查凭据、工作空间和模型配置。
