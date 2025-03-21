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
#include "napi_data_ability_helper.h"
#include "data_ability_helper.h"
#include "uri.h"
#include <cstring>
#include <vector>
#include <uv.h>
#include "securec.h"
#include "hilog_wrapper.h"

using namespace OHOS::AAFwk;
using namespace OHOS::AppExecFwk;

namespace OHOS {
namespace AppExecFwk {
napi_value g_dataAbilityHelper;
std::list<std::shared_ptr<DataAbilityHelper>> g_dataAbilityHelperList;

/**
 * @brief DataAbilityHelper NAPI module registration.
 *
 * @param env The environment that the Node-API call is invoked under.
 * @param exports An empty object via the exports parameter as a convenience.
 *
 * @return The return value from Init is treated as the exports object for the module.
 */
napi_value DataAbilityHelperInit(napi_env env, napi_value exports)
{
    HILOG_INFO("%{public}s,called", __func__);
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("insert", NAPI_Insert),
        DECLARE_NAPI_FUNCTION("delete", NAPI_Delete),
        DECLARE_NAPI_FUNCTION("query", NAPI_Query),
        DECLARE_NAPI_FUNCTION("update", NAPI_Update),
        DECLARE_NAPI_FUNCTION("batchInsert", NAPI_BatchInsert),
        DECLARE_NAPI_FUNCTION("openFile", NAPI_OpenFile),
        DECLARE_NAPI_FUNCTION("getType", NAPI_GetType),
        DECLARE_NAPI_FUNCTION("getFileTypes", NAPI_GetFileTypes),
        DECLARE_NAPI_FUNCTION("normalizeUri", NAPI_NormalizeUri),
        DECLARE_NAPI_FUNCTION("denormalizeUri", NAPI_DenormalizeUri),
        DECLARE_NAPI_FUNCTION("release", NAPI_Release),
    };
    NAPI_CALL(env,
        napi_define_class(env,
            "dataAbilityHelper",
            NAPI_AUTO_LENGTH,
            DataAbilityHelperConstructor,
            nullptr,
            sizeof(properties) / sizeof(*properties),
            properties,
            &g_dataAbilityHelper));
    g_dataAbilityHelperList.clear();
    return exports;
}

napi_value DataAbilityHelperConstructor(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s,called", __func__);
    size_t argc = 1;
    napi_value argv[1];
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr));
    NAPI_ASSERT(env, argc > 0, "Wrong number of arguments");
    std::string strUri = NapiValueToStringUtf8(env, argv[0]);
    HILOG_INFO("%{public}s,strUri = %{public}s", __func__, strUri.c_str());

    napi_value global = nullptr;
    NAPI_CALL(env, napi_get_global(env, &global));

    napi_value abilityObj = nullptr;
    NAPI_CALL(env, napi_get_named_property(env, global, "ability", &abilityObj));

    Ability *ability = nullptr;
    NAPI_CALL(env, napi_get_value_external(env, abilityObj, (void **)&ability));

    HILOG_INFO("ability = %{public}p strUri = %{public}s", ability, strUri.c_str());
    HILOG_INFO("g_dataAbilityHelperList.size = %{public}zu", g_dataAbilityHelperList.size());
    std::shared_ptr<DataAbilityHelper> dataAbilityHelper =
        DataAbilityHelper::Creator(ability->GetContext(), std::make_shared<Uri>(strUri));
    HILOG_INFO("dataAbilityHelper = %{public}p", dataAbilityHelper.get());
    g_dataAbilityHelperList.emplace_back(dataAbilityHelper);
    HILOG_INFO("g_dataAbilityHelperList.size = %{public}zu", g_dataAbilityHelperList.size());

    napi_wrap(
        env,
        thisVar,
        dataAbilityHelper.get(),
        [](napi_env env, void *data, void *hint) {
            DataAbilityHelper *objectInfo = (DataAbilityHelper *)data;
            HILOG_INFO("DataAbilityHelper finalize_cb objectInfo = %{public}p", objectInfo);
            HILOG_INFO(
                "DataAbilityHelper finalize_cb registerInstances_.size = %{public}zu", registerInstances_.size());
            auto helper = std::find_if(registerInstances_.begin(),
                registerInstances_.end(),
                [&objectInfo](const DAHelperOnOffCB *helper) { return helper->dataAbilityHelper == objectInfo; });
            if (helper != registerInstances_.end()) {
                HILOG_INFO("DataAbilityHelper finalize_cb find helper");
                (*helper)->dataAbilityHelper->Release();
                delete *helper;
                registerInstances_.erase(helper);
            }
            HILOG_INFO(
                "DataAbilityHelper finalize_cb registerInstances_.size = %{public}zu", registerInstances_.size());
            HILOG_INFO("DataAbilityHelper finalize_cb g_dataAbilityHelperList.size = %{public}zu",
                g_dataAbilityHelperList.size());
            g_dataAbilityHelperList.remove_if(
                [objectInfo](const std::shared_ptr<DataAbilityHelper> &dataAbilityHelper) {
                    return objectInfo == dataAbilityHelper.get();
                });
            HILOG_INFO("DataAbilityHelper finalize_cb g_dataAbilityHelperList.size = %{public}zu",
                g_dataAbilityHelperList.size());
            HILOG_INFO("DataAbilityHelper finalize_cb end.");
        },
        nullptr,
        nullptr);

    HILOG_INFO("%{public}s,called end", __func__);
    return thisVar;
}

/**
 * @brief DataAbilityHelper NAPI method : insert.
 *
 * @param env The environment that the Node-API call is invoked under.
 * @param info The callback info passed into the callback function.
 *
 * @return The return value from NAPI C++ to JS for the module.
 */
napi_value NAPI_Insert(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s,called", __func__);
    DAHelperInsertCB *insertCB = new (std::nothrow) DAHelperInsertCB{
        .cbBase.cbInfo.env = env,
        .cbBase.asyncWork = nullptr,
        .cbBase.deferred = nullptr,
        .cbBase.ability = nullptr,
    };
    if (insertCB == nullptr) {
        HILOG_ERROR("%{public}s, insertCB == nullptr.", __func__);
        return WrapVoidToJS(env);
    }

    napi_value ret = InsertWrap(env, info, insertCB);
    if (ret == nullptr) {
        HILOG_ERROR("%{public}s, ret == nullptr.", __func__);
        if (insertCB != nullptr) {
            delete insertCB;
            insertCB = nullptr;
        }
        ret = WrapVoidToJS(env);
    }
    HILOG_INFO("%{public}s,called end", __func__);
    return ret;
}

/**
 * @brief Insert processing function.
 *
 * @param env The environment that the Node-API call is invoked under.
 * @param insertCB Process data asynchronously.
 *
 * @return Return JS data successfully, otherwise return nullptr.
 */
napi_value InsertWrap(napi_env env, napi_callback_info info, DAHelperInsertCB *insertCB)
{
    HILOG_INFO("%{public}s,called", __func__);
    size_t argcAsync = ARGS_THREE;
    const size_t argcPromise = ARGS_TWO;
    const size_t argCountWithAsync = argcPromise + ARGS_ASYNC_COUNT;
    napi_value args[ARGS_MAX_COUNT] = {nullptr};
    napi_value ret = 0;
    napi_value thisVar = nullptr;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argcAsync, args, &thisVar, nullptr));
    if (argcAsync > argCountWithAsync || argcAsync > ARGS_MAX_COUNT) {
        HILOG_ERROR("%{public}s, Wrong argument count.", __func__);
        return nullptr;
    }

    napi_valuetype valuetype = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, args[PARAM0], &valuetype));
    if (valuetype == napi_string) {
        insertCB->uri = NapiValueToStringUtf8(env, args[PARAM0]);
        HILOG_INFO("%{public}s,uri=%{public}s", __func__, insertCB->uri.c_str());
    } else {
        HILOG_ERROR("%{public}s, Wrong argument type.", __func__);
        return nullptr;
    }

    std::string strValue;
    UnwrapValuesBucket(strValue, env, args[PARAM1]);
    HILOG_INFO("%{public}s,valueBucket=%{public}s", __func__, strValue.c_str());
    ValuesBucket valueBucket(strValue);
    insertCB->valueBucket = ValuesBucket(valueBucket);

    DataAbilityHelper *objectInfo = nullptr;
    napi_unwrap(env, thisVar, (void **)&objectInfo);
    HILOG_INFO("%{public}s,DataAbilityHelper objectInfo = %{public}p", __func__, objectInfo);
    insertCB->dataAbilityHelper = objectInfo;

    if (argcAsync > argcPromise) {
        ret = InsertAsync(env, args, argcAsync, argcPromise, insertCB);
    } else {
        ret = InsertPromise(env, insertCB);
    }
    HILOG_INFO("%{public}s,end", __func__);
    return ret;
}

napi_value InsertAsync(
    napi_env env, napi_value *args, size_t argcAsync, const size_t argcPromise, DAHelperInsertCB *insertCB)
{
    HILOG_INFO("%{public}s, asyncCallback.", __func__);
    if (args == nullptr || insertCB == nullptr) {
        HILOG_ERROR("%{public}s, param == nullptr.", __func__);
        return nullptr;
    }
    napi_value resourceName = 0;
    NAPI_CALL(env, napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName));

    napi_valuetype valuetype = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, args[argcPromise], &valuetype));
    if (valuetype == napi_function) {
        NAPI_CALL(env, napi_create_reference(env, args[argcPromise], 1, &insertCB->cbBase.cbInfo.callback));
    }

    NAPI_CALL(env,
        napi_create_async_work(env,
            nullptr,
            resourceName,
            InsertExecuteCB,
            InsertAsyncCompleteCB,
            (void *)insertCB,
            &insertCB->cbBase.asyncWork));
    NAPI_CALL(env, napi_queue_async_work(env, insertCB->cbBase.asyncWork));
    napi_value result = 0;
    NAPI_CALL(env, napi_get_null(env, &result));
    HILOG_INFO("%{public}s, asyncCallback end", __func__);
    return result;
}

napi_value InsertPromise(napi_env env, DAHelperInsertCB *insertCB)
{
    HILOG_INFO("%{public}s, promise.", __func__);
    if (insertCB == nullptr) {
        HILOG_ERROR("%{public}s, param == nullptr.", __func__);
        return nullptr;
    }
    napi_value resourceName;
    NAPI_CALL(env, napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName));
    napi_deferred deferred;
    napi_value promise = 0;
    NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
    insertCB->cbBase.deferred = deferred;

    NAPI_CALL(env,
        napi_create_async_work(env,
            nullptr,
            resourceName,
            InsertExecuteCB,
            InsertPromiseCompleteCB,
            (void *)insertCB,
            &insertCB->cbBase.asyncWork));
    NAPI_CALL(env, napi_queue_async_work(env, insertCB->cbBase.asyncWork));
    HILOG_INFO("%{public}s, promise end", __func__);
    return promise;
}

void InsertExecuteCB(napi_env env, void *data)
{
    HILOG_INFO("NAPI_Insert, worker pool thread execute.");
    DAHelperInsertCB *insertCB = (DAHelperInsertCB *)data;
    if (insertCB->dataAbilityHelper != nullptr) {
        OHOS::Uri uri(insertCB->uri);
        insertCB->result = insertCB->dataAbilityHelper->Insert(uri, insertCB->valueBucket);
    } else {
        HILOG_ERROR("NAPI_Insert, dataAbilityHelper == nullptr.");
    }
    HILOG_INFO("NAPI_Insert, worker pool thread execute end.");
}

void InsertAsyncCompleteCB(napi_env env, napi_status status, void *data)
{
    HILOG_INFO("NAPI_Insert, main event thread complete.");
    DAHelperInsertCB *insertCB = (DAHelperInsertCB *)data;
    napi_value callback = nullptr;
    napi_value undefined = nullptr;
    napi_value result[ARGS_TWO] = {nullptr};
    napi_value callResult = nullptr;
    NAPI_CALL_RETURN_VOID(env, napi_get_undefined(env, &undefined));
    NAPI_CALL_RETURN_VOID(env, napi_get_reference_value(env, insertCB->cbBase.cbInfo.callback, &callback));

    result[PARAM0] = GetCallbackErrorValue(env, NO_ERROR);
    napi_create_int32(env, insertCB->result, &result[PARAM1]);
    NAPI_CALL_RETURN_VOID(env, napi_call_function(env, undefined, callback, ARGS_TWO, &result[PARAM0], &callResult));

    if (insertCB->cbBase.cbInfo.callback != nullptr) {
        NAPI_CALL_RETURN_VOID(env, napi_delete_reference(env, insertCB->cbBase.cbInfo.callback));
    }
    NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, insertCB->cbBase.asyncWork));
    delete insertCB;
    insertCB = nullptr;
    HILOG_INFO("NAPI_Insert, main event thread complete end.");
}

void InsertPromiseCompleteCB(napi_env env, napi_status status, void *data)
{
    HILOG_INFO("NAPI_Insert,  main event thread complete.");
    DAHelperInsertCB *insertCB = (DAHelperInsertCB *)data;
    napi_value result = nullptr;
    napi_create_int32(env, insertCB->result, &result);
    NAPI_CALL_RETURN_VOID(env, napi_resolve_deferred(env, insertCB->cbBase.deferred, result));
    NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, insertCB->cbBase.asyncWork));
    delete insertCB;
    insertCB = nullptr;
    HILOG_INFO("NAPI_Insert,  main event thread complete end.");
}

/**
 * @brief Parse the ValuesBucket parameters.
 *
 * @param param Indicates the want parameters saved the parse result.
 * @param env The environment that the Node-API call is invoked under.
 * @param args Indicates the arguments passed into the callback.
 *
 * @return The return value from NAPI C++ to JS for the module.
 */
napi_value UnwrapValuesBucket(std::string &value, napi_env env, napi_value args)
{
    HILOG_INFO("%{public}s,called", __func__);
    napi_valuetype valueType = napi_undefined;
    napi_typeof(env, args, &valueType);
    if (valueType != napi_object) {
        HILOG_ERROR("%{public}s, valueType != napi_object.", __func__);
        return nullptr;
    }

    std::string strValue = "";
    if (UnwrapStringByPropertyName(env, args, "value", strValue)) {
        HILOG_INFO("%{public}s,strValue=%{public}s", __func__, strValue.c_str());
        value = strValue;
    } else {
        HILOG_ERROR("%{public}s, value == nullptr.", __func__);
        return nullptr;
    }

    napi_value result;
    NAPI_CALL(env, napi_create_int32(env, 1, &result));
    HILOG_INFO("%{public}s,end", __func__);
    return result;
}

napi_value NAPI_GetType(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s,called", __func__);
    DAHelperGetTypeCB *gettypeCB = new (std::nothrow) DAHelperGetTypeCB{
        .cbBase.cbInfo.env = env,
        .cbBase.asyncWork = nullptr,
        .cbBase.deferred = nullptr,
        .cbBase.ability = nullptr,
    };

    napi_value ret = GetTypeWrap(env, info, gettypeCB);
    if (ret == nullptr) {
        HILOG_ERROR("%{public}s,ret == nullptr", __func__);
        if (gettypeCB != nullptr) {
            delete gettypeCB;
            gettypeCB = nullptr;
        }
        ret = WrapVoidToJS(env);
    }
    HILOG_INFO("%{public}s,end", __func__);
    return ret;
}

napi_value GetTypeWrap(napi_env env, napi_callback_info info, DAHelperGetTypeCB *gettypeCB)
{
    HILOG_INFO("%{public}s,called", __func__);
    size_t argcAsync = ARGS_TWO;
    const size_t argcPromise = ARGS_ONE;
    const size_t argCountWithAsync = argcPromise + ARGS_ASYNC_COUNT;
    napi_value args[ARGS_MAX_COUNT] = {nullptr};
    napi_value ret = 0;
    napi_value thisVar = nullptr;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argcAsync, args, &thisVar, nullptr));
    if (argcAsync > argCountWithAsync || argcAsync > ARGS_MAX_COUNT) {
        HILOG_ERROR("%{public}s, Wrong argument count.", __func__);
        return nullptr;
    }

    napi_valuetype valuetype = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, args[PARAM0], &valuetype));
    if (valuetype == napi_string) {
        gettypeCB->uri = NapiValueToStringUtf8(env, args[PARAM0]);
        HILOG_INFO("%{public}s,uri=%{public}s", __func__, gettypeCB->uri.c_str());
    } else {
        HILOG_ERROR("%{public}s, Wrong argument type.", __func__);
        return nullptr;
    }

    DataAbilityHelper *objectInfo = nullptr;
    napi_unwrap(env, thisVar, (void **)&objectInfo);
    HILOG_INFO("%{public}s,DataAbilityHelper objectInfo = %{public}p", __func__, objectInfo);
    gettypeCB->dataAbilityHelper = objectInfo;

    if (argcAsync > argcPromise) {
        ret = GetTypeAsync(env, args, argcAsync, argcPromise, gettypeCB);
    } else {
        ret = GetTypePromise(env, gettypeCB);
    }
    HILOG_INFO("%{public}s,end", __func__);
    return ret;
}

napi_value GetTypeAsync(
    napi_env env, napi_value *args, size_t argcAsync, const size_t argcPromise, DAHelperGetTypeCB *gettypeCB)
{
    HILOG_INFO("%{public}s, asyncCallback.", __func__);
    if (args == nullptr || gettypeCB == nullptr) {
        HILOG_ERROR("%{public}s, param == nullptr.", __func__);
        return nullptr;
    }
    napi_value resourceName = 0;
    NAPI_CALL(env, napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName));

    napi_valuetype valuetype = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, args[argcPromise], &valuetype));
    if (valuetype == napi_function) {
        NAPI_CALL(env, napi_create_reference(env, args[argcPromise], 1, &gettypeCB->cbBase.cbInfo.callback));
    }

    NAPI_CALL(env,
        napi_create_async_work(env,
            nullptr,
            resourceName,
            GetTypeExecuteCB,
            GetTypeAsyncCompleteCB,
            (void *)gettypeCB,
            &gettypeCB->cbBase.asyncWork));
    NAPI_CALL(env, napi_queue_async_work(env, gettypeCB->cbBase.asyncWork));
    napi_value result = 0;
    NAPI_CALL(env, napi_get_null(env, &result));
    HILOG_INFO("%{public}s, asyncCallback end", __func__);
    return result;
}

napi_value GetTypePromise(napi_env env, DAHelperGetTypeCB *gettypeCB)
{
    HILOG_INFO("%{public}s, promise.", __func__);
    if (gettypeCB == nullptr) {
        HILOG_ERROR("%{public}s, param == nullptr.", __func__);
        return nullptr;
    }
    napi_value resourceName;
    NAPI_CALL(env, napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName));
    napi_deferred deferred;
    napi_value promise = 0;
    NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
    gettypeCB->cbBase.deferred = deferred;

    NAPI_CALL(env,
        napi_create_async_work(env,
            nullptr,
            resourceName,
            GetTypeExecuteCB,
            GetTypePromiseCompleteCB,
            (void *)gettypeCB,
            &gettypeCB->cbBase.asyncWork));
    NAPI_CALL(env, napi_queue_async_work(env, gettypeCB->cbBase.asyncWork));
    HILOG_INFO("%{public}s, promise end.", __func__);
    return promise;
}

void GetTypeExecuteCB(napi_env env, void *data)
{
    HILOG_INFO("NAPI_GetType, worker pool thread execute.");
    DAHelperGetTypeCB *gettypeCB = (DAHelperGetTypeCB *)data;
    if (gettypeCB->dataAbilityHelper != nullptr) {
        OHOS::Uri uri(gettypeCB->uri);
        gettypeCB->result = gettypeCB->dataAbilityHelper->GetType(uri);
    } else {
        HILOG_ERROR("NAPI_GetType, dataAbilityHelper == nullptr.");
    }
    HILOG_INFO("NAPI_GetType, worker pool thread execute end.");
}

void GetTypeAsyncCompleteCB(napi_env env, napi_status status, void *data)
{
    HILOG_INFO("NAPI_GetType, main event thread complete.");
    DAHelperGetTypeCB *gettypeCB = (DAHelperGetTypeCB *)data;
    napi_value callback = nullptr;
    napi_value undefined = nullptr;
    napi_value result[ARGS_TWO] = {nullptr};
    napi_value callResult = nullptr;
    NAPI_CALL_RETURN_VOID(env, napi_get_undefined(env, &undefined));
    NAPI_CALL_RETURN_VOID(env, napi_get_reference_value(env, gettypeCB->cbBase.cbInfo.callback, &callback));

    result[PARAM0] = GetCallbackErrorValue(env, NO_ERROR);
    napi_create_string_utf8(env, gettypeCB->result.c_str(), NAPI_AUTO_LENGTH, &result[PARAM1]);

    NAPI_CALL_RETURN_VOID(env, napi_call_function(env, undefined, callback, ARGS_TWO, &result[PARAM0], &callResult));

    if (gettypeCB->cbBase.cbInfo.callback != nullptr) {
        NAPI_CALL_RETURN_VOID(env, napi_delete_reference(env, gettypeCB->cbBase.cbInfo.callback));
    }
    NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, gettypeCB->cbBase.asyncWork));
    delete gettypeCB;
    gettypeCB = nullptr;
    HILOG_INFO("NAPI_GetType, main event thread complete end.");
}

void GetTypePromiseCompleteCB(napi_env env, napi_status status, void *data)
{
    HILOG_INFO("NAPI_GetType,  main event thread complete.");
    DAHelperGetTypeCB *gettypeCB = (DAHelperGetTypeCB *)data;
    napi_value result = nullptr;
    NAPI_CALL_RETURN_VOID(env, napi_create_string_utf8(env, gettypeCB->result.c_str(), NAPI_AUTO_LENGTH, &result));
    NAPI_CALL_RETURN_VOID(env, napi_resolve_deferred(env, gettypeCB->cbBase.deferred, result));
    NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, gettypeCB->cbBase.asyncWork));
    delete gettypeCB;
    gettypeCB = nullptr;
    HILOG_INFO("NAPI_GetType,  main event thread complete end.");
}

napi_value NAPI_GetFileTypes(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s,called", __func__);
    DAHelperGetFileTypesCB *getfiletypesCB = new (std::nothrow) DAHelperGetFileTypesCB{
        .cbBase.cbInfo.env = env,
        .cbBase.asyncWork = nullptr,
        .cbBase.deferred = nullptr,
        .cbBase.ability = nullptr,
    };

    napi_value ret = GetFileTypesWrap(env, info, getfiletypesCB);
    if (ret == nullptr) {
        HILOG_ERROR("%{public}s,ret == nullptr", __func__);
        if (getfiletypesCB != nullptr) {
            delete getfiletypesCB;
            getfiletypesCB = nullptr;
        }
        ret = WrapVoidToJS(env);
    }
    HILOG_INFO("%{public}s,end", __func__);
    return ret;
}

napi_value GetFileTypesWrap(napi_env env, napi_callback_info info, DAHelperGetFileTypesCB *getfiletypesCB)
{
    HILOG_INFO("%{public}s,called", __func__);
    size_t argcAsync = ARGS_THREE;
    const size_t argcPromise = ARGS_TWO;
    const size_t argCountWithAsync = argcPromise + ARGS_ASYNC_COUNT;
    napi_value args[ARGS_MAX_COUNT] = {nullptr};
    napi_value ret = 0;
    napi_value thisVar = nullptr;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argcAsync, args, &thisVar, nullptr));
    if (argcAsync > argCountWithAsync || argcAsync > ARGS_MAX_COUNT) {
        HILOG_ERROR("%{public}s, Wrong argument count.", __func__);
        return nullptr;
    }

    napi_valuetype valuetype = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, args[PARAM0], &valuetype));
    if (valuetype == napi_string) {
        getfiletypesCB->uri = NapiValueToStringUtf8(env, args[PARAM0]);
        HILOG_INFO("%{public}s,uri=%{public}s", __func__, getfiletypesCB->uri.c_str());
    }

    NAPI_CALL(env, napi_typeof(env, args[PARAM1], &valuetype));
    if (valuetype == napi_string) {
        getfiletypesCB->mimeTypeFilter = NapiValueToStringUtf8(env, args[PARAM1]);
        HILOG_INFO("%{public}s,mimeTypeFilter=%{public}s", __func__, getfiletypesCB->mimeTypeFilter.c_str());
    }

    DataAbilityHelper *objectInfo = nullptr;
    napi_unwrap(env, thisVar, (void **)&objectInfo);
    HILOG_INFO("%{public}s,DataAbilityHelper objectInfo = %{public}p", __func__, objectInfo);
    getfiletypesCB->dataAbilityHelper = objectInfo;

    if (argcAsync > argcPromise) {
        ret = GetFileTypesAsync(env, args, argcAsync, argcPromise, getfiletypesCB);
    } else {
        ret = GetFileTypesPromise(env, getfiletypesCB);
    }
    HILOG_INFO("%{public}s,end", __func__);
    return ret;
}
napi_value GetFileTypesAsync(
    napi_env env, napi_value *args, size_t argcAsync, const size_t argcPromise, DAHelperGetFileTypesCB *getfiletypesCB)
{
    HILOG_INFO("%{public}s, asyncCallback.", __func__);
    if (args == nullptr || getfiletypesCB == nullptr) {
        HILOG_ERROR("%{public}s, param == nullptr.", __func__);
        return nullptr;
    }
    napi_value resourceName = 0;
    NAPI_CALL(env, napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName));

    napi_valuetype valuetype = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, args[argcPromise], &valuetype));
    if (valuetype == napi_function) {
        NAPI_CALL(env, napi_create_reference(env, args[argcPromise], 1, &getfiletypesCB->cbBase.cbInfo.callback));
    }

    NAPI_CALL(env,
        napi_create_async_work(env,
            nullptr,
            resourceName,
            GetFileTypesExecuteCB,
            GetFileTypesAsyncCompleteCB,
            (void *)getfiletypesCB,
            &getfiletypesCB->cbBase.asyncWork));
    NAPI_CALL(env, napi_queue_async_work(env, getfiletypesCB->cbBase.asyncWork));
    napi_value result = 0;
    NAPI_CALL(env, napi_get_null(env, &result));
    HILOG_INFO("%{public}s, asyncCallback end.", __func__);
    return result;
}

napi_value GetFileTypesPromise(napi_env env, DAHelperGetFileTypesCB *getfiletypesCB)
{
    HILOG_INFO("%{public}s, promise.", __func__);
    if (getfiletypesCB == nullptr) {
        HILOG_ERROR("%{public}s, param == nullptr.", __func__);
        return nullptr;
    }
    napi_value resourceName;
    NAPI_CALL(env, napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName));
    napi_deferred deferred;
    napi_value promise = 0;
    NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
    getfiletypesCB->cbBase.deferred = deferred;

    NAPI_CALL(env,
        napi_create_async_work(env,
            nullptr,
            resourceName,
            GetFileTypesExecuteCB,
            GetFileTypesPromiseCompleteCB,
            (void *)getfiletypesCB,
            &getfiletypesCB->cbBase.asyncWork));
    NAPI_CALL(env, napi_queue_async_work(env, getfiletypesCB->cbBase.asyncWork));
    HILOG_INFO("%{public}s, promise end.", __func__);
    return promise;
}

void GetFileTypesExecuteCB(napi_env env, void *data)
{
    HILOG_INFO("NAPI_GetFileTypes, worker pool thread execute.");
    DAHelperGetFileTypesCB *getfiletypesCB = (DAHelperGetFileTypesCB *)data;
    if (getfiletypesCB->dataAbilityHelper != nullptr) {
        OHOS::Uri uri(getfiletypesCB->uri);
        HILOG_INFO("NAPI_GetFileTypes, uri:%{public}s", uri.ToString().c_str());
        HILOG_INFO("NAPI_GetFileTypes, mimeTypeFilter:%{public}s", getfiletypesCB->mimeTypeFilter.c_str());
        getfiletypesCB->result = getfiletypesCB->dataAbilityHelper->GetFileTypes(uri, getfiletypesCB->mimeTypeFilter);
    } else {
        HILOG_INFO("NAPI_GetFileTypes, dataAbilityHelper == nullptr.");
    }
    HILOG_INFO("NAPI_GetFileTypes, worker pool thread execute end.");
}

void GetFileTypesAsyncCompleteCB(napi_env env, napi_status status, void *data)
{
    HILOG_INFO("NAPI_GetFileTypes, main event thread complete.");
    DAHelperGetFileTypesCB *getfiletypesCB = (DAHelperGetFileTypesCB *)data;
    napi_value callback = nullptr;
    napi_value undefined = nullptr;
    napi_value result[ARGS_TWO] = {nullptr};
    napi_value callResult = nullptr;

    NAPI_CALL_RETURN_VOID(env, napi_get_undefined(env, &undefined));
    NAPI_CALL_RETURN_VOID(env, napi_get_reference_value(env, getfiletypesCB->cbBase.cbInfo.callback, &callback));

    result[PARAM0] = GetCallbackErrorValue(env, NO_ERROR);
    result[PARAM1] = WrapGetFileTypesCB(env, *getfiletypesCB);

    NAPI_CALL_RETURN_VOID(env, napi_call_function(env, undefined, callback, ARGS_TWO, &result[PARAM0], &callResult));

    if (getfiletypesCB->cbBase.cbInfo.callback != nullptr) {
        NAPI_CALL_RETURN_VOID(env, napi_delete_reference(env, getfiletypesCB->cbBase.cbInfo.callback));
    }
    NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, getfiletypesCB->cbBase.asyncWork));
    delete getfiletypesCB;
    getfiletypesCB = nullptr;
    HILOG_INFO("NAPI_GetFileTypes, main event thread complete end.");
}

napi_value WrapGetFileTypesCB(napi_env env, const DAHelperGetFileTypesCB &getfiletypesCB)
{
    HILOG_INFO("WrapGetFileTypesCB, called.");
    HILOG_INFO("NAPI_GetFileTypes, result.size:%{public}zu", getfiletypesCB.result.size());
    for (size_t i = 0; i < getfiletypesCB.result.size(); i++) {
        HILOG_INFO("NAPI_GetFileTypes, result[%{public}zu]:%{public}s", i, getfiletypesCB.result.at(i).c_str());
    }
    napi_value proValue = nullptr;

    napi_value jsArrayresult = nullptr;
    NAPI_CALL(env, napi_create_array(env, &jsArrayresult));
    for (size_t i = 0; i < getfiletypesCB.result.size(); i++) {
        proValue = nullptr;
        NAPI_CALL(env, napi_create_string_utf8(env, getfiletypesCB.result.at(i).c_str(), NAPI_AUTO_LENGTH, &proValue));
        NAPI_CALL(env, napi_set_element(env, jsArrayresult, i, proValue));
    }
    HILOG_INFO("WrapGetFileTypesCB, end.");
    return jsArrayresult;
}

void GetFileTypesPromiseCompleteCB(napi_env env, napi_status status, void *data)
{
    HILOG_INFO("NAPI_GetFileTypes,  main event thread complete.");
    DAHelperGetFileTypesCB *getfiletypesCB = (DAHelperGetFileTypesCB *)data;
    napi_value result = nullptr;

    result = WrapGetFileTypesCB(env, *getfiletypesCB);
    NAPI_CALL_RETURN_VOID(env, napi_resolve_deferred(env, getfiletypesCB->cbBase.deferred, result));
    NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, getfiletypesCB->cbBase.asyncWork));
    delete getfiletypesCB;
    getfiletypesCB = nullptr;
    HILOG_INFO("NAPI_GetFileTypes,  main event thread complete end.");
}

napi_value NAPI_NormalizeUri(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s,called", __func__);
    DAHelperNormalizeUriCB *normalizeuriCB = new (std::nothrow) DAHelperNormalizeUriCB{
        .cbBase.cbInfo.env = env,
        .cbBase.asyncWork = nullptr,
        .cbBase.deferred = nullptr,
        .cbBase.ability = nullptr,
    };

    napi_value ret = NormalizeUriWrap(env, info, normalizeuriCB);
    if (ret == nullptr) {
        HILOG_ERROR("%{public}s,ret == nullptr", __func__);
        if (normalizeuriCB != nullptr) {
            delete normalizeuriCB;
            normalizeuriCB = nullptr;
        }
        ret = WrapVoidToJS(env);
    }
    HILOG_INFO("%{public}s,end", __func__);
    return ret;
}

napi_value NormalizeUriWrap(napi_env env, napi_callback_info info, DAHelperNormalizeUriCB *normalizeuriCB)
{
    HILOG_INFO("%{public}s,called", __func__);
    size_t argcAsync = ARGS_TWO;
    const size_t argcPromise = ARGS_ONE;
    const size_t argCountWithAsync = argcPromise + ARGS_ASYNC_COUNT;
    napi_value args[ARGS_MAX_COUNT] = {nullptr};
    napi_value ret = 0;
    napi_value thisVar = nullptr;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argcAsync, args, &thisVar, nullptr));
    if (argcAsync > argCountWithAsync || argcAsync > ARGS_MAX_COUNT) {
        HILOG_ERROR("%{public}s, Wrong argument count.", __func__);
        return nullptr;
    }

    napi_valuetype valuetype = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, args[PARAM0], &valuetype));
    if (valuetype == napi_string) {
        normalizeuriCB->uri = NapiValueToStringUtf8(env, args[PARAM0]);
        HILOG_INFO("%{public}s,uri=%{public}s", __func__, normalizeuriCB->uri.c_str());
    }

    DataAbilityHelper *objectInfo = nullptr;
    napi_unwrap(env, thisVar, (void **)&objectInfo);
    HILOG_INFO("%{public}s,DataAbilityHelper objectInfo = %{public}p", __func__, objectInfo);
    normalizeuriCB->dataAbilityHelper = objectInfo;

    if (argcAsync > argcPromise) {
        ret = NormalizeUriAsync(env, args, argcAsync, argcPromise, normalizeuriCB);
    } else {
        ret = NormalizeUriPromise(env, normalizeuriCB);
    }
    HILOG_INFO("%{public}s,end", __func__);
    return ret;
}
napi_value NormalizeUriAsync(
    napi_env env, napi_value *args, size_t argcAsync, const size_t argcPromise, DAHelperNormalizeUriCB *normalizeuriCB)
{
    HILOG_INFO("%{public}s, asyncCallback.", __func__);
    if (args == nullptr || normalizeuriCB == nullptr) {
        HILOG_ERROR("%{public}s, param == nullptr.", __func__);
        return nullptr;
    }
    napi_value resourceName = 0;
    NAPI_CALL(env, napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName));

    napi_valuetype valuetype = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, args[argcPromise], &valuetype));
    if (valuetype == napi_function) {
        NAPI_CALL(env, napi_create_reference(env, args[argcPromise], 1, &normalizeuriCB->cbBase.cbInfo.callback));
    }

    NAPI_CALL(env,
        napi_create_async_work(env,
            nullptr,
            resourceName,
            NormalizeUriExecuteCB,
            NormalizeUriAsyncCompleteCB,
            (void *)normalizeuriCB,
            &normalizeuriCB->cbBase.asyncWork));
    NAPI_CALL(env, napi_queue_async_work(env, normalizeuriCB->cbBase.asyncWork));
    napi_value result = 0;
    NAPI_CALL(env, napi_get_null(env, &result));
    HILOG_INFO("%{public}s, asyncCallback end.", __func__);
    return result;
}

napi_value NormalizeUriPromise(napi_env env, DAHelperNormalizeUriCB *normalizeuriCB)
{
    HILOG_INFO("%{public}s, promise.", __func__);
    if (normalizeuriCB == nullptr) {
        HILOG_ERROR("%{public}s, param == nullptr.", __func__);
        return nullptr;
    }
    napi_value resourceName;
    NAPI_CALL(env, napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName));
    napi_deferred deferred;
    napi_value promise = 0;
    NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
    normalizeuriCB->cbBase.deferred = deferred;

    NAPI_CALL(env,
        napi_create_async_work(env,
            nullptr,
            resourceName,
            NormalizeUriExecuteCB,
            NormalizeUriPromiseCompleteCB,
            (void *)normalizeuriCB,
            &normalizeuriCB->cbBase.asyncWork));
    NAPI_CALL(env, napi_queue_async_work(env, normalizeuriCB->cbBase.asyncWork));
    HILOG_INFO("%{public}s, promise end.", __func__);
    return promise;
}

void NormalizeUriExecuteCB(napi_env env, void *data)
{
    HILOG_INFO("NAPI_NormalizeUri, worker pool thread execute.");
    DAHelperNormalizeUriCB *normalizeuriCB = (DAHelperNormalizeUriCB *)data;
    Uri uriValue(normalizeuriCB->uri);
    if (normalizeuriCB->dataAbilityHelper != nullptr) {
        OHOS::Uri uri(normalizeuriCB->uri);
        uriValue = normalizeuriCB->dataAbilityHelper->NormalizeUri(uri);
        normalizeuriCB->result = uriValue.ToString();
    } else {
        HILOG_INFO("NAPI_NormalizeUri, dataAbilityHelper == nullptr");
    }
    HILOG_INFO("NAPI_NormalizeUri, worker pool thread execute end.");
}

void NormalizeUriAsyncCompleteCB(napi_env env, napi_status status, void *data)
{
    HILOG_INFO("NAPI_NormalizeUri, main event thread complete.");
    DAHelperNormalizeUriCB *normalizeuriCB = (DAHelperNormalizeUriCB *)data;
    napi_value callback = nullptr;
    napi_value undefined = nullptr;
    napi_value result[ARGS_TWO] = {nullptr};
    napi_value callResult = nullptr;
    NAPI_CALL_RETURN_VOID(env, napi_get_undefined(env, &undefined));
    NAPI_CALL_RETURN_VOID(env, napi_get_reference_value(env, normalizeuriCB->cbBase.cbInfo.callback, &callback));

    result[PARAM0] = GetCallbackErrorValue(env, NO_ERROR);
    NAPI_CALL_RETURN_VOID(
        env, napi_create_string_utf8(env, normalizeuriCB->result.c_str(), NAPI_AUTO_LENGTH, &result[PARAM1]));

    NAPI_CALL_RETURN_VOID(env, napi_call_function(env, undefined, callback, ARGS_TWO, &result[PARAM0], &callResult));

    if (normalizeuriCB->cbBase.cbInfo.callback != nullptr) {
        NAPI_CALL_RETURN_VOID(env, napi_delete_reference(env, normalizeuriCB->cbBase.cbInfo.callback));
    }
    NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, normalizeuriCB->cbBase.asyncWork));
    delete normalizeuriCB;
    normalizeuriCB = nullptr;
    HILOG_INFO("NAPI_NormalizeUri, main event thread complete end.");
}

void NormalizeUriPromiseCompleteCB(napi_env env, napi_status status, void *data)
{
    HILOG_INFO("NAPI_NormalizeUri,  main event thread complete.");
    DAHelperNormalizeUriCB *normalizeuriCB = (DAHelperNormalizeUriCB *)data;
    napi_value result = nullptr;
    NAPI_CALL_RETURN_VOID(env, napi_create_string_utf8(env, normalizeuriCB->result.c_str(), NAPI_AUTO_LENGTH, &result));
    NAPI_CALL_RETURN_VOID(env, napi_resolve_deferred(env, normalizeuriCB->cbBase.deferred, result));
    NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, normalizeuriCB->cbBase.asyncWork));
    delete normalizeuriCB;
    normalizeuriCB = nullptr;
    HILOG_INFO("NAPI_NormalizeUri,  main event thread complete end.");
}

napi_value NAPI_DenormalizeUri(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s,called", __func__);
    DAHelperDenormalizeUriCB *denormalizeuriCB = new (std::nothrow) DAHelperDenormalizeUriCB{
        .cbBase.cbInfo.env = env,
        .cbBase.asyncWork = nullptr,
        .cbBase.deferred = nullptr,
        .cbBase.ability = nullptr,
    };

    napi_value ret = DenormalizeUriWrap(env, info, denormalizeuriCB);
    if (ret == nullptr) {
        HILOG_ERROR("%{public}s,ret == nullptr", __func__);
        if (denormalizeuriCB != nullptr) {
            delete denormalizeuriCB;
            denormalizeuriCB = nullptr;
        }
        ret = WrapVoidToJS(env);
    }
    HILOG_INFO("%{public}s,end", __func__);
    return ret;
}

napi_value DenormalizeUriWrap(napi_env env, napi_callback_info info, DAHelperDenormalizeUriCB *denormalizeuriCB)
{
    HILOG_INFO("%{public}s,called", __func__);
    size_t argcAsync = ARGS_TWO;
    const size_t argcPromise = ARGS_ONE;
    const size_t argCountWithAsync = argcPromise + ARGS_ASYNC_COUNT;
    napi_value args[ARGS_MAX_COUNT] = {nullptr};
    napi_value ret = 0;
    napi_value thisVar = nullptr;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argcAsync, args, &thisVar, nullptr));
    if (argcAsync > argCountWithAsync || argcAsync > ARGS_MAX_COUNT) {
        HILOG_ERROR("%{public}s, Wrong argument count.", __func__);
        return nullptr;
    }

    napi_valuetype valuetype = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, args[PARAM0], &valuetype));
    if (valuetype == napi_string) {
        denormalizeuriCB->uri = NapiValueToStringUtf8(env, args[PARAM0]);
        HILOG_INFO("%{public}s,uri=%{public}s", __func__, denormalizeuriCB->uri.c_str());
    }

    DataAbilityHelper *objectInfo = nullptr;
    napi_unwrap(env, thisVar, (void **)&objectInfo);
    HILOG_INFO("%{public}s,DataAbilityHelper objectInfo = %{public}p", __func__, objectInfo);
    denormalizeuriCB->dataAbilityHelper = objectInfo;

    if (argcAsync > argcPromise) {
        ret = DenormalizeUriAsync(env, args, argcAsync, argcPromise, denormalizeuriCB);
    } else {
        ret = DenormalizeUriPromise(env, denormalizeuriCB);
    }
    HILOG_INFO("%{public}s,end", __func__);
    return ret;
}
napi_value DenormalizeUriAsync(napi_env env, napi_value *args, size_t argcAsync, const size_t argcPromise,
    DAHelperDenormalizeUriCB *denormalizeuriCB)
{
    HILOG_INFO("%{public}s, asyncCallback.", __func__);
    if (args == nullptr || denormalizeuriCB == nullptr) {
        HILOG_ERROR("%{public}s, param == nullptr.", __func__);
        return nullptr;
    }
    napi_value resourceName = 0;
    NAPI_CALL(env, napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName));

    napi_valuetype valuetype = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, args[argcPromise], &valuetype));
    if (valuetype == napi_function) {
        NAPI_CALL(env, napi_create_reference(env, args[argcPromise], 1, &denormalizeuriCB->cbBase.cbInfo.callback));
    }

    NAPI_CALL(env,
        napi_create_async_work(env,
            nullptr,
            resourceName,
            DenormalizeUriExecuteCB,
            DenormalizeUriAsyncCompleteCB,
            (void *)denormalizeuriCB,
            &denormalizeuriCB->cbBase.asyncWork));
    NAPI_CALL(env, napi_queue_async_work(env, denormalizeuriCB->cbBase.asyncWork));
    napi_value result = 0;
    NAPI_CALL(env, napi_get_null(env, &result));
    HILOG_INFO("%{public}s, asyncCallback end.", __func__);
    return result;
}

napi_value DenormalizeUriPromise(napi_env env, DAHelperDenormalizeUriCB *denormalizeuriCB)
{
    HILOG_INFO("%{public}s, promise.", __func__);
    if (denormalizeuriCB == nullptr) {
        HILOG_ERROR("%{public}s, param == nullptr.", __func__);
        return nullptr;
    }
    napi_value resourceName;
    NAPI_CALL(env, napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName));
    napi_deferred deferred;
    napi_value promise = 0;
    NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
    denormalizeuriCB->cbBase.deferred = deferred;

    NAPI_CALL(env,
        napi_create_async_work(env,
            nullptr,
            resourceName,
            DenormalizeUriExecuteCB,
            DenormalizeUriPromiseCompleteCB,
            (void *)denormalizeuriCB,
            &denormalizeuriCB->cbBase.asyncWork));
    NAPI_CALL(env, napi_queue_async_work(env, denormalizeuriCB->cbBase.asyncWork));
    HILOG_INFO("%{public}s, promise end.", __func__);
    return promise;
}

void DenormalizeUriExecuteCB(napi_env env, void *data)
{
    HILOG_INFO("NAPI_DenormalizeUri, worker pool thread execute.");
    DAHelperDenormalizeUriCB *denormalizeuriCB = (DAHelperDenormalizeUriCB *)data;
    Uri uriValue(denormalizeuriCB->uri);
    if (denormalizeuriCB->dataAbilityHelper != nullptr) {
        OHOS::Uri uri(denormalizeuriCB->uri);
        uriValue = denormalizeuriCB->dataAbilityHelper->DenormalizeUri(uri);
        denormalizeuriCB->result = uriValue.ToString();
    } else {
        HILOG_ERROR("NAPI_DenormalizeUri, dataAbilityHelper == nullptr");
    }
    HILOG_INFO("NAPI_DenormalizeUri, worker pool thread execute end.");
}

void DenormalizeUriAsyncCompleteCB(napi_env env, napi_status status, void *data)
{
    HILOG_INFO("NAPI_DenormalizeUri, main event thread complete.");
    DAHelperDenormalizeUriCB *denormalizeuriCB = (DAHelperDenormalizeUriCB *)data;
    napi_value callback = nullptr;
    napi_value undefined = nullptr;
    napi_value result[ARGS_TWO] = {nullptr};
    napi_value callResult = nullptr;
    NAPI_CALL_RETURN_VOID(env, napi_get_undefined(env, &undefined));
    NAPI_CALL_RETURN_VOID(env, napi_get_reference_value(env, denormalizeuriCB->cbBase.cbInfo.callback, &callback));

    result[PARAM0] = GetCallbackErrorValue(env, NO_ERROR);
    NAPI_CALL_RETURN_VOID(
        env, napi_create_string_utf8(env, denormalizeuriCB->result.c_str(), NAPI_AUTO_LENGTH, &result[PARAM1]));

    NAPI_CALL_RETURN_VOID(env, napi_call_function(env, undefined, callback, ARGS_TWO, &result[PARAM0], &callResult));

    if (denormalizeuriCB->cbBase.cbInfo.callback != nullptr) {
        NAPI_CALL_RETURN_VOID(env, napi_delete_reference(env, denormalizeuriCB->cbBase.cbInfo.callback));
    }
    NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, denormalizeuriCB->cbBase.asyncWork));
    delete denormalizeuriCB;
    denormalizeuriCB = nullptr;
    HILOG_INFO("NAPI_DenormalizeUri, main event thread complete end.");
}

void DenormalizeUriPromiseCompleteCB(napi_env env, napi_status status, void *data)
{
    HILOG_INFO("NAPI_DenormalizeUri,  main event thread complete.");
    DAHelperDenormalizeUriCB *denormalizeuriCB = (DAHelperDenormalizeUriCB *)data;
    napi_value result = nullptr;
    NAPI_CALL_RETURN_VOID(
        env, napi_create_string_utf8(env, denormalizeuriCB->result.c_str(), NAPI_AUTO_LENGTH, &result));
    NAPI_CALL_RETURN_VOID(env, napi_resolve_deferred(env, denormalizeuriCB->cbBase.deferred, result));
    NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, denormalizeuriCB->cbBase.asyncWork));
    delete denormalizeuriCB;
    denormalizeuriCB = nullptr;
    HILOG_INFO("NAPI_DenormalizeUri,  main event thread complete end.");
}

napi_value UnwrapDataAbilityPredicates(std::string &value, napi_env env, napi_value args)
{
    HILOG_INFO("%{public}s,called", __func__);
    napi_valuetype valueType = napi_undefined;
    napi_typeof(env, args, &valueType);
    if (valueType != napi_object) {
        HILOG_ERROR("%{public}s, valueType != napi_object.", __func__);
        return nullptr;
    }

    std::string strValue = "";
    if (UnwrapStringByPropertyName(env, args, "value", strValue)) {
        HILOG_INFO("%{public}s,strValue=%{public}s", __func__, strValue.c_str());
        value = strValue;
    } else {
        HILOG_ERROR("%{public}s, value == nullptr.", __func__);
        return nullptr;
    }

    napi_value result;
    NAPI_CALL(env, napi_create_int32(env, 1, &result));
    HILOG_INFO("%{public}s,end", __func__);
    return result;
}

/**
 * @brief DataAbilityHelper NAPI method : insert.
 *
 * @param env The environment that the Node-API call is invoked under.
 * @param info The callback info passed into the callback function.
 *
 * @return The return value from NAPI C++ to JS for the module.
 */
napi_value NAPI_Delete(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s,called", __func__);
    DAHelperDeleteCB *deleteCB = new (std::nothrow) DAHelperDeleteCB{
        .cbBase.cbInfo.env = env,
        .cbBase.asyncWork = nullptr,
        .cbBase.deferred = nullptr,
        .cbBase.ability = nullptr,
    };

    napi_value ret = DeleteWrap(env, info, deleteCB);
    if (ret == nullptr) {
        HILOG_ERROR("%{public}s,ret == nullptr", __func__);
        if (deleteCB != nullptr) {
            delete deleteCB;
            deleteCB = nullptr;
        }
        ret = WrapVoidToJS(env);
    }
    HILOG_INFO("%{public}s,end", __func__);
    return ret;
}

/**
 * @brief Insert processing function.
 *
 * @param env The environment that the Node-API call is invoked under.
 * @param insertCB Process data asynchronously.
 *
 * @return Return JS data successfully, otherwise return nullptr.
 */
napi_value DeleteWrap(napi_env env, napi_callback_info info, DAHelperDeleteCB *deleteCB)
{
    HILOG_INFO("%{public}s,called", __func__);
    size_t argcAsync = ARGS_THREE;
    const size_t argcPromise = ARGS_TWO;
    const size_t argCountWithAsync = argcPromise + ARGS_ASYNC_COUNT;
    napi_value args[ARGS_MAX_COUNT] = {nullptr};
    napi_value ret = 0;
    napi_value thisVar = nullptr;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argcAsync, args, &thisVar, nullptr));
    if (argcAsync > argCountWithAsync || argcAsync > ARGS_MAX_COUNT) {
        HILOG_ERROR("%{public}s, Wrong argument count.", __func__);
        return nullptr;
    }

    napi_valuetype valuetype = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, args[PARAM0], &valuetype));
    if (valuetype == napi_string) {
        deleteCB->uri = NapiValueToStringUtf8(env, args[PARAM0]);
        HILOG_INFO("%{public}s,uri=%{public}s", __func__, deleteCB->uri.c_str());
    }

    std::string strValue;
    UnwrapDataAbilityPredicates(strValue, env, args[PARAM1]);
    HILOG_INFO("%{public}s,valueBucket=%{public}s", __func__, strValue.c_str());
    DataAbilityPredicates dataAbpredicates(strValue);
    deleteCB->predicates = DataAbilityPredicates(dataAbpredicates);

    DataAbilityHelper *objectInfo = nullptr;
    napi_unwrap(env, thisVar, (void **)&objectInfo);
    HILOG_INFO("%{public}s,DataAbilityHelper objectInfo = %{public}p", __func__, objectInfo);
    deleteCB->dataAbilityHelper = objectInfo;

    if (argcAsync > argcPromise) {
        ret = DeleteAsync(env, args, argcAsync, argcPromise, deleteCB);
    } else {
        ret = DeletePromise(env, deleteCB);
    }
    HILOG_INFO("%{public}s,end", __func__);
    return ret;
}

napi_value DeleteAsync(
    napi_env env, napi_value *args, size_t argcAsync, const size_t argcPromise, DAHelperDeleteCB *deleteCB)
{
    HILOG_INFO("%{public}s, asyncCallback.", __func__);
    if (args == nullptr || deleteCB == nullptr) {
        HILOG_ERROR("%{public}s, param == nullptr.", __func__);
        return nullptr;
    }
    napi_value resourceName = 0;
    NAPI_CALL(env, napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName));

    napi_valuetype valuetype = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, args[argcPromise], &valuetype));
    if (valuetype == napi_function) {
        NAPI_CALL(env, napi_create_reference(env, args[argcPromise], 1, &deleteCB->cbBase.cbInfo.callback));
    }

    NAPI_CALL(env,
        napi_create_async_work(env,
            nullptr,
            resourceName,
            DeleteExecuteCB,
            DeleteAsyncCompleteCB,
            (void *)deleteCB,
            &deleteCB->cbBase.asyncWork));
    NAPI_CALL(env, napi_queue_async_work(env, deleteCB->cbBase.asyncWork));
    napi_value result = 0;
    NAPI_CALL(env, napi_get_null(env, &result));
    HILOG_INFO("%{public}s, asyncCallback end.", __func__);
    return result;
}

napi_value DeletePromise(napi_env env, DAHelperDeleteCB *deleteCB)
{
    HILOG_INFO("%{public}s, promise.", __func__);
    if (deleteCB == nullptr) {
        HILOG_ERROR("%{public}s, param == nullptr.", __func__);
        return nullptr;
    }
    napi_value resourceName;
    NAPI_CALL(env, napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName));
    napi_deferred deferred;
    napi_value promise = 0;
    NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
    deleteCB->cbBase.deferred = deferred;

    NAPI_CALL(env,
        napi_create_async_work(env,
            nullptr,
            resourceName,
            DeleteExecuteCB,
            DeletePromiseCompleteCB,
            (void *)deleteCB,
            &deleteCB->cbBase.asyncWork));
    NAPI_CALL(env, napi_queue_async_work(env, deleteCB->cbBase.asyncWork));
    HILOG_INFO("%{public}s, promise end.", __func__);
    return promise;
}

void DeleteExecuteCB(napi_env env, void *data)
{
    HILOG_INFO("NAPI_Delete, worker pool thread execute.");
    DAHelperDeleteCB *deleteCB = (DAHelperDeleteCB *)data;
    if (deleteCB->dataAbilityHelper != nullptr) {
        OHOS::Uri uri(deleteCB->uri);
        deleteCB->result = deleteCB->dataAbilityHelper->Delete(uri, deleteCB->predicates);
    } else {
        HILOG_ERROR("NAPI_Delete, dataAbilityHelper == nullptr");
    }
    HILOG_INFO("NAPI_Delete, worker pool thread execute end.");
}

void DeleteAsyncCompleteCB(napi_env env, napi_status status, void *data)
{
    HILOG_INFO("NAPI_Delete, main event thread complete.");
    DAHelperDeleteCB *DeleteCB = (DAHelperDeleteCB *)data;
    napi_value callback = nullptr;
    napi_value undefined = nullptr;
    napi_value result[ARGS_TWO] = {nullptr};
    napi_value callResult = nullptr;
    NAPI_CALL_RETURN_VOID(env, napi_get_undefined(env, &undefined));
    NAPI_CALL_RETURN_VOID(env, napi_get_reference_value(env, DeleteCB->cbBase.cbInfo.callback, &callback));

    result[PARAM0] = GetCallbackErrorValue(env, NO_ERROR);
    napi_create_int32(env, DeleteCB->result, &result[PARAM1]);
    NAPI_CALL_RETURN_VOID(env, napi_call_function(env, undefined, callback, ARGS_TWO, &result[PARAM0], &callResult));

    if (DeleteCB->cbBase.cbInfo.callback != nullptr) {
        NAPI_CALL_RETURN_VOID(env, napi_delete_reference(env, DeleteCB->cbBase.cbInfo.callback));
    }
    NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, DeleteCB->cbBase.asyncWork));
    delete DeleteCB;
    DeleteCB = nullptr;
    HILOG_INFO("NAPI_Delete, main event thread complete end.");
}

void DeletePromiseCompleteCB(napi_env env, napi_status status, void *data)
{
    HILOG_INFO("NAPI_Delete,  main event thread complete.");
    DAHelperDeleteCB *DeleteCB = (DAHelperDeleteCB *)data;
    napi_value result = nullptr;
    napi_create_int32(env, DeleteCB->result, &result);
    NAPI_CALL_RETURN_VOID(env, napi_resolve_deferred(env, DeleteCB->cbBase.deferred, result));
    NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, DeleteCB->cbBase.asyncWork));
    delete DeleteCB;
    DeleteCB = nullptr;
    HILOG_INFO("NAPI_Delete,  main event thread complete end.");
}

/**
 * @brief DataAbilityHelper NAPI method : insert.
 *
 * @param env The environment that the Node-API call is invoked under.
 * @param info The callback info passed into the callback function.
 *
 * @return The return value from NAPI C++ to JS for the module.
 */
napi_value NAPI_Update(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s,called", __func__);
    DAHelperUpdateCB *updateCB = new (std::nothrow) DAHelperUpdateCB{
        .cbBase.cbInfo.env = env,
        .cbBase.asyncWork = nullptr,
        .cbBase.deferred = nullptr,
        .cbBase.ability = nullptr,
    };

    napi_value ret = UpdateWrap(env, info, updateCB);
    if (ret == nullptr) {
        HILOG_ERROR("%{public}s,ret == nullptr", __func__);
        if (updateCB != nullptr) {
            delete updateCB;
            updateCB = nullptr;
        }
        ret = WrapVoidToJS(env);
    }
    HILOG_INFO("%{public}s,end", __func__);
    return ret;
}

/**
 * @brief Insert processing function.
 *
 * @param env The environment that the Node-API call is invoked under.
 * @param insertCB Process data asynchronously.
 *
 * @return Return JS data successfully, otherwise return nullptr.
 */
napi_value UpdateWrap(napi_env env, napi_callback_info info, DAHelperUpdateCB *updateCB)
{
    HILOG_INFO("%{public}s,called", __func__);
    size_t argcAsync = ARGS_FOUR;
    const size_t argcPromise = ARGS_THREE;
    const size_t argCountWithAsync = argcPromise + ARGS_ASYNC_COUNT;
    napi_value args[ARGS_MAX_COUNT] = {nullptr};
    napi_value ret = 0;
    napi_value thisVar = nullptr;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argcAsync, args, &thisVar, nullptr));
    if (argcAsync > argCountWithAsync || argcAsync > ARGS_MAX_COUNT) {
        HILOG_ERROR("%{public}s, Wrong argument count.", __func__);
        return nullptr;
    }

    napi_valuetype valuetype = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, args[PARAM0], &valuetype));
    if (valuetype == napi_string) {
        updateCB->uri = NapiValueToStringUtf8(env, args[PARAM0]);
        HILOG_INFO("%{public}s,uri=%{public}s", __func__, updateCB->uri.c_str());
    }

    std::string strValue;
    UnwrapValuesBucket(strValue, env, args[PARAM1]);
    HILOG_INFO("%{public}s,ValuesBucket=%{public}s", __func__, strValue.c_str());
    ValuesBucket valueBucket(strValue);
    updateCB->valueBucket = ValuesBucket(valueBucket);

    std::string strValue2;
    UnwrapDataAbilityPredicates(strValue2, env, args[PARAM2]);
    HILOG_INFO("%{public}s,DataAbilityPredicates=%{public}s", __func__, strValue2.c_str());
    DataAbilityPredicates dataAbpredicates(strValue2);
    updateCB->predicates = DataAbilityPredicates(dataAbpredicates);

    DataAbilityHelper *objectInfo = nullptr;
    napi_unwrap(env, thisVar, (void **)&objectInfo);
    HILOG_INFO("%{public}s,DataAbilityHelper objectInfo = %{public}p", __func__, objectInfo);
    updateCB->dataAbilityHelper = objectInfo;

    if (argcAsync > argcPromise) {
        ret = UpdateAsync(env, args, argcAsync, argcPromise, updateCB);
    } else {
        ret = UpdatePromise(env, updateCB);
    }
    HILOG_INFO("%{public}s,end", __func__);
    return ret;
}

napi_value UpdateAsync(
    napi_env env, napi_value *args, size_t argcAsync, const size_t argcPromise, DAHelperUpdateCB *updateCB)
{
    HILOG_INFO("%{public}s, asyncCallback.", __func__);
    if (args == nullptr || updateCB == nullptr) {
        HILOG_ERROR("%{public}s, param == nullptr.", __func__);
        return nullptr;
    }
    napi_value resourceName = 0;
    NAPI_CALL(env, napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName));

    napi_valuetype valuetype = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, args[argcPromise], &valuetype));
    if (valuetype == napi_function) {
        NAPI_CALL(env, napi_create_reference(env, args[argcPromise], 1, &updateCB->cbBase.cbInfo.callback));
    }

    NAPI_CALL(env,
        napi_create_async_work(env,
            nullptr,
            resourceName,
            UpdateExecuteCB,
            UpdateAsyncCompleteCB,
            (void *)updateCB,
            &updateCB->cbBase.asyncWork));
    NAPI_CALL(env, napi_queue_async_work(env, updateCB->cbBase.asyncWork));
    napi_value result = 0;
    NAPI_CALL(env, napi_get_null(env, &result));
    HILOG_INFO("%{public}s, asyncCallback end.", __func__);
    return result;
}

napi_value UpdatePromise(napi_env env, DAHelperUpdateCB *updateCB)
{
    HILOG_INFO("%{public}s, promise.", __func__);
    if (updateCB == nullptr) {
        HILOG_ERROR("%{public}s, param == nullptr.", __func__);
        return nullptr;
    }
    napi_value resourceName;
    NAPI_CALL(env, napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName));
    napi_deferred deferred;
    napi_value promise = 0;
    NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
    updateCB->cbBase.deferred = deferred;

    NAPI_CALL(env,
        napi_create_async_work(env,
            nullptr,
            resourceName,
            UpdateExecuteCB,
            UpdatePromiseCompleteCB,
            (void *)updateCB,
            &updateCB->cbBase.asyncWork));
    NAPI_CALL(env, napi_queue_async_work(env, updateCB->cbBase.asyncWork));
    HILOG_INFO("%{public}s, promise end.", __func__);
    return promise;
}

void UpdateExecuteCB(napi_env env, void *data)
{
    HILOG_INFO("NAPI_Update, worker pool thread execute.");
    DAHelperUpdateCB *updateCB = (DAHelperUpdateCB *)data;
    if (updateCB->dataAbilityHelper != nullptr) {
        OHOS::Uri uri(updateCB->uri);
        updateCB->result = updateCB->dataAbilityHelper->Update(uri, updateCB->valueBucket, updateCB->predicates);
    } else {
        HILOG_ERROR("NAPI_Update, dataAbilityHelper == nullptr");
    }
    HILOG_INFO("NAPI_Update, worker pool thread execute end.");
}

void UpdateAsyncCompleteCB(napi_env env, napi_status status, void *data)
{
    HILOG_INFO("NAPI_Update, main event thread complete.");
    DAHelperUpdateCB *updateCB = (DAHelperUpdateCB *)data;
    napi_value callback = nullptr;
    napi_value undefined = nullptr;
    napi_value result[ARGS_TWO] = {nullptr};
    napi_value callResult = nullptr;
    NAPI_CALL_RETURN_VOID(env, napi_get_undefined(env, &undefined));
    NAPI_CALL_RETURN_VOID(env, napi_get_reference_value(env, updateCB->cbBase.cbInfo.callback, &callback));

    result[PARAM0] = GetCallbackErrorValue(env, NO_ERROR);
    napi_create_int32(env, updateCB->result, &result[PARAM1]);
    NAPI_CALL_RETURN_VOID(env, napi_call_function(env, undefined, callback, ARGS_TWO, &result[PARAM0], &callResult));

    if (updateCB->cbBase.cbInfo.callback != nullptr) {
        NAPI_CALL_RETURN_VOID(env, napi_delete_reference(env, updateCB->cbBase.cbInfo.callback));
    }
    NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, updateCB->cbBase.asyncWork));
    delete updateCB;
    updateCB = nullptr;
    HILOG_INFO("NAPI_Update, main event thread complete end.");
}

void UpdatePromiseCompleteCB(napi_env env, napi_status status, void *data)
{
    HILOG_INFO("NAPI_Update,  main event thread complete.");
    DAHelperUpdateCB *updateCB = (DAHelperUpdateCB *)data;
    napi_value result = nullptr;
    napi_create_int32(env, updateCB->result, &result);
    NAPI_CALL_RETURN_VOID(env, napi_resolve_deferred(env, updateCB->cbBase.deferred, result));
    NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, updateCB->cbBase.asyncWork));
    delete updateCB;
    updateCB = nullptr;
    HILOG_INFO("NAPI_Update,  main event thread complete end.");
}

/**
 * @brief DataAbilityHelper NAPI method : insert.
 *
 * @param env The environment that the Node-API call is invoked under.
 * @param info The callback info passed into the callback function.
 *
 * @return The return value from NAPI C++ to JS for the module.
 */
napi_value NAPI_OpenFile(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s,called", __func__);
    DAHelperOpenFileCB *openFileCB = new (std::nothrow) DAHelperOpenFileCB{
        .cbBase.cbInfo.env = env,
        .cbBase.asyncWork = nullptr,
        .cbBase.deferred = nullptr,
        .cbBase.ability = nullptr,
    };

    napi_value ret = OpenFileWrap(env, info, openFileCB);
    if (ret == nullptr) {
        HILOG_ERROR("%{public}s,ret == nullptr", __func__);
        if (openFileCB != nullptr) {
            delete openFileCB;
            openFileCB = nullptr;
        }
        ret = WrapVoidToJS(env);
    }
    HILOG_INFO("%{public}s,end", __func__);
    return ret;
}

/**
 * @brief Insert processing function.
 *
 * @param env The environment that the Node-API call is invoked under.
 * @param insertCB Process data asynchronously.
 *
 * @return Return JS data successfully, otherwise return nullptr.
 */
napi_value OpenFileWrap(napi_env env, napi_callback_info info, DAHelperOpenFileCB *openFileCB)
{
    HILOG_INFO("%{public}s,called", __func__);
    size_t argcAsync = ARGS_THREE;
    const size_t argcPromise = ARGS_TWO;
    const size_t argCountWithAsync = argcPromise + ARGS_ASYNC_COUNT;
    napi_value args[ARGS_MAX_COUNT] = {nullptr};
    napi_value ret = 0;
    napi_value thisVar = nullptr;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argcAsync, args, &thisVar, nullptr));
    if (argcAsync > argCountWithAsync || argcAsync > ARGS_MAX_COUNT) {
        HILOG_ERROR("%{public}s, Wrong argument count.", __func__);
        return nullptr;
    }

    napi_valuetype valuetype = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, args[PARAM0], &valuetype));
    if (valuetype == napi_string) {
        openFileCB->uri = NapiValueToStringUtf8(env, args[PARAM0]);
        HILOG_INFO("%{public}s,uri=%{public}s", __func__, openFileCB->uri.c_str());
    }

    NAPI_CALL(env, napi_typeof(env, args[PARAM1], &valuetype));
    if (valuetype == napi_string) {
        openFileCB->mode = NapiValueToStringUtf8(env, args[PARAM1]);
        HILOG_INFO("%{public}s,mode=%{public}s", __func__, openFileCB->mode.c_str());
    }

    DataAbilityHelper *objectInfo = nullptr;
    napi_unwrap(env, thisVar, (void **)&objectInfo);
    HILOG_INFO("%{public}s,DataAbilityHelper objectInfo = %{public}p", __func__, objectInfo);
    openFileCB->dataAbilityHelper = objectInfo;

    if (argcAsync > argcPromise) {
        ret = OpenFileAsync(env, args, argcAsync, argcPromise, openFileCB);
    } else {
        ret = OpenFilePromise(env, openFileCB);
    }
    HILOG_INFO("%{public}s,end", __func__);
    return ret;
}

napi_value OpenFileAsync(
    napi_env env, napi_value *args, size_t argcAsync, const size_t argcPromise, DAHelperOpenFileCB *openFileCB)
{
    HILOG_INFO("%{public}s, asyncCallback.", __func__);
    if (args == nullptr || openFileCB == nullptr) {
        HILOG_ERROR("%{public}s, param == nullptr.", __func__);
        return nullptr;
    }
    napi_value resourceName = 0;
    NAPI_CALL(env, napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName));

    napi_valuetype valuetype = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, args[argcPromise], &valuetype));
    if (valuetype == napi_function) {
        NAPI_CALL(env, napi_create_reference(env, args[argcPromise], 1, &openFileCB->cbBase.cbInfo.callback));
    }

    NAPI_CALL(env,
        napi_create_async_work(env,
            nullptr,
            resourceName,
            OpenFileExecuteCB,
            OpenFileAsyncCompleteCB,
            (void *)openFileCB,
            &openFileCB->cbBase.asyncWork));
    NAPI_CALL(env, napi_queue_async_work(env, openFileCB->cbBase.asyncWork));
    napi_value result = 0;
    NAPI_CALL(env, napi_get_null(env, &result));
    HILOG_INFO("%{public}s, asyncCallback end.", __func__);
    return result;
}

napi_value OpenFilePromise(napi_env env, DAHelperOpenFileCB *openFileCB)
{
    HILOG_INFO("%{public}s, promise.", __func__);
    if (openFileCB == nullptr) {
        HILOG_ERROR("%{public}s, param == nullptr.", __func__);
        return nullptr;
    }
    napi_value resourceName;
    NAPI_CALL(env, napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName));
    napi_deferred deferred;
    napi_value promise = 0;
    NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
    openFileCB->cbBase.deferred = deferred;

    NAPI_CALL(env,
        napi_create_async_work(env,
            nullptr,
            resourceName,
            OpenFileExecuteCB,
            OpenFilePromiseCompleteCB,
            (void *)openFileCB,
            &openFileCB->cbBase.asyncWork));
    NAPI_CALL(env, napi_queue_async_work(env, openFileCB->cbBase.asyncWork));
    HILOG_INFO("%{public}s, promise end.", __func__);
    return promise;
}

void OpenFileExecuteCB(napi_env env, void *data)
{
    HILOG_INFO("NAPI_OpenFile, worker pool thread execute.");
    DAHelperOpenFileCB *OpenFileCB = (DAHelperOpenFileCB *)data;
    if (OpenFileCB->dataAbilityHelper != nullptr) {
        OHOS::Uri uri(OpenFileCB->uri);
        OpenFileCB->result = OpenFileCB->dataAbilityHelper->OpenFile(uri, OpenFileCB->mode);
    } else {
        HILOG_ERROR("NAPI_OpenFile, dataAbilityHelper == nullptr");
    }
    HILOG_INFO("NAPI_OpenFile, worker pool thread execute end.");
}

void OpenFileAsyncCompleteCB(napi_env env, napi_status status, void *data)
{
    HILOG_INFO("NAPI_OpenFile, main event thread complete.");
    DAHelperOpenFileCB *OpenFileCB = (DAHelperOpenFileCB *)data;
    napi_value callback = nullptr;
    napi_value undefined = nullptr;
    napi_value result[ARGS_TWO] = {nullptr};
    napi_value callResult = nullptr;
    NAPI_CALL_RETURN_VOID(env, napi_get_undefined(env, &undefined));
    NAPI_CALL_RETURN_VOID(env, napi_get_reference_value(env, OpenFileCB->cbBase.cbInfo.callback, &callback));

    result[PARAM0] = GetCallbackErrorValue(env, NO_ERROR);
    napi_create_int32(env, OpenFileCB->result, &result[PARAM1]);
    NAPI_CALL_RETURN_VOID(env, napi_call_function(env, undefined, callback, ARGS_TWO, &result[PARAM0], &callResult));

    if (OpenFileCB->cbBase.cbInfo.callback != nullptr) {
        NAPI_CALL_RETURN_VOID(env, napi_delete_reference(env, OpenFileCB->cbBase.cbInfo.callback));
    }
    NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, OpenFileCB->cbBase.asyncWork));
    delete OpenFileCB;
    OpenFileCB = nullptr;
    HILOG_INFO("NAPI_OpenFile, main event thread complete end.");
}

void OpenFilePromiseCompleteCB(napi_env env, napi_status status, void *data)
{
    HILOG_INFO("NAPI_OpenFileCB,  main event thread complete.");
    DAHelperOpenFileCB *OpenFileCB = (DAHelperOpenFileCB *)data;
    napi_value result = nullptr;
    napi_create_int32(env, OpenFileCB->result, &result);
    NAPI_CALL_RETURN_VOID(env, napi_resolve_deferred(env, OpenFileCB->cbBase.deferred, result));
    NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, OpenFileCB->cbBase.asyncWork));
    delete OpenFileCB;
    OpenFileCB = nullptr;
    HILOG_INFO("NAPI_OpenFileCB,  main event thread complete end.");
}

/**
 * @brief DataAbilityHelper NAPI method : insert.
 *
 * @param env The environment that the Node-API call is invoked under.
 * @param info The callback info passed into the callback function.
 *
 * @return The return value from NAPI C++ to JS for the module.
 */
napi_value NAPI_BatchInsert(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s,called", __func__);
    DAHelperBatchInsertCB *BatchInsertCB = new (std::nothrow) DAHelperBatchInsertCB{
        .cbBase.cbInfo.env = env,
        .cbBase.asyncWork = nullptr,
        .cbBase.deferred = nullptr,
        .cbBase.ability = nullptr,
    };

    napi_value ret = BatchInsertWrap(env, info, BatchInsertCB);
    if (ret == nullptr) {
        HILOG_ERROR("%{public}s,ret == nullptr", __func__);
        if (BatchInsertCB != nullptr) {
            delete BatchInsertCB;
            BatchInsertCB = nullptr;
        }
        ret = WrapVoidToJS(env);
    }
    HILOG_INFO("%{public}s,end", __func__);
    return ret;
}

std::vector<ValuesBucket> NapiValueObject(napi_env env, napi_value param)
{
    HILOG_INFO("%{public}s,called", __func__);
    std::vector<ValuesBucket> result;
    UnwrapArrayObjectFromJS(env, param, result);
    return result;
}

bool UnwrapArrayObjectFromJS(napi_env env, napi_value param, std::vector<ValuesBucket> &value)
{
    HILOG_INFO("%{public}s,called", __func__);
    uint32_t arraySize = 0;
    napi_value jsValue = nullptr;
    std::string strValue = "";

    if (!IsArrayForNapiValue(env, param, arraySize)) {
        HILOG_INFO("%{public}s, IsArrayForNapiValue is false", __func__);
        return false;
    }

    value.clear();
    for (uint32_t i = 0; i < arraySize; i++) {
        jsValue = nullptr;
        strValue = "";
        if (napi_get_element(env, param, i, &jsValue) != napi_ok) {
            HILOG_INFO("%{public}s, napi_get_element is false", __func__);
            return false;
        }

        UnwrapValuesBucket(strValue, env, jsValue);
        ValuesBucket valueBucket(strValue);

        value.push_back(valueBucket);
    }
    HILOG_INFO("%{public}s,end", __func__);
    return true;
}

/**
 * @brief Insert processing function.
 *
 * @param env The environment that the Node-API call is invoked under.
 * @param insertCB Process data asynchronously.
 *
 * @return Return JS data successfully, otherwise return nullptr.
 */
napi_value BatchInsertWrap(napi_env env, napi_callback_info info, DAHelperBatchInsertCB *batchInsertCB)
{
    HILOG_INFO("%{public}s,called", __func__);
    size_t argcAsync = ARGS_THREE;
    const size_t argcPromise = ARGS_TWO;
    const size_t argCountWithAsync = argcPromise + ARGS_ASYNC_COUNT;
    napi_value args[ARGS_MAX_COUNT] = {nullptr};
    napi_value ret = 0;
    napi_value thisVar = nullptr;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argcAsync, args, &thisVar, nullptr));
    if (argcAsync > argCountWithAsync || argcAsync > ARGS_MAX_COUNT) {
        HILOG_ERROR("%{public}s, Wrong argument count.", __func__);
        return nullptr;
    }

    napi_valuetype valuetype = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, args[PARAM0], &valuetype));
    if (valuetype == napi_string) {
        batchInsertCB->uri = NapiValueToStringUtf8(env, args[PARAM0]);
        HILOG_INFO("%{public}s,uri=%{public}s", __func__, batchInsertCB->uri.c_str());
    }

    batchInsertCB->values = NapiValueObject(env, args[PARAM1]);

    DataAbilityHelper *objectInfo = nullptr;
    napi_unwrap(env, thisVar, (void **)&objectInfo);
    HILOG_INFO("%{public}s,DataAbilityHelper objectInfo = %{public}p", __func__, objectInfo);
    batchInsertCB->dataAbilityHelper = objectInfo;

    if (argcAsync > argcPromise) {
        ret = BatchInsertAsync(env, args, argcAsync, argcPromise, batchInsertCB);
    } else {
        ret = BatchInsertPromise(env, batchInsertCB);
    }
    HILOG_INFO("%{public}s,end", __func__);
    return ret;
}

napi_value BatchInsertAsync(
    napi_env env, napi_value *args, size_t argcAsync, const size_t argcPromise, DAHelperBatchInsertCB *batchInsertCB)
{
    HILOG_INFO("%{public}s, asyncCallback.", __func__);
    if (args == nullptr || batchInsertCB == nullptr) {
        HILOG_ERROR("%{public}s, param == nullptr.", __func__);
        return nullptr;
    }
    napi_value resourceName = 0;
    NAPI_CALL(env, napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName));

    napi_valuetype valuetype = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, args[argcPromise], &valuetype));
    if (valuetype == napi_function) {
        NAPI_CALL(env, napi_create_reference(env, args[argcPromise], 1, &batchInsertCB->cbBase.cbInfo.callback));
    }

    NAPI_CALL(env,
        napi_create_async_work(env,
            nullptr,
            resourceName,
            BatchInsertExecuteCB,
            BatchInsertAsyncCompleteCB,
            (void *)batchInsertCB,
            &batchInsertCB->cbBase.asyncWork));
    NAPI_CALL(env, napi_queue_async_work(env, batchInsertCB->cbBase.asyncWork));
    napi_value result = 0;
    NAPI_CALL(env, napi_get_null(env, &result));
    HILOG_INFO("%{public}s, asyncCallback end.", __func__);
    return result;
}

napi_value BatchInsertPromise(napi_env env, DAHelperBatchInsertCB *batchInsertCB)
{
    HILOG_INFO("%{public}s, promise.", __func__);
    if (batchInsertCB == nullptr) {
        HILOG_ERROR("%{public}s, param == nullptr.", __func__);
        return nullptr;
    }
    napi_value resourceName;
    NAPI_CALL(env, napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName));
    napi_deferred deferred;
    napi_value promise = 0;
    NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
    batchInsertCB->cbBase.deferred = deferred;

    NAPI_CALL(env,
        napi_create_async_work(env,
            nullptr,
            resourceName,
            BatchInsertExecuteCB,
            BatchInsertPromiseCompleteCB,
            (void *)batchInsertCB,
            &batchInsertCB->cbBase.asyncWork));
    NAPI_CALL(env, napi_queue_async_work(env, batchInsertCB->cbBase.asyncWork));
    HILOG_INFO("%{public}s, promise end.", __func__);
    return promise;
}

void BatchInsertExecuteCB(napi_env env, void *data)
{
    HILOG_INFO("NAPI_BatchInsert, worker pool thread execute.");
    DAHelperBatchInsertCB *batchInsertCB = (DAHelperBatchInsertCB *)data;
    if (batchInsertCB->dataAbilityHelper != nullptr) {
        OHOS::Uri uri(batchInsertCB->uri);
        batchInsertCB->result = batchInsertCB->dataAbilityHelper->BatchInsert(uri, batchInsertCB->values);
    } else {
        HILOG_ERROR("NAPI_BatchInsert, dataAbilityHelper == nullptr");
    }
    HILOG_INFO("NAPI_BatchInsert, worker pool thread execute end.");
}

void BatchInsertAsyncCompleteCB(napi_env env, napi_status status, void *data)
{
    HILOG_INFO("NAPI_BatchInsert, main event thread complete.");
    DAHelperBatchInsertCB *BatchInsertCB = (DAHelperBatchInsertCB *)data;
    napi_value callback = nullptr;
    napi_value undefined = nullptr;
    napi_value result[ARGS_TWO] = {nullptr};
    napi_value callResult = nullptr;
    NAPI_CALL_RETURN_VOID(env, napi_get_undefined(env, &undefined));
    NAPI_CALL_RETURN_VOID(env, napi_get_reference_value(env, BatchInsertCB->cbBase.cbInfo.callback, &callback));

    result[PARAM0] = GetCallbackErrorValue(env, NO_ERROR);
    napi_create_int32(env, BatchInsertCB->result, &result[PARAM1]);
    NAPI_CALL_RETURN_VOID(env, napi_call_function(env, undefined, callback, ARGS_TWO, &result[PARAM0], &callResult));

    if (BatchInsertCB->cbBase.cbInfo.callback != nullptr) {
        NAPI_CALL_RETURN_VOID(env, napi_delete_reference(env, BatchInsertCB->cbBase.cbInfo.callback));
    }
    NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, BatchInsertCB->cbBase.asyncWork));
    delete BatchInsertCB;
    BatchInsertCB = nullptr;
    HILOG_INFO("NAPI_BatchInsert, main event thread complete end.");
}

void BatchInsertPromiseCompleteCB(napi_env env, napi_status status, void *data)
{
    HILOG_INFO("NAPI_BatchInsertCB,  main event thread complete.");
    DAHelperBatchInsertCB *BatchInsertCB = (DAHelperBatchInsertCB *)data;
    napi_value result = nullptr;
    napi_create_int32(env, BatchInsertCB->result, &result);
    NAPI_CALL_RETURN_VOID(env, napi_resolve_deferred(env, BatchInsertCB->cbBase.deferred, result));
    NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, BatchInsertCB->cbBase.asyncWork));
    delete BatchInsertCB;
    BatchInsertCB = nullptr;
    HILOG_INFO("NAPI_BatchInsertCB,  main event thread complete end.");
}

/**
 * @brief DataAbilityHelper NAPI method : insert.
 *
 * @param env The environment that the Node-API call is invoked under.
 * @param info The callback info passed into the callback function.
 *
 * @return The return value from NAPI C++ to JS for the module.
 */
napi_value NAPI_Query(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s,called", __func__);
    DAHelperQueryCB *QueryCB = new (std::nothrow) DAHelperQueryCB{
        .cbBase.cbInfo.env = env,
        .cbBase.asyncWork = nullptr,
        .cbBase.deferred = nullptr,
        .cbBase.ability = nullptr,
    };

    napi_value ret = QueryWrap(env, info, QueryCB);
    if (ret == nullptr) {
        HILOG_ERROR("%{public}s,ret == nullptr", __func__);
        if (QueryCB != nullptr) {
            delete QueryCB;
            QueryCB = nullptr;
        }
        ret = WrapVoidToJS(env);
    }
    HILOG_INFO("%{public}s,end", __func__);
    return ret;
}

/**
 * @brief Insert processing function.
 *
 * @param env The environment that the Node-API call is invoked under.
 * @param insertCB Process data asynchronously.
 *
 * @return Return JS data successfully, otherwise return nullptr.
 */
napi_value QueryWrap(napi_env env, napi_callback_info info, DAHelperQueryCB *queryCB)
{
    HILOG_INFO("%{public}s,called", __func__);
    size_t argcAsync = ARGS_FOUR;
    const size_t argcPromise = ARGS_THREE;
    const size_t argCountWithAsync = argcPromise + ARGS_ASYNC_COUNT;
    napi_value args[ARGS_MAX_COUNT] = {nullptr};
    napi_value ret = 0;
    napi_value thisVar = nullptr;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argcAsync, args, &thisVar, nullptr));
    if (argcAsync > argCountWithAsync || argcAsync > ARGS_MAX_COUNT) {
        HILOG_ERROR("%{public}s, Wrong argument count.", __func__);
        return nullptr;
    }

    napi_valuetype valuetype = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, args[PARAM0], &valuetype));
    if (valuetype == napi_string) {
        queryCB->uri = NapiValueToStringUtf8(env, args[PARAM0]);
        HILOG_INFO("%{public}s,uri=%{public}s", __func__, queryCB->uri.c_str());
    }

    std::vector<std::string> result;
    bool arrayStringbool = false;
    arrayStringbool = NapiValueToArrayStringUtf8(env, args[PARAM1], result);
    if (arrayStringbool == false) {
        HILOG_ERROR("%{public}s, The return value of arraystringbool is false", __func__);
        return nullptr;
    }
    queryCB->columns = result;
    for (size_t i = 0; i < queryCB->columns.size(); i++) {
        HILOG_INFO("%{public}s,columns=%{public}s", __func__, queryCB->columns.at(i).c_str());
    }

    std::string strValue;
    UnwrapDataAbilityPredicates(strValue, env, args[PARAM2]);
    HILOG_INFO("%{public}s,dataAbilityPredicates=%{public}s", __func__, strValue.c_str());
    DataAbilityPredicates dataAbilityPredicates(strValue);
    queryCB->predicates = DataAbilityPredicates(dataAbilityPredicates);

    DataAbilityHelper *objectInfo = nullptr;
    napi_unwrap(env, thisVar, (void **)&objectInfo);
    HILOG_INFO("%{public}s,DataAbilityHelper objectInfo = %{public}p", __func__, objectInfo);
    queryCB->dataAbilityHelper = objectInfo;

    if (argcAsync > argcPromise) {
        ret = QueryAsync(env, args, argcAsync, argcPromise, queryCB);
    } else {
        ret = QueryPromise(env, queryCB);
    }
    HILOG_INFO("%{public}s,end", __func__);
    return ret;
}

napi_value QueryAsync(
    napi_env env, napi_value *args, size_t argcAsync, const size_t argcPromise, DAHelperQueryCB *queryCB)
{
    HILOG_INFO("%{public}s, asyncCallback.", __func__);
    if (args == nullptr || queryCB == nullptr) {
        HILOG_ERROR("%{public}s, param == nullptr.", __func__);
        return nullptr;
    }
    napi_value resourceName = 0;
    NAPI_CALL(env, napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName));

    napi_valuetype valuetype = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, args[argcPromise], &valuetype));
    if (valuetype == napi_function) {
        NAPI_CALL(env, napi_create_reference(env, args[argcPromise], 1, &queryCB->cbBase.cbInfo.callback));
    }

    NAPI_CALL(env,
        napi_create_async_work(env,
            nullptr,
            resourceName,
            QueryExecuteCB,
            QueryAsyncCompleteCB,
            (void *)queryCB,
            &queryCB->cbBase.asyncWork));
    NAPI_CALL(env, napi_queue_async_work(env, queryCB->cbBase.asyncWork));
    napi_value result = 0;
    NAPI_CALL(env, napi_get_null(env, &result));
    HILOG_INFO("%{public}s, asyncCallback end.", __func__);
    return result;
}

napi_value QueryPromise(napi_env env, DAHelperQueryCB *queryCB)
{
    HILOG_INFO("%{public}s, promise.", __func__);
    if (queryCB == nullptr) {
        HILOG_ERROR("%{public}s, param == nullptr.", __func__);
        return nullptr;
    }
    napi_value resourceName;
    NAPI_CALL(env, napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName));
    napi_deferred deferred;
    napi_value promise = 0;
    NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
    queryCB->cbBase.deferred = deferred;

    NAPI_CALL(env,
        napi_create_async_work(env,
            nullptr,
            resourceName,
            QueryExecuteCB,
            QueryPromiseCompleteCB,
            (void *)queryCB,
            &queryCB->cbBase.asyncWork));
    NAPI_CALL(env, napi_queue_async_work(env, queryCB->cbBase.asyncWork));
    HILOG_INFO("%{public}s, promise end.", __func__);
    return promise;
}

void QueryPromiseCompleteCB(napi_env env, napi_status status, void *data)
{
    HILOG_INFO("NAPI_DAHelperQueryCB,  main event thread complete.");
    DAHelperQueryCB *QueryCB = (DAHelperQueryCB *)data;
    napi_value result = nullptr;
    result = WrapResultSet(env, QueryCB->result);
    NAPI_CALL_RETURN_VOID(env, napi_resolve_deferred(env, QueryCB->cbBase.deferred, result));
    NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, QueryCB->cbBase.asyncWork));
    delete QueryCB;
    QueryCB = nullptr;
    HILOG_INFO("NAPI_DAHelperQueryCB,  main event thread complete end.");
}

void QueryExecuteCB(napi_env env, void *data)
{
    HILOG_INFO("NAPI_Query, worker pool thread execute.");
    DAHelperQueryCB *queryCB = (DAHelperQueryCB *)data;
    std::shared_ptr<ResultSet> resultset = nullptr;
    if (queryCB->dataAbilityHelper != nullptr) {
        OHOS::Uri uri(queryCB->uri);
        resultset = queryCB->dataAbilityHelper->Query(uri, queryCB->columns, queryCB->predicates);
        if (resultset != nullptr) {
            queryCB->result.testInf_ = resultset->testInf_;
        } else {
            HILOG_INFO("NAPI_Query, resultset == nullptr.");
        }
    } else {
        HILOG_ERROR("NAPI_Query, dataAbilityHelper == nullptr");
    }
    HILOG_INFO("NAPI_Query, worker pool thread execute end.");
}

void QueryAsyncCompleteCB(napi_env env, napi_status status, void *data)
{
    HILOG_INFO("NAPI_Query, main event thread complete.");
    DAHelperQueryCB *queryCB = (DAHelperQueryCB *)data;
    napi_value callback = nullptr;
    napi_value undefined = nullptr;
    napi_value result[ARGS_TWO] = {nullptr};
    napi_value callResult = nullptr;
    NAPI_CALL_RETURN_VOID(env, napi_get_undefined(env, &undefined));
    NAPI_CALL_RETURN_VOID(env, napi_get_reference_value(env, queryCB->cbBase.cbInfo.callback, &callback));

    result[PARAM0] = GetCallbackErrorValue(env, NO_ERROR);
    result[PARAM1] = WrapResultSet(env, queryCB->result);

    NAPI_CALL_RETURN_VOID(env, napi_call_function(env, undefined, callback, ARGS_TWO, &result[PARAM0], &callResult));

    if (queryCB->cbBase.cbInfo.callback != nullptr) {
        NAPI_CALL_RETURN_VOID(env, napi_delete_reference(env, queryCB->cbBase.cbInfo.callback));
    }
    NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, queryCB->cbBase.asyncWork));
    delete queryCB;
    queryCB = nullptr;
    HILOG_INFO("NAPI_Query, main event thread complete end.");
}

napi_value WrapResultSet(napi_env env, const ResultSet &resultSet)
{
    HILOG_INFO("%{public}s,called", __func__);
    napi_value result = nullptr;
    napi_value proValue = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    NAPI_CALL(env, napi_create_string_utf8(env, resultSet.testInf_.c_str(), NAPI_AUTO_LENGTH, &proValue));
    NAPI_CALL(env, napi_set_named_property(env, result, "value", proValue));
    HILOG_INFO("%{public}s,end", __func__);
    return result;
}

napi_value NAPI_Release(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s,called", __func__);
    DAHelperReleaseCB *releaseCB = new (std::nothrow) DAHelperReleaseCB{
        .cbBase.cbInfo.env = env,
        .cbBase.asyncWork = nullptr,
        .cbBase.deferred = nullptr,
    };

    napi_value ret = ReleaseWrap(env, info, releaseCB);
    if (ret == nullptr) {
        HILOG_ERROR("%{public}s,ret == nullptr", __func__);
        if (releaseCB != nullptr) {
            delete releaseCB;
            releaseCB = nullptr;
        }
        ret = WrapVoidToJS(env);
    }
    HILOG_INFO("%{public}s,end", __func__);
    return ret;
}

napi_value ReleaseWrap(napi_env env, napi_callback_info info, DAHelperReleaseCB *releaseCB)
{
    HILOG_INFO("%{public}s,called", __func__);
    size_t argcAsync = ARGS_TWO;
    const size_t argcPromise = ARGS_ONE;
    const size_t argCountWithAsync = argcPromise + ARGS_ASYNC_COUNT;
    napi_value args[ARGS_MAX_COUNT] = {nullptr};
    napi_value ret = nullptr;
    napi_value thisVar = nullptr;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argcAsync, args, &thisVar, nullptr));
    if (argcAsync > argCountWithAsync || argcAsync > ARGS_MAX_COUNT) {
        HILOG_ERROR("%{public}s, Wrong argument count.", __func__);
        return nullptr;
    }

    DataAbilityHelper *objectInfo = nullptr;
    napi_unwrap(env, thisVar, (void **)&objectInfo);
    HILOG_INFO("DataAbilityHelper ReleaseWrap objectInfo = %{public}p", objectInfo);
    releaseCB->dataAbilityHelper = objectInfo;

    if (argcAsync > argcPromise) {
        ret = ReleaseAsync(env, args, argcAsync, argcPromise, releaseCB);
    } else {
        ret = ReleasePromise(env, releaseCB);
    }
    HILOG_INFO("%{public}s,end", __func__);
    return ret;
}

napi_value ReleaseAsync(
    napi_env env, napi_value *args, size_t argcAsync, const size_t argcPromise, DAHelperReleaseCB *releaseCB)
{
    HILOG_INFO("%{public}s, asyncCallback.", __func__);
    if (args == nullptr || releaseCB == nullptr) {
        HILOG_ERROR("%{public}s, param == nullptr.", __func__);
        return nullptr;
    }
    napi_value resourceName = 0;
    NAPI_CALL(env, napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName));

    napi_valuetype valuetype = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, args[argcPromise], &valuetype));
    if (valuetype == napi_function) {
        NAPI_CALL(env, napi_create_reference(env, args[argcPromise], 1, &releaseCB->cbBase.cbInfo.callback));
    }

    NAPI_CALL(env,
        napi_create_async_work(env,
            nullptr,
            resourceName,
            ReleaseExecuteCB,
            ReleaseAsyncCompleteCB,
            (void *)releaseCB,
            &releaseCB->cbBase.asyncWork));
    NAPI_CALL(env, napi_queue_async_work(env, releaseCB->cbBase.asyncWork));
    napi_value result = 0;
    NAPI_CALL(env, napi_get_null(env, &result));
    HILOG_INFO("%{public}s, asyncCallback end.", __func__);
    return result;
}

napi_value ReleasePromise(napi_env env, DAHelperReleaseCB *releaseCB)
{
    HILOG_INFO("%{public}s, promise.", __func__);
    if (releaseCB == nullptr) {
        HILOG_ERROR("%{public}s, param == nullptr.", __func__);
        return nullptr;
    }
    napi_value resourceName;
    NAPI_CALL(env, napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName));
    napi_deferred deferred;
    napi_value promise = 0;
    NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
    releaseCB->cbBase.deferred = deferred;

    NAPI_CALL(env,
        napi_create_async_work(env,
            nullptr,
            resourceName,
            ReleaseExecuteCB,
            ReleasePromiseCompleteCB,
            (void *)releaseCB,
            &releaseCB->cbBase.asyncWork));
    NAPI_CALL(env, napi_queue_async_work(env, releaseCB->cbBase.asyncWork));
    HILOG_INFO("%{public}s, promise end.", __func__);
    return promise;
}

void ReleaseExecuteCB(napi_env env, void *data)
{
    HILOG_INFO("NAPI_Release, worker pool thread execute.");
    DAHelperReleaseCB *releaseCB = (DAHelperReleaseCB *)data;
    if (releaseCB->dataAbilityHelper != nullptr) {
        releaseCB->result = releaseCB->dataAbilityHelper->Release();
    } else {
        HILOG_ERROR("NAPI_Release, dataAbilityHelper == nullptr");
    }
    HILOG_INFO("NAPI_Release, worker pool thread execute end.");
}

void ReleaseAsyncCompleteCB(napi_env env, napi_status status, void *data)
{
    HILOG_INFO("NAPI_Release, main event thread complete.");
    DAHelperReleaseCB *releaseCB = (DAHelperReleaseCB *)data;
    napi_value callback = nullptr;
    napi_value undefined = nullptr;
    napi_value result[ARGS_TWO] = {nullptr};
    napi_value callResult = nullptr;
    NAPI_CALL_RETURN_VOID(env, napi_get_undefined(env, &undefined));
    NAPI_CALL_RETURN_VOID(env, napi_get_reference_value(env, releaseCB->cbBase.cbInfo.callback, &callback));

    result[PARAM0] = GetCallbackErrorValue(env, NO_ERROR);
    napi_get_boolean(env, releaseCB->result, &result[PARAM1]);
    NAPI_CALL_RETURN_VOID(env, napi_call_function(env, undefined, callback, ARGS_TWO, &result[PARAM0], &callResult));

    if (releaseCB->cbBase.cbInfo.callback != nullptr) {
        NAPI_CALL_RETURN_VOID(env, napi_delete_reference(env, releaseCB->cbBase.cbInfo.callback));
    }
    NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, releaseCB->cbBase.asyncWork));
    delete releaseCB;
    releaseCB = nullptr;
    HILOG_INFO("NAPI_Release, main event thread complete end.");
}

void ReleasePromiseCompleteCB(napi_env env, napi_status status, void *data)
{
    HILOG_INFO("NAPI_Release,  main event thread complete.");
    DAHelperReleaseCB *releaseCB = (DAHelperReleaseCB *)data;
    napi_value result = nullptr;
    napi_get_boolean(env, releaseCB->result, &result);
    NAPI_CALL_RETURN_VOID(env, napi_resolve_deferred(env, releaseCB->cbBase.deferred, result));
    NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, releaseCB->cbBase.asyncWork));
    delete releaseCB;
    releaseCB = nullptr;
    HILOG_INFO("NAPI_Release,  main event thread complete end.");
}

}  // namespace AppExecFwk
}  // namespace OHOS