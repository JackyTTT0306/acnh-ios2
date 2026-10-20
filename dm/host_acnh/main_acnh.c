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
// 实验：单例 +0 字段为空导致 main+0x19a6250 崩溃；塞一个假对象（+0x20 指向宿主回调）看能否越过
// Experiment: the singleton +0 field is null; install a fake object (with a host callback at +0x20)
static volatile uint64_t g_obj;   // 循环等待的对象（[this+0x38]），从探针里捕获
static void *flag_thread(void *arg) {
    (void)arg;
    for (int i = 0; i < 100; i++) {
        usleep(100000);
        if (g_obj) {
            *(volatile uint64_t *)(uintptr_t)(g_obj + 0x120) = 1;
            LOG("flag: set [%p+0x120] = 1", (void *)(uintptr_t)g_obj);
            return NULL;
        }
    }
    LOG("flag: object never captured");
    return NULL;
}
static uint64_t fake_cb(void *self) {
    static int n;
    uint64_t lr; __asm__ volatile("mov %0, x30" : "=r"(lr));
    if (n < 6 || (n % 2000) == 0) LOG("fake_cb #%d lr=%#llx self=%p", n, (unsigned long long)lr, self);
    n++;
    return 0;
}
// 实验：把游戏里若干“全局函数指针表”指向宿主桩表，避免空表项导致的崩溃
// Experiment: point some guest global function-pointer tables at a host stub table
static uint64_t g_stubtbl[64];
static void install_global_stubs(void) {
    extern module_t g_mod[]; extern int g_nmod;
    for (int i = 0; i < 64; i++) g_stubtbl[i] = (uint64_t)fake_cb;
    uintptr_t base = 0;
    for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) base = (uintptr_t)g_mod[i].base;
    if (!base) return;
    static const uintptr_t offs[] = { 0x33186e8, 0x33186f0, 0x3318728, 0x3318900, 0x3318908, 0x3318910 };
    for (unsigned k = 0; k < sizeof(offs) / sizeof(offs[0]); k++) {
        uint64_t **tp = (uint64_t **)(base + offs[k]);
        uint64_t *tbl = *tp;
        if (!tbl || (uintptr_t)tbl < 0x10000) { LOG("stubtbl: [%#lx] null table", (unsigned long)offs[k]); continue; }
        int filled = 0;
        for (int i = 0; i < 64; i++) if (!tbl[i]) { tbl[i] = (uint64_t)fake_cb; filled++; }   // 只填空槽 | fill null slots only
        LOG("stubtbl: [%#lx] table=%p filled %d null slots", (unsigned long)offs[k], (void *)tbl, filled);
    }
}
static void install_fake_singleton(void) {
    extern module_t g_mod[]; extern int g_nmod;
    for (int i = 0; i < g_nmod; i++)
        if (!strcmp(g_mod[i].name, "main")) {
            uint8_t *obj = (uint8_t *)((uintptr_t)g_mod[i].base + 0x3a73750);
            uint8_t *fake = calloc(1, 0x200);
            for (int k = 0; k < 0x100; k += 8) *(uint64_t *)(fake + k) = (uint64_t)fake_cb;   // 整表都指向宿主空实现 | whole table points at the host stub
            *(uint64_t *)obj = (uint64_t)fake;
            LOG("fake: singleton[0] = %p", fake);
        }
}
// 用 RTTI 打印对象类型：obj -> vtable -> typeinfo -> name
// Print the object's RTTI: obj -> vtable -> typeinfo -> name
static void print_rtti(const char *tag, uint64_t p) {
    if (!p || (p & 7)) return;
    uint64_t vt = 0, ti = 0, np = 0; vm_size_t got = 0;
    if (vm_read_overwrite(mach_task_self(), (vm_address_t)p, 8, (vm_address_t)&vt, &got) != KERN_SUCCESS || got != 8 || !vt) return;
    if (vm_read_overwrite(mach_task_self(), (vm_address_t)(vt - 8), 8, (vm_address_t)&ti, &got) != KERN_SUCCESS || got != 8 || !ti) return;
    if (vm_read_overwrite(mach_task_self(), (vm_address_t)(ti + 8), 8, (vm_address_t)&np, &got) != KERN_SUCCESS || got != 8 || !np) return;
    char buf[160]; memset(buf, 0, sizeof buf);
    if (vm_read_overwrite(mach_task_self(), (vm_address_t)np, sizeof(buf) - 1, (vm_address_t)buf, &got) == KERN_SUCCESS)
        LOG("  %s rtti: vtable=%#llx typeinfo=%#llx name=%s", tag, (unsigned long long)vt, (unsigned long long)ti, buf);
}
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
    if (regs[19]) {   // 捕获循环等待的对象：obj = [this + 0x38]
        uint64_t o = 0; vm_size_t g2 = 0;
        if (vm_read_overwrite(mach_task_self(), (vm_address_t)(regs[19] + 0x38), 8, (vm_address_t)&o, &g2) == KERN_SUCCESS && o)
            g_obj = o;
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

// 临时探针：把客体的轮询函数入口改成 brk #0，宿主在这里打印 x0，并可在若干次后注入状态
// Temporary probe: the guest poll entry is patched to brk #0; the host logs x0 and can inject state after N hits
void crash_set_trap_hook(int (*fn)(ucontext_t *uc));
static int trap_probe(ucontext_t *uc) {
    static int n;
    extern module_t g_mod[]; extern int g_nmod;
    uint64_t pc = uc->uc_mcontext->__ss.__pc;
    uint64_t x0 = uc->uc_mcontext->__ss.__x[0];
    uintptr_t base = 0;
    for (int i = 0; i < g_nmod; i++) if (!strcmp(g_mod[i].name, "main")) base = (uintptr_t)g_mod[i].base;
    uint64_t off = base ? pc - base : 0;
    if (n < 4) LOG("trap: pc=%#llx off=%#llx x0=%#llx x1=%#llx x2=%#llx", (unsigned long long)pc, (unsigned long long)off,
                   (unsigned long long)x0, (unsigned long long)uc->uc_mcontext->__ss.__x[1], (unsigned long long)uc->uc_mcontext->__ss.__x[2]);
    n++;
    if (off == 0x197ce70) {   // dump this 的虚表槽与字段 | dump the vtable slots and fields of this
        uint64_t x19 = uc->uc_mcontext->__ss.__x[19];
        uint64_t vt = 0; vm_size_t g = 0;
        if (!vm_read_overwrite(mach_task_self(), (vm_address_t)x19, 8, (vm_address_t)&vt, &g) || !vt) {
            LOG("dump: no vtable, this=%#llx", (unsigned long long)x19);
            for (int o = 0; o < 0x60; o += 8) {
                uint64_t v = 0;
                if (vm_read_overwrite(mach_task_self(), (vm_address_t)(x19 + o), 8, (vm_address_t)&v, &g) == KERN_SUCCESS)
                    LOG("  this+%#x = %#llx", o, (unsigned long long)v);
            }
        }
        else {
            LOG("dump: this=%#llx vt=%#llx (vt-main=%#llx)", (unsigned long long)x19, (unsigned long long)vt,
                (unsigned long long)(base ? vt - base : 0));
            for (int s = 0x58; s <= 0x80; s += 8) {
                uint64_t fn = 0;
                if (vm_read_overwrite(mach_task_self(), (vm_address_t)(vt + s), 8, (vm_address_t)&fn, &g) == KERN_SUCCESS)
                    LOG("  slot %#x = %#llx (main+%#llx)", s, (unsigned long long)fn, (unsigned long long)(base && fn > base ? fn - base : 0));
            }
            for (int o = 0x30; o <= 0x60; o += 8) {
                uint64_t v = 0;
                if (vm_read_overwrite(mach_task_self(), (vm_address_t)(x19 + o), 8, (vm_address_t)&v, &g) == KERN_SUCCESS)
                    LOG("  this+%#x = %#llx", o, (unsigned long long)v);
            }
        }
    }
    if (off == 0x197ce70) {   // 打印对象与 this 的 RTTI | print RTTI of the object and this
        uint64_t x19 = uc->uc_mcontext->__ss.__x[19];
        uint64_t cand[2] = { x0, x19 };
        for (int k = 0; k < 2; k++) {
            uint64_t vt = 0, ti = 0, np = 0; vm_size_t g = 0; char nm[128]; memset(nm, 0, sizeof nm);
            if (!cand[k] || vm_read_overwrite(mach_task_self(), (vm_address_t)cand[k], 8, (vm_address_t)&vt, &g) != KERN_SUCCESS || !vt) continue;
            if (vm_read_overwrite(mach_task_self(), (vm_address_t)(vt - 8), 8, (vm_address_t)&ti, &g) != KERN_SUCCESS || !ti) continue;
            if (vm_read_overwrite(mach_task_self(), (vm_address_t)(ti + 8), 8, (vm_address_t)&np, &g) != KERN_SUCCESS || !np) continue;
            if (vm_read_overwrite(mach_task_self(), (vm_address_t)np, sizeof(nm) - 1, (vm_address_t)nm, &g) != KERN_SUCCESS) continue;
            LOG("trap: %s=%#llx vt=%#llx ti=%#llx name=%s", k ? "this" : "obj", (unsigned long long)cand[k],
                (unsigned long long)vt, (unsigned long long)ti, nm);
        }
    }
    if (x0 && base && (off == 0x197ce70)) {
        *(volatile uint64_t *)(uintptr_t)(x0 + 0x120) = 1;   // 把这个对象标记为已就绪 | mark the object ready
        if (n <= 3) LOG("trap: set [%#llx+0x120] = 1", (unsigned long long)x0);
        uc->uc_mcontext->__ss.__pc = base + 0x197c8c0;        // 直接跳到 poll 函数本体 | jump to the poll body
        return 1;
    }
    uc->uc_mcontext->__ss.__pc = pc + 4;
    return 1;
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
    if (getenv("DM_FAKE")) install_fake_singleton();
    if (getenv("DM_STUBTBL")) install_global_stubs();
    if (getenv("DM_FLAG")) { pthread_t th; pthread_create(&th, NULL, flag_thread, NULL); }
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
    crash_set_trap_hook(trap_probe);
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
