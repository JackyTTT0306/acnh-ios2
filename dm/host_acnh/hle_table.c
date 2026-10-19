// libc 直通表：与 AAPCS64 一致、无结构布局依赖的 C 库函数，直接接苹果 libc。进 swhle 段。
// libc passthrough table: C library functions consistent with AAPCS64 and with no struct layout dependency, connected directly to Apple libc. Goes into the swhle section.
#include "dm.h"
#include <math.h>
#include <ctype.h>
#include <time.h>
typedef struct { const char *name; void *fn; } hle_ent;
#define L(n) __attribute__((used, section("__DATA,swhle"))) static const hle_ent hle_lib_##n = { #n, (void *)n };
// 诊断：监视某个地址范围被 memset 清零（找 main+0x32e7ad8 单例槽的清除者）。
// Diagnostic: watch an address range being cleared by memset (find who clears the main+0x32e7ad8 slot).
uintptr_t g_watch_lo, g_watch_hi;
static void *hle_watch_memset(void *d, int c, size_t n) {
    uintptr_t a = (uintptr_t)d;
    if (g_watch_hi && a <= g_watch_hi && a + n >= g_watch_lo) {
        uint64_t lr; __asm__ volatile("mov %0, x30" : "=r"(lr));
        LOG("watch: memset(%p, %d, %zu) host-lr=%#llx", d, c, n, (unsigned long long)lr);
    }
    return memset(d, c, n);
}
__attribute__((used, section("__DATA,swhle"))) static const hle_ent hle_lib_memset = { "memset", (void *)hle_watch_memset };
static void *hle_watch_memcpy(void *d, const void *s, size_t n) {
    uintptr_t a = (uintptr_t)d;
    if (g_watch_hi && a <= g_watch_hi && a + n >= g_watch_lo) {
        uint64_t lr; __asm__ volatile("mov %0, x30" : "=r"(lr));
        LOG("watch: memcpy(%p, %p, %zu) host-lr=%#llx", d, s, n, (unsigned long long)lr);
    }
    return memcpy(d, s, n);
}
__attribute__((used, section("__DATA,swhle"))) static const hle_ent hle_lib_memcpy = { "memcpy", (void *)hle_watch_memcpy };
static void *hle_watch_memmove(void *d, const void *s, size_t n) {
    uintptr_t a = (uintptr_t)d;
    if (g_watch_hi && a <= g_watch_hi && a + n >= g_watch_lo) {
        uint64_t lr; __asm__ volatile("mov %0, x30" : "=r"(lr));
        LOG("watch: memmove(%p, %p, %zu) host-lr=%#llx", d, s, n, (unsigned long long)lr);
    }
    return memmove(d, s, n);
}
__attribute__((used, section("__DATA,swhle"))) static const hle_ent hle_lib_memmove = { "memmove", (void *)hle_watch_memmove };
L(memcmp) L(memchr) L(strlen) L(strnlen) L(strcmp) L(strncmp)
L(strcpy) L(strncpy) L(strcat) L(strncat) L(strchr) L(strrchr) L(strstr) L(strspn) L(strcspn)
L(strpbrk) L(strcasecmp) L(strncasecmp) L(atoi) L(atof) L(strtol) L(strtoul) L(strtoll) L(strtoull)
L(strtod) L(strtof) L(sqrtf) L(cbrtf) L(expf) L(acosf) L(cosf) L(sinf) L(atan2f) L(fmodf) L(cos)
L(sin) L(powf) L(atanf) L(tanf) L(asinf) L(log10) L(pow) L(logf) L(ldexpf) L(log10f) L(log)
L(ldexp) L(frexp) L(atan2) L(modff) L(exp2f) L(exp2) L(log2f) L(log2) L(lroundf)
L(mktime) L(timegm) L(gmtime_r) L(localtime_r) L(strftime)
