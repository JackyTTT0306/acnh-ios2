// nn::vi（显示层）：真机经 vi 服务开显示、建图层、取原生窗口句柄；句柄交给 NVN 的窗口 builder。
// 宿主：返回宿主静态对象；接 Metal 时原生窗口对应前端的 CAMetalLayer（front_metal_layer()）。
// nn::vi (display layer): on a real device, the vi service is used to open the display, create a layer and get the native window handle; the handle is passed to NVN's window builder.
// Host: returns host static objects; when hooking up Metal, the native window corresponds to the front end's CAMetalLayer (front_metal_layer()).
#include "../hle.h"

static struct { uint32_t magic; } g_display = {0x50534944}, g_layer = {0x5259414c};
static struct { uint32_t magic; void *layer; } g_native = {0x4e49574e, 0};

HLE_FORCE("_ZN2nn2vi10InitializeEv", "显示服务初始化；宿主无 vi 服务", void, h_vi_Init, void) {}
HLE_FORCE("_ZN2nn2vi18OpenDefaultDisplayEPPNS0_7DisplayE", "开默认显示；宿主给静态对象", Result, h_vi_OpenDisp, void **out) {
    *out = &g_display; return RES_OK;
}
HLE_FORCE("_ZN2nn2vi11CreateLayerEPPNS0_5LayerEPNS0_7DisplayE", "建图层；宿主给静态对象", Result, h_vi_CreateLayer, void **out, void *disp) {
    (void)disp; *out = &g_layer; return RES_OK;
}
HLE_FORCE("_ZN2nn2vi15GetNativeWindowEPPvPNS0_5LayerE", "原生窗口句柄，交给 NVN 窗口 builder；宿主给静态对象", Result, h_vi_GetNative, void **out, void *layer) {
    (void)layer; *out = &g_native; return RES_OK;
}

void hle_signal_system_event(void *ev);   // hle_thread.c：客体 SystemEvent 触发（须在客体线程上）

// ACNH 用它等 vsync 推进帧；宿主没有真帧循环，取用时立即触发一次。
// ACNH uses it to wait for vsync to advance frames; the host has no real frame loop, so signal once on retrieval.
HLE_FORCE("_ZN2nn2vi20GetDisplayVsyncEventEPNS_2os15SystemEventTypeEPNS0_7DisplayE",
          "显示 vsync 事件；宿主立即触发一次", Result, h_vi_GetVsync, void *ev, void *disp) {
    (void)disp;
    if (ev) hle_signal_system_event(ev);
    return RES_OK;
}
