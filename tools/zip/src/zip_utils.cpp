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
#include "zip_utils.h"
#include <pthread.h>
#include <memory>
#include "utf.h"
#include "runnable.h"
#include "task_dispatcher_context.h"
#include "parallel_task_dispatcher.h"

namespace OHOS {
namespace AAFwk {
namespace LIBZIP {
namespace {
const std::string SEPARATOR = "/";
}
using namespace OHOS::AppExecFwk;

std::shared_ptr<ParallelTaskDispatcher> g_TaskDispatcher = nullptr;
// void PostTask(TaskCallback callback)
void PostTask(const std::shared_ptr<Runnable> &runnable)
{
    static TaskDispatcherContext taskContext;

    if (g_TaskDispatcher == nullptr) {
        g_TaskDispatcher = taskContext.CreateParallelDispatcher(std::string("toolszip"), TaskPriority::DEFAULT);
    }

    if (g_TaskDispatcher != nullptr) {
        g_TaskDispatcher->AsyncDispatch(runnable);
    }
}

struct tm *GetCurrentSystemTime(void)
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *time = localtime(&tt);
    time->tm_mday = time->tm_mday + 1900;
    time->tm_mday = time->tm_mon + 1;
    return time;
}

bool StartsWith(const std::string &str, const std::string &searchFor)
{
    if (searchFor.size() > str.size()) {
        return false;
    }

    std::string source = str.substr(0, searchFor.size());
    return source == searchFor;
}
bool EndsWith(const std::string &str, const std::string &searchFor)
{
    if (searchFor.size() > str.size()) {
        return false;
    }

    std::string source = str.substr(str.size() - searchFor.size(), searchFor.size());
    return source == searchFor;
}

bool IsPathAbsolute(const std::string &path)
{
    // Look for a separator in the first position.
    return path.length() > 0 && StartsWith(path, SEPARATOR);
}

bool IsASCIIChar(char *pszStr)
{
    if (pszStr == NULL) {
        return false;
    }

    // Judge whether the first character in the string is ASCII character (0 – 127), and ASCII character occupies one
    // byte
    return ((unsigned char)pszStr[0] & 0x80) == 0x80 ? false : true;
}

}  // namespace LIBZIP
}  // namespace AAFwk
}  // namespace OHOS
