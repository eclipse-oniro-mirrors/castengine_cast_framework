/*
 * Copyright (C) 2023-2023 Huawei Device Co., Ltd.
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
 * Description: supply untils realization for napi interface.
 * Author: zhangjingnan
 * Create: 2022-7-11
 */

#include <uv.h>
#include <memory>
#include "securec.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "cast_engine_log.h"
#include "cast_engine_common.h"
#include "napi_cast_session.h"
#include "napi_castengine_utils.h"

using namespace std;
using namespace OHOS::CastEngine;

namespace OHOS {
namespace CastEngine {
namespace CastEngineClient {
DEFINE_CAST_ENGINE_LABEL("Cast-Napi-Utils");

napi_value GetUndefinedValue(napi_env env)
{
    napi_value result {};
    napi_get_undefined(env, &result);
    return result;
}

string ParseString(napi_env env, napi_value args)
{
    string result {};
    if (args == nullptr) {
        CLOGE("args is nullptr");
        return result;
    }
    napi_valuetype valueType;
    napi_status status = napi_typeof(env, args, &valueType);
    CLOGD("param=%{public}d.", valueType);
    if (status != napi_ok || valueType != napi_string) {
        CLOGE("Wrong argument type. String expected.");
        result = "";
        return result;
    }
    size_t size = 0;
    size_t bufSize = 0;

    if (napi_get_value_string_utf8(env, args, nullptr, bufSize, &size) != napi_ok) {
        CLOGE("can not get string size");
        result = "";
        return result;
    }
    result.reserve(size + 1);
    result.resize(size);
    if (napi_get_value_string_utf8(env, args, result.data(), (size + 1), &size) != napi_ok) {
        CLOGE("can not get string value");
        result = "";
        return result;
    }
    return result;
}

int32_t ParseInt32(napi_env env, napi_value args)
{
    int32_t param = 0;
    if (args == nullptr) {
        CLOGE("args is nullptr");
        return param;
    }
    napi_valuetype valueType;
    napi_status status = napi_typeof(env, args, &valueType);
    if (status != napi_ok) {
        CLOGE("napi_typeof failed.");
        return param;
    }
    CLOGD("param=%{public}d.", valueType);
    if (valueType != napi_number) {
        CLOGE("Wrong argument type. Int32 expected.");
        return param;
    }
    status = napi_get_value_int32(env, args, &param);
    if (status != napi_ok) {
        CLOGE("napi_get_value_int32 failed.");
        return param;
    }
    return param;
}

uint32_t ParseUint32(napi_env env, napi_value args)
{
    uint32_t param = 0;
    if (args == nullptr) {
        CLOGE("args is nullptr");
        return param;
    }
    napi_valuetype valueType;
    napi_status status = napi_typeof(env, args, &valueType);
    if (status != napi_ok) {
        CLOGE("napi_typeof failed.");
        return param;
    }
    CLOGD("param=%{public}d.", valueType);
    if (valueType != napi_number) {
        CLOGE("Wrong argument type. Int32 expected.");
        return param;
    }
    status = napi_get_value_uint32(env, args, &param);
    if (status != napi_ok) {
        CLOGE("napi_get_value_int32 failed.");
        return param;
    }
    return param;
}

bool ParseBool(napi_env env, napi_value args)
{
    bool param = false;
    if (args == nullptr) {
        CLOGE("args is nullptr");
        return param;
    }
    napi_valuetype valueType;
    napi_status status = napi_typeof(env, args, &valueType);
    if (status != napi_ok) {
        CLOGE("napi_typeof failed.");
        return param;
    }

    CLOGD("param=%{public}d.", valueType);
    if (valueType != napi_boolean) {
        CLOGE("Wrong argument type. bool expected.");
        return param;
    }
    status = napi_get_value_bool(env, args, &param);
    if (status != napi_ok) {
        CLOGE("napi_get_value_bool failed.");
        return param;
    }
    return param;
}

napi_value ConvertDeviceListToJS(napi_env env, const vector<CastRemoteDevice> &devices)
{
    int count = 0;
    napi_value devicesList = nullptr;
    NAPI_CALL(env, napi_create_array(env, &devicesList));
    for (CastRemoteDevice vec : devices) {
        napi_value deviceResult = ConvertCastRemoteDeviceToJS(env, vec);
        NAPI_CALL(env, napi_set_element(env, devicesList, count, deviceResult));
        count++;
    }

    if (devicesList == nullptr) {
        CLOGE("devicesList is null");
    }
    return devicesList;
}

napi_value ConvertDeviceStateInfoToJS(napi_env env, const DeviceStateInfo &stateEvent)
{
    napi_value stateEventCallback = nullptr;
    NAPI_CALL(env, napi_create_object(env, &stateEventCallback));

    napi_value deviceState = nullptr;
    NAPI_CALL(env, napi_create_int32(env, static_cast<int32_t>(stateEvent.deviceState), &deviceState));
    NAPI_CALL(env, napi_set_named_property(env, stateEventCallback, "deviceState", deviceState));

    napi_value deviceId = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, stateEvent.deviceId.c_str(), NAPI_AUTO_LENGTH, &deviceId));
    NAPI_CALL(env, napi_set_named_property(env, stateEventCallback, "deviceId", deviceId));

    napi_value reasonCode = nullptr;
    NAPI_CALL(env, napi_create_int32(env, static_cast<int32_t>(stateEvent.reasonCode), &reasonCode));
    NAPI_CALL(env, napi_set_named_property(env, stateEventCallback, "reasonCode", reasonCode));

    return stateEventCallback;
}

CastRemoteDevice GetCastRemoteDeviceFromJS(napi_env env, napi_value &object)
{
    CastRemoteDevice castRemoteDevice = CastRemoteDevice();

    castRemoteDevice.deviceId = JsObjectToString(env, object, "deviceId");
    castRemoteDevice.deviceName = JsObjectToString(env, object, "deviceName");
    int32_t deviceTypeCallback = JsObjectToInt32(env, object, "deviceType");
    DeviceType deviceType = static_cast<DeviceType>(deviceTypeCallback);
    int32_t subDeviceTypeCallback = JsObjectToInt32(env, object, "subDeviceType");
    SubDeviceType subDeviceType = static_cast<SubDeviceType>(subDeviceTypeCallback);
    castRemoteDevice.ipAddress = JsObjectToString(env, object, "ipAddress");
    int32_t channelTypeInt = JsObjectToInt32(env, object, "channelType");
    ChannelType channelType = static_cast<ChannelType>(channelTypeInt);
    castRemoteDevice.networkId = JsObjectToString(env, object, "networkId");
    castRemoteDevice.deviceType = deviceType;
    castRemoteDevice.subDeviceType = subDeviceType;
    castRemoteDevice.channelType = channelType;
    return castRemoteDevice;
}

WindowProperty GetWindowPropertyFromJS(napi_env env, napi_value &object)
{
    WindowProperty windowProperty = WindowProperty();

    napi_value windowPropertyCallback = nullptr;
    bool hasProperty = false;
    napi_status status = napi_has_named_property(env, object, "windowProperty", &hasProperty);
    if (status == napi_ok && hasProperty) {
        status = napi_get_named_property(env, object, "windowProperty", &windowPropertyCallback);
        if (status != napi_ok) {
            CLOGE("napi_get_named_property failed.");
            return windowProperty;
        }
        uint32_t width = JsObjectToUint32(env, windowPropertyCallback, "width");
        uint32_t height = JsObjectToUint32(env, windowPropertyCallback, "height");
        uint32_t startX = JsObjectToUint32(env, windowPropertyCallback, "startX");
        uint32_t startY = JsObjectToUint32(env, windowPropertyCallback, "startY");

        windowProperty.startX = startX;
        windowProperty.startY = startY;
        windowProperty.width = width;
        windowProperty.height = height;
    }
    return windowProperty;
}

bool GetProtocolTypesFromJS(napi_env env, napi_value &object, int &protocolTypes)
{
    bool isArray = false;
    NAPI_CALL_BASE(env, napi_is_array(env, object, &isArray), false);
    if (!isArray) {
        CLOGE("protocolType is not array.");
        return false;
    }
    uint32_t arrLen = 0;
    NAPI_CALL_BASE(env, napi_get_array_length(env, object, &arrLen), false);
    if (arrLen == 0) {
        CLOGE("mediaInfoList len is invalid");
        return false;
    }
    uint32_t ret = 0;
    for (uint32_t i = 0; i < arrLen; i++) {
        napi_value item = nullptr;
        NAPI_CALL_BASE(env, napi_get_element(env, object, i, &item), false);
        ret = ret | static_cast<uint32_t>(ParseInt32(env, item));
    }
    protocolTypes = static_cast<int>(ret);
    CLOGI("GetProtocolTypesFromJS finished, protocolTypes: %{public}d", protocolTypes);
    return true;
}

bool GetMediaInfoHolderFromJS(napi_env env, napi_value &object, MediaInfoHolder &mediaInfoHolder)
{
    napi_value mediaInfoList = nullptr;
    bool hasProperty = false;

    mediaInfoHolder.currentIndex = JsObjectToUint32(env, object, "currentIndex");
    mediaInfoHolder.progressRefreshInterval = JsObjectToUint32(env, object, "progressRefreshInterval");
    NAPI_CALL_BASE(env, napi_has_named_property(env, object, "mediaInfoList", &hasProperty), false);
    if (!hasProperty) {
        CLOGE("mediaInfoList is not exit");
        return false;
    }
    NAPI_CALL_BASE(env, napi_get_named_property(env, object, "mediaInfoList", &mediaInfoList), false);
    bool isArray = false;
    NAPI_CALL_BASE(env, napi_is_array(env, mediaInfoList, &isArray), false);
    if (!isArray) {
        CLOGE("mediaInfoList is not array.");
        return false;
    }
    uint32_t arrLen = 0;
    NAPI_CALL_BASE(env, napi_get_array_length(env, mediaInfoList, &arrLen), false);
    if (arrLen == 0) {
        CLOGE("mediaInfoList len is invalid");
        return false;
    }
    for (uint32_t i = 0; i < arrLen; i++) {
        napi_value item = nullptr;
        NAPI_CALL_BASE(env, napi_get_element(env, mediaInfoList, i, &item), false);
        MediaInfo mediaInfo = MediaInfo{};
        GetMediaInfoFromJS(env, item, mediaInfo);
        mediaInfoHolder.mediaInfoList.push_back(mediaInfo);
    }
    return true;
}

bool GetMediaInfoFromJS(napi_env env, napi_value &object, MediaInfo &mediaInfo)
{
    mediaInfo.mediaId = JsObjectToString(env, object, "mediaId");
    mediaInfo.mediaName = JsObjectToString(env, object, "mediaName");
    mediaInfo.mediaUrl = JsObjectToString(env, object, "mediaUrl");
    mediaInfo.mediaType = JsObjectToString(env, object, "mediaType");
    mediaInfo.albumCoverUrl = JsObjectToString(env, object, "albumCoverUrl");
    mediaInfo.albumTitle = JsObjectToString(env, object, "albumTitle");
    mediaInfo.mediaArtist = JsObjectToString(env, object, "mediaArtist");
    mediaInfo.lrcUrl = JsObjectToString(env, object, "lrcUrl");
    mediaInfo.lrcContent = JsObjectToString(env, object, "lrcContent");
    mediaInfo.appIconUrl = JsObjectToString(env, object, "appIconUrl");
    mediaInfo.appName = JsObjectToString(env, object, "appName");
    mediaInfo.mediaSize = JsObjectToUint32(env, object, "mediaSize");
    mediaInfo.startPosition = JsObjectToUint32(env, object, "startPosition");
    mediaInfo.duration = JsObjectToUint32(env, object, "duration");
    mediaInfo.closingCreditsPosition = JsObjectToUint32(env, object, "closingCreditsPosition");
    return true;
}

CastSessionProperty GetCastSessionPropertyFromJS(napi_env env, napi_value &object)
{
    CastSessionProperty castSessionProperty = CastSessionProperty();

    int32_t protocolTypeInt = JsObjectToInt32(env, object, "protocolType");
    ProtocolType protocolType = static_cast<ProtocolType>(protocolTypeInt);
    int32_t endTypeInt = JsObjectToInt32(env, object, "endType");
    EndType endType = static_cast<EndType>(endTypeInt);
    napi_value audioPropertyCallback = nullptr;
    bool hasProperty = false;
    napi_status status = napi_has_named_property(env, object, "audioProperty", &hasProperty);
    if (status == napi_ok && hasProperty) {
        status = napi_get_named_property(env, object, "audioProperty", &audioPropertyCallback);
        if (status != napi_ok) {
            CLOGE("napi_get_named_property failed.");
            return castSessionProperty;
        }
        AudioProperty audioProperty = GetAudioPropertyFromJS(env, audioPropertyCallback);
        castSessionProperty.audioProperty = audioProperty;
    }

    napi_value videoPropertyCallback = nullptr;
    hasProperty = false;
    status = napi_has_named_property(env, object, "videoProperty", &hasProperty);
    if (status == napi_ok && hasProperty) {
        status = napi_get_named_property(env, object, "videoProperty", &videoPropertyCallback);
        if (status != napi_ok) {
            CLOGE("napi_get_named_property failed.");
            return castSessionProperty;
        }
        VideoProperty videoProperty = GetVideoPropertyFromJS(env, videoPropertyCallback);
        castSessionProperty.videoProperty = videoProperty;
    }
    WindowProperty windowProperty = GetWindowPropertyFromJS(env, object);

    castSessionProperty.protocolType = protocolType;
    castSessionProperty.endType = endType;
    castSessionProperty.windowProperty = windowProperty;
    return castSessionProperty;
}

AudioProperty GetAudioPropertyFromJS(napi_env env, napi_value &object)
{
    AudioProperty audioProperty = AudioProperty();

    uint32_t sampleRate = JsObjectToUint32(env, object, "sampleRate");
    uint32_t channelConfig = JsObjectToUint32(env, object, "channelConfig");
    uint32_t bitrate = JsObjectToUint32(env, object, "bitrate");
    uint32_t codec = JsObjectToUint32(env, object, "codec");

    audioProperty.sampleRate = sampleRate;
    audioProperty.channelConfig = channelConfig;
    audioProperty.bitrate = bitrate;
    audioProperty.codec = codec;
    return audioProperty;
}

VideoProperty GetVideoPropertyFromJS(napi_env env, napi_value &object)
{
    VideoProperty videoProperty = VideoProperty();
    uint32_t videoWidth = JsObjectToUint32(env, object, "videoWidth");
    uint32_t videoHeight = JsObjectToUint32(env, object, "videoHeight");
    uint32_t fps = JsObjectToUint32(env, object, "fps");
    int32_t codecTypeInt = JsObjectToInt32(env, object, "codecType");
    VideoCodecType codecType = static_cast<VideoCodecType>(codecTypeInt);
    uint32_t gop = JsObjectToUint32(env, object, "gop");
    uint32_t bitrate = JsObjectToUint32(env, object, "bitrate");
    uint32_t minBitrate = JsObjectToUint32(env, object, "minBitrate");
    uint32_t maxBitrate = JsObjectToUint32(env, object, "maxBitrate");
    uint32_t dpi = JsObjectToUint32(env, object, "dpi");
    int32_t colorStandardInt = JsObjectToInt32(env, object, "colorStandard");
    ColorStandard colorStandard = static_cast<ColorStandard>(colorStandardInt);
    uint32_t screenWidth = JsObjectToUint32(env, object, "screenWidth");
    uint32_t screenHeight = JsObjectToUint32(env, object, "screenHeight");
    uint32_t profile = JsObjectToUint32(env, object, "profile");
    uint32_t level = JsObjectToUint32(env, object, "level");

    videoProperty.videoWidth = videoWidth;
    videoProperty.videoHeight = videoHeight;
    videoProperty.fps = fps;
    videoProperty.codecType = codecType;
    videoProperty.gop = gop;
    videoProperty.bitrate = bitrate;
    videoProperty.minBitrate = minBitrate;
    videoProperty.maxBitrate = maxBitrate;
    videoProperty.dpi = dpi;
    videoProperty.colorStandard = colorStandard;
    videoProperty.screenWidth = screenWidth;
    videoProperty.screenHeight = screenHeight;
    videoProperty.profile = profile;
    videoProperty.level = level;
    return videoProperty;
}

bool Equals(napi_env env, napi_value value, napi_ref copy)
{
    if (copy == nullptr) {
        return (value == nullptr);
    }

    napi_value copyValue = nullptr;
    if (napi_get_reference_value(env, copy, &copyValue) != napi_ok) {
        CLOGE("get ref value failed");
        return false;
    }
    bool isEquals = false;
    if (napi_strict_equals(env, value, copyValue, &isEquals) != napi_ok) {
        CLOGE("get equals result failed");
        return false;
    }
    return isEquals;
}

napi_status GetRefByCallback(napi_env env, std::list<napi_ref> &callbackList,
    napi_value callback, napi_ref &callbackRef)
{
    for (auto ref = callbackList.begin(); ref != callbackList.end(); ++ref) {
        if (Equals(env, callback, *ref)) {
            CLOGD("Callback has been matched");
            callbackRef = *ref;
            break;
        }
    }
    return napi_ok;
}

napi_value ConvertCastRemoteDeviceToJS(napi_env env, const CastRemoteDevice &castRemoteDevice)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    napi_value deviceId = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, castRemoteDevice.deviceId.c_str(), NAPI_AUTO_LENGTH, &deviceId));
    NAPI_CALL(env, napi_set_named_property(env, result, "deviceId", deviceId));
    napi_value deviceName = nullptr;
    CLOGD("ConvertCastRemoteDeviceToJS deviceName %{public}s", castRemoteDevice.deviceName.c_str());
    NAPI_CALL(env, napi_create_string_utf8(env, castRemoteDevice.deviceName.c_str(), NAPI_AUTO_LENGTH, &deviceName));
    NAPI_CALL(env, napi_set_named_property(env, result, "deviceName", deviceName));
    napi_value deviceType = nullptr;
    NAPI_CALL(env, napi_create_int32(env, static_cast<int32_t>(castRemoteDevice.deviceType), &deviceType));
    NAPI_CALL(env, napi_set_named_property(env, result, "deviceType", deviceType));
    napi_value subDeviceType = nullptr;
    NAPI_CALL(env, napi_create_int32(env, static_cast<int32_t>(castRemoteDevice.subDeviceType), &subDeviceType));
    NAPI_CALL(env, napi_set_named_property(env, result, "subDeviceType", subDeviceType));
    napi_value ipAddress = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, castRemoteDevice.ipAddress.c_str(), NAPI_AUTO_LENGTH, &ipAddress));
    NAPI_CALL(env, napi_set_named_property(env, result, "ipAddress", ipAddress));
    napi_value channelType = nullptr;
    NAPI_CALL(env, napi_create_int32(env, static_cast<int32_t>(castRemoteDevice.channelType), &channelType));
    NAPI_CALL(env, napi_set_named_property(env, result, "channelType", channelType));
    napi_value networkId = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, castRemoteDevice.networkId.c_str(), NAPI_AUTO_LENGTH, &networkId));
    NAPI_CALL(env, napi_set_named_property(env, result, "networkId", networkId));
    napi_value isLegacy = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, castRemoteDevice.isLegacy, &isLegacy));
    NAPI_CALL(env, napi_set_named_property(env, result, "isLegacy", isLegacy));
    napi_value mediumTypes = nullptr;
    NAPI_CALL(env, napi_create_int32(env, static_cast<int32_t>(castRemoteDevice.mediumTypes), &mediumTypes));
    NAPI_CALL(env, napi_set_named_property(env, result, "mediumTypes", mediumTypes));
    napi_value protocolCapabilities = nullptr;
    NAPI_CALL(env,
        napi_create_int32(env, static_cast<int32_t>(castRemoteDevice.protocolCapabilities), &protocolCapabilities));
    NAPI_CALL(env, napi_set_named_property(env, result, "protocolCapabilities", protocolCapabilities));
    return result;
}

string JsObjectToString(napi_env env, napi_value &object, const char *fieldStr)
{
    string fieldRef {};
    if (object == nullptr) {
        CLOGE("args is nullptr");
        return fieldRef;
    }
    napi_value field = nullptr;
    bool hasProperty = false;

    napi_status status = napi_has_named_property(env, object, fieldStr, &hasProperty);
    if (status == napi_ok && hasProperty) {
        status = napi_get_named_property(env, object, fieldStr, &field);
        if (status != napi_ok) {
            CLOGE("napi_get_named_property failed.");
            return fieldRef;
        }
        fieldRef = ParseString(env, field);
        return fieldRef;
    } else {
        CLOGE("Js obj to str no property: %{public}s", fieldStr);
    }
    return fieldRef;
}

int32_t JsObjectToInt32(napi_env env, napi_value &object, const char *fieldStr)
{
    int32_t fieldRef = 0;
    if (object == nullptr) {
        CLOGE("args is nullptr");
        return fieldRef;
    }
    bool hasProperty = false;
    napi_status status = napi_has_named_property(env, object, fieldStr, &hasProperty);
    if (status == napi_ok && hasProperty) {
        napi_value field;
        napi_valuetype valueType;
        status = napi_get_named_property(env, object, fieldStr, &field);
        if (status != napi_ok) {
            CLOGE("napi_get_named_property failed.");
            return fieldRef;
        }
        status = napi_typeof(env, field, &valueType);
        if (status != napi_ok || valueType != napi_number) {
            CLOGE("Wrong argument type. Number expected.");
            return fieldRef;
        }
        status = napi_get_value_int32(env, field, &fieldRef);
        if (status != napi_ok) {
            CLOGE("napi_get_value_int32 failed");
            return fieldRef;
        }
        return fieldRef;
    } else {
        CLOGE("Js to int32_t no property: %{public}s", fieldStr);
    }
    return fieldRef;
}

bool JsObjectToBool(napi_env env, napi_value &object, const char *fieldStr)
{
    bool fieldRef = false;
    if (object == nullptr) {
        CLOGE("args is nullptr");
        return fieldRef;
    }
    bool hasProperty = false;
    napi_status status = napi_has_named_property(env, object, fieldStr, &hasProperty);
    if (status == napi_ok && hasProperty) {
        napi_value field;
        napi_valuetype valueType;
        status = napi_get_named_property(env, object, fieldStr, &field);
        if (status != napi_ok) {
            CLOGE("napi_get_named_property failed.");
            return fieldRef;
        }
        status = napi_typeof(env, field, &valueType);
        if (status != napi_ok || valueType != napi_boolean) {
            CLOGE("Wrong argument type. Bool expected.");
            return fieldRef;
        }
        status = napi_get_value_bool(env, field, &fieldRef);
        if (status != napi_ok) {
            CLOGE("napi_get_value_bool failed");
            return fieldRef;
        }
        return fieldRef;
    } else {
        CLOGE("Js to bool no property: %{public}s", fieldStr);
    }
    return fieldRef;
}

uint32_t JsObjectToUint32(napi_env env, napi_value &object, const char *fieldStr)
{
    uint32_t fieldRef = 0;
    if (object == nullptr) {
        CLOGE("args is nullptr");
        return fieldRef;
    }
    bool hasProperty = false;
    napi_status status = napi_has_named_property(env, object, fieldStr, &hasProperty);
    if (status == napi_ok && hasProperty) {
        napi_value field;
        napi_valuetype valueType;
        status = napi_get_named_property(env, object, fieldStr, &field);
        if (status != napi_ok) {
            CLOGE("napi_get_named_property failed.");
            return fieldRef;
        }
        status = napi_typeof(env, field, &valueType);
        if (status != napi_ok || valueType != napi_number) {
            CLOGE("Wrong argument type. Number expected.");
            return fieldRef;
        }
        status = napi_get_value_uint32(env, field, &fieldRef);
        if (status != napi_ok) {
            CLOGE("napi_get_value_uint32 failed");
            return fieldRef;
        }
        return fieldRef;
    } else {
        CLOGE("Js to uint32_t no property: %{public}s", fieldStr);
    }
    return fieldRef;
}

double JsObjectToDouble(napi_env env, napi_value &object, const char *fieldStr)
{
    double fieldRef = 0;
    if (object == nullptr) {
        CLOGE("args is nullptr");
        return fieldRef;
    }
    bool hasProperty = false;
    napi_status status = napi_has_named_property(env, object, fieldStr, &hasProperty);
    if (status == napi_ok && hasProperty) {
        napi_value field;
        napi_valuetype valueType;
        status = napi_get_named_property(env, object, fieldStr, &field);
        if (status != napi_ok) {
            CLOGE("napi_get_named_property failed.");
            return fieldRef;
        }
        status = napi_typeof(env, field, &valueType);
        if (status != napi_ok || valueType != napi_number) {
            CLOGE("Wrong argument type. Number expected.");
            return fieldRef;
        }
        status = napi_get_value_double(env, field, &fieldRef);
        if (status != napi_ok) {
            CLOGE("napi_get_value_double failed");
            return fieldRef;
        }
        return fieldRef;
    } else {
        CLOGE("Js to double no property: %{public}s", fieldStr);
    }
    return fieldRef;
}

int64_t JsObjectToInt64(napi_env env, napi_value &object, const char *fieldStr)
{
    int64_t fieldRef = 0;
    if (object == nullptr) {
        CLOGE("args is nullptr");
        return fieldRef;
    }
    bool hasProperty = false;
    napi_status status = napi_has_named_property(env, object, fieldStr, &hasProperty);
    if (status == napi_ok && hasProperty) {
        napi_value field;
        napi_valuetype valueType;
        status = napi_get_named_property(env, object, fieldStr, &field);
        if (status != napi_ok) {
            CLOGE("napi_get_named_property failed");
            return fieldRef;
        }
        status = napi_typeof(env, field, &valueType);
        if (status != napi_ok || valueType != napi_number) {
            CLOGE("Wrong argument type. Number expected.");
            return fieldRef;
        }
        status = napi_get_value_int64(env, field, &fieldRef);
        if (status != napi_ok) {
            CLOGE("napi_get_value_int64 failed");
            return fieldRef;
        }
        return fieldRef;
    } else {
        CLOGE("Js to int64_t no property: %{public}s", fieldStr);
    }
    return fieldRef;
}

napi_value ConvertMediaInfoToJS(napi_env env, const MediaInfo &mediaInfo)
{
    CLOGD("ConvertMediaInfoToJS start");
    napi_value result = nullptr;
    napi_value value = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    NAPI_CALL(env, napi_create_string_utf8(env, mediaInfo.mediaId.c_str(), NAPI_AUTO_LENGTH, &value));
    NAPI_CALL(env, napi_set_named_property(env, result, "mediaId", value));
    NAPI_CALL(env, napi_create_string_utf8(env, mediaInfo.mediaName.c_str(), NAPI_AUTO_LENGTH, &value));
    NAPI_CALL(env, napi_set_named_property(env, result, "mediaName", value));
    NAPI_CALL(env, napi_create_string_utf8(env, mediaInfo.mediaUrl.c_str(), NAPI_AUTO_LENGTH, &value));
    NAPI_CALL(env, napi_set_named_property(env, result, "mediaUrl", value));
    NAPI_CALL(env, napi_create_string_utf8(env, mediaInfo.mediaType.c_str(), NAPI_AUTO_LENGTH, &value));
    NAPI_CALL(env, napi_set_named_property(env, result, "mediaType", value));
    NAPI_CALL(env, napi_create_string_utf8(env, mediaInfo.albumCoverUrl.c_str(), NAPI_AUTO_LENGTH, &value));
    NAPI_CALL(env, napi_set_named_property(env, result, "albumCoverUrl", value));
    NAPI_CALL(env, napi_create_string_utf8(env, mediaInfo.albumTitle.c_str(), NAPI_AUTO_LENGTH, &value));
    NAPI_CALL(env, napi_set_named_property(env, result, "albumTitle", value));
    NAPI_CALL(env, napi_create_string_utf8(env, mediaInfo.mediaArtist.c_str(), NAPI_AUTO_LENGTH, &value));
    NAPI_CALL(env, napi_set_named_property(env, result, "mediaArtist", value));
    NAPI_CALL(env, napi_create_string_utf8(env, mediaInfo.lrcUrl.c_str(), NAPI_AUTO_LENGTH, &value));
    NAPI_CALL(env, napi_set_named_property(env, result, "lrcUrl", value));
    NAPI_CALL(env, napi_create_string_utf8(env, mediaInfo.lrcContent.c_str(), NAPI_AUTO_LENGTH, &value));
    NAPI_CALL(env, napi_set_named_property(env, result, "lrcContent", value));
    NAPI_CALL(env, napi_create_string_utf8(env, mediaInfo.appIconUrl.c_str(), NAPI_AUTO_LENGTH, &value));
    NAPI_CALL(env, napi_set_named_property(env, result, "appIconUrl", value));
    NAPI_CALL(env, napi_create_string_utf8(env, mediaInfo.appName.c_str(), NAPI_AUTO_LENGTH, &value));
    NAPI_CALL(env, napi_set_named_property(env, result, "appName", value));

    NAPI_CALL(env, napi_create_uint32(env, mediaInfo.mediaSize, &value));
    NAPI_CALL(env, napi_set_named_property(env, result, "mediaSize", value));
    NAPI_CALL(env, napi_create_uint32(env, mediaInfo.startPosition, &value));
    NAPI_CALL(env, napi_set_named_property(env, result, "startPosition", value));
    NAPI_CALL(env, napi_create_uint32(env, mediaInfo.duration, &value));
    NAPI_CALL(env, napi_set_named_property(env, result, "duration", value));
    NAPI_CALL(env, napi_create_uint32(env, mediaInfo.closingCreditsPosition, &value));
    NAPI_CALL(env, napi_set_named_property(env, result, "closingCreditsPosition", value));
    CLOGD("ConvertMediaInfoToJS end");

    return result;
}

napi_value ConvertMediaInfoHolderToJS(napi_env env, const MediaInfoHolder &mediaInfoHolder)
{
    CLOGD("ConvertMediaInfoHolderToJS start");
    napi_value result = nullptr;
    napi_value value = nullptr;
    size_t len = mediaInfoHolder.mediaInfoList.size();
    if (len == 0) {
        CLOGE("mediaInfoList len is invalid");
        return result;
    }
    NAPI_CALL(env, napi_create_object(env, &result));
    NAPI_CALL(env, napi_create_uint32(env, mediaInfoHolder.currentIndex, &value));
    NAPI_CALL(env, napi_set_named_property(env, result, "currentIndex", value));
    NAPI_CALL(env, napi_create_uint32(env, mediaInfoHolder.progressRefreshInterval, &value));
    NAPI_CALL(env, napi_set_named_property(env, result, "progressRefreshInterval", value));
    NAPI_CALL(env, napi_create_array_with_length(env, len, &value));
    for (size_t i = 0; i < len; i++) {
        napi_value mediaInfo = ConvertMediaInfoToJS(env, mediaInfoHolder.mediaInfoList[i]);
        NAPI_CALL(env, napi_set_element(env, value, i, mediaInfo));
    }
    NAPI_CALL(env, napi_set_named_property(env, result, "mediaInfoList", value));
    CLOGD("ConvertMediaInfoHolderToJS end");
    return result;
}

void CallJSFunc(napi_env env, napi_ref func, size_t argc, napi_value argv[])
{
    napi_value callback = nullptr;
    napi_value undefined = nullptr;
    napi_value callResult = nullptr;
    if (napi_get_undefined(env, &undefined) != napi_ok) {
        CLOGE("napi_get_undefined failed");
        return;
    }
    if (napi_get_reference_value(env, func, &callback) != napi_ok) {
        CLOGE("napi_get_reference_value failed");
        return;
    }
    if (napi_call_function(env, undefined, callback, argc, argv, &callResult) != napi_ok) {
        CLOGE("napi_call_function failed");
    }
}

bool GetJSFuncParams(napi_env env, napi_callback_info info, napi_value argv[], size_t expectedArgc,
    napi_valuetype expectedTypes[])
{
    napi_value thisVar = nullptr;
    size_t argc = expectedArgc;
    napi_status status = napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    if (status != napi_ok || argc != expectedArgc) {
        CLOGE("napi_get_cb_info failed");
        return false;
    }

    for (size_t i = 0; i < expectedArgc; i++) {
        napi_valuetype valueType = napi_undefined;
        if (napi_typeof(env, argv[i], &valueType) != napi_ok) {
            CLOGE("napi_typeof failed");
            return false;
        }
        if (valueType != expectedTypes[i]) {
            CLOGE("Wrong argument type. type:%d expected", expectedTypes[i]);
            return false;
        }
    }

    return true;
}

bool CheckJSParamsType(napi_env env, napi_value argv[], size_t expectedArgc, napi_valuetype expectedTypes[])
{
    for (size_t i = 0; i < expectedArgc; i++) {
        napi_valuetype valueType = napi_undefined;
        if (napi_typeof(env, argv[i], &valueType) != napi_ok) {
            CLOGE("napi_typeof failed");
            return false;
        }
        if (valueType != expectedTypes[i]) {
            CLOGE("Wrong argument type. type:%d expected", expectedTypes[i]);
            return false;
        }
    }

    return true;
}
} // namespace CastEngineClient
} // namespace CastEngine
} // namespace OHOS
