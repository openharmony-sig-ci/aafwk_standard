/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FOUNDATION_AAFWK_SERVICES_TEST_MT_MOCK_APP_MGR_CLIENT_H
#define FOUNDATION_AAFWK_SERVICES_TEST_MT_MOCK_APP_MGR_CLIENT_H

#include <gmock/gmock.h>

#include "app_mgr_client.h"

namespace OHOS {
namespace AppExecFwk {

class MockAppMgrClient : public AppMgrClient {
public:
    MockAppMgrClient();
    ~MockAppMgrClient();
    MOCK_METHOD4(LoadAbility, AppMgrResultCode(const sptr<IRemoteObject> &, const sptr<IRemoteObject> &,
                                  const AbilityInfo &, const ApplicationInfo &));
    MOCK_METHOD1(TerminateAbility, AppMgrResultCode(const sptr<IRemoteObject> &));
    MOCK_METHOD2(UpdateAbilityState, AppMgrResultCode(const sptr<IRemoteObject> &token, const AbilityState state));
    MOCK_METHOD1(KillApplication, AppMgrResultCode(const std::string &));
    MOCK_METHOD1(KillProcessByAbilityToken, AppMgrResultCode(const sptr<IRemoteObject> &token));
    MOCK_METHOD4(CompelVerifyPermission, int(const std::string &permission, int pid, int uid, std::string &message));
    MOCK_METHOD1(AbilityAttachTimeOut, void(const sptr<IRemoteObject> &token));

    AppMgrResultCode AbilityBehaviorAnalysis(const sptr<IRemoteObject> &token, const sptr<IRemoteObject> &preToken,
        const int32_t visibility, const int32_t perceptibility, const int32_t connectionState) override;
    AppMgrResultCode ConnectAppMgrService() override;
    AppMgrResultCode RegisterAppStateCallback(const sptr<IAppStateCallback> &callback) override;
};

}  // namespace AppExecFwk
}  // namespace OHOS

#endif  // FOUNDATION_AAFWK_SERVICES_TEST_MT_MOCK_APP_MGR_CLIENT_H
