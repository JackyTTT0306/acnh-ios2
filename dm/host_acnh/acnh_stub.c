// ACNH 宿主的最小 stub：D2R 专用的功能层在 ACNH 里没有对应物，这里给空实现，
// 让通用的装载 / 系统调用 / 服务层能链接起来。后续按 ACNH 的真实需求逐个替换。
// Minimal stubs for the ACNH host: D2R's feature layer has no counterpart here; empty
// implementations let the generic loader / SVC / service layer link. Replace them one by one
// as ACNH's real requirements become known.
#include "dm.h"
#include "front.h"

void dm_prof_add(int id, uint64_t ns, uint64_t bytes) { (void)id; (void)ns; (void)bytes; }
void dm_prof_mark(const char *seg) { (void)seg; }
uint64_t dm_prof_ribbon_ns(void) { return 0; }
void dm_backtrace(uint64_t fp) { (void)fp; }

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
