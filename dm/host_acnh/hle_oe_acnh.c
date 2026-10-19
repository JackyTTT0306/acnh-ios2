// nn::oe / nn::settings 的 ACNH 宿主实现：离线运行，返回固定默认值（语言 en-US、区域 US、掌机模式、聚焦）。
// ACNH host implementation of nn::oe / nn::settings: offline, fixed defaults (language en-US, region US,
// handheld mode, focused).
#include "hle.h"
#include <string.h>

// ---- nn::oe ----
// oe::Initialize 已在 hle_sdkinit.c 注册（d2r 原有），这里不重复。
// oe::Initialize is already registered in hle_sdkinit.c (from d2r); do not duplicate it here.
HLE_FORCE("_ZN2nn2oe18GetDesiredLanguageEv", "期望语言；固定 en-US(1)", int, h_oe_Lang, void) { return 1; }
HLE_FORCE("_ZN2nn2oe13GetRegionCodeEv", "区域；固定 US(1)", int, h_oe_Region, void) { return 1; }
HLE_FORCE("_ZN2nn2oe16GetOperationModeEv", "运行模式；0 掌机", int, h_oe_OpMode, void) { return 0; }
HLE_FORCE("_ZN2nn2oe18GetPerformanceModeEv", "性能模式；1 boost", int, h_oe_PerfMode, void) { return 1; }
HLE_FORCE("_ZN2nn2oe20GetCurrentFocusStateEv", "焦点状态；1 聚焦", int, h_oe_Focus, void) { return 1; }
HLE_FORCE("_ZN2nn2oe25GetProgramTotalActiveTimeEv", "程序活跃时长；0", int64_t, h_oe_Active, void) { return 0; }
HLE_FORCE("_ZN2nn2oe20SetFocusHandlingModeENS0_17FocusHandlingModeE", "焦点处理模式；空操作", void, h_oe_SetFocus, int m) { (void)m; }
HLE_FORCE("_ZN2nn2oe15SetCpuBoostModeENS0_12CpuBoostModeE", "CPU boost；空操作", void, h_oe_Boost, int m) { (void)m; }
HLE_FORCE("_ZN2nn2oe17FinishStartupLogoEv", "启动 logo；空操作", void, h_oe_Logo, void) {}
HLE_FORCE("_ZN2nn2oe27SetPerformanceConfigurationENS0_15PerformanceModeEi", "性能配置；空操作", void, h_oe_PerfCfg, int m, int c) { (void)m; (void)c; }
HLE_FORCE("_ZN2nn2oe25TryPopNotificationMessageEPj", "通知消息；无消息，返回 0", int, h_oe_Notify, unsigned *out) { if (out) *out = 0; return 0; }
HLE_FORCE("_ZN2nn2oe28SetResumeNotificationEnabledEb", "恢复通知；空操作", void, h_oe_Resume, int on) { (void)on; }
HLE_FORCE("_ZN2nn2oe31EnterExitRequestHandlingSectionEv", "退出请求段；空操作", void, h_oe_Enter, void) {}
HLE_FORCE("_ZN2nn2oe31LeaveExitRequestHandlingSectionEv", "退出请求段；空操作", void, h_oe_Leave, void) {}
HLE_FORCE("_ZN2nn2oe42SetOperationModeChangedNotificationEnabledEb", "模式变更通知；空操作", void, h_oe_OpNotif, int on) { (void)on; }
HLE_FORCE("_ZN2nn2oe44SetPerformanceModeChangedNotificationEnabledEb", "性能变更通知；空操作", void, h_oe_PerfNotif, int on) { (void)on; }
HLE_FORCE("_ZN2nn2oe24GetExpectedVolumeBalanceEPfS1_", "音量平衡；1.0 / 1.0", void, h_oe_Vol, float *l, float *r) {
    if (l) *l = 1.0f; if (r) *r = 1.0f;
}
HLE_FORCE("_ZN2nn2oe24SetExpectedVolumeBalanceEff", "音量平衡；空操作", void, h_oe_SetVol, float l, float r) { (void)l; (void)r; }

// ---- nn::settings（ACNH 只用到 LanguageCode）----
HLE_FORCE("_ZN2nn8settings12LanguageCode4MakeENS0_8LanguageE", "LanguageCode；固定 en-US", void, h_set_LangMake, void *out, int lang) {
    (void)lang;
    if (out) { memset(out, 0, 8); memcpy(out, "en-US", 5); }
}
HLE_FORCE("_ZN2nn8settingseqERKNS0_12LanguageCodeERKNS0_8LanguageE", "LanguageCode == Language；不相等", int, h_set_LangEq1, const void *code, const int *lang) {
    (void)code; (void)lang; return 0;
}
HLE_FORCE("_ZN2nn8settingseqERKNS0_8LanguageERKNS0_12LanguageCodeE", "Language == LanguageCode；不相等", int, h_set_LangEq2, const int *lang, const void *code) {
    (void)lang; (void)code; return 0;
}
