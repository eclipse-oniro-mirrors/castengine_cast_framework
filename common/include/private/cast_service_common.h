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
 * Description: Cast engine service related common data stucture definitions.
 * Author: zhangge
 * Create: 2022-10-26
 */

#ifndef CAST_SERVICE_COMMON_H
#define CAST_SERVICE_COMMON_H

#include <string>

#include "cast_engine_common.h"

namespace OHOS {
namespace CastEngine {
struct CastInnerRemoteDevice {
    std::string deviceId;
    std::string deviceName;
    DeviceType deviceType{ DeviceType::DEVICE_OTHERS };
    int deviceTypeId = 0;
    SubDeviceType subDeviceType{ SubDeviceType::SUB_DEVICE_DEFAULT };
    std::string ipAddress;
    TriggerType triggerType{ TriggerType::UNSPEC_TAG };
    std::string authData;
    int sessionId{ INVALID_ID };
    ChannelType channelType{ ChannelType::SOFT_BUS };
    uint8_t sessionKey[16] = { 0 };
    uint32_t sessionKeyLength{ 0 };
    std::string bleMac;
    std::string localWifiIp;
    std::string wifiIp;
    uint16_t wifiPort = 0;
    std::string customData;
    int rtspPort{ INVALID_PORT };
    ProtocolType protocolType;
    std::string udid;

    std::string localIp;
    std::string remoteIp;
    std::string networkId;

    bool operator==(const CastInnerRemoteDevice &rhs) const
    {
        return deviceId == rhs.deviceId && deviceName == rhs.deviceName && bleMac == rhs.bleMac
               && wifiIp == rhs.wifiIp && wifiPort == rhs.wifiPort && customData == rhs.customData;
    }

    bool operator!=(const CastInnerRemoteDevice &rhs) const
    {
        return !(rhs == *this);
    }
};

inline constexpr char PKG_NAME[] = "CastEngineService";
} // namespace CastEngine
} // namespace OHOS

namespace std {
using namespace OHOS::CastEngine;
template<>
struct hash<CastInnerRemoteDevice> {
    std::size_t operator()(const CastInnerRemoteDevice &remoteDevice) const
    {
        return std::hash<std::string>()(remoteDevice.deviceId)
                ^ std::hash<std::string>()(remoteDevice.deviceName)
                ^ std::hash<std::string>()(remoteDevice.bleMac)
                ^ std::hash<std::string>()(remoteDevice.wifiIp)
                ^ std::hash<uint16_t>()(remoteDevice.wifiPort)
                ^ std::hash<std::string>()(remoteDevice.customData);
    }
};
}
#endif
