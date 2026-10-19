// ACNH 无窗口前端：CI 与命令行下跑客体用（没有 Metal、输入与音频）。
// ACNH windowless front end: used to run the guest in CI and from the command line (no Metal, input or audio).
#include "front.h"
#include <stdint.h>

void front_run(void (*guest)(void)) { guest(); }
void *front_metal_layer(void) { return 0; }

// ACNH 原版输出：掌机 1280x720、主机 1920x1080。
// ACNH native output: 1280x720 handheld, 1920x1080 docked.
uint64_t front_game_res(int docked) {
    return docked ? (((uint64_t)1080 << 32) | 1920) : (((uint64_t)720 << 32) | 1280);
}

int front_text_input(const char *title, const char *sub, const char *guide,
                     const char *initial, int max_chars, char *out, int out_cap) {
    (void)title; (void)sub; (void)guide; (void)initial; (void)max_chars; (void)out; (void)out_cap;
    return -1;
}

void front_save_failed(int err) { (void)err; }
void dm_host_preload_start(int argc, char **argv) { (void)argc; (void)argv; }

void dm_touch_set(const dm_touch *t, int n) { (void)t; (void)n; }
