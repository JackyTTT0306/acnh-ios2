// ACNH 宿主的最小 stub：D2R 专用的功能层在 ACNH 里没有对应物，这里给空实现，
// 让通用的装载 / 系统调用 / 服务层能链接起来。后续按 ACNH 的真实需求逐个替换。
// Minimal stubs for the ACNH host: D2R's feature layer has no counterpart here; empty
// implementations let the generic loader / SVC / service layer link. Replace them one by one
// as ACNH's real requirements become known.
#include "dm.h"
#include "front.h"
#include <stdint.h>

void dm_prof_add(int id, uint64_t ns, uint64_t bytes) { (void)id; (void)ns; (void)bytes; }
void dm_prof_mark(const char *seg) { (void)seg; }
uint64_t dm_prof_ribbon_ns(void) { return 0; }
void dm_backtrace(uint64_t fp) { (void)fp; }

// 函数指针变量：diag.c 在 D2R 里初始化时登记；ACNH 最小集没有 diag.c，保持空。
// Function pointer variable: registered by diag.c at init in D2R; the ACNH minimal set has no diag.c, so keep it null.
uint32_t (*dm_thread_tick_fn)(void) = 0;

// 资源计数回调：图形层注册；ACNH 最小集无图形层，恒空。
// Resource-count callback: registered by the graphics layer; the ACNH minimal set has none, so always null.
static void (*g_res_cb)(uint32_t, const char *);
void (*dm_res_report_cb(void))(uint32_t, const char *why) { (void)why; return g_res_cb; }
void dm_res_report_register(void (*fn)(uint32_t, const char *)) { g_res_cb = fn; }

// 分辨率参数位：按 front.h 的约定返回 (u64)高 << 32 | 宽。
// Resolution parameter slot: return (u64)height << 32 | width as agreed in front.h.
uint64_t dm_game_res(int docked) { return front_game_res(docked); }

// i18n 占位：D2R 的 dm_i18n.c 提供真实翻译表；ACNH 最小集直接返回 key。
// i18n placeholder: D2R's dm_i18n.c provides the real table; the ACNH minimal set returns the key.
const char *dm_ui(const char *key) { return key; }

// 剪口宿主调用（0x80 起）：ACNH 还没有任何登记，恒未处理。
// Host calls from cuts (0x80 and up): ACNH has none registered yet, so always unhandled.
uint64_t svc_host_call(int num, uint64_t *regs) { (void)num; (void)regs; return 0; }

// 客体地址 → 宿主对象旁表（hle_os.c / hle_audio.c 用）。ACNH 最小宿主先用最简单的数组实现。
// Guest address → host object side table (used by hle_os.c / hle_audio.c). The minimal ACNH host uses a plain array.
static const void *g_obj_key[512];
static void *g_obj_val[512];
static int g_obj_n;

void *objmap_get(const void *guest) {
    for (int i = 0; i < g_obj_n; i++) if (g_obj_key[i] == guest) return g_obj_val[i];
    return 0;
}
void objmap_put(const void *guest, void *host) {
    for (int i = 0; i < g_obj_n; i++) if (g_obj_key[i] == guest) { g_obj_val[i] = host; return; }
    if (g_obj_n < 512) { g_obj_key[g_obj_n] = guest; g_obj_val[g_obj_n] = host; g_obj_n++; }
}
void *objmap_del(const void *guest) {
    for (int i = 0; i < g_obj_n; i++) if (g_obj_key[i] == guest) {
        void *v = g_obj_val[i];
        g_obj_key[i] = g_obj_key[g_obj_n - 1];
        g_obj_val[i] = g_obj_val[g_obj_n - 1];
        g_obj_n--;
        return v;
    }
    return 0;
}

// hle_fs.c 的 D2R 语言钩子：ACNH 用 nn::oe 的语言接口，这里给固定值。
// D2R language hooks used by hle_fs.c: ACNH goes through nn::oe, so return fixed values.
int dm_sys_lang(void) { return 0; }            // 0 = 日语（D2R 的 Settings.json 用；对 ACNH 无影响）
int dm_custom_lang_explicit(void) { return -1; } // -1 = 未设置，hle_fs 跳过语言改写
