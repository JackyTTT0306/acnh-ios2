// ACNH 最小宿主引导（headless）：加载 main/sdk/subsdk0，照 SDK 的 nn::init::Start 顺序初始化，再进 nnMain。
// 与 D2R 的 main.c 的区别：去掉存档节流、固定种子、触屏布局、TACT 预取、脚本按键等本游戏专用逻辑。
// ACNH minimal host bootstrap (headless): load main/sdk/subsdk0, initialize the SDK runtime in nn::init::Start order, then enter nnMain.
// Differences from D2R's main.c: no save throttling, fixed seed, touch layout, TACT prefetch or scripted keys.
#include "dm.h"
#include "front.h"
#include <stdatomic.h>
#include <unistd.h>

void guest_thread_init(void);
uint32_t svc_thread_main_init(void);
void crash_install(void);

typedef void (*fn0)(void);
typedef void (*fn2)(uint64_t, uint64_t);
typedef void (*fnp)(void *);

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
    LOG(">>> nnMain");
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
