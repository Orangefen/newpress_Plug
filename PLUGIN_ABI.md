# 官方插件业务 ABI v1

插件市场只分发经过官方发布流程签名的插件。普通插件是独立 WASM，默认不安装；下载后默认停用，管理员先保存设置和权限，再启用。`comment-origin` 保留现有官方内置适配方式。友链、反馈等主题附带页面不属于插件。

真实、可重建的示例：`examples/plugins/content-age/`。C 源码通过 Zig 编译为无 WASI 的 `plugin.wasm`，文章时效判断和自定义文案在插件中执行，核心只传递公开摘要并渲染结果。不要使用只有 `np_api_version` 的探测模块冒充业务插件。

## 导出与传参

Runtime 为 `wasm/core-v1`，模块必须导出线性 `memory` 和以下函数：

```c
uint64_t np_api_version(void);                 // 返回 1
uint32_t np_alloc(uint32_t bytes);             // 返回非零指针，0 表示失败
void np_free(uint32_t pointer, uint32_t bytes);
uint64_t np_handle(uint32_t pointer, uint32_t bytes);
```

所有请求、响应均为 UTF-8 JSON。业务函数返回 `(uint64_t(pointer) << 32) | length`。宿主复制结果后分别释放输出、输入；插件可使用固定缓冲区与空操作 `np_free`，因为每次调用都销毁整个实例。导出名称由 manifest `hooks` 指定，不必固定为 `np_handle`，签名必须一致。

目前实际接入两个 hook，每个 hook 一个导出：

| Hook | 触发与输入 | 输出 |
| --- | --- | --- |
| `content.render` | 渲染公开文章；提供 `post` 摘要 | 可选安全提示片段 |
| `admin.action` | 管理员调用 `/api/v1/admin/plugins/{slug}/execute` | JSON 结果及可选提示片段 |

```json
{"schema":1,"event":"content.render","now":1794312000,"post":{"id":1,"title":"文章标题","url":"/posts/example","updated_at":1700000000,"published_at":1690000000,"excerpt":"公开摘要"}}
```

未授权 `content.read` 时不传 `post`。`admin.action` 的自定义参数位于 `payload`，不会自动把 HTTP 请求头、cookie、登录令牌或用户私密资料交给插件。

```json
{"fragment":{"title":"留意内容时效","text":"部分信息可能发生变化。","tone":"warning"},"data":{"ok":true}}
```

`fragment` 和 `data` 都可省略。片段只允许标题（120 字）、正文（2000 字）、`info / warning / success` 色调，要求 `render.fragment` 权限，由主题转义成普通文本。没有任意 HTML、JavaScript、模板或 CSS 注入。宿主当前在公开文章正文前渲染，不调用草稿、预览或普通页面的 hook；第三方主题需显式接入 `PluginFragments` 才会显示。

## 宿主接口

唯一允许导入：

```c
__attribute__((import_module("newpress"), import_name("host_call")))
uint32_t host_call(uint32_t request, uint32_t size,
                   uint32_t response, uint32_t capacity);
```

返回实际响应字节数，内存越界 / 超预算返回 `UINT32_MAX`。业务错误以 `{"ok":false,"error":"..."}` 返回，成功为 `{"ok":true,"data":...}`。导入请求与响应缓冲区均最多 64 KiB。每次操作同时校验 manifest 声明和管理员授权。

| `method` | 权限 | 参数与行为 |
| --- | --- | --- |
| `settings.get` | `settings.read` | 返回本插件配置值，不包含其他插件或全站设置 |
| `settings.set` | `settings.write` | `value` 是完整配置对象；仅管理员业务操作，校验 schema 与版本 |
| `cache.get` | `cache` | `key` 读取本插件独立持久 KV，未找到返回 null |
| `cache.set` | `cache` | `key`、JSON `value`；仅管理员业务操作 |
| `cache.delete` | `cache` | `key` 删除；仅管理员业务操作 |
| `content.get` | `content.read` | `id` 读取已发布且到达发布时间的文章公开摘要 |
| `http.get` | `http.outbound` | `url` 必须匹配 manifest `outbound_hosts` 精确域名；公网 HTTPS 443，不自动重定向；返回 `status` 和最多 32 KiB 的 `body` |

`cache` 是插件隔离的持久 KV，不提供 TTL；总计最多 64 KiB、128 个键。键格式为 `[A-Za-z_][A-Za-z0-9_]{0,95}`，无法拼接文件路径。公开渲染只能读取，不能通过 GET 产生持久写入。管理员业务可多次调用宿主，各写入独立提交；插件应设计幂等操作，不能假设整次业务具有跨调用事务。

当前不实现内容写入、后台任务队列、任意用户 API 或自定义管理导航。声明 `content.write`、`jobs.enqueue` 或未接入 hook 的业务插件会被明确拒绝，不能把计划中的能力当作可用接口。

## 配置与权限

manifest `settings` 是最多 32 项的数组，字段为 `key / label / help / type / default / min / max`，类型支持 `text / number / boolean`。文本 `max` 最大 4000 字；数字为非负整数且受 min/max 限制。所有字段必须提供默认值。

系统自动生成 `/admin/plugins/{slug}/` 配置与授权页，不运行插件提供的管理脚本。保存接口 `/api/v1/admin/plugins/{slug}/configuration` 接收 `configuration:{version,values,grants}` 和 `expected_user_id`。执行接口接收 `payload` 和 `expected_user_id`。两者要求 `plugins:manage`、同源验证、CSRF 和真实登录状态；配置冲突返回 409，执行限制每分钟 30 次。

业务实例不共享内存，无 WASI、系统环境、文件或进程入口。默认内存上限 32 MiB，每次实例化和函数调用使用 250 ms deadline，宿主最多 32 次调用，全站最多 8 个并发实例；文章总调用窗口 400 ms、最多展示 8 个结果。连续 3 次执行失败后冷却 30 秒，页面继续显示原内容。设置页显示当前服务进程的调用数、失败数与最近耗时，不记录用户正文或密钥。

每次执行核对官方安装凭据、manifest 和模块摘要。停用、改权限、卸载会取消旧一代请求；过期请求不能把数据重新写回已卸载的目录。插件数据仅存于 `plugins/{slug}/data/`。卸载先原子移走整个命名空间，再清除文件；中断清理由启动恢复完成。原地升级与保留数据迁移尚未提供，请先备份再卸载重装。

## 发布与兼容

将 `plugin.json` 和 `plugin.wasm` 放在 ZIP 根目录，不允许携带 `data`、`.official.json`、`.integrity.json` 或 `.disabled`。官方发布工具支持多个审核后的 ZIP：

```sh
go run ./cmd/pluginmarket -key /secure/signing.key -out /tmp/release \
  -sequence 2 -expires 2027-11-10T00:00:00Z \
  -packages /tmp/content-age-1.0.0.zip -catalog-name catalog.runtime-v1.json
```

`-packages` 用逗号分隔。工具验证与签名同一份包快照，再生成签名目录。主程序默认使用新版 `catalog.runtime-v1.json`；旧 `catalog.json` 保留，以免旧版严格解码器因未知配置字段拒绝整个市场。两个镜像上传相同目录和包，序号必须递增；绝不把私钥放入仓库。示例源码和可复现构建命令随插件仓库公开。
