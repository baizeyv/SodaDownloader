# SodaDownloader

SodaDownloader 是一个针对汽水音乐通过分享链接来下载高质量歌曲的小工具.

还有一些瑕疵,不知道如何解决: 部分flac解密后无法播放,但是大部分还是可以播放的.

## 使用方式

通过 `SodaDownloader.exe -o your_output_path -a your_aid -s your_session` 来初始化配置

后续只需要通过 `SodaDownloader.exe -l "《foorbar》@汽水音乐 https://qishui.douyin.com/s/foobar/"`

来进入程序, 之后选择自己想下载的 bitrate 的文件即可

注意: 部分flac通过这个程序无法成功解密,请下载后核对可以播放,否则下载个高质量m4a也是可以的

## 灵感来源

[music-lib]("https://github.com/guohuiyuan/music-lib")

[qishui-decrypt]("https://github.com/helloplhm-qwq/qishui-decrypt")

## 免责声明

本项目仅供个人学习和技术研究使用。在使用本库时，请遵守相关法律法规及音乐平台用户协议。通过本库获取的资源，请在 24 小时内删除。