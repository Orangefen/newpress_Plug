# 文章时效提醒

独立 WASM 插件示例，源码在 `plugin.c`。不随主程序安装。

用 Zig 0.16 的 C 编译器构建，无 WASI、文件系统或网络运行库：

```sh
zig cc -target wasm32-freestanding -O2 -nostdlib -Wl,--no-entry -Wl,--export-memory -Wl,-z,stack-size=65536 -Wl,--initial-memory=262144 -Wl,--max-memory=33554432 -o plugin.wasm plugin.c
```

将 `plugin.json`、`plugin.wasm` 打包到 ZIP 根目录，通过官方签名目录发布。安装后进入设置与权限，保存配置并授予声明的三个权限，再启用。后台业务操作输入 `{"action":"preview"}` 可验证自定义文案。停用后文章立即停止显示；卸载删除插件与所有配置。

插件接收公开文章的更新时间，自己比较阈值，并通过 `newpress.host_call` 读取自己的设置。核心程序仅负责传递公开事实、检查权限和安全展示结果。
