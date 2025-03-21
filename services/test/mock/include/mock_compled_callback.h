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

#ifndef BASE_NOTIFICATION_ANS_STANDARD_KITS_NATIVE_WANTAGENT_COMPLETED_CALLBACK_MOCK_H
#define BASE_NOTIFICATION_ANS_STANDARD_KITS_NATIVE_WANTAGENT_COMPLETED_CALLBACK_MOCK_H

#include "gmock/gmock.h"
#include "completed_callback.h"

namespace OHOS::Notification::WantAgent {
class MockCompletedCallback : public CompletedCallback{
public:
    MOCK_METHOD4(OnSendFinished, void(const AAFwk::Want &want, int resultCode, const std::string &resultData,
        const AAFwk::WantParams &resultExtras));
};




}  // namespace OHOS::Notification::WantAgent
#endif