// nn::hid 的 ACNH 宿主实现：离线运行，所有输入设备初始化视为成功、状态恒为空（无手柄/触摸/键盘）。
// ACNH host implementation of nn::hid: offline, all device inits succeed and states are always empty.
#include "hle.h"
#include <string.h>

// ---- 初始化（空操作）----
HLE_FORCE("_ZN2nn3hid18InitializeDebugPadEv", "调试手柄初始化；空操作", void, h_hid_InitDebugPad, void) { LOG("hid: InitializeDebugPad"); }
HLE_FORCE("_ZN2nn3hid15InitializeMouseEv", "鼠标初始化；空操作", void, h_hid_InitMouse, void) { LOG("hid: InitializeMouse"); }
HLE_FORCE("_ZN2nn3hid14InitializeNpadEv", "手柄初始化；空操作", void, h_hid_InitNpad, void) { LOG("hid: InitializeNpad"); }
HLE_FORCE("_ZN2nn3hid21InitializeTouchScreenEv", "触摸屏初始化；空操作", void, h_hid_InitTouch, void) { LOG("hid: InitializeTouchScreen"); }
HLE_FORCE("_ZN2nn3hid18InitializeKeyboardEv", "键盘初始化；空操作", void, h_hid_InitKbd, void) { LOG("hid: InitializeKeyboard"); }
HLE_FORCE("_ZN2nn3hid25InitializeVibrationDeviceERKNS0_21VibrationDeviceHandleE", "震动设备初始化；空操作", void, h_hid_InitVib, const void *h) { (void)h; }

// ---- 查询（恒 0：没有设备）----
HLE_FORCE("_ZN2nn3hid24GetSupportedNpadStyleSetEv", "支持的手柄样式；0", int, h_hid_SupStyle, void) { return 0; }
HLE_FORCE("_ZN2nn3hid15GetNpadStyleSetERKj", "手柄样式；0", int, h_hid_Style, const unsigned *id) { (void)id; return 0; }
HLE_FORCE("_ZN2nn3hid20GetNpadJoyAssignmentERKj", "手柄分配；0", int, h_hid_JoyAssign, const unsigned *id) { (void)id; return 0; }
HLE_FORCE("_ZN2nn3hid19GetPlayerLedPatternERKj", "玩家灯样式；0", int, h_hid_Led, const unsigned *id) { (void)id; return 0; }

// ---- 设置（空操作）----
HLE_FORCE("_ZN2nn3hid24SetSupportedNpadStyleSetENS_4util10BitFlagSetILi32ENS0_12NpadStyleTagEEE", "设置支持的样式；空操作", void, h_hid_SetStyle, unsigned bits) { (void)bits; }
HLE_FORCE("_ZN2nn3hid22SetSupportedNpadIdTypeEPKjm", "设置支持的手柄 ID；空操作", void, h_hid_SetIds, const unsigned *ids, unsigned long n) { (void)ids; (void)n; }
HLE_FORCE("_ZN2nn3hid18SetNpadJoyHoldTypeENS0_15NpadJoyHoldTypeE", "手柄握持方式；空操作", void, h_hid_HoldType, int t) { (void)t; }

// ---- 状态读取（返回 0 条）----
HLE_FORCE("_ZN2nn3hid20GetTouchScreenStatesILm16EEEiPNS0_16TouchScreenStateIXT_EEEi", "触摸状态；0 条", int, h_hid_TouchStates, void *st, int n) { (void)st; (void)n; return 0; }
HLE_FORCE("_ZN2nn3hid13GetNpadStatesEPNS0_16NpadFullKeyStateEiRKj", "手柄状态；0 条", int, h_hid_Npad1, void *st, int n, const unsigned *id) { (void)st; (void)n; (void)id; return 0; }
HLE_FORCE("_ZN2nn3hid13GetNpadStatesEPNS0_16NpadJoyDualStateEiRKj", "手柄状态；0 条", int, h_hid_Npad2, void *st, int n, const unsigned *id) { (void)st; (void)n; (void)id; return 0; }
HLE_FORCE("_ZN2nn3hid13GetNpadStatesEPNS0_16NpadJoyLeftStateEiRKj", "手柄状态；0 条", int, h_hid_Npad3, void *st, int n, const unsigned *id) { (void)st; (void)n; (void)id; return 0; }
HLE_FORCE("_ZN2nn3hid13GetNpadStatesEPNS0_17NpadHandheldStateEiRKj", "手柄状态；0 条", int, h_hid_Npad4, void *st, int n, const unsigned *id) { (void)st; (void)n; (void)id; return 0; }
HLE_FORCE("_ZN2nn3hid13GetNpadStatesEPNS0_17NpadJoyRightStateEiRKj", "手柄状态；0 条", int, h_hid_Npad5, void *st, int n, const unsigned *id) { (void)st; (void)n; (void)id; return 0; }
HLE_FORCE("_ZN2nn3hid16GetKeyboardStateEPNS0_13KeyboardStateE", "键盘状态；空操作", void, h_hid_KbdState, void *st) { (void)st; }

// ---- 震动 / 六轴（空实现）----
HLE_FORCE("_ZN2nn3hid18SendVibrationValueERKNS0_21VibrationDeviceHandleERKNS0_14VibrationValueE", "发送震动；空操作", void, h_hid_Vib, const void *h, const void *v) { (void)h; (void)v; }
HLE_FORCE("_ZN2nn3hid18StartSixAxisSensorERKNS0_19SixAxisSensorHandleE", "启动六轴；空操作", void, h_hid_StartSix, const void *h) { (void)h; }
