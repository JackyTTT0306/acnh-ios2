// nn::account 的补充接管：ACNH 用到、但 d2r 的 hle_account.c 没覆盖的 9 个函数。
// 离线语义：1 个固定本地用户、网络服务账号不可用、头像加载失败。
// Supplementary nn::account takeovers for ACNH: the nine functions the game imports that d2r's
// hle_account.c does not cover. Offline semantics: one fixed local user, network service account
// unavailable, profile image load fails.
#include "hle.h"
#include <stdbool.h>

typedef struct { uint64_t v[2]; } Uid;                 // nn::account::Uid，16 字节
typedef struct { uint64_t v[3]; } UserHandle;          // nn::account::UserHandle，24 字节
static const Uid k_uid = {{0x78626c0000000001ull, 0x53574e4154495645ull}};   // 与 hle_account.c 同值

HLE_FORCE("_ZN2nn7account12AsyncContext6CancelEv", "异步取消；空操作", void, h_acc_AcCancel, void *self) { (void)self; }
HLE_FORCE("_ZN2nn7account12AsyncContextD1Ev", "异步析构；空操作", void, h_acc_AcDtor, void *self) { (void)self; }
HLE_FORCE("_ZN2nn7account9CloseUserERKNS0_10UserHandleE", "关闭用户；空操作", void, h_acc_CloseUser, const UserHandle *h) { (void)h; }
HLE_FORCE("_ZN2nn7account32IsNetworkServiceAccountAvailableEPbRKNS0_10UserHandleE", "网络服务账号；不可用", Result, h_acc_IsNsa, bool *out, const UserHandle *h) {
    (void)h; if (out) *out = false; return RES_OK;
}
HLE_FORCE("_ZN2nn7account12ListAllUsersEPiPNS0_3UidEi", "全部用户；恒 1 个", Result, h_acc_ListAll, int *n, Uid *out, int max) {
    *n = 0; if (max > 0) { out[0] = k_uid; *n = 1; } return RES_OK;
}
HLE_FORCE("_ZN2nn7account16GetUserExistenceEPbRKNS0_3UidE", "用户存在；恒真", Result, h_acc_Exists, bool *out, const Uid *uid) {
    (void)uid; if (out) *out = true; return RES_OK;
}
HLE_FORCE("_ZN2nn7account18ListQualifiedUsersEPiPNS0_3UidEi", "合格用户；恒 1 个", Result, h_acc_ListQual, int *n, Uid *out, int max) {
    *n = 0; if (max > 0) { out[0] = k_uid; *n = 1; } return RES_OK;
}
HLE_FORCE("_ZN2nn7account12AsyncContext7HasDoneEPb", "异步完成；恒真", Result, h_acc_AcHasDone, void *self, bool *out) {
    (void)self; if (out) *out = true; return RES_OK;
}
HLE_FORCE("_ZN2nn7account16LoadProfileImageEPmPvmRKNS0_3UidE", "头像；无，失败", Result, h_acc_LoadImg, size_t *n, void *buf, size_t sz, const Uid *uid) {
    (void)buf; (void)sz; (void)uid; if (n) *n = 0; return RES_FAIL;
}
