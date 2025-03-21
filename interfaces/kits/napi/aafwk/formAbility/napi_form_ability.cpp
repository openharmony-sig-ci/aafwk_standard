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
#include "napi_form_ability.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include <uv.h>
#include <vector>
#include <exception>

using namespace OHOS;
using namespace OHOS::AAFwk;
using namespace OHOS::AppExecFwk;

namespace {
    constexpr size_t ARGS_SIZE_ONE = 1;
    constexpr size_t ARGS_SIZE_TWO = 2;
    constexpr size_t ARGS_SIZE_THREE = 3;
}

/**
 * @brief Get a C++ string value from Node-API
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[in] value This is an opaque pointer that is used to represent a JavaScript value
 *
 * @return Return a C++ string
 */
static std::string GetStringFromNAPI(napi_env env, napi_value value)
{
    std::string result;
    size_t size = 0;

    if (napi_get_value_string_utf8(env, value, nullptr, 0, &size) != napi_ok) {
        HILOG_ERROR("%{public}s, can not get string size", __func__);
        return "";
    }
    result.reserve(size + 1);
    result.resize(size);
    if (napi_get_value_string_utf8(env, value, result.data(), (size + 1), &size) != napi_ok) {
        HILOG_ERROR("%{public}s, can not get string value", __func__);
        return "";
    }
    return result;
}

/**
 * @brief NapiGetNull
 *
 * @param[in] env The environment that the Node-API call is invoked under
 *
 * @return napi_value
 */
napi_value NapiGetNull(napi_env env)
{
    napi_value result = 0;
    napi_get_null(env, &result);

    return result;
}

/**
 * @brief Parse form info from JavaScript value
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] formReqInfo This is a C++ value, parsed from JavaScript
 * @param[in] args This is an opaque pointer that is used to represent a JavaScript value
 *
 * @return Return an opaque pointer that is used to represent a JavaScript value
 */
static napi_value ParseNapiIntoFormInfo(napi_env env, 
                                        FormReqInfo &formReqInfo, 
                                        napi_value args)
{
    napi_valuetype valueType;
    NAPI_CALL(env, napi_typeof(env, args, &valueType));
    NAPI_ASSERT(env, valueType == napi_object, "The arguments[0] type of acquireForm is incorrect,\
    expected type is object.");

    napi_value wantProp = nullptr;
    napi_status status = napi_get_named_property(env, args, "want", &wantProp);
    NAPI_ASSERT(env, status == napi_ok, "The arguments[0] info of acquireForm is incorrect,\
    expected key is want.");

    // elementName
    napi_value elementNameProp = nullptr;
    status = napi_get_named_property(env, wantProp, "elementName", &elementNameProp);
    NAPI_ASSERT(env, status == napi_ok, "The arguments[0] want info of acquireForm is incorrect,\
    expected key is elementName.");
    napi_typeof(env, elementNameProp, &valueType);
    NAPI_ASSERT(env, valueType == napi_object, "The arguments[0] elementName type of acquireForm is incorrect,\
    expected type is object.");
    
    // elementName:deviceId property
    napi_value prop = nullptr;
    status = napi_get_named_property(env, elementNameProp, "deviceId", &prop);
    NAPI_ASSERT(env, status == napi_ok, "The arguments[0] elementName info of acquireForm is incorrect,\
    expected key is deviceId.");

    napi_typeof(env, prop, &valueType);
    NAPI_ASSERT(env, valueType == napi_string, "The arguments[0] deviceId type of acquireForm is incorrect,\
    expected type is string.");

    formReqInfo.deviceId = GetStringFromNAPI(env, prop);
    HILOG_DEBUG("%{public}s, acquireForm deviceId=%{public}s.", __func__, formReqInfo.deviceId.c_str());
    
    // elementName:bundleName property
    prop = nullptr;
    status = napi_get_named_property(env, elementNameProp, "bundleName", &prop);
    NAPI_ASSERT(env, status == napi_ok, "The arguments[0] elementName info of acquireForm is incorrect,\
    expected key is bundleName.");
    napi_typeof(env, prop, &valueType);
    NAPI_ASSERT(env, valueType == napi_string, "The arguments[0] bundleName type of acquireForm is incorrect,\
    expected type is string.");

    formReqInfo.bundleName = GetStringFromNAPI(env, prop);
    HILOG_DEBUG("%{public}s, acquireForm bundleName=%{public}s.", __func__, formReqInfo.bundleName.c_str());
    
    // elementName:abilityName property
    prop = nullptr;
    status = napi_get_named_property(env, elementNameProp, "abilityName", &prop);
    NAPI_ASSERT(env, status == napi_ok, "The arguments[0] elementName info of acquireForm is incorrect,\
    expected key is abilityName.");

    napi_typeof(env, prop, &valueType);
    NAPI_ASSERT(env, valueType == napi_string, "The arguments[0] abilityName type of acquireForm is incorrect,\
    expected type is string.");

    formReqInfo.abilityName = GetStringFromNAPI(env, prop);
    HILOG_DEBUG("%{public}s, acquireForm abilityName=%{public}s.", __func__, formReqInfo.abilityName.c_str());

    // formReqInfo:formId property
    prop = nullptr;
    status = napi_get_named_property(env, wantProp, "formId", &prop);
    if(status == napi_ok ) {
        napi_typeof(env, prop, &valueType);
        HILOG_DEBUG("%{public}s, acquireForm formId type=%{public}d.", __func__, valueType);

        if(valueType == napi_undefined || valueType == napi_null) {
            formReqInfo.formId = 0;
        } else if (valueType == napi_number) {
            status = napi_get_value_int64(env, prop, &formReqInfo.formId);
        }  else if (valueType == napi_string) {
//            try {
                std::string strFormId = GetStringFromNAPI(env, prop);
                formReqInfo.formId = std::stoll(strFormId);
            // } catch (const std::invalid_argument&) {
            //     NAPI_ASSERT(env, false, "The formId is incorrect, expected type is number, string, undefined OR null");
            // } catch (const std::out_of_range&) {
            //     NAPI_ASSERT(env, false, "The formId is incorrect, expected the value doesn't exceed 64 bits.");
            // } catch (const std::exception &) {
            //     NAPI_ASSERT(env, false, "The formId is incorrect, expected type is number or string and no more than 64 bits.");
            // }
        } else {
            NAPI_ASSERT(env, false, "The formId must be number, string, undefined OR null");
        }
    } else {
        formReqInfo.formId = 0;
    }
    //HILOG_DEBUG("%{public}s, acquireForm formId=%{public}lld.", __func__, formReqInfo.formId);

    // formReqInfo:formName property
    prop = nullptr;
    status = napi_get_named_property(env, wantProp, "formName", &prop);
    NAPI_ASSERT(env, status == napi_ok, "The arguments[0] want info of acquireForm is incorrect,\
    expected key is formName.");

    napi_typeof(env, prop, &valueType);
    NAPI_ASSERT(env, valueType == napi_string, "The arguments[0] formName type of acquireForm is incorrect,\
    expected type is string.");

    formReqInfo.formName = GetStringFromNAPI(env, prop);
    HILOG_DEBUG("%{public}s, acquireForm formName=%{public}s.", __func__, formReqInfo.formName.c_str());

    // formReqInfo:moduleName property
    prop = nullptr;
    status = napi_get_named_property(env, wantProp, "moduleName", &prop);
    NAPI_ASSERT(env, status == napi_ok, "The arguments[0] want info of acquireForm is incorrect,\
    expected key is moduleName.");
    napi_typeof(env, prop, &valueType);
    NAPI_ASSERT(env, valueType == napi_string, "The arguments[0] moduleName type of acquireForm is incorrect,\
    expected type is string.");

    formReqInfo.moduleName = GetStringFromNAPI(env, prop);
    HILOG_DEBUG("%{public}s, acquireForm moduleName=%{public}s.", __func__, formReqInfo.moduleName.c_str());    

    // formReqInfo:tempFormFlag property
    prop = nullptr;
    status = napi_get_named_property(env, wantProp, "tempFormFlag", &prop);
    NAPI_ASSERT(env, status == napi_ok, "The arguments[0] want info of acquireForm is incorrect,\
    expected key is tempFormFlag.");

    napi_typeof(env, prop, &valueType);
    NAPI_ASSERT(env, valueType == napi_boolean, "The arguments[0] tempFormFlag type of acquireForm is incorrect,\
    expected type is boolean.");

    status = napi_get_value_bool(env, prop, &formReqInfo.tempFormFlag);
    NAPI_ASSERT(env, status == napi_ok, "The arguments[0] tempFormFlag is incorrect,\
    get value failed.");

    HILOG_DEBUG("%{public}s, acquireForm tempFormFlag=%{public}d.", __func__, formReqInfo.tempFormFlag);

    // // formReqInfo:formWidth property
    // prop = nullptr;
    // status = napi_get_named_property(env, wantProp, "formWidth", &prop);
    // NAPI_ASSERT(env, status == napi_ok, "The arguments[0] want info of acquireForm is incorrect,\
    // expected key is formWidth.");
    // napi_typeof(env, prop, &valueType);
    // NAPI_ASSERT(env, valueType == napi_number, "The arguments[0] formWidth type of acquireForm is incorrect,\
    // expected type is number.");
    // status = napi_get_value_int32(env, prop, &formReqInfo.formWidth);
    // NAPI_ASSERT(env, status == napi_ok, "The arguments[0] formWidth is incorrect,\
    // get value failed.");
    // HILOG_DEBUG("%{public}s, acquireForm formWidth=%{public}d.", __func__, formReqInfo.formWidth);

    // // formReqInfo:formHeight property
    // prop = nullptr;
    // status = napi_get_named_property(env, wantProp, "formHeight", &prop);
    // NAPI_ASSERT(env, status == napi_ok, "The arguments[0] want info of acquireForm is incorrect,\
    // expected key is formHeight.");
    // napi_typeof(env, prop, &valueType);
    // NAPI_ASSERT(env, valueType == napi_number, "The arguments[0] formHeight type of acquireForm is incorrect,\
    // expected type is number.");
    // status = napi_get_value_int32(env, prop, &formReqInfo.formHeight);
    // NAPI_ASSERT(env, status == napi_ok, "The arguments[0] formHeight is incorrect,\
    // get value failed.");
    // HILOG_DEBUG("%{public}s, acquireForm formHeight=%{public}d.", __func__, formReqInfo.formHeight);

    // formReqInfo:formDimension property
    prop = nullptr;
    status = napi_get_named_property(env, wantProp, "formDimension", &prop);
    NAPI_ASSERT(env, status == napi_ok, "The arguments[0] want info of acquireForm is incorrect,\
    expected key is formDimension.");
    napi_typeof(env, prop, &valueType);
    NAPI_ASSERT(env, valueType == napi_number, "The arguments[0] formDimension type of acquireForm is incorrect,\
    expected type is number.");
    status = napi_get_value_int32(env, prop, &formReqInfo.formDimension);
    NAPI_ASSERT(env, status == napi_ok, "The arguments[0] formDimension is incorrect,\
    get value failed.");
    HILOG_DEBUG("%{public}s, acquireForm formDimension=%{public}d.", __func__, formReqInfo.formDimension);

    return NapiGetNull(env);
}

/**
 * @brief Parse arguments of the callback function from JavaScript value
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] jsCallbackInfo This is a C++ value, parsed from JavaScript
 * @param[in] args This is an opaque pointer that is used to represent a JavaScript value
 *
 * @return Return an opaque pointer that is used to represent a JavaScript value
 */
static napi_value ParseJsObjectCallbackParam(napi_env env, 
                                             JsCallbackInfo &jsCallbackInfo, 
                                             napi_value args)
{
    napi_valuetype valueType;
    NAPI_CALL(env, napi_typeof(env, args, &valueType));
    NAPI_ASSERT(env, valueType == napi_object, "The arguments[1] type of acquireForm is incorrect,\
    expected type is object.");

    napi_value callbackProp = nullptr;
    napi_status status = napi_get_named_property(env, args, "callback", &callbackProp);
    NAPI_ASSERT(env, status == napi_ok, "The arguments[1] callback of acquireForm is incorrect,\
    expected key is callback.");

    // get callback: onAcquired
    napi_value prop = nullptr;
    status = napi_get_named_property(env, callbackProp, "onAcquired", &prop);
    NAPI_ASSERT(env, status == napi_ok, "The arguments[1] callback of acquireForm is incorrect,\
    expected key is onAcquired.");

    napi_typeof(env, prop, &valueType);
    NAPI_ASSERT(env, valueType == napi_function, "The arguments[1] type of onAcquired is incorrect,\
    expected type is function.");

    napi_create_reference(env, prop, 1, &jsCallbackInfo.onAcquired);

    // get callback: onError
    prop = nullptr;
    status = napi_get_named_property(env, callbackProp, "onError", &prop);
    NAPI_ASSERT(env, status == napi_ok, "The arguments[1] callback of acquireForm is incorrect,\
    expected key is onError.");

    napi_typeof(env, prop, &valueType);
    NAPI_ASSERT(env, valueType == napi_function, "The arguments[1] type of onError is incorrect,\
    expected type is function.");

    napi_create_reference(env, prop, 1, &jsCallbackInfo.onError);

    HILOG_DEBUG("%{public}s end", __func__);

    return NapiGetNull(env);
}

/**
 * @brief Parse form JavaScript info into Node-API
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[in] formInfo it is used for return forminfo to JavaScript
 * @param[out] result This is an opaque pointer that is used to represent a JavaScript value
 *
 * @return void
 */
static void ParseFormJsInfoIntoNapi(napi_env env, const FormJsInfo &formInfo, napi_value &result)
{
    // formId
    napi_value formId;
    std::string strFormId = std::to_string(formInfo.formId);
    napi_create_string_utf8(env, strFormId.c_str(), NAPI_AUTO_LENGTH, &formId);
    //HILOG_DEBUG("%{public}s, formId=%{public}lld.", __func__, formInfo.formId);
    napi_set_named_property(env, result, "formId", formId);

    // formName
    napi_value formName;
    napi_create_string_utf8(env, formInfo.formName.c_str(), NAPI_AUTO_LENGTH, &formName);
    HILOG_DEBUG("%{public}s, formName=%{public}s.", __func__, formInfo.formName.c_str());
    napi_set_named_property(env, result, "formName", formName);

    // bundleName
    napi_value bundleName;
    napi_create_string_utf8(env, formInfo.bundleName.c_str(), NAPI_AUTO_LENGTH, &bundleName);
    HILOG_DEBUG("%{public}s, bundleName=%{public}s.", __func__, formInfo.bundleName.c_str());
    napi_set_named_property(env, result, "bundleName", bundleName);

    // abilityName
    napi_value abilityName;
    napi_create_string_utf8(env, formInfo.abilityName.c_str(), NAPI_AUTO_LENGTH, &abilityName);
    HILOG_DEBUG("%{public}s, abilityName=%{public}s.", __func__, formInfo.abilityName.c_str());
    napi_set_named_property(env, result, "abilityName", abilityName);

    // formTempFlg
    napi_value formTempFlg;
    napi_create_int32(env, formInfo.formTempFlg, &formTempFlg);
    HILOG_DEBUG("%{public}s, formTempFlg=%{public}d.", __func__, formInfo.formTempFlg);
    napi_set_named_property(env, result, "formTempFlg", formTempFlg);

    // htmlPath
    napi_value htmlPath;
    napi_create_string_utf8(env, formInfo.htmlPath.c_str(), NAPI_AUTO_LENGTH, &htmlPath);
    HILOG_DEBUG("%{public}s, htmlPath=%{public}s.", __func__, formInfo.htmlPath.c_str());
    napi_set_named_property(env, result, "htmlPath", htmlPath);

    // cssPath
    napi_value cssPath;
    napi_create_string_utf8(env, formInfo.cssPath.c_str(), NAPI_AUTO_LENGTH, &cssPath);
    HILOG_DEBUG("%{public}s, cssPath=%{public}s.", __func__, formInfo.cssPath.c_str());
    napi_set_named_property(env, result, "cssPath", cssPath);

    // jsPath
    napi_value jsPath;
    napi_create_string_utf8(env, formInfo.jsPath.c_str(), NAPI_AUTO_LENGTH, &jsPath);
    HILOG_DEBUG("%{public}s, jsPath=%{public}s.", __func__, formInfo.jsPath.c_str());
    napi_set_named_property(env, result, "jsPath", jsPath);

    // fileReousePath
    napi_value fileReousePath;
    napi_create_string_utf8(env, formInfo.fileReousePath.c_str(), NAPI_AUTO_LENGTH, &fileReousePath);
    HILOG_DEBUG("%{public}s, fileReousePath=%{public}s.", __func__, formInfo.fileReousePath.c_str());
    napi_set_named_property(env, result, "fileReousePath", fileReousePath);

    // formData
    napi_value formData;
    napi_create_string_utf8(env, formInfo.formData.c_str(), NAPI_AUTO_LENGTH, &formData);
    HILOG_DEBUG("%{public}s, formData=%{public}s.", __func__, formInfo.formData.c_str());
    napi_set_named_property(env, result, "formData", formData);

    return;
}

/**
 * @brief Parse form info into Node-API
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[in] formInfo it is used for return forminfo to JavaScript
 * @param[out] result This is an opaque pointer that is used to represent a JavaScript value
 *
 * @return void
 */
static void ParseFormInfoIntoNapi(napi_env env, const FormInfo &formInfo, napi_value &result)
{
    // bundleName
    napi_value bundleName;
    napi_create_string_utf8(env, formInfo.bundleName.c_str(), NAPI_AUTO_LENGTH, &bundleName);
    HILOG_DEBUG("%{public}s, bundleName=%{public}s.", __func__, formInfo.bundleName.c_str());
    napi_set_named_property(env, result, "bundleName", bundleName);

    // moduleName
    napi_value moduleName;
    napi_create_string_utf8(env, formInfo.moduleName.c_str(), NAPI_AUTO_LENGTH, &moduleName);
    HILOG_DEBUG("%{public}s, moduleName=%{public}s.", __func__, formInfo.moduleName.c_str());
    napi_set_named_property(env, result, "moduleName", moduleName);

    // abilityName
    napi_value abilityName;
    napi_create_string_utf8(env, formInfo.abilityName.c_str(), NAPI_AUTO_LENGTH, &abilityName);
    HILOG_DEBUG("%{public}s, abilityName=%{public}s.", __func__, formInfo.abilityName.c_str());
    napi_set_named_property(env, result, "abilityName", abilityName);

    // name
    napi_value name;
    napi_create_string_utf8(env, formInfo.name.c_str(), NAPI_AUTO_LENGTH, &name);
    HILOG_DEBUG("%{public}s, name=%{public}s.", __func__, formInfo.name.c_str());
    napi_set_named_property(env, result, "name", name);

    // description
    napi_value description;
    napi_create_string_utf8(env, formInfo.description.c_str(), NAPI_AUTO_LENGTH, &description);
    HILOG_DEBUG("%{public}s, description=%{public}s.", __func__, formInfo.description.c_str());
    napi_set_named_property(env, result, "description", description);

    // descriptionId
    napi_value descriptionId;
    napi_create_int32(env, formInfo.descriptionId, &descriptionId);
    HILOG_DEBUG("%{public}s, descriptionId=%{public}d.", __func__, formInfo.descriptionId);
    napi_set_named_property(env, result, "descriptionId", descriptionId);

    // type
    napi_value type;
    FormType formType = formInfo.type;
    napi_create_int32(env, (int32_t)formType, &type);
    HILOG_DEBUG("%{public}s, formInfo_type=%{public}d.", __func__, (int32_t)formType);
    napi_set_named_property(env, result, "type", type);

    // jsComponentName
    napi_value jsComponentName;
    napi_create_string_utf8(env, formInfo.jsComponentName.c_str(), NAPI_AUTO_LENGTH, &jsComponentName);
    HILOG_DEBUG("%{public}s, jsComponentName=%{public}s.", __func__, formInfo.jsComponentName.c_str());
    napi_set_named_property(env, result, "jsComponentName", jsComponentName);

    // colorMode
    napi_value colorMode;
    FormsColorMode  formsColorMode = formInfo.colorMode;
    napi_create_int32(env, (int32_t)formsColorMode, &colorMode);
    HILOG_DEBUG("%{public}s, formInfo_type=%{public}d.", __func__, (int32_t)formsColorMode);
    napi_set_named_property(env, result, "colorMode", colorMode);

    // defaultFlag
    napi_value defaultFlag;
    napi_create_int32(env, (int32_t)formInfo.defaultFlag, &defaultFlag);
    HILOG_DEBUG("%{public}s, defaultFlag=%{public}d.", __func__, formInfo.defaultFlag);
    napi_set_named_property(env, result, "defaultFlag", defaultFlag);

    // formVisibleNotify
    napi_value formVisibleNotify;
    napi_create_int32(env, (int32_t)formInfo.formVisibleNotify, &formVisibleNotify);
    HILOG_DEBUG("%{public}s, formVisibleNotify=%{public}d.", __func__, formInfo.formVisibleNotify);
    napi_set_named_property(env, result, "formVisibleNotify", formVisibleNotify);

    // formConfigAbility
    napi_value formConfigAbility;
    napi_create_string_utf8(env, formInfo.formConfigAbility.c_str(), NAPI_AUTO_LENGTH, &formConfigAbility);
    HILOG_DEBUG("%{public}s, formConfigAbility=%{public}s.", __func__, formInfo.formConfigAbility.c_str());
    napi_set_named_property(env, result, "formConfigAbility", formConfigAbility);
 
    // updateDuration
    napi_value updateDuration;
    napi_create_int32(env, formInfo.updateDuration, &updateDuration);
    HILOG_DEBUG("%{public}s, updateDuration=%{public}d.", __func__, formInfo.updateDuration);
    napi_set_named_property(env, result, "updateDuration", updateDuration);

    // defaultDimension
    napi_value defaultDimension;
    napi_create_int32(env, formInfo.defaultDimension, &defaultDimension);
    HILOG_DEBUG("%{public}s, defaultDimension=%{public}d.", __func__, formInfo.defaultDimension);
    napi_set_named_property(env, result, "defaultDimension", defaultDimension);

    // supportDimensions
    napi_value supportDimensions;
    napi_create_array(env, &supportDimensions);
    int iDimensionsCount = 0;
    for (auto  dimension : formInfo.supportDimensions) {
        napi_value dimensionInfo = nullptr;
        napi_create_int32(env, dimension, &dimensionInfo);
        napi_set_element(env, supportDimensions, iDimensionsCount, dimensionInfo);
        ++iDimensionsCount;
    }
    //HILOG_DEBUG("%{public}s, supportDimensions size=%{public}d.", __func__, formInfo.supportDimensions.size());
    napi_set_named_property(env, result, "supportDimensions", supportDimensions);

    // customizeDatas
    napi_value customizeDatas;
    napi_create_array(env, &customizeDatas);
    int iCustomizeDatasCount = 0;
    for (auto  customizeData : formInfo.customizeDatas) {

        napi_value customizeDataInfo = nullptr;
        napi_create_array(env, &customizeDataInfo);

        // customizeData : name
        napi_value customizeDataName;
        napi_create_string_utf8(env, customizeData.name.c_str(), NAPI_AUTO_LENGTH, &customizeDataName);
        HILOG_DEBUG("%{public}s, customizeData.name=%{public}s.", __func__, customizeData.name.c_str());
        napi_set_named_property(env, customizeDataInfo, "name", customizeDataName);

        // customizeData : value
        napi_value customizeDataValue;
        napi_create_string_utf8(env, customizeData.value.c_str(), NAPI_AUTO_LENGTH, &customizeDataValue);
        HILOG_DEBUG("%{public}s, customizeData.value=%{public}s.", __func__, customizeData.value.c_str());
        napi_set_named_property(env, customizeDataInfo, "value", customizeDataValue);

        napi_set_element(env, customizeDatas, iCustomizeDatasCount, customizeDataInfo);
        ++iDimensionsCount;
    }
    //HILOG_DEBUG("%{public}s, customizeDatas size=%{public}d.", __func__, formInfo.customizeDatas.size());
    napi_set_named_property(env, result, "supportDimensions", customizeDatas);

    // relatedBundleName
    napi_value relatedBundleName;
    napi_create_string_utf8(env, formInfo.relatedBundleName.c_str(), NAPI_AUTO_LENGTH, &relatedBundleName);
    HILOG_DEBUG("%{public}s, relatedBundleName=%{public}s.", __func__, formInfo.relatedBundleName.c_str());
    napi_set_named_property(env, result, "relatedBundleName", relatedBundleName);

    return;
}

/**
 * @brief  Receive the native kit reply and trigger JavaScript function object to be called from Node-API
 *
 * @param[in] result Receive the result from the native kit 
 * @param[in] formInfo This C++ forminfo will be parsed by JavaScript
 * 
 * @return void
 */
void NapiFormAbility::OnAcquired(const int32_t result, const OHOS::AppExecFwk::FormJsInfo &formJsInfo) const
{
    HILOG_INFO("%{public}s called, OnAcquired result[%{public}d] env[%{public}p]", 
    __func__, result, jsCallbackInfomation.env);

    uv_loop_s *loop = nullptr;

#if NAPI_VERSION >= 2
    HILOG_INFO("%{public}s called, napi_get_uv_event_loop start", __func__);
    napi_get_uv_event_loop(jsCallbackInfomation.env, &loop);
    HILOG_INFO("%{public}s called, napi_get_uv_event_loop end", __func__);
#endif  // NAPI_VERSION >= 2

    AddFormCallbackInfo *addFormCallbackInfo = new 
    (std::nothrow) AddFormCallbackInfo {
        .jsCallbackInfo = jsCallbackInfomation,
        .result = result,
        .formInfo = formJsInfo,
        .asyncWork = nullptr,
    };

    uv_work_t *work = new uv_work_t;
    work->data = (void *)addFormCallbackInfo;
    HILOG_INFO("%{public}s called, start uv_queue_work", __func__);
    uv_queue_work(
        loop,
        work,
        [](uv_work_t *work) {},
        [](uv_work_t *work, int status) {
            HILOG_INFO("%{public}s, uv_queue_work enter", __func__);

            AddFormCallbackInfo *event = (AddFormCallbackInfo *)work->data;
            if(event->result == 0) {
                napi_value result;
                napi_create_object(event->jsCallbackInfo.env, &result);
                ParseFormJsInfoIntoNapi(event->jsCallbackInfo.env, event->formInfo, result);
                napi_value callback = 0;
                if(event->jsCallbackInfo.onAcquired == NULL) {
                    HILOG_ERROR("%{public}s, AcquireForm uv_queue_work onAcquired == NULL", __func__);
                }

                napi_get_reference_value(event->jsCallbackInfo.env, event->jsCallbackInfo.onAcquired, &callback);
                napi_value callResult = 0;
                napi_value undefined = 0;
                napi_get_undefined(event->jsCallbackInfo.env, &undefined);
                napi_call_function(event->jsCallbackInfo.env, undefined, callback, 1, &result, &callResult);
            } else {
                napi_value result;
                napi_create_int32(event->jsCallbackInfo.env, event->result, &result);
                napi_value callback = 0;
                napi_get_reference_value(event->jsCallbackInfo.env, event->jsCallbackInfo.onError, &callback);
                napi_value callResult = 0;
                napi_value undefined = 0;
                napi_get_undefined(event->jsCallbackInfo.env, &undefined);
                napi_call_function(event->jsCallbackInfo.env, undefined, callback, 1, &result, &callResult);
            }

            delete event;
            delete work;
            HILOG_INFO("%{public}s, uv_queue_work end", __func__);
        });

    return;
 
}

/**
 * @brief  Receive the native kit reply and trigger JavaScript function object to be called from Node-API
 *
 * @param[in] result Receive the result from the native kit 
 * @param[in] formInfo This C++ forminfo will be parsed by JavaScript
 * 
 * @return void
 */
void NapiFormAbility::OnUpdate(const int32_t result, const OHOS::AppExecFwk::FormJsInfo &formJsInfo) const
{
    HILOG_INFO("%{public}s called, OnUpdate result[%{public}d] env[%{public}p]", 
    __func__, result, jsCallbackInfomation.env);

    uv_loop_s *loop = nullptr;

#if NAPI_VERSION >= 2
    HILOG_INFO("%{public}s called, napi_get_uv_event_loop start", __func__);
    napi_get_uv_event_loop(jsCallbackInfomation.env, &loop);
    HILOG_INFO("%{public}s called, napi_get_uv_event_loop end", __func__);
#endif  // NAPI_VERSION >= 2

    AddFormCallbackInfo *addFormCallbackInfo = new 
    (std::nothrow) AddFormCallbackInfo {
        .jsCallbackInfo = jsCallbackInfomation,
        .result = result,
        .formInfo = formJsInfo,
        .asyncWork = nullptr,
    };

    uv_work_t *work = new uv_work_t;
    work->data = (void *)addFormCallbackInfo;

    HILOG_INFO("%{public}s called, start uv_queue_work", __func__);

    uv_queue_work(
        loop,
        work,
        [](uv_work_t *work) {},
        [](uv_work_t *work, int status) {
            HILOG_INFO("%{public}s, uv_queue_work enter", __func__);

            AddFormCallbackInfo *event = (AddFormCallbackInfo *)work->data;
            if(event->result == 0) {
                napi_value result;
                napi_create_object(event->jsCallbackInfo.env, &result);
                ParseFormJsInfoIntoNapi(event->jsCallbackInfo.env, event->formInfo, result);
                napi_value callback = 0;
                if(event->jsCallbackInfo.onAcquired == NULL) {
                    HILOG_ERROR("%{public}s, uv_queue_work onAcquired == NULL", __func__);
                }

                napi_get_reference_value(event->jsCallbackInfo.env, event->jsCallbackInfo.onAcquired, &callback);
                napi_value callResult = 0;
                napi_value undefined = 0;
                napi_get_undefined(event->jsCallbackInfo.env, &undefined);
                napi_call_function(event->jsCallbackInfo.env, undefined, callback, 1, &result, &callResult);
            } else {
                napi_value result;
                napi_create_int32(event->jsCallbackInfo.env, event->result, &result);
                napi_value callback = 0;
                napi_get_reference_value(event->jsCallbackInfo.env, event->jsCallbackInfo.onError, &callback);
                napi_value callResult = 0;
                napi_value undefined = 0;
                napi_get_undefined(event->jsCallbackInfo.env, &undefined);
                napi_call_function(event->jsCallbackInfo.env, undefined, callback, 1, &result, &callResult);
            }

            delete event;
            delete work;
            HILOG_INFO("%{public}s called, uv_queue_work end", __func__);
        });

    return;
}

/**
 * @brief  Receive the native kit reply and trigger JavaScript function object to be called from Node-API
 *
 * @param[in] result Receive the uninstall message from the native kit 
 * @param[in] formInfo This C++ forminfo will be parsed by JavaScript
 * 
 * @return void
 */
void NapiFormAbility::OnFormUninstall(const int64_t formId) const
{
}

/**
 * @brief  Call native kit function: AcquireForm
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] asyncCallbackInfo Reference, callback info via Node-API
 * 
 * @return void
 */
static void InnerAcquireForm(napi_env env, AsyncAddFormCallbackInfo* const asyncCallbackInfo)
{
    HILOG_DEBUG("%{public}s called.", __func__);

    // Set Want info begin
    Want want;
    want.SetParam(Constants::PARAM_FORM_DIMENSION_KEY, asyncCallbackInfo->param.formDimension);
    // want.SetParam(Constants::PARAM_FORM_WIDTH_KEY, asyncCallbackInfo->param.formWidth);
    // want.SetParam(Constants::PARAM_FORM_HEIGHT_KEY, asyncCallbackInfo->param.formHeight);
    want.SetParam(Constants::PARAM_FORM_NAME_KEY, asyncCallbackInfo->param.formName);
    want.SetParam(Constants::PARAM_MODULE_NAME_KEY, asyncCallbackInfo->param.moduleName);
    want.SetParam(Constants::PARAM_FORM_TEMPORARY_KEY, asyncCallbackInfo->param.tempFormFlag);
    want.SetElementName(asyncCallbackInfo->param.deviceId, 
    asyncCallbackInfo->param.bundleName, 
    asyncCallbackInfo->param.abilityName);
    // Set Want info end

    std::shared_ptr<OHOS::AppExecFwk::Ability::FormCallback> callbackInfo(asyncCallbackInfo->napiFormAbility);
    OHOS::AppExecFwk::Ability *ability = asyncCallbackInfo->ability;
    bool ret = ability->AcquireForm(asyncCallbackInfo->param.formId, want, callbackInfo);
    if (ret) {
        asyncCallbackInfo->result = 1;
    } else {
        asyncCallbackInfo->result = 0;
    }
}


/**
 * @brief  The implementation of Node-API interface: acquireForm 
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] info An opaque datatype that is passed to a callback function
 * 
 * @return This is an opaque pointer that is used to represent a JavaScript value
 */
napi_value NAPI_AcquireForm(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s called, AcquireForm env[%{public}p]", __func__, env);

    napi_value thisVar = nullptr;
    void* data = nullptr;

    // Check the number of the arguments
    size_t argc = ARGS_SIZE_THREE;
    napi_value argv[ARGS_SIZE_THREE] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, &data));
    if (argc > ARGS_SIZE_THREE) {
        HILOG_ERROR("%{public}s, Wrong argument count.", __func__);
        return nullptr;
    }
    //HILOG_INFO("%{public}s, argc = [%{public}d]", __func__, argc);

    // Parse form info from JavaScript value
    FormReqInfo formReqInfo;
    if (ParseNapiIntoFormInfo(env, formReqInfo, argv[0]) == nullptr) {
        return NapiGetNull(env);
    }

    // Obtains the C++ instance
    NapiFormAbility* napiFormAbility = nullptr;
    napi_status status = napi_unwrap(env, thisVar, (void **)&napiFormAbility);
    if(status == napi_ok) {
        HILOG_DEBUG("%{public}s, status == napi_ok", __func__);
    } else {
        HILOG_DEBUG("%{public}s, status != napi_ok", __func__);
    }
    
    if(napiFormAbility == nullptr) {
        HILOG_DEBUG("%{public}s, napiFormAbility == nullptr", __func__);
    } else {
        HILOG_DEBUG("%{public}s, napiFormAbility != nullptr", __func__);
    }

    NAPI_ASSERT(env, status == napi_ok, "When acquireForm is called,\
    napi_unwrap obtains the C++ instance failed!");

    // get global value
    napi_value global = nullptr;
    napi_get_global(env, &global);
    
    // get ability
    napi_value abilityObj = nullptr;
    napi_get_named_property(env, global, "ability", &abilityObj);

    // get ability pointer
    OHOS::AppExecFwk::Ability* ability = nullptr;
    napi_get_value_external(env, abilityObj, (void**)&ability);
    HILOG_INFO("%{public}s, ability = [%{public}p]", __func__, ability);


    // Parse arguments of the callback function from JavaScript value
    napiFormAbility->jsCallbackInfomation.env = env;
    if (ParseJsObjectCallbackParam(env, napiFormAbility->jsCallbackInfomation, argv[1]) == nullptr) {
        return NapiGetNull(env);
    }

    AsyncAddFormCallbackInfo *asyncCallbackInfo = new 
    AsyncAddFormCallbackInfo {
        .env = env,
        .ability = ability,
        .asyncWork = nullptr,
        .deferred = nullptr,
        .callback = nullptr,
        .param = formReqInfo,
        .napiFormAbility = napiFormAbility,
    };

    if (argc == ARGS_SIZE_THREE) {
        HILOG_INFO("%{public}s, asyncCallback.", __func__);

        // Check the value type of the arguments
        napi_valuetype valueType;
        NAPI_CALL(env, napi_typeof(env, argv[2], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "The arguments[2] type of acquireForm is incorrect,\
        expected type is function.");

        napi_create_reference(env, argv[2], 1, &asyncCallbackInfo->callback);

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, napi_create_async_work running", __func__);
                AsyncAddFormCallbackInfo *asyncCallbackInfo = (AsyncAddFormCallbackInfo *)data;
                InnerAcquireForm(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                AsyncAddFormCallbackInfo *asyncCallbackInfo = (AsyncAddFormCallbackInfo *)data;
                HILOG_INFO("%{public}s, napi_create_async_work complete", __func__);

                if (asyncCallbackInfo->callback != nullptr) {
                    napi_value result;
                    napi_create_int32(env, asyncCallbackInfo->result, &result);
                    napi_value callback;
                    napi_value undefined;
                    napi_get_undefined(env, &undefined);
                    napi_get_reference_value(env, asyncCallbackInfo->callback, &callback);
                    napi_value callResult;
                    napi_call_function(env, undefined, callback, 1, &result, &callResult);
                    napi_delete_reference(env, asyncCallbackInfo->callback);
                }
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);

        napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
 
        napi_value result = 0;
        napi_get_null(env, &result);
        return result;
    } else {
        HILOG_INFO("%{public}s, promise.", __func__);
        napi_deferred deferred;
        napi_value promise;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, promise runnning", __func__);
                AsyncAddFormCallbackInfo *asyncCallbackInfo = (AsyncAddFormCallbackInfo *)data;
                InnerAcquireForm(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("%{public}s, promise complete", __func__);
                AsyncAddFormCallbackInfo *asyncCallbackInfo = (AsyncAddFormCallbackInfo *)data;
                napi_value result;
                napi_create_int32(env, asyncCallbackInfo->result, &result);
                napi_resolve_deferred(asyncCallbackInfo->env, asyncCallbackInfo->deferred, result);
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
        return promise;
    }
}

/**
 * @brief  Call native kit function: DeleteForm
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] asyncCallbackInfo Reference, callback info via Node-API
 * 
 * @return void
 */
static void InnerDelForm(napi_env env, AsyncDelFormCallbackInfo* const asyncCallbackInfo)
{
    HILOG_DEBUG("%{public}s called.", __func__);
    OHOS::AppExecFwk::Ability *ability = asyncCallbackInfo->ability;
    bool ret = ability->DeleteForm(asyncCallbackInfo->formId);
    if (ret) {
        asyncCallbackInfo->result = 1;
    } else {
        asyncCallbackInfo->result = 0;
    }
    HILOG_DEBUG("%{public}s, end", __func__);
}

/**
 * @brief  The implementation of Node-API interface: deleteForm 
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] info An opaque datatype that is passed to a callback function
 * 
 * @return This is an opaque pointer that is used to represent a JavaScript value
 */
napi_value NAPI_DeleteForm(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s called.", __func__);

    // Check the number of the arguments
    size_t argc = ARGS_SIZE_TWO;
    napi_value argv[ARGS_SIZE_TWO] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc > ARGS_SIZE_TWO) {
        HILOG_ERROR("%{public}s, wrong number of arguments.", __func__);
        return nullptr;
    }
    //HILOG_INFO("%{public}s, argc = [%{public}d]", __func__, argc);

    // get global value
    napi_value global = nullptr;
    napi_get_global(env, &global);
    
    // get ability
    napi_value abilityObj = nullptr;
    napi_get_named_property(env, global, "ability", &abilityObj);

    // get ability pointer
    OHOS::AppExecFwk::Ability* ability = nullptr;
    napi_get_value_external(env, abilityObj, (void**)&ability);
    HILOG_INFO("%{public}s, ability = [%{public}p]", __func__, ability);

    // Check the value type of the arguments
    napi_valuetype valueType;
    NAPI_CALL(env, napi_typeof(env, argv[0], &valueType));
    NAPI_ASSERT(env, valueType == napi_string, "The arguments[0] type of deleteForm is incorrect,\
    expected type is string.");

    std::string strFormId = GetStringFromNAPI(env, argv[0]);
    int64_t formId = std::stoll(strFormId);

    AsyncDelFormCallbackInfo *asyncCallbackInfo = new 
    AsyncDelFormCallbackInfo {
        .env = env,
        .ability = ability,
        .asyncWork = nullptr,
        .deferred = nullptr,
        .callback = nullptr,
        .formId = 0,
        .result = 0,
    };
    asyncCallbackInfo->formId = formId;

    if (argc == ARGS_SIZE_TWO) {
        HILOG_INFO("%{public}s, asyncCallback.", __func__);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[1], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "The arguments[1] type of deleteForm is incorrect,\
        expected type is function.");

        napi_create_reference(env, argv[1], 1, &asyncCallbackInfo->callback);

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, napi_create_async_work running", __func__);
                AsyncDelFormCallbackInfo *asyncCallbackInfo = (AsyncDelFormCallbackInfo *)data;
                InnerDelForm(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                AsyncDelFormCallbackInfo *asyncCallbackInfo = (AsyncDelFormCallbackInfo *)data;
                HILOG_INFO("%{public}s, napi_create_async_work complete", __func__);

                if (asyncCallbackInfo->callback != nullptr) {
                    napi_value result;
                    napi_create_int32(env, asyncCallbackInfo->result, &result);
                    napi_value callback;
                    napi_value undefined;
                    napi_get_undefined(env, &undefined);
                    napi_get_reference_value(env, asyncCallbackInfo->callback, &callback);
                    napi_value callResult;
                    napi_call_function(env, undefined, callback, 1, &result, &callResult);
                    napi_delete_reference(env, asyncCallbackInfo->callback);
                }
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        NAPI_CALL(env, napi_queue_async_work(env, asyncCallbackInfo->asyncWork));
 
        napi_value result;
        NAPI_CALL(env, napi_create_int32(env, 1, &result));
        return result;
    } else {
        HILOG_INFO("%{public}s, promise.", __func__);
        napi_deferred deferred;
        napi_value promise;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, promise runnning", __func__);
                AsyncDelFormCallbackInfo *asyncCallbackInfo = (AsyncDelFormCallbackInfo *)data;
                InnerDelForm(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("%{public}s, promise complete", __func__);
                AsyncDelFormCallbackInfo *asyncCallbackInfo = (AsyncDelFormCallbackInfo *)data;

                napi_value result;
                napi_create_int32(env, asyncCallbackInfo->result, &result);
                napi_resolve_deferred(asyncCallbackInfo->env, asyncCallbackInfo->deferred, result);
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
        return promise;
    }
}

/**
 * @brief  Call native kit function: ReleaseForm
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] asyncCallbackInfo Reference, callback info via Node-API
 * 
 * @return void
 */
static void InnerReleaseForm(napi_env env, AsyncReleaseFormCallbackInfo* const asyncCallbackInfo)
{
    HILOG_DEBUG("%{public}s called.", __func__);
    OHOS::AppExecFwk::Ability *ability = asyncCallbackInfo->ability;
    bool ret = ability->ReleaseForm(asyncCallbackInfo->formId, asyncCallbackInfo->isReleaseCache);
    if (ret) {
        asyncCallbackInfo->result = 1;
    } else {
        asyncCallbackInfo->result = 0;
    }
    HILOG_DEBUG("%{public}s end", __func__);
}

/**
 * @brief  The implementation of Node-API interface: releaseForm 
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] info An opaque datatype that is passed to a callback function
 * 
 * @return This is an opaque pointer that is used to represent a JavaScript value
 */
napi_value NAPI_ReleaseForm(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s called.", __func__);

    // Check the number of the arguments
    size_t argc = ARGS_SIZE_THREE;
    napi_value argv[ARGS_SIZE_THREE] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc < ARGS_SIZE_TWO || argc > ARGS_SIZE_THREE ) {
        HILOG_ERROR("%{public}s, wrong number of arguments.", __func__);
        return nullptr;
    }
    //HILOG_INFO("%{public}s, argc = [%{public}d]", __func__, argc);

    // Check the value type of the arguments
    napi_valuetype valueType;
    NAPI_CALL(env, napi_typeof(env, argv[0], &valueType));
    NAPI_ASSERT(env, valueType == napi_string, "The arguments[0] type of releaseForm is incorrect,\
    expected type is string.");

    std::string strFormId = GetStringFromNAPI(env, argv[0]);
    int64_t formId = std::stoll(strFormId);

    valueType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[1], &valueType));
    NAPI_ASSERT(env, valueType == napi_boolean, "The arguments[1] type of releaseForm is incorrect,\
    expected type is boolean.");

    bool isReleaseCache;
    napi_get_value_bool(env, argv[1], &isReleaseCache);

    // get global value
    napi_value global = nullptr;
    napi_get_global(env, &global);
    
    // get ability
    napi_value abilityObj = nullptr;
    napi_get_named_property(env, global, "ability", &abilityObj);

    // get ability pointer
    OHOS::AppExecFwk::Ability* ability = nullptr;
    napi_get_value_external(env, abilityObj, (void**)&ability);
    HILOG_INFO("%{public}s, ability = [%{public}p]", __func__, ability);

    AsyncReleaseFormCallbackInfo *asyncCallbackInfo = new 
    AsyncReleaseFormCallbackInfo {
        .env = env,
        .ability = ability,
        .asyncWork = nullptr,
        .deferred = nullptr,
        .callback = nullptr,
        .formId = formId,
        .isReleaseCache = isReleaseCache,
        .result = 0,
    };
    
    if (argc == ARGS_SIZE_THREE) {
        HILOG_INFO("%{public}s, asyncCallback.", __func__);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[2], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "The arguments[2] type of releaseForm is incorrect,\
        expected type is function.");

        napi_create_reference(env, argv[2], 1, &asyncCallbackInfo->callback);

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, napi_create_async_work running", __func__);
                AsyncReleaseFormCallbackInfo *asyncCallbackInfo = (AsyncReleaseFormCallbackInfo *)data;
                InnerReleaseForm(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                AsyncReleaseFormCallbackInfo *asyncCallbackInfo = (AsyncReleaseFormCallbackInfo *)data;
                HILOG_INFO("%{public}s, napi_create_async_work complete", __func__);

                if (asyncCallbackInfo->callback != nullptr) {
                    napi_value result;
                    napi_create_int32(env, asyncCallbackInfo->result, &result);
                    napi_value callback;
                    napi_value undefined;
                    napi_get_undefined(env, &undefined);
                    napi_get_reference_value(env, asyncCallbackInfo->callback, &callback);
                    napi_value callResult;
                    napi_call_function(env, undefined, callback, 1, &result, &callResult);
                    napi_delete_reference(env, asyncCallbackInfo->callback);
                }
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        NAPI_CALL(env, napi_queue_async_work(env, asyncCallbackInfo->asyncWork));

        napi_value result;
        NAPI_CALL(env, napi_create_int32(env, 1, &result));
        return result;
    } else {
        HILOG_INFO("%{public}s, promise.", __func__);
        napi_deferred deferred;
        napi_value promise;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, promise running,", __func__);
                AsyncReleaseFormCallbackInfo *asyncCallbackInfo = (AsyncReleaseFormCallbackInfo *)data;
                InnerReleaseForm(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("%{public}s, promise complete", __func__);
                AsyncReleaseFormCallbackInfo *asyncCallbackInfo = (AsyncReleaseFormCallbackInfo *)data;

                napi_value result;
                napi_create_int32(env, asyncCallbackInfo->result, &result);
                napi_resolve_deferred(asyncCallbackInfo->env, asyncCallbackInfo->deferred, result);
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
        return promise;
    }
}

/**
 * @brief  Call native kit function: RequestForm
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] asyncCallbackInfo Reference, callback info via Node-API
 * 
 * @return void
 */
static void InnerRequestForm(napi_env env, AsyncRequestFormCallbackInfo* const asyncCallbackInfo)
{
    HILOG_DEBUG("%{public}s called.", __func__);
    OHOS::AppExecFwk::Ability *ability = asyncCallbackInfo->ability;
    bool ret = ability->RequestForm(asyncCallbackInfo->formId);
    if (ret) {
        asyncCallbackInfo->result = 1;
    } else {
        asyncCallbackInfo->result = 0;
    }
    HILOG_DEBUG("%{public}s, end", __func__);
}

/**
 * @brief  The implementation of Node-API interface: requestForm 
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] info An opaque datatype that is passed to a callback function
 * 
 * @return This is an opaque pointer that is used to represent a JavaScript value
 */
napi_value NAPI_RequestForm(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s called.", __func__);

    // Check the number of the arguments
    size_t argc = ARGS_SIZE_TWO;
    napi_value argv[ARGS_SIZE_TWO] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc > ARGS_SIZE_TWO) {
        HILOG_ERROR("%{public}s, wrong number of arguments.", __func__);
        return nullptr;
    }
    //HILOG_INFO("%{public}s, argc = [%{public}d]", __func__, argc);

    // Check the value type of the arguments
    napi_valuetype valueType;
    NAPI_CALL(env, napi_typeof(env, argv[0], &valueType));
    NAPI_ASSERT(env, valueType == napi_string, "The arguments[0] type of requestForm is incorrect,\
    expected type is string.");

    std::string strFormId = GetStringFromNAPI(env, argv[0]);
    int64_t formId = std::stoll(strFormId);

    // get global value
    napi_value global = nullptr;
    napi_get_global(env, &global);
    
    // get ability
    napi_value abilityObj = nullptr;
    napi_get_named_property(env, global, "ability", &abilityObj);

    // get ability pointer
    OHOS::AppExecFwk::Ability* ability = nullptr;
    napi_get_value_external(env, abilityObj, (void**)&ability);
    HILOG_INFO("%{public}s, ability = [%{public}p]", __func__, ability);

    AsyncRequestFormCallbackInfo *asyncCallbackInfo = new 
    AsyncRequestFormCallbackInfo {
        .env = env,
        .ability = ability,
        .asyncWork = nullptr,
        .deferred = nullptr,
        .callback = nullptr,
        .formId = formId,
        .result = 0,
    };

    if (argc == ARGS_SIZE_TWO) {
        HILOG_INFO("%{public}s, asyncCallback.", __func__);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[1], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "The arguments[1] type of requestForm is incorrect,\
        expected type is function.");

        napi_create_reference(env, argv[1], 1, &asyncCallbackInfo->callback);

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, napi_create_async_work running", __func__);
                AsyncRequestFormCallbackInfo *asyncCallbackInfo = (AsyncRequestFormCallbackInfo *)data;
                InnerRequestForm(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                AsyncRequestFormCallbackInfo *asyncCallbackInfo = (AsyncRequestFormCallbackInfo *)data;
                HILOG_INFO("%{public}s, napi_create_async_work complete", __func__);

                if (asyncCallbackInfo->callback != nullptr) {
                    napi_value result;
                    napi_create_int32(env, asyncCallbackInfo->result, &result);
                    napi_value callback;
                    napi_value undefined;
                    napi_get_undefined(env, &undefined);
                    napi_get_reference_value(env, asyncCallbackInfo->callback, &callback);
                    napi_value callResult;
                    napi_call_function(env, undefined, callback, 1, &result, &callResult);
                    napi_delete_reference(env, asyncCallbackInfo->callback);
                }
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        NAPI_CALL(env, napi_queue_async_work(env, asyncCallbackInfo->asyncWork));
 
        napi_value result;
        NAPI_CALL(env, napi_create_int32(env, 1, &result));
        return result;
    } else {
        HILOG_INFO("%{public}s, promise.", __func__);
        napi_deferred deferred;
        napi_value promise;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, promise runnning", __func__);
                AsyncRequestFormCallbackInfo *asyncCallbackInfo = (AsyncRequestFormCallbackInfo *)data;
                InnerRequestForm(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("%{public}s, promise complete", __func__);
                AsyncRequestFormCallbackInfo *asyncCallbackInfo = (AsyncRequestFormCallbackInfo *)data;

                napi_value result;
                napi_create_int32(env, asyncCallbackInfo->result, &result);
                napi_resolve_deferred(asyncCallbackInfo->env, asyncCallbackInfo->deferred, result);
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
        return promise;
    }
}

/**
 * @brief  Call native kit function: SetFormNextRefreshTime
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] asyncCallbackInfo Reference, callback info via Node-API
 * 
 * @return void
 */
static void InnerSetFormNextRefreshTime(napi_env env, AsyncNextRefreshTimeFormCallbackInfo* const asyncCallbackInfo)
{
    HILOG_DEBUG("%{public}s called.", __func__);
    OHOS::AppExecFwk::Ability *ability = asyncCallbackInfo->ability;
    bool ret = ability->SetFormNextRefreshTime(asyncCallbackInfo->formId, asyncCallbackInfo->time);
    if (ret) {
        asyncCallbackInfo->result = 1;
    } else {
        asyncCallbackInfo->result = 0;
    }
    HILOG_DEBUG("%{public}s, end", __func__);
}

/**
 * @brief  The implementation of Node-API interface: setFormNextRefreshTime 
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] info An opaque datatype that is passed to a callback function
 * 
 * @return This is an opaque pointer that is used to represent a JavaScript value
 */
napi_value NAPI_SetFormNextRefreshTime(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s called.", __func__);

    // Check the number of the arguments
    size_t argc = ARGS_SIZE_THREE;
    napi_value argv[ARGS_SIZE_THREE] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc > ARGS_SIZE_THREE || argc < ARGS_SIZE_TWO) {
        HILOG_ERROR("%{public}s, wrong number of arguments.", __func__);
        return nullptr;
    }
    //HILOG_INFO("%{public}s, argc = [%{public}d]", __func__, argc);

    // Check the value type of the arguments
    napi_valuetype valueType;
    NAPI_CALL(env, napi_typeof(env, argv[0], &valueType));
    NAPI_ASSERT(env, valueType == napi_string, "The arguments[0] type of setFormNextRefreshTime is incorrect,\
    expected type is string.");

    std::string strFormId = GetStringFromNAPI(env, argv[0]);
    int64_t formId = std::stoll(strFormId);

    valueType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[1], &valueType));
    NAPI_ASSERT(env, valueType == napi_number, "The arguments[1] type of setFormNextRefreshTime is incorrect,\
    expected type is number.");

    int32_t time;
    napi_get_value_int32(env, argv[1], &time);

    // get global value
    napi_value global = nullptr;
    napi_get_global(env, &global);
    
    // get ability
    napi_value abilityObj = nullptr;
    napi_get_named_property(env, global, "ability", &abilityObj);

    // get ability pointer
    OHOS::AppExecFwk::Ability* ability = nullptr;
    napi_get_value_external(env, abilityObj, (void**)&ability);
    HILOG_INFO("%{public}s, ability = [%{public}p]", __func__, ability);

    AsyncNextRefreshTimeFormCallbackInfo *asyncCallbackInfo = new 
    AsyncNextRefreshTimeFormCallbackInfo {
        .env = env,
        .ability = ability,
        .asyncWork = nullptr,
        .deferred = nullptr,
        .callback = nullptr,
        .formId = formId,
        .time = time,
        .result = 0,
    };

    if (argc == ARGS_SIZE_THREE) {
        HILOG_INFO("%{public}s, asyncCallback.", __func__);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[2], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "The arguments[2] type of setFormNextRefreshTime is incorrect,\
        expected type is function.");

        napi_create_reference(env, argv[1], 1, &asyncCallbackInfo->callback);

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, napi_create_async_work running", __func__);

                AsyncNextRefreshTimeFormCallbackInfo *asyncCallbackInfo = 
                (AsyncNextRefreshTimeFormCallbackInfo *)data;

                InnerSetFormNextRefreshTime(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                AsyncNextRefreshTimeFormCallbackInfo *asyncCallbackInfo = 
                (AsyncNextRefreshTimeFormCallbackInfo *)data;

                HILOG_INFO("%{public}s, napi_create_async_work complete", __func__);

                if (asyncCallbackInfo->callback != nullptr) {
                    napi_value result;
                    napi_create_int32(env, asyncCallbackInfo->result, &result);
                    napi_value callback;
                    napi_value undefined;
                    napi_get_undefined(env, &undefined);
                    napi_get_reference_value(env, asyncCallbackInfo->callback, &callback);
                    napi_value callResult;
                    napi_call_function(env, undefined, callback, 1, &result, &callResult);
                    napi_delete_reference(env, asyncCallbackInfo->callback);
                }
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        NAPI_CALL(env, napi_queue_async_work(env, asyncCallbackInfo->asyncWork));
 
        napi_value result;
        NAPI_CALL(env, napi_create_int32(env, 1, &result));
        return result;
    } else {
        HILOG_INFO("%{public}s, promise.", __func__);
        napi_deferred deferred;
        napi_value promise;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, promise runnning", __func__);
                AsyncNextRefreshTimeFormCallbackInfo *asyncCallbackInfo = 
                (AsyncNextRefreshTimeFormCallbackInfo *)data;

                InnerSetFormNextRefreshTime(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("%{public}s, promise complete", __func__);
                AsyncNextRefreshTimeFormCallbackInfo *asyncCallbackInfo = 
                (AsyncNextRefreshTimeFormCallbackInfo *)data;

                napi_value result;
                napi_create_int32(env, asyncCallbackInfo->result, &result);
                napi_resolve_deferred(asyncCallbackInfo->env, asyncCallbackInfo->deferred, result);
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
        return promise;
    }
}

/**
 * @brief  Call native kit function: UpdateForm
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] asyncCallbackInfo Reference, callback info via Node-API
 * 
 * @return void
 */
static void InnerUpdateForm(napi_env env, AsyncUpdateFormCallbackInfo* const asyncCallbackInfo)
{
    HILOG_DEBUG("%{public}s called.", __func__);
    OHOS::AppExecFwk::Ability *ability = asyncCallbackInfo->ability;
    bool ret = ability->UpdateForm(asyncCallbackInfo->formId, *asyncCallbackInfo->formProviderData);
    if (ret) {
        asyncCallbackInfo->result = 1;
    } else {
        asyncCallbackInfo->result = 0;
    }
    HILOG_DEBUG("%{public}s, end", __func__);
}

/**
 * @brief  The implementation of Node-API interface: updateForm 
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] info An opaque datatype that is passed to a callback function
 * 
 * @return This is an opaque pointer that is used to represent a JavaScript value
 */
napi_value NAPI_UpdateForm(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s called.", __func__);

    // Check the number of the arguments
    size_t argc = ARGS_SIZE_THREE;
    napi_value argv[ARGS_SIZE_THREE] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc > ARGS_SIZE_THREE || argc < ARGS_SIZE_TWO) {
        HILOG_ERROR("%{public}s, wrong number of arguments.", __func__);
        return nullptr;
    }
    //HILOG_INFO("%{public}s, argc = [%{public}d]", __func__, argc);

    // Check the value type of the arguments
    napi_valuetype valueType;
    NAPI_CALL(env, napi_typeof(env, argv[0], &valueType));
    NAPI_ASSERT(env, valueType == napi_string, "The arguments[0] type of updateForm is incorrect,\
    expected type is string.");

    std::string strFormId = GetStringFromNAPI(env, argv[0]);
    int64_t formId = std::stoll(strFormId);

    NAPI_CALL(env, napi_typeof(env, argv[1], &valueType));
    NAPI_ASSERT(env, valueType == napi_string, "The arguments[1] type of updateForm is incorrect,\
    expected type is string.");

    OHOS::AppExecFwk::FormProviderData *formProviderData = nullptr;
    napi_unwrap(env, argv[1], (void**)&formProviderData);

    // get global value
    napi_value global = nullptr;
    napi_get_global(env, &global);
    
    // get ability
    napi_value abilityObj = nullptr;
    napi_get_named_property(env, global, "ability", &abilityObj);

    // get ability pointer
    OHOS::AppExecFwk::Ability* ability = nullptr;
    napi_get_value_external(env, abilityObj, (void**)&ability);
    HILOG_INFO("%{public}s, ability = [%{public}p]", __func__, ability);

    AsyncUpdateFormCallbackInfo *asyncCallbackInfo = new 
    AsyncUpdateFormCallbackInfo {
        .env = env,
        .ability = ability,
        .asyncWork = nullptr,
        .deferred = nullptr,
        .callback = nullptr,
        .formId = formId,
        .formProviderData = std::shared_ptr<OHOS::AppExecFwk::FormProviderData>(formProviderData),
        .result = 0,
    };

    if (argc == ARGS_SIZE_THREE) {
        HILOG_INFO("%{public}s, asyncCallback.", __func__);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[2], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "The arguments[2] type of updateForm is incorrect,\
        expected type is function.");

        napi_create_reference(env, argv[1], 1, &asyncCallbackInfo->callback);

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, napi_create_async_work running", __func__);
                AsyncUpdateFormCallbackInfo *asyncCallbackInfo = (AsyncUpdateFormCallbackInfo *)data;
                InnerUpdateForm(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                AsyncUpdateFormCallbackInfo *asyncCallbackInfo = (AsyncUpdateFormCallbackInfo *)data;
                HILOG_INFO("%{public}s, napi_create_async_work complete", __func__);

                if (asyncCallbackInfo->callback != nullptr) {
                    napi_value result;
                    napi_create_int32(env, asyncCallbackInfo->result, &result);
                    napi_value callback;
                    napi_value undefined;
                    napi_get_undefined(env, &undefined);
                    napi_get_reference_value(env, asyncCallbackInfo->callback, &callback);
                    napi_value callResult;
                    napi_call_function(env, undefined, callback, 1, &result, &callResult);
                    napi_delete_reference(env, asyncCallbackInfo->callback);
                }
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        NAPI_CALL(env, napi_queue_async_work(env, asyncCallbackInfo->asyncWork));
 
        napi_value result;
        NAPI_CALL(env, napi_create_int32(env, 1, &result));
        return result;
    } else {
        HILOG_INFO("%{public}s, promise.", __func__);
        napi_deferred deferred;
        napi_value promise;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, promise runnning", __func__);
                AsyncUpdateFormCallbackInfo *asyncCallbackInfo = (AsyncUpdateFormCallbackInfo *)data;
                InnerUpdateForm(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("%{public}s, promise complete", __func__);
                AsyncUpdateFormCallbackInfo *asyncCallbackInfo = (AsyncUpdateFormCallbackInfo *)data;

                napi_value result;
                napi_create_int32(env, asyncCallbackInfo->result, &result);
                napi_resolve_deferred(asyncCallbackInfo->env, asyncCallbackInfo->deferred, result);
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
        return promise;
    }
}

/**
 * @brief  Call native kit function: CastTempForm
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] asyncCallbackInfo Reference, callback info via Node-API
 * 
 * @return void
 */
static void InnerCastTempForm(napi_env env, AsyncCastTempFormCallbackInfo* const asyncCallbackInfo)
{
    HILOG_DEBUG("%{public}s called.", __func__);
    OHOS::AppExecFwk::Ability *ability = asyncCallbackInfo->ability;
    bool ret = ability->CastTempForm(asyncCallbackInfo->formId);
    if (ret) {
        asyncCallbackInfo->result = 1;
    } else {
        asyncCallbackInfo->result = 0;
    }
    HILOG_DEBUG("%{public}s, end", __func__);
}

/**
 * @brief  The implementation of Node-API interface: castTempForm 
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] info An opaque datatype that is passed to a callback function
 * 
 * @return This is an opaque pointer that is used to represent a JavaScript value
 */
napi_value NAPI_CastTempForm(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s called.", __func__);

    // Check the number of the arguments
    size_t argc = ARGS_SIZE_TWO;
    napi_value argv[ARGS_SIZE_TWO] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc > ARGS_SIZE_TWO) {
        HILOG_ERROR("%{public}s, wrong number of arguments.", __func__);
        return nullptr;
    }
    //HILOG_INFO("%{public}s, argc = [%{public}d]", __func__, argc);

    // Check the value type of the arguments
    napi_valuetype valueType;
    NAPI_CALL(env, napi_typeof(env, argv[0], &valueType));
    NAPI_ASSERT(env, valueType == napi_string, "The arguments[0] type of castTempForm is incorrect,\
    expected type is string.");

    std::string strFormId = GetStringFromNAPI(env, argv[0]);
    int64_t formId = std::stoll(strFormId);

    // get global value
    napi_value global = nullptr;
    napi_get_global(env, &global);
    
    // get ability
    napi_value abilityObj = nullptr;
    napi_get_named_property(env, global, "ability", &abilityObj);

    // get ability pointer
    OHOS::AppExecFwk::Ability* ability = nullptr;
    napi_get_value_external(env, abilityObj, (void**)&ability);
    HILOG_INFO("%{public}s, ability = [%{public}p]", __func__, ability);

    AsyncCastTempFormCallbackInfo *asyncCallbackInfo = new 
    AsyncCastTempFormCallbackInfo {
        .env = env,
        .ability = ability,
        .asyncWork = nullptr,
        .deferred = nullptr,
        .callback = nullptr,
        .formId = formId,
        .result = 0,
    };

    if (argc == ARGS_SIZE_TWO) {
        HILOG_INFO("%{public}s, asyncCallback.", __func__);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[1], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "The arguments[1] type of castTempForm is incorrect,\
        expected type is function.");

        napi_create_reference(env, argv[1], 1, &asyncCallbackInfo->callback);

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, napi_create_async_work running", __func__);
                AsyncCastTempFormCallbackInfo *asyncCallbackInfo = (AsyncCastTempFormCallbackInfo *)data;
                InnerCastTempForm(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                AsyncCastTempFormCallbackInfo *asyncCallbackInfo = (AsyncCastTempFormCallbackInfo *)data;
                HILOG_INFO("%{public}s, napi_create_async_work complete", __func__);

                if (asyncCallbackInfo->callback != nullptr) {
                    napi_value result;
                    napi_create_int32(env, asyncCallbackInfo->result, &result);
                    napi_value callback;
                    napi_value undefined;
                    napi_get_undefined(env, &undefined);
                    napi_get_reference_value(env, asyncCallbackInfo->callback, &callback);
                    napi_value callResult;
                    napi_call_function(env, undefined, callback, 1, &result, &callResult);
                    napi_delete_reference(env, asyncCallbackInfo->callback);
                }
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        NAPI_CALL(env, napi_queue_async_work(env, asyncCallbackInfo->asyncWork));
 
        napi_value result;
        NAPI_CALL(env, napi_create_int32(env, 1, &result));
        return result;
    } else {
        HILOG_INFO("%{public}s, promise.", __func__);
        napi_deferred deferred;
        napi_value promise;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, promise runnning", __func__);
                AsyncCastTempFormCallbackInfo *asyncCallbackInfo = (AsyncCastTempFormCallbackInfo *)data;
                InnerCastTempForm(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("%{public}s, promise complete", __func__);
                AsyncCastTempFormCallbackInfo *asyncCallbackInfo = (AsyncCastTempFormCallbackInfo *)data;

                napi_value result;
                napi_create_int32(env, asyncCallbackInfo->result, &result);
                napi_resolve_deferred(asyncCallbackInfo->env, asyncCallbackInfo->deferred, result);
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
        return promise;
    }
}

/**
 * @brief  Call native kit function: NotifyVisibleForms
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] asyncCallbackInfo Reference, callback info via Node-API
 * 
 * @return void
 */
static void InnerNotifyVisibleForms(napi_env env, AsyncNotifyVisibleFormsCallbackInfo* const asyncCallbackInfo)
{
    HILOG_DEBUG("%{public}s called.", __func__);
    OHOS::AppExecFwk::Ability *ability = asyncCallbackInfo->ability;
    ability->NotifyVisibleForms(asyncCallbackInfo->formIds);
    HILOG_DEBUG("%{public}s, end", __func__);
}

/**
 * @brief  The implementation of Node-API interface: notifyVisibleForms 
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] info An opaque datatype that is passed to a callback function
 * 
 * @return This is an opaque pointer that is used to represent a JavaScript value
 */
napi_value NAPI_NotifyVisibleForms(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s called.", __func__);

    // Check the number of the arguments
    size_t argc = ARGS_SIZE_TWO;
    napi_value argv[ARGS_SIZE_TWO] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc > ARGS_SIZE_TWO) {
        HILOG_ERROR("%{public}s, wrong number of arguments.", __func__);
        return nullptr;
    }
    //HILOG_INFO("%{public}s, argc = [%{public}d]", __func__, argc);

    uint32_t arrayLength = 0;
    NAPI_CALL(env, napi_get_array_length(env, argv[0], &arrayLength));
    NAPI_ASSERT(env, arrayLength > 0, "The arguments[0] value of notifyVisibleForms is incorrect,\
    this array is empty.");

    std::vector<int64_t> formIds;
    formIds.clear();
    napi_valuetype valueType;
    for (size_t i = 0; i < arrayLength; i++) {
        napi_value napiFormId;
        napi_get_element(env, argv[0], i, &napiFormId);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, napiFormId, &valueType));
        NAPI_ASSERT(env, valueType == napi_string, "The arguments[0] value type of notifyVisibleForms is incorrect,\
        expected type is string.");

        std::string strFormId = GetStringFromNAPI(env, napiFormId);
        int64_t formIdValue = std::stoll(strFormId);

        formIds.push_back(formIdValue);
    }

    // get global value
    napi_value global = nullptr;
    napi_get_global(env, &global);
    
    // get ability
    napi_value abilityObj = nullptr;
    napi_get_named_property(env, global, "ability", &abilityObj);

    // get ability pointer
    OHOS::AppExecFwk::Ability* ability = nullptr;
    napi_get_value_external(env, abilityObj, (void**)&ability);
    HILOG_INFO("%{public}s, ability = [%{public}p]", __func__, ability);

    AsyncNotifyVisibleFormsCallbackInfo *asyncCallbackInfo = new 
    AsyncNotifyVisibleFormsCallbackInfo {
        .env = env,
        .ability = ability,
        .asyncWork = nullptr,
        .deferred = nullptr,
        .callback = nullptr,
        .formIds = formIds,
        .result = 1,
    };

    if (argc == ARGS_SIZE_TWO) {
        HILOG_INFO("%{public}s, asyncCallback.", __func__);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[1], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "The arguments[1] type of notifyVisibleForms is incorrect,\
        expected type is function.");

        napi_create_reference(env, argv[1], 1, &asyncCallbackInfo->callback);

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, napi_create_async_work running", __func__);

                AsyncNotifyVisibleFormsCallbackInfo *asyncCallbackInfo = 
                (AsyncNotifyVisibleFormsCallbackInfo *)data;

                InnerNotifyVisibleForms(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                AsyncNotifyVisibleFormsCallbackInfo *asyncCallbackInfo = 
                (AsyncNotifyVisibleFormsCallbackInfo *)data;

                HILOG_INFO("%{public}s, napi_create_async_work complete", __func__);

                if (asyncCallbackInfo->callback != nullptr) {
                    napi_value result;
                    napi_create_int32(env, asyncCallbackInfo->result, &result);
                    napi_value callback;
                    napi_value undefined;
                    napi_get_undefined(env, &undefined);
                    napi_get_reference_value(env, asyncCallbackInfo->callback, &callback);
                    napi_value callResult;
                    napi_call_function(env, undefined, callback, 1, &result, &callResult);
                    napi_delete_reference(env, asyncCallbackInfo->callback);
                }
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        NAPI_CALL(env, napi_queue_async_work(env, asyncCallbackInfo->asyncWork));
 
        napi_value result;
        NAPI_CALL(env, napi_create_int32(env, 1, &result));
        return result;
    } else {
        HILOG_INFO("%{public}s, promise.", __func__);
        napi_deferred deferred;
        napi_value promise;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, promise runnning", __func__);

                AsyncNotifyVisibleFormsCallbackInfo *asyncCallbackInfo = 
                (AsyncNotifyVisibleFormsCallbackInfo *)data;

                InnerNotifyVisibleForms(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("%{public}s, promise complete", __func__);

                AsyncNotifyVisibleFormsCallbackInfo *asyncCallbackInfo = 
                (AsyncNotifyVisibleFormsCallbackInfo *)data;

                napi_value result;
                napi_create_int32(env, asyncCallbackInfo->result, &result);
                napi_resolve_deferred(asyncCallbackInfo->env, asyncCallbackInfo->deferred, result);
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
        return promise;
    }    
}

/**
 * @brief  Call native kit function: NotifyInvisibleForms
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] asyncCallbackInfo Reference, callback info via Node-API
 * 
 * @return void
 */
static void InnerNotifyInvisibleForms(napi_env env, AsyncNotifyInvisibleFormsCallbackInfo* const asyncCallbackInfo)
{
    HILOG_DEBUG("%{public}s called.", __func__);
    OHOS::AppExecFwk::Ability *ability = asyncCallbackInfo->ability;
    ability->NotifyInvisibleForms(asyncCallbackInfo->formIds);
    HILOG_DEBUG("%{public}s, end", __func__);
}

/**
 * @brief  The implementation of Node-API interface: notifyInvisibleForms 
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] info An opaque datatype that is passed to a callback function
 * 
 * @return This is an opaque pointer that is used to represent a JavaScript value
 */
napi_value NAPI_NotifyInvisibleForms(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s called.", __func__);

    // Check the number of the arguments
    size_t argc = ARGS_SIZE_TWO;
    napi_value argv[ARGS_SIZE_TWO] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc > ARGS_SIZE_TWO) {
        HILOG_ERROR("%{public}s, wrong number of arguments.", __func__);
        return nullptr;
    }
    //HILOG_INFO("%{public}s, argc = [%{public}d]", __func__, argc);

    uint32_t arrayLength = 0;
    NAPI_CALL(env, napi_get_array_length(env, argv[0], &arrayLength));
    NAPI_ASSERT(env, arrayLength > 0, "The arguments[0] value of notifyInvisibleForms is incorrect,\
    this array is empty.");

    std::vector<int64_t> formIds;
    formIds.clear();
    napi_valuetype valueType;
    for (size_t i = 0; i < arrayLength; i++) {
        napi_value napiFormId;
        napi_get_element(env, argv[0], i, &napiFormId);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, napiFormId, &valueType));
        NAPI_ASSERT(env, valueType == napi_string, "The arguments[0] value type of notifyInvisibleForms \
        is incorrect, expected type is string.");

        std::string strFormId = GetStringFromNAPI(env, napiFormId);
        int64_t formIdValue = std::stoll(strFormId);

        formIds.push_back(formIdValue);
    }

    // get global value
    napi_value global = nullptr;
    napi_get_global(env, &global);
    
    // get ability
    napi_value abilityObj = nullptr;
    napi_get_named_property(env, global, "ability", &abilityObj);

    // get ability pointer
    OHOS::AppExecFwk::Ability* ability = nullptr;
    napi_get_value_external(env, abilityObj, (void**)&ability);
    HILOG_INFO("%{public}s, ability = [%{public}p]", __func__, ability);

    AsyncNotifyInvisibleFormsCallbackInfo *asyncCallbackInfo = new 
    AsyncNotifyInvisibleFormsCallbackInfo {
        .env = env,
        .ability = ability,
        .asyncWork = nullptr,
        .deferred = nullptr,
        .callback = nullptr,
        .formIds = formIds,
        .result = 1,
    };

    if (argc == ARGS_SIZE_TWO) {
        HILOG_INFO("%{public}s, asyncCallback.", __func__);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[1], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "The arguments[1] type of notifyInvisibleForms is incorrect,\
        expected type is function.");

        napi_create_reference(env, argv[1], 1, &asyncCallbackInfo->callback);

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, napi_create_async_work running", __func__);

                AsyncNotifyInvisibleFormsCallbackInfo *asyncCallbackInfo = 
                (AsyncNotifyInvisibleFormsCallbackInfo *)data;

                InnerNotifyInvisibleForms(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                AsyncNotifyInvisibleFormsCallbackInfo *asyncCallbackInfo = 
                (AsyncNotifyInvisibleFormsCallbackInfo *)data;

                HILOG_INFO("%{public}s, napi_create_async_work complete", __func__);

                if (asyncCallbackInfo->callback != nullptr) {
                    napi_value result;
                    napi_create_int32(env, asyncCallbackInfo->result, &result);
                    napi_value callback;
                    napi_value undefined;
                    napi_get_undefined(env, &undefined);
                    napi_get_reference_value(env, asyncCallbackInfo->callback, &callback);
                    napi_value callResult;
                    napi_call_function(env, undefined, callback, 1, &result, &callResult);
                    napi_delete_reference(env, asyncCallbackInfo->callback);
                }
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        NAPI_CALL(env, napi_queue_async_work(env, asyncCallbackInfo->asyncWork));
 
        napi_value result;
        NAPI_CALL(env, napi_create_int32(env, 1, &result));
        return result;
    } else {
        HILOG_INFO("%{public}s, promise.", __func__);
        napi_deferred deferred;
        napi_value promise;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, promise runnning", __func__);

                AsyncNotifyInvisibleFormsCallbackInfo *asyncCallbackInfo = 
                (AsyncNotifyInvisibleFormsCallbackInfo *)data;

                InnerNotifyInvisibleForms(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("%{public}s, promise complete", __func__);

                AsyncNotifyInvisibleFormsCallbackInfo *asyncCallbackInfo = 
                (AsyncNotifyInvisibleFormsCallbackInfo *)data;

                napi_value result;
                napi_create_int32(env, asyncCallbackInfo->result, &result);
                napi_resolve_deferred(asyncCallbackInfo->env, asyncCallbackInfo->deferred, result);
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
        return promise;
    }    
}

/**
 * @brief  Call native kit function: EnableUpdateForm
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] asyncCallbackInfo Reference, callback info via Node-API
 * 
 * @return void
 */
static void InnerEnableFormsUpdate(napi_env env, AsyncEnableUpdateFormCallbackInfo* const asyncCallbackInfo)
{
    HILOG_DEBUG("%{public}s called.", __func__);
    OHOS::AppExecFwk::Ability *ability = asyncCallbackInfo->ability;
    bool ret = ability->EnableUpdateForm(asyncCallbackInfo->formIds);
    if (ret) {
        asyncCallbackInfo->result = 1;
    } else {
        asyncCallbackInfo->result = 0;
    }
    HILOG_DEBUG("%{public}s, end", __func__);
}

/**
 * @brief  The implementation of Node-API interface: enableFormsUpdate 
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] info An opaque datatype that is passed to a callback function
 * 
 * @return This is an opaque pointer that is used to represent a JavaScript value
 */
napi_value NAPI_EnableFormsUpdate(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s called.", __func__);

    // Check the number of the arguments
    size_t argc = ARGS_SIZE_TWO;
    napi_value argv[ARGS_SIZE_TWO] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc > ARGS_SIZE_TWO) {
        HILOG_ERROR("%{public}s, wrong number of arguments.", __func__);
        return nullptr;
    }
    //HILOG_INFO("%{public}s, argc = [%{public}d]", __func__, argc);

    uint32_t arrayLength = 0;
    NAPI_CALL(env, napi_get_array_length(env, argv[0], &arrayLength));
    NAPI_ASSERT(env, arrayLength > 0, "The arguments[0] value of enableFormsUpdate \
    is incorrect, this array is empty.");

    std::vector<int64_t> formIds;
    formIds.clear();
    napi_valuetype valueType;
    for (size_t i = 0; i < arrayLength; i++) {
        napi_value napiFormId;
        napi_get_element(env, argv[0], i, &napiFormId);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, napiFormId, &valueType));
        NAPI_ASSERT(env, valueType == napi_string, "The arguments[0] value type of enableFormsUpdate \
        is incorrect, expected type is string.");

        std::string strFormId = GetStringFromNAPI(env, napiFormId);
        int64_t formIdValue = std::stoll(strFormId);

        formIds.push_back(formIdValue);
    }

    // get global value
    napi_value global = nullptr;
    napi_get_global(env, &global);
    
    // get ability
    napi_value abilityObj = nullptr;
    napi_get_named_property(env, global, "ability", &abilityObj);

    // get ability pointer
    OHOS::AppExecFwk::Ability* ability = nullptr;
    napi_get_value_external(env, abilityObj, (void**)&ability);
    HILOG_INFO("%{public}s, ability = [%{public}p]", __func__, ability);

    AsyncEnableUpdateFormCallbackInfo *asyncCallbackInfo = new 
    AsyncEnableUpdateFormCallbackInfo {
        .env = env,
        .ability = ability,
        .asyncWork = nullptr,
        .deferred = nullptr,
        .callback = nullptr,
        .formIds = formIds,
        .result = 0,
    };

    if (argc == ARGS_SIZE_TWO) {
        HILOG_INFO("%{public}s, asyncCallback.", __func__);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[1], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "The arguments[1] type of enableFormsUpdate \
        is incorrect, expected type is function.");

        napi_create_reference(env, argv[1], 1, &asyncCallbackInfo->callback);

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, napi_create_async_work running", __func__);

                AsyncEnableUpdateFormCallbackInfo *asyncCallbackInfo = 
                (AsyncEnableUpdateFormCallbackInfo *)data;

                InnerEnableFormsUpdate(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                AsyncEnableUpdateFormCallbackInfo *asyncCallbackInfo = 
                (AsyncEnableUpdateFormCallbackInfo *)data;

                HILOG_INFO("%{public}s, napi_create_async_work complete", __func__);

                if (asyncCallbackInfo->callback != nullptr) {
                    napi_value result;
                    napi_create_int32(env, asyncCallbackInfo->result, &result);
                    napi_value callback;
                    napi_value undefined;
                    napi_get_undefined(env, &undefined);
                    napi_get_reference_value(env, asyncCallbackInfo->callback, &callback);
                    napi_value callResult;
                    napi_call_function(env, undefined, callback, 1, &result, &callResult);
                    napi_delete_reference(env, asyncCallbackInfo->callback);
                }
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        NAPI_CALL(env, napi_queue_async_work(env, asyncCallbackInfo->asyncWork));
 
        napi_value result;
        NAPI_CALL(env, napi_create_int32(env, 1, &result));
        return result;
    } else {
        HILOG_INFO("%{public}s, promise.", __func__);
        napi_deferred deferred;
        napi_value promise;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, promise runnning", __func__);
                AsyncEnableUpdateFormCallbackInfo *asyncCallbackInfo = 
                (AsyncEnableUpdateFormCallbackInfo *)data;

                InnerEnableFormsUpdate(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("%{public}s, promise complete", __func__);

                AsyncEnableUpdateFormCallbackInfo *asyncCallbackInfo = 
                (AsyncEnableUpdateFormCallbackInfo *)data;

                napi_value result;
                napi_create_int32(env, asyncCallbackInfo->result, &result);
                napi_resolve_deferred(asyncCallbackInfo->env, asyncCallbackInfo->deferred, result);
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
        return promise;
    }    
}

/**
 * @brief  Call native kit function: DisableUpdateForm
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] asyncCallbackInfo Reference, callback info via Node-API
 * 
 * @return void
 */
static void InnerDisableFormsUpdate(napi_env env, AsyncDisableUpdateFormCallbackInfo* const asyncCallbackInfo)
{
    HILOG_DEBUG("%{public}s called.", __func__);
    OHOS::AppExecFwk::Ability *ability = asyncCallbackInfo->ability;
    bool ret = ability->DisableUpdateForm(asyncCallbackInfo->formIds);
    if (ret) {
        asyncCallbackInfo->result = 1;
    } else {
        asyncCallbackInfo->result = 0;
    }
    HILOG_DEBUG("%{public}s, end", __func__);
}

/**
 * @brief  The implementation of Node-API interface: disableFormsUpdate 
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] info An opaque datatype that is passed to a callback function
 * 
 * @return This is an opaque pointer that is used to represent a JavaScript value
 */
napi_value NAPI_DisableFormsUpdate(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s called.", __func__);

    // Check the number of the arguments
    size_t argc = ARGS_SIZE_TWO;
    napi_value argv[ARGS_SIZE_TWO] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc > ARGS_SIZE_TWO) {
        HILOG_ERROR("%{public}s, wrong number of arguments.", __func__);
        return nullptr;
    }
    //HILOG_INFO("%{public}s, argc = [%{public}d]", __func__, argc);

    uint32_t arrayLength = 0;
    NAPI_CALL(env, napi_get_array_length(env, argv[0], &arrayLength));
    NAPI_ASSERT(env, arrayLength > 0, "The arguments[0] value of disableFormsUpdate \
    is incorrect, this array is empty.");

    std::vector<int64_t> formIds;
    formIds.clear();
    napi_valuetype valueType;
    for (size_t i = 0; i < arrayLength; i++) {
        napi_value napiFormId;
        napi_get_element(env, argv[0], i, &napiFormId);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, napiFormId, &valueType));
        NAPI_ASSERT(env, valueType == napi_string, "The arguments[0] value type of disableFormsUpdate \
        is incorrect, expected type is string.");

        std::string strFormId = GetStringFromNAPI(env, napiFormId);
        int64_t formIdValue = std::stoll(strFormId);

        formIds.push_back(formIdValue);
    }

    // get global value
    napi_value global = nullptr;
    napi_get_global(env, &global);
    
    // get ability
    napi_value abilityObj = nullptr;
    napi_get_named_property(env, global, "ability", &abilityObj);

    // get ability pointer
    OHOS::AppExecFwk::Ability* ability = nullptr;
    napi_get_value_external(env, abilityObj, (void**)&ability);
    HILOG_INFO("%{public}s, ability = [%{public}p]", __func__, ability);

    AsyncDisableUpdateFormCallbackInfo *asyncCallbackInfo = new 
    AsyncDisableUpdateFormCallbackInfo {
        .env = env,
        .ability = ability,
        .asyncWork = nullptr,
        .deferred = nullptr,
        .callback = nullptr,
        .formIds = formIds,
        .result = 0,
    };

    if (argc == ARGS_SIZE_TWO) {
        HILOG_INFO("%{public}s, asyncCallback.", __func__);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[1], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "The arguments[1] type of disableFormsUpdate \
        is incorrect, expected type is function.");

        napi_create_reference(env, argv[1], 1, &asyncCallbackInfo->callback);

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, napi_create_async_work running", __func__);

                AsyncDisableUpdateFormCallbackInfo *asyncCallbackInfo = 
                (AsyncDisableUpdateFormCallbackInfo *)data;

                InnerDisableFormsUpdate(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                AsyncDisableUpdateFormCallbackInfo *asyncCallbackInfo = 
                (AsyncDisableUpdateFormCallbackInfo *)data;

                HILOG_INFO("%{public}s, napi_create_async_work complete", __func__);

                if (asyncCallbackInfo->callback != nullptr) {
                    napi_value result;
                    napi_create_int32(env, asyncCallbackInfo->result, &result);
                    napi_value callback;
                    napi_value undefined;
                    napi_get_undefined(env, &undefined);
                    napi_get_reference_value(env, asyncCallbackInfo->callback, &callback);
                    napi_value callResult;
                    napi_call_function(env, undefined, callback, 1, &result, &callResult);
                    napi_delete_reference(env, asyncCallbackInfo->callback);
                }
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        NAPI_CALL(env, napi_queue_async_work(env, asyncCallbackInfo->asyncWork));
 
        napi_value result;
        NAPI_CALL(env, napi_create_int32(env, 1, &result));
        return result;
    } else {
        HILOG_INFO("%{public}s, promise.", __func__);
        napi_deferred deferred;
        napi_value promise;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, promise runnning", __func__);

                AsyncDisableUpdateFormCallbackInfo *asyncCallbackInfo = 
                (AsyncDisableUpdateFormCallbackInfo *)data;

                InnerDisableFormsUpdate(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("%{public}s, promise complete", __func__);

                AsyncDisableUpdateFormCallbackInfo *asyncCallbackInfo = 
                (AsyncDisableUpdateFormCallbackInfo *)data;

                napi_value result;
                napi_create_int32(env, asyncCallbackInfo->result, &result);
                napi_resolve_deferred(asyncCallbackInfo->env, asyncCallbackInfo->deferred, result);
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
        return promise;
    }    
}

/**
 * @brief  Call native kit function: CheckFMSReady
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] asyncCallbackInfo Reference, callback info via Node-API
 * 
 * @return void
 */
static void InnerCheckFMSReady(napi_env env, AsyncCheckFMSReadyCallbackInfo* const asyncCallbackInfo)
{
    HILOG_DEBUG("%{public}s called.", __func__);
    OHOS::AppExecFwk::Ability *ability = asyncCallbackInfo->ability;
    asyncCallbackInfo->isFMSReady = ability->CheckFMSReady();
    HILOG_DEBUG("%{public}s, end", __func__);
}

/**
 * @brief  The implementation of Node-API interface: checkFMSReady 
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] info An opaque datatype that is passed to a callback function
 * 
 * @return This is an opaque pointer that is used to represent a JavaScript value
 */
napi_value NAPI_CheckFMSReady(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s called.", __func__);

    // Check the number of the arguments
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc > ARGS_SIZE_ONE) {
        HILOG_ERROR("%{public}s, wrong number of arguments.", __func__);
        return nullptr;
    }
   // HILOG_INFO("%{public}s, argc = [%{public}d]", __func__, argc);

    // get global value
    napi_value global = nullptr;
    napi_get_global(env, &global);
    
    // get ability
    napi_value abilityObj = nullptr;
    napi_get_named_property(env, global, "ability", &abilityObj);

    // get ability pointer
    OHOS::AppExecFwk::Ability* ability = nullptr;
    napi_get_value_external(env, abilityObj, (void**)&ability);
    HILOG_INFO("%{public}s, ability = [%{public}p]", __func__, ability);

    AsyncCheckFMSReadyCallbackInfo *asyncCallbackInfo = new
    AsyncCheckFMSReadyCallbackInfo {
        .env = env,
        .ability = ability,
        .asyncWork = nullptr,
        .deferred = nullptr,
        .callback = nullptr,
        .isFMSReady = false,
    };

    if (argc == ARGS_SIZE_ONE) {
        HILOG_INFO("%{public}s, asyncCallback.", __func__);

        // Check the value type of the arguments
        napi_valuetype valueType;
        NAPI_CALL(env, napi_typeof(env, argv[0], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "The arguments[0] type of checkFMSReady is incorrect,\
        expected type is function.");
        
        napi_create_reference(env, argv[0], 1, &asyncCallbackInfo->callback);
        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, napi_create_async_work running", __func__);

                AsyncCheckFMSReadyCallbackInfo *asyncCallbackInfo = 
                (AsyncCheckFMSReadyCallbackInfo *)data;

                InnerCheckFMSReady(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("%{public}s, napi_create_async_work complete", __func__);

                AsyncCheckFMSReadyCallbackInfo *asyncCallbackInfo = 
                (AsyncCheckFMSReadyCallbackInfo *)data;

                if (asyncCallbackInfo->callback != nullptr) {
                    napi_value isFMSReadyResult;
                    napi_create_int32(env, asyncCallbackInfo->isFMSReady, &isFMSReadyResult);
                    napi_value callback;
                    napi_get_reference_value(env, asyncCallbackInfo->callback, &callback);
                    napi_value callResult;
                    napi_call_function(env, nullptr, callback, 1, &isFMSReadyResult, &callResult);
                    napi_delete_reference(env, asyncCallbackInfo->callback);
                }
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        NAPI_CALL(env, napi_queue_async_work(env, asyncCallbackInfo->asyncWork));
 
        napi_value result;
        NAPI_CALL(env, napi_create_int32(env, 1, &result));
        return result;
    } else {
        HILOG_INFO("%{public}s, promise.", __func__);
        napi_deferred deferred;
        napi_value promise;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, promise runnning", __func__);

                AsyncCheckFMSReadyCallbackInfo *asyncCallbackInfo = 
                (AsyncCheckFMSReadyCallbackInfo *)data;

                InnerCheckFMSReady(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("%{public}s, promise complete", __func__);

                AsyncCheckFMSReadyCallbackInfo *asyncCallbackInfo = 
                (AsyncCheckFMSReadyCallbackInfo *)data;

                napi_value result;
                napi_create_int32(env, asyncCallbackInfo->isFMSReady, &result);
                napi_resolve_deferred(asyncCallbackInfo->env, asyncCallbackInfo->deferred, result);
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
        return promise;
    } 
}

/**
 * @brief  Call native kit function: GetAllFormsInfo
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] asyncCallbackInfo Reference, callback info via Node-API
 * 
 * @return void
 */
static void InnerGetAllFormsInfo(napi_env env, AsyncGetAllFormsCallbackInfo* const asyncCallbackInfo)
{
    HILOG_DEBUG("%{public}s called.", __func__);
    OHOS::AppExecFwk::Ability *ability = asyncCallbackInfo->ability;
    bool ret = ability->GetAllFormsInfo(asyncCallbackInfo->formInfos);
    if (ret) {
        asyncCallbackInfo->result = 1;
    } else {
        asyncCallbackInfo->result = 0;
    }
    HILOG_DEBUG("%{public}s, end", __func__);
}

/**
 * @brief  The implementation of Node-API interface: getAllFormsInfo 
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] info An opaque datatype that is passed to a callback function
 * 
 * @return This is an opaque pointer that is used to represent a JavaScript value
 */
napi_value NAPI_GetAllFormsInfo(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s called.", __func__);

    // Check the number of the arguments
    size_t argc = ARGS_SIZE_ONE;
    napi_value argv[ARGS_SIZE_ONE] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc > ARGS_SIZE_ONE) {
        HILOG_ERROR("%{public}s, wrong number of arguments.", __func__);
        return nullptr;
    }
   // HILOG_INFO("%{public}s, argc = [%{public}d]", __func__, argc);

    std::vector<OHOS::AppExecFwk::FormInfo> formInfos;
    formInfos.clear();
    
    // get global value
    napi_value global = nullptr;
    napi_get_global(env, &global);
    
    // get ability
    napi_value abilityObj = nullptr;
    napi_get_named_property(env, global, "ability", &abilityObj);

    // get ability pointer
    OHOS::AppExecFwk::Ability* ability = nullptr;
    napi_get_value_external(env, abilityObj, (void**)&ability);
    HILOG_INFO("%{public}s, ability = [%{public}p]", __func__, ability);

    AsyncGetAllFormsCallbackInfo *asyncCallbackInfo = new 
    AsyncGetAllFormsCallbackInfo {
        .env = env,
        .ability = ability,
        .asyncWork = nullptr,
        .deferred = nullptr,
        .callback = nullptr,
        .formInfos = formInfos,
        .result = 0,
    };

    if (argc == ARGS_SIZE_ONE) {
        HILOG_INFO("%{public}s, asyncCallback.", __func__);

        // Check the value type of the arguments
        napi_valuetype valueType;
        NAPI_CALL(env, napi_typeof(env, argv[0], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "The arguments[0] type of getAllFormsInfo is incorrect,\
        expected type is function.");
        
        napi_create_reference(env, argv[0], 1, &asyncCallbackInfo->callback);
        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, napi_create_async_work running", __func__);

                AsyncGetAllFormsCallbackInfo *asyncCallbackInfo = 
                (AsyncGetAllFormsCallbackInfo *)data;

                InnerGetAllFormsInfo(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("%{public}s, napi_create_async_work complete", __func__); 

                AsyncGetAllFormsCallbackInfo *asyncCallbackInfo = 
                (AsyncGetAllFormsCallbackInfo *)data; 

                napi_value arrayFormInfos;
                napi_create_array(env, &arrayFormInfos);
                if (asyncCallbackInfo->result) {
                    int iFormInfoCount = 0;
                    for (auto  formInfo : asyncCallbackInfo->formInfos) {
                        napi_value formInfoObject = nullptr;
                        napi_create_object(env, &formInfoObject);
                        ParseFormInfoIntoNapi(env, formInfo, formInfoObject);
                        napi_set_element(env, arrayFormInfos, iFormInfoCount, formInfoObject);
                        ++iFormInfoCount;
                    }
                }

                if (asyncCallbackInfo->callback != nullptr) {
                    napi_value callbackValues[2] = {0};
                    napi_value callback;
                    napi_value resultCode;
                    napi_create_int32(env, asyncCallbackInfo->result, &resultCode);
                    callbackValues[0] = resultCode;
                    callbackValues[1] = arrayFormInfos;
                    napi_get_reference_value(env, asyncCallbackInfo->callback, &callback);
                    napi_value callResult;
                    napi_call_function(env, nullptr, callback, 2, callbackValues, &callResult);
                    napi_delete_reference(env, asyncCallbackInfo->callback);
                }
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        NAPI_CALL(env, napi_queue_async_work(env, asyncCallbackInfo->asyncWork));
        napi_value result;
        NAPI_CALL(env, napi_create_int32(env, 1, &result));
        return result;
    } else {
        HILOG_INFO("%{public}s, promise.", __func__);
        napi_deferred deferred;
        napi_value promise;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, promise runnning", __func__);

                AsyncGetAllFormsCallbackInfo *asyncCallbackInfo = 
                (AsyncGetAllFormsCallbackInfo *)data;

                InnerGetAllFormsInfo(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("%{public}s, promise complete", __func__);

                AsyncGetAllFormsCallbackInfo *asyncCallbackInfo = 
                (AsyncGetAllFormsCallbackInfo *)data;

                napi_value arrayFormInfos;
                napi_create_array(env, &arrayFormInfos);
                if (asyncCallbackInfo->result) {
                    int iFormInfoCount = 0;
                    for (auto  formInfo : asyncCallbackInfo->formInfos) {
                        napi_value formInfoObject = nullptr;
                        napi_create_object(env, &formInfoObject);
                        ParseFormInfoIntoNapi(env, formInfo, formInfoObject);
                        napi_set_element(env, arrayFormInfos, iFormInfoCount, formInfoObject);
                        ++iFormInfoCount;
                    }
                }
                napi_resolve_deferred(asyncCallbackInfo->env, asyncCallbackInfo->deferred, arrayFormInfos);
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
        return promise;
    }
}

/**
 * @brief  Call native kit function: GetFormsInfoByApp
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] asyncCallbackInfo Reference, callback info via Node-API
 * 
 * @return void
 */
static void InnerGetFormsInfoByApp(napi_env env, AsyncGetFormsInfoByAppCallbackInfo* const asyncCallbackInfo)
{
    HILOG_DEBUG("%{public}s called.", __func__);
    OHOS::AppExecFwk::Ability *ability = asyncCallbackInfo->ability;
    bool ret = ability->GetFormsInfoByApp(asyncCallbackInfo->bundleName, 
    asyncCallbackInfo->formInfos);
    if (ret) {
        asyncCallbackInfo->result = 1;
    } else {
        asyncCallbackInfo->result = 0;
    }
    HILOG_DEBUG("%{public}s, end", __func__);
}

/**
 * @brief  The implementation of Node-API interface: getFormsInfoByApp 
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] info An opaque datatype that is passed to a callback function
 * 
 * @return This is an opaque pointer that is used to represent a JavaScript value
 */
napi_value NAPI_GetFormsInfoByApp(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s called.", __func__);

    // Check the number of the arguments
    size_t argc = ARGS_SIZE_TWO;
    napi_value argv[ARGS_SIZE_TWO] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc > ARGS_SIZE_TWO) {
        HILOG_ERROR("%{public}s, wrong number of arguments.", __func__);
        return nullptr;
    }
    //HILOG_INFO("%{public}s, argc = [%{public}d]", __func__, argc);

    // Check the value type of the arguments
    napi_valuetype valueType;
    NAPI_CALL(env, napi_typeof(env, argv[0], &valueType));
    NAPI_ASSERT(env, valueType == napi_string, "The arguments[0] type of getFormsInfoByApp is incorrect,\
    expected type is string.");
    
    std::string bundleNameInfo = GetStringFromNAPI(env, argv[0]);
    HILOG_INFO("%{public}s, bundleName=%{public}s.", __func__, bundleNameInfo.c_str()); 

    std::vector<OHOS::AppExecFwk::FormInfo> formInfos;
    formInfos.clear();
    
    // get global value
    napi_value global = nullptr;
    napi_get_global(env, &global);
    
    // get ability
    napi_value abilityObj = nullptr;
    napi_get_named_property(env, global, "ability", &abilityObj);

    // get ability pointer
    OHOS::AppExecFwk::Ability* ability = nullptr;
    napi_get_value_external(env, abilityObj, (void**)&ability);
    HILOG_INFO("%{public}s, ability = [%{public}p]", __func__, ability);

    AsyncGetFormsInfoByAppCallbackInfo *asyncCallbackInfo = new 
    AsyncGetFormsInfoByAppCallbackInfo {
        .env = env,
        .ability = ability,
        .asyncWork = nullptr,
        .deferred = nullptr,
        .callback = nullptr,
        .formInfos = formInfos,
        .bundleName = bundleNameInfo,
        .result = 0,
    };

    if (argc == ARGS_SIZE_TWO) {
        HILOG_INFO("%{public}s, asyncCallback.", __func__);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[1], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "The arguments[1] type of getFormsInfoByApp is incorrect,\
        expected type is function.");

        napi_create_reference(env, argv[1], 1, &asyncCallbackInfo->callback);
        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, napi_create_async_work running", __func__);

                AsyncGetFormsInfoByAppCallbackInfo *asyncCallbackInfo = 
                (AsyncGetFormsInfoByAppCallbackInfo *)data;

                InnerGetFormsInfoByApp(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("%{public}s, napi_create_async_work complete", __func__);

                AsyncGetFormsInfoByAppCallbackInfo *asyncCallbackInfo = 
                (AsyncGetFormsInfoByAppCallbackInfo *)data;

                napi_value arrayFormInfos;
                napi_create_array(env, &arrayFormInfos);
                if (asyncCallbackInfo->result) {
                    int iFormInfoCount = 0;
                    for (auto  formInfo : asyncCallbackInfo->formInfos) {
                        napi_value formInfoObject = nullptr;
                        napi_create_object(env, &formInfoObject);
                        ParseFormInfoIntoNapi(env, formInfo, formInfoObject);
                        napi_set_element(env, arrayFormInfos, iFormInfoCount, formInfoObject);
                        ++iFormInfoCount;
                    }
                }
                if (asyncCallbackInfo->callback != nullptr) {
                    napi_value callbackValues[2] = {0};
                    napi_value callback;
                    napi_value resultCode;
                    napi_create_int32(env, asyncCallbackInfo->result, &resultCode);
                    callbackValues[0] = resultCode;
                    callbackValues[1] = arrayFormInfos;
                    napi_get_reference_value(env, asyncCallbackInfo->callback, &callback);
                    napi_value callResult;
                    napi_call_function(env, nullptr, callback, 2, callbackValues, &callResult);
                    napi_delete_reference(env, asyncCallbackInfo->callback);
                }
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        NAPI_CALL(env, napi_queue_async_work(env, asyncCallbackInfo->asyncWork));
 
        napi_value result;
        NAPI_CALL(env, napi_create_int32(env, 1, &result));
        return result;
    } else {
        HILOG_INFO("%{public}s, promise.", __func__);
        napi_deferred deferred;
        napi_value promise;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, promise runnning", __func__);

                AsyncGetFormsInfoByAppCallbackInfo *asyncCallbackInfo = 
                (AsyncGetFormsInfoByAppCallbackInfo *)data;

                InnerGetFormsInfoByApp(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("%{public}s, promise complete", __func__);

                AsyncGetFormsInfoByAppCallbackInfo *asyncCallbackInfo = 
                (AsyncGetFormsInfoByAppCallbackInfo *)data;
                
                napi_value arrayFormInfos;
                napi_create_array(env, &arrayFormInfos);
                if (asyncCallbackInfo->result) {
                    int iFormInfoCount = 0;
                    for (auto  formInfo : asyncCallbackInfo->formInfos) {
                        napi_value formInfoObject = nullptr;
                        napi_create_object(env, &formInfoObject);
                        ParseFormInfoIntoNapi(env, formInfo, formInfoObject);
                        napi_set_element(env, arrayFormInfos, iFormInfoCount, formInfoObject);
                        ++iFormInfoCount;
                    }
                }
                napi_resolve_deferred(asyncCallbackInfo->env, asyncCallbackInfo->deferred, arrayFormInfos);
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
        return promise;
    }
}

/**
 * @brief  Call native kit function: GetFormsInfoByModule
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] asyncCallbackInfo Reference, callback info via Node-API
 * 
 * @return void
 */
static void InnerGetFormsInfoByModule(napi_env env, AsyncGetFormsInfoByModuleCallbackInfo* const asyncCallbackInfo)
{
    HILOG_DEBUG("%{public}s called.", __func__);
    OHOS::AppExecFwk::Ability *ability = asyncCallbackInfo->ability;

    bool ret = ability->GetFormsInfoByModule(
        asyncCallbackInfo->bundleName, 
        asyncCallbackInfo->moduleName, 
        asyncCallbackInfo->formInfos);

    if (ret) {
        asyncCallbackInfo->result = 1;
    } else {
        asyncCallbackInfo->result = 0;
    }
    HILOG_DEBUG("%{public}s, end", __func__);
}

/**
 * @brief  The implementation of Node-API interface: getFormsInfoByModule 
 *
 * @param[in] env The environment that the Node-API call is invoked under
 * @param[out] info An opaque datatype that is passed to a callback function
 * 
 * @return This is an opaque pointer that is used to represent a JavaScript value
 */
napi_value NAPI_GetFormsInfoByModule(napi_env env, napi_callback_info info)
{
    HILOG_INFO("%{public}s called.", __func__);

    // Check the number of the arguments
    size_t argc = ARGS_SIZE_THREE;
    napi_value argv[ARGS_SIZE_THREE] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));
    if (argc > ARGS_SIZE_THREE) {
        HILOG_ERROR("%{public}s, wrong number of arguments.", __func__);
        return nullptr;
    }
    //HILOG_INFO("%{public}s, argc = [%{public}d]", __func__, argc);

    // Check the value type of the arguments
    napi_valuetype valueType;
    NAPI_CALL(env, napi_typeof(env, argv[0], &valueType));
    NAPI_ASSERT(env, valueType == napi_string, "The arguments[0] type of getFormsInfoByModule is incorrect,\
    expected type is string.");

    std::string bundleNameInfo = GetStringFromNAPI(env, argv[0]);
    HILOG_INFO("%{public}s, bundleName=%{public}s.", __func__, bundleNameInfo.c_str()); 

    valueType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[1], &valueType));
    NAPI_ASSERT(env, valueType == napi_string, "The arguments[1] type of getFormsInfoByModule is incorrect,\
    expected type is string.");

    std::string moduleNameInfo = GetStringFromNAPI(env, argv[1]);
    HILOG_INFO("%{public}s, moduleName=%{public}s.", __func__, moduleNameInfo.c_str()); 

    std::vector<OHOS::AppExecFwk::FormInfo> formInfos;
    formInfos.clear();

    // get global value
    napi_value global = nullptr;
    napi_get_global(env, &global);
    
    // get ability
    napi_value abilityObj = nullptr;
    napi_get_named_property(env, global, "ability", &abilityObj);

    // get ability pointer
    OHOS::AppExecFwk::Ability* ability = nullptr;
    napi_get_value_external(env, abilityObj, (void**)&ability);
    HILOG_INFO("%{public}s, ability = [%{public}p]", __func__, ability);

    AsyncGetFormsInfoByModuleCallbackInfo *asyncCallbackInfo = new
    AsyncGetFormsInfoByModuleCallbackInfo {
        .env = env,
        .ability = ability,
        .asyncWork = nullptr,
        .deferred = nullptr,
        .callback = nullptr,
        .formInfos = formInfos,
        .bundleName = bundleNameInfo,
        .moduleName = moduleNameInfo,
        .result = 0,
    };

    if (argc == ARGS_SIZE_THREE) {
        HILOG_INFO("%{public}s, asyncCallback.", __func__);

        // Check the value type of the arguments
        valueType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[2], &valueType));
        NAPI_ASSERT(env, valueType == napi_function, "The arguments[2] type of getFormsInfoByModule \
        is incorrect, expected type is function.");
        
        napi_create_reference(env, argv[2], 1, &asyncCallbackInfo->callback);
        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, napi_create_async_work running", __func__);

                AsyncGetFormsInfoByModuleCallbackInfo *asyncCallbackInfo = 
                (AsyncGetFormsInfoByModuleCallbackInfo *)data;

                InnerGetFormsInfoByModule(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("%{public}s, napi_create_async_work complete", __func__);

                AsyncGetFormsInfoByModuleCallbackInfo *asyncCallbackInfo = 
                (AsyncGetFormsInfoByModuleCallbackInfo *)data;

                napi_value arrayFormInfos;
                napi_create_array(env, &arrayFormInfos);
                if (asyncCallbackInfo->result) {
                    int iFormInfoCount = 0;
                    for (auto  formInfo : asyncCallbackInfo->formInfos) {
                        napi_value formInfoObject = nullptr;
                        napi_create_object(env, &formInfoObject);
                        ParseFormInfoIntoNapi(env, formInfo, formInfoObject);
                        napi_set_element(env, arrayFormInfos, iFormInfoCount, formInfoObject);
                        ++iFormInfoCount;
                    }
                }
                if (asyncCallbackInfo->callback != nullptr) {
                    napi_value callbackValues[2] = {0};
                    napi_value callback;
                    napi_value resultCode;
                    napi_create_int32(env, asyncCallbackInfo->result, &resultCode);
                    callbackValues[0] = resultCode;
                    callbackValues[1] = arrayFormInfos;
                    napi_get_reference_value(env, asyncCallbackInfo->callback, &callback);
                    napi_value callResult;
                    napi_call_function(env, nullptr, callback, 2, callbackValues, &callResult);
                    napi_delete_reference(env, asyncCallbackInfo->callback);
                }
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        NAPI_CALL(env, napi_queue_async_work(env, asyncCallbackInfo->asyncWork));
 
        napi_value result;
        NAPI_CALL(env, napi_create_int32(env, 1, &result));
        return result;
    } else {
        HILOG_INFO("%{public}s, promise.", __func__);
        napi_deferred deferred;
        napi_value promise;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));
        asyncCallbackInfo->deferred = deferred;

        napi_value resourceName;
        napi_create_string_latin1(env, __func__, NAPI_AUTO_LENGTH, &resourceName);
        napi_create_async_work(
            env,
            nullptr,
            resourceName,
            [](napi_env env, void *data) {
                HILOG_INFO("%{public}s, promise runnning", __func__);

                AsyncGetFormsInfoByModuleCallbackInfo *asyncCallbackInfo = 
                (AsyncGetFormsInfoByModuleCallbackInfo *)data;

                InnerGetFormsInfoByModule(env, asyncCallbackInfo);
            },
            [](napi_env env, napi_status status, void *data) {
                HILOG_INFO("%{public}s, promise complete", __func__);
                
                AsyncGetFormsInfoByModuleCallbackInfo *asyncCallbackInfo = 
                (AsyncGetFormsInfoByModuleCallbackInfo *)data;

                napi_value arrayFormInfos;
                napi_create_array(env, &arrayFormInfos);
                if (asyncCallbackInfo->result) {
                    int iFormInfoCount = 0;
                    for (auto  formInfo : asyncCallbackInfo->formInfos) {
                        napi_value formInfoObject = nullptr;
                        napi_create_object(env, &formInfoObject);
                        ParseFormInfoIntoNapi(env, formInfo, formInfoObject);
                        napi_set_element(env, arrayFormInfos, iFormInfoCount, formInfoObject);
                        ++iFormInfoCount;
                    }
                }
                napi_resolve_deferred(asyncCallbackInfo->env, asyncCallbackInfo->deferred, arrayFormInfos);
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void *)asyncCallbackInfo,
            &asyncCallbackInfo->asyncWork);
        napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
        return promise;
    }
}
