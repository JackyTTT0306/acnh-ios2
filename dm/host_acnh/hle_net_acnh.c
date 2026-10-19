// nn::nifm（主机网络接口管理）：ACNH 离线运行，宿主没有网络服务，全部按“无网络”应答。
// nn::nifm (host network interface management): ACNH runs offline here and the host has no network
// service, so every entry answers "no network".
#include "hle.h"
#include <string.h>

HLE_FORCE("_ZN2nn4nifm10InitializeEv", "nifm 初始化；宿主无网络服务", void, h_nifm_Init, void) {}
HLE_FORCE("_ZN2nn4nifm18IsNetworkAvailableEv", "网络可用性；恒无网络", int, h_nifm_Avail, void) { return 0; }
HLE_FORCE("_ZN2nn4nifm19SetLocalNetworkModeEb", "本地网络模式；空操作", void, h_nifm_Local, int on) { (void)on; }
HLE_FORCE("_ZN2nn4nifm22GetCurrentIpConfigInfoEP7in_addrS2_S2_S2_S2_",
          "IP 配置；无网络", Result, h_nifm_IpCfg, void *a, void *b, void *c, void *d, void *e) {
    (void)a; (void)b; (void)c; (void)d; (void)e; return RES_FAIL;
}
HLE_FORCE("_ZN2nn4nifm27SubmitNetworkRequestAndWaitEv", "网络请求；无网络", Result, h_nifm_Req, void) { return RES_FAIL; }
HLE_FORCE("_ZN2nn4nifm22GetCurrentProxySettingEPNS0_12ProxySettingE", "代理设置；不用代理", Result, h_nifm_Proxy, void *out) {
    if (out) memset(out, 0, 0x10);
    return RES_OK;
}
HLE_FORCE("_ZN2nn4nifm26GetCurrentPrimaryIpAddressEP7in_addr", "主 IP；0.0.0.0", Result, h_nifm_Primary, void *out) {
    if (out) memset(out, 0, 4);
    return RES_FAIL;
}
HLE_FORCE("_ZN2nn4nifm31HandleNetworkRequestErrorResultEv", "网络错误处理；空操作", void, h_nifm_Err, void) {}
