# NewPress 官方插件

NewPress 的独立插件分发仓库。主程序与主题源码保留在各自仓库，这里仅发布经过签名的目录、插件包和发布说明。

| 来源 | 用途 |
| --- | --- |
| `Orangefen/newpress_Plug`（GitHub） | 官方发布源 |
| `chunyangnet_0/newpress_plug`（Gitee） | 面向无法稳定访问 GitHub 的国内服务器的镜像 |

两个仓库应推送同一提交。NewPress 从部署服务器并行检测来源，验证签名后优先使用较新的目录，同版本按响应时间选择。包下载失败会切换到目录内容一致的可用镜像。浏览者无需选择源，也不会因访问插件市场而被要求登录 GitHub 或 Gitee。

## 安装

配套的 NewPress 主程序已内置这组官方目录与公钥，更新部署后，在后台「官方插件」中刷新目录，按需安装并启用即可。`deployment.env.example` 用于需要显式配置或固定发布提交的部署。两个仓库及文件必须公开可读；Gitee raw 文件到官方 `raw.giteeusercontent.com` 的同路径签名跳转由主程序安全适配。

插件默认不自动安装；安装后默认停用。停用保留插件设置，完整卸载清理插件独立目录和私有数据。

## 当前插件

| 插件 | 版本 | 功能 |
| --- | --- | --- |
| 评论来源（`comment-origin`） | 1.0.0 | IP 城市归属地、PC/移动设备与浏览器标签、作者用户组显示权限、自定义查询服务 |

评论来源使用 `builtin/comment-origin-v1`：实际业务代码随 NewPress 主程序提供，仓库中的包用于激活已审核扩展。功能升级需要同时更新 NewPress。此包不会下载并执行任意 Go 代码；通用 WASM 业务 Hook 和保留数据的原地升级尚未提供。

## 文件

- `catalog.json`：供 NewPress 验证的 Ed25519 签名目录。
- `catalog.readable.json`：方便查看的目录原文，不作为信任来源。
- `packages/`：由签名目录指定、带 SHA-256 和长度校验的插件 ZIP。
- `signing-public-key.txt`：公开验证密钥。
- `deployment.env.example`：双源配置示例，仅包含公开信息。

私钥、站点数据、API 密钥和访问令牌不属于这个仓库。发布与镜像同步步骤见 [PUBLISHING.md](PUBLISHING.md)。
