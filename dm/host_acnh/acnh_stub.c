// ACNH 宿主的最小 stub：D2R 专用的功能层在 ACNH 里没有对应物，这里给空实现，
// 让通用的装载 / 系统调用 / 服务层能链接起来。后续按 ACNH 的真实需求逐个替换。
// Minimal stubs for the ACNH host: D2R's feature layer has no counterpart here; empty
// implementations let the generic loader / SVC / service layer link. Replace them one by one
// as ACNH's real requirements become known.
//
// 注意：dm_backtrace / dm_res_report_cb / dm_res_report_register 由 crash.c 提供，
// objmap_* 由 hle_os.c 提供——不要在这里重复定义（链接器会报 duplicate symbols）。
// Note: dm_backtrace / dm_res_report_cb / dm_res_report_register come from crash.c and
// objmap_* from hle_os.c -- do not define them here again (the linker reports duplicate symbols).
#include "dm.h"
#include "front.h"
#include <stdint.h>

void dm_prof_add(int id, uint64_t ns, uint64_t bytes) { (void)id; (void)ns; (void)bytes; }
void dm_prof_mark(const char *seg) { (void)seg; }
uint64_t dm_prof_ribbon_ns(void) { return 0; }

// 函数指针变量：D2R 里由 diag.c 初始化时登记；ACNH 最小集没有 diag.c，保持空。
// Function pointer variable: registered by diag.c at init in D2R; the ACNH minimal set has no diag.c, so keep it null.
uint32_t (*dm_thread_tick_fn)(void) = 0;

// 分辨率参数位：按 front.h 的约定返回 (u64)高 << 32 | 宽。
// Resolution parameter slot: return (u64)height << 32 | width as agreed in front.h.
uint64_t dm_game_res(int docked) { return front_game_res(docked); }

// i18n 占位：D2R 的 dm_i18n.c 提供真实翻译表；ACNH 最小集直接返回 key。
// i18n placeholder: D2R's dm_i18n.c provides the real table; the ACNH minimal set returns the key.
const char *dm_ui(const char *key) { return key; }

// 剪口宿主调用（0x80 起）：ACNH 还没有任何登记，恒未处理。
// Host calls from cuts (0x80 and up): ACNH has none registered yet, so always unhandled.
uint64_t svc_host_call(int num, uint64_t *regs) { (void)num; (void)regs; return 0; }

// hle_fs.c 的 D2R 语言钩子：ACNH 用 nn::oe 的语言接口，这里给固定值。
// D2R language hooks used by hle_fs.c: ACNH goes through nn::oe, so return fixed values.
int dm_sys_lang(void) { return 0; }              // 0 = 日语（D2R 的 Settings.json 用；对 ACNH 无影响）
int dm_custom_lang_explicit(void) { return -1; } // -1 = 未设置，hle_fs 跳过语言改写

// 性能计数（d2r 的 hle_count.c）与 UI 消息订阅（diag.c）：ACNH 最小集不需要，空实现。
// Performance counters (D2R's hle_count.c) and UI message subscription (diag.c): not needed here.
void *hlecount_wrap(const char *kind, const char *name, void *fn) { (void)kind; (void)name; return fn; }
void hlecount_seal(void) {}
void hlecount_svc(int num) { (void)num; }
void uimsg_subscribe(void (*fn)(const char *, const char *, const char *)) { (void)fn; }

// 音频生命周期（前端在切后台 / 回前台时调）：ACNH 最小集还没有音频前端，空实现。
// Audio lifecycle (called by the front end on background/foreground): no audio front end yet.
void dm_audio_suspend(int on) { (void)on; }
uint64_t dm_audio_render_calls(void) { return 0; }
