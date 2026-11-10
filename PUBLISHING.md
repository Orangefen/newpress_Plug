# 发布与镜像同步

此仓库只接受 NewPress 官方发布内容，不开放任意第三方插件上传。

## 首次上线

1. 在 GitHub 的 `Orangefen` 账号下创建公开空仓库 `newpress_Plug`。
2. 在 Gitee 的 `chunyangnet_0` 账号下创建公开空仓库 `newpress_Plug`，不自动生成 README、许可证或 `.gitignore`。
3. 本地使用已有 Git 凭据分别推送 `main`，无需把令牌写进远程 URL：

```sh
git push -u origin main
git push gitee main
```

4. 分别匿名访问两个 raw 目录和 ZIP，确认不是登录页、拦截页或跳转响应。Gitee 仓库是否真正公开，以匿名 raw GET 为准。
5. 配置 `deployment.env.example` 中的两个目录 URL 和公钥，执行后台刷新、安装、启用、停用和卸载检查。

## 后续发布

在 NewPress 主程序源码目录使用 `cmd/pluginmarket`，私钥路径始终指向仓库之外：

```sh
go run ./cmd/pluginmarket -key /secure/newpress-plugin-signing.key -out /tmp/newpress-release-2 -sequence 2 -expires 2027-11-10T00:00:00Z
```

`sequence` 必须大于此前发布过的所有序号，到期时间必须在未来。复制新生成的 `catalog.json`、`catalog.readable.json` 及 `packages/` 到本仓库，一次提交后推送两个远程。保留既有版本 ZIP，避免仍使用旧目录的站点无法下载；同版本包内容不应被原地替换。

不要分别为两个镜像重新生成目录。两端必须使用相同的签名文件和插件包；同序号改内容会被客户端拒绝，需要使用新序号发布。Gitee 落后时客户端只使用已观察到的较新目录，不会为图快而降级。两个源均不可达时不会禁用已安装插件。

默认示例使用 `main` 作为签名发布通道，刷新目录时可读取后续发布。要求固定版本的部署也可将两个 URL 的 `main` 替换为同一提交 SHA；升级时需同步更改这两个 URL。

签名公钥是站点信任根。妥善离线备份私钥；更换公钥需要通过可信的主程序发布或部署配置更新完成，不能只依赖镜像中的新公钥文件。本文不记录私钥的本机路径，也不要求将私钥放到 GitHub/Gitee 或 CI。

目前发布工具只生成评论来源插件包。通用插件运行时及保留数据的原地升级仍需在主程序实现后再发布。
