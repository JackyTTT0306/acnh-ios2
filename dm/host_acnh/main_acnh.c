// ACNH 最小宿主引导（headless）：加载 main/sdk/subsdk0，照 SDK 的 nn::init::Start 顺序初始化，再进 nnMain。
// 与 D2R 的 main.c 的区别：去掉存档节流、固定种子、触屏布局、TACT 预取、脚本按键等本游戏专用逻辑。
// ACNH minimal host bootstrap (headless): load main/sdk/subsdk0, initialize the SDK runtime in nn::init::Start order, then enter nnMain.
// Differences from D2R's main.c: no save throttling, fixed seed, touch layout, TACT prefetch or scripted keys.
#define _XOPEN_SOURCE 700   // macOS 的 ucontext（crash.c 的信号恢复用）需要它
#include "dm.h"
#include "front.h"
#include <stdatomic.h>
#include <unistd.h>
#include <sys/mman.h>
#include <ucontext.h>
#include <errno.h>
#include <mach/mach.h>

void guest_thread_init(void);
uint32_t svc_thread_main_init(void);
void crash_install(void);
void dm_fault_hook_register(int (*fn)(void *addr, int is_write, void *uc));
extern uintptr_t g_watch_lo, g_watch_hi;   // hle_table.c：memset 监视范围

// 诊断：监视 main+0x32e7ad8 所在页的写入，记录写入者 PC（该全局指针指向一个未构造的 BSS 对象）。
// Diagnostic: watch writes to the page holding main+0x32e7ad8 and record the writer's PC (that global
// points at an unconstructed BSS object).
static uintptr_t g_watch_addr;
static int watch_fault(void *addr, int is_write, void *uc) {
    if (!is_write) return 0;
    ucontext_t *u = (ucontext_t *)uc;
    LOG("watch: 写 %p（pc=%#llx），页恢复可写", addr, (unsigned long long)u->uc_mcontext->__ss.__pc);
    mprotect((void *)(g_watch_addr & ~(uintptr_t)0xfff), 0x1000, PROT_READ | PROT_WRITE);
    return 1;
}

typedef void (*fn0)(void);
typedef void (*fn2)(uint64_t, uint64_t);
typedef void (*fnp)(void *);

// 初始化链路探针（改写器 OBS_TRACE 门）：只读寄存器并打印，便于定位“谁调用了谁”。
// 注意：regs[30] 是门块返回点（补丁区地址），不是 guest 的 lr；guest 的调用者要从 x29 帧链回溯
// （fn_B 的 prologue 还没执行，所以 regs[29] 就是调用者的帧指针，[fp+8] 是调用者的返回地址）。
// Init-chain probe (rewriter's OBS_TRACE gate): reads registers and prints them, to see who calls whom.
// Note: regs[30] is the gate block's return point (a patch-area address), not the guest's lr; the guest
// caller comes from the x29 frame chain (fn_B's prologue has not run yet, so regs[29] is the caller's
// frame pointer and [fp+8] is the caller's return address).
static void trace_cb(const uint64_t *regs) {
    LOG("trace: x0=%#llx x1=%#llx x2=%#llx x3=%#llx x4=%#llx x5=%#llx x6=%#llx x7=%#llx x19=%#llx x21=%#llx x29=%#llx",
        (unsigned long long)regs[0], (unsigned long long)regs[1], (unsigned long long)regs[2],
        (unsigned long long)regs[3], (unsigned long long)regs[4], (unsigned long long)regs[5],
        (unsigned long long)regs[6], (unsigned long long)regs[7], (unsigned long long)regs[19], (unsigned long long)regs[21], (unsigned long long)regs[29]);
    // 安全读 [x21]（vm_read 失败不影响进程）：看单例 +0 字段与它指向的对象
    // Safely read [x21] (vm_read failure does not affect the process): the singleton +0 field and what it points to
    if (regs[21]) {
        uint64_t v = 0, w = 0; vm_size_t got = 0;
        if (vm_read_overwrite(mach_task_self(), (vm_address_t)regs[21], 8, (vm_address_t)&v, &got) == KERN_SUCCESS && got == 8) {
            LOG("  [x21] = %#llx", (unsigned long long)v);
            if (v && vm_read_overwrite(mach_task_self(), (vm_address_t)v, 8, (vm_address_t)&w, &got) == KERN_SUCCESS && got == 8)
                LOG("  [x21]+0 = %#llx", (unsigned long long)w);
        }
    }
    uint64_t fp = regs[29];
    for (int i = 0; i < 5 && fp && !(fp & 7); i++) {
        uint64_t nfp = ((uint64_t *)fp)[0], ra = ((uint64_t *)fp)[1];
        LOG("  caller[%d]: %#llx", i, (unsigned long long)ra);
        if (nfp <= fp) break;
        fp = nfp;
    }
}

struct dm_flags g_flag;
uint64_t g_t0_ns;

static char *g_argv[64] = {"AnimalCrossing"};
static int g_argc = 1;

static void *req(const char *n) {
    void *p = guest_sym(n);
    if (!p) die("缺符号 %s", n);
    return p;
}

static void flags_init(void) {
    g_flag.thr = !!getenv("DM_THR");
    g_flag.svc = !!getenv("DM_SVC");
    g_flag.fs = !!getenv("DM_FS");
    g_flag.movie = 0;
    g_flag.dbg = !!getenv("DM_DBG");
    g_flag.imports = !!getenv("DM_IMPORTS");
    g_flag.bootprof = !!getenv("DM_BOOTPROF");
}

static void guest_main(void) {
    guest_thread_init();
    uint32_t hmain = svc_thread_main_init();   // 主线程句柄：nnosInitialize 的 x0
    ((fn0)req("_init_libc0"))();
    LOG(">>> nnosInitialize");
    ((fn2)req("nnosInitialize"))(hmain, 0);
    ((fn0)req("_init_libc1"))();
    LOG(">>> nninitInitializeSdkModule");
    ((fn0)req("nninitInitializeSdkModule"))();
    LOG(">>> nninitStartup");
    ((fn0)req("nninitStartup"))();
    ((fn0)req("_init_libc2"))();
    dm_prof_mark("SDK init");
    run_module_inits();   // rtld 回调 B：各模块 DT_INIT（全局构造函数）
    dm_prof_mark("init_array");
    ((fnp)req("_ZN2nn2os11SetHostArgcEi"))((void *)(uint64_t)g_argc);
    ((fnp)req("_ZN2nn2os11SetHostArgvEPPc"))(g_argv);
    {   // 诊断：全局单例槽（ACNH 的 GameSystem 之类）。nnMain 早期虚调用读它，为空即崩在 main+0x19a6250。
        // Diagnostic: the global singleton slot (ACNH's GameSystem and the like). An early virtual call in
        // nnMain reads it; when it is null the guest crashes at main+0x19a6250.
        for (int i = 0; i < g_nmod; i++)
            if (!strcmp(g_mod[i].name, "main")) {
                void *inst = *(void **)(g_mod[i].base + 0x32e7ad8);
                uint64_t *p = (uint64_t *)inst;
                LOG("diag: base=%p inst=%p (inst-base=%#llx) [0]=%#llx [8]=%#llx（进 nnMain 前）",
                    g_mod[i].base, inst, (unsigned long long)((uintptr_t)inst - (uintptr_t)g_mod[i].base),
                    p ? (unsigned long long)p[0] : 0, p ? (unsigned long long)p[1] : 0);
                g_watch_lo = (uintptr_t)(g_mod[i].base + 0x32e7ad8);
                g_watch_hi = g_watch_lo + 8;
            }
    }
    LOG(">>> nnMain");
    if (getenv("DM_TRACE")) gate_obs_set(OBS_TRACE, trace_cb);
    ((fn0)req("nnMain"))();
    LOG("nnMain returned");
}

int main(int argc, char **argv) {
    g_t0_ns = now_ns();
    flags_init();
    if (argc < 2) die("usage: dmhost_acnh <rewrite dir> [mod...] [-- game args...]");
    crash_install();
    const char *names[MAX_MOD];
    int n = 0;
    int i = 2;
    for (; i < argc && strcmp(argv[i], "--") && n < MAX_MOD; i++) names[n++] = argv[i];
    if (i < argc && !strcmp(argv[i], "--"))
        for (i++; i < argc && g_argc < 63; i++) g_argv[g_argc++] = argv[i];
    if (!n) { names[n++] = "main"; names[n++] = "sdk"; names[n++] = "subsdk0"; }
    load_modules(argv[1], names, n);
    front_run(guest_main);
    return 0;
}
