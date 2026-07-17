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
 * Description: implement the cast source discovery
 * Author: zhangge
 * Create: 2022-08-23
 */
#include <ipc_skeleton.h>
#include <algorithm>
#include <thread>

#include "discovery_manager.h"
#include "connection_manager.h"
#include "cast_device_data_manager.h"
#include "cast_engine_dfx.h"
#include "cast_engine_log.h"
#include "cast_meta_node_constant.h"
#include "dm_constants.h"
#include "parameters.h"
#include "securec.h"
#include "softbus_common.h"
#include "utils.h"
#include "json.hpp"

using namespace OHOS::DistributedHardware;
using namespace OHOS::AppExecFwk;

using nlohmann::json;

namespace OHOS {
namespace CastEngine {
namespace CastEngineService {
DEFINE_CAST_ENGINE_LABEL("Cast-Discovery-Manager");

namespace {
constexpr int AV_SESSION_UID = 6700;
constexpr int REMOTE_DIED_SLEEP = 4000;
constexpr int DISCOVERY_DELAY_TIME = 3000;
constexpr int TIMEOUT_COUNT = 40;
constexpr int EVENT_START_DISCOVERY = 1;
constexpr int EVENT_CONTINUE_DISCOVERY = 2;
const std::string DISCOVERY_TRUST_VALUE = R"({"filters": [{"type": "isTrusted", "value": 2}]})";
const std::string SINGLE_CUST_DATA = R"({"castPlus":"C020"})";
constexpr int CAST_DATA_LENGTH = 4;

constexpr int INT_THREE = 3;
constexpr int INT_FOUR = 4;
constexpr int INT_SEVEN = 7;

// Bit bit of byte 0
constexpr uint32_t BASE = 1;
constexpr uint32_t MIRROR_CAPABILITY = BASE << 3;

DeviceType ConvertDeviceType(uint16_t deviceTypeId)
{
    switch (deviceTypeId) {
        case DEVICE_TYPE_TV:
            return DeviceType::DEVICE_HW_TV;
        case DEVICE_TYPE_PAD:
            return DeviceType::DEVICE_PAD;
        case DEVICE_TYPE_CAR:
            return DeviceType::DEVICE_HICAR;
        case DEVICE_TYPE_2IN1:
            return DeviceType::DEVICE_TYPE_2IN1;
        default:
            return DeviceType::DEVICE_CAST_PLUS;
    }
}
} // namespace

void CastDmInitCallback::OnRemoteDied()
{
    CLOGE("DM is dead, deinit the DiscoveryManager");
    std::thread([]() {
        Utils::SetThreadName("DmOnRemoteDied");
        std::this_thread::sleep_for(std::chrono::milliseconds(REMOTE_DIED_SLEEP));
        constexpr int sleepTime = 100;       // uint: ms
        constexpr int retryTimes = 10 * 10;  // total 10s
        for (int32_t retryTime = 0; retryTime < retryTimes; ++retryTime) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
            if (DeviceManager::GetInstance().InitDeviceManager(PKG_NAME,
                std::make_shared<CastDmInitCallback>()) == DM_OK) {
                CLOGI("DeviceManager onDied, try to init success, retryTime = %d", retryTime);
                return;
            }
            CLOGI("DeviceManager onDied, try to init fail, retryTime = %d", retryTime);
        }
        CLOGI("DeviceManager onDied, try to init has reached the maximum, but still failed");
        DiscoveryManager::GetInstance().Deinit();
        return;
    }).detach();
}

DiscoveryManager &DiscoveryManager::GetInstance()
{
    static DiscoveryManager instance{};
    return instance;
}

void DiscoveryManager::Init(std::shared_ptr<IDiscoveryManagerListener> listener)
{
    CLOGD("init start");
    eventRunner_ = EventRunner::Create(false);
    eventHandler_ = std::make_shared<DiscoveryEventHandler>(eventRunner_);
    if (listener == nullptr) {
        CLOGE("The input listener is null!");
        return;
    }

    if (HasListener()) {
        CLOGE("Already inited");
        return;
    }

    if (DeviceManager::GetInstance().InitDeviceManager(PKG_NAME, std::make_shared<CastDmInitCallback>()) != DM_OK) {
        CLOGE("Failed to InitDeviceManager");
        return;
    }

    SetListener(listener);
    CLOGD("init done");
}

void DiscoveryManager::Deinit()
{
    if (!HasListener()) {
        return;
    }

    StopDiscovery();
    ResetListener();
    DeviceManager::GetInstance().UnInitDeviceManager(PKG_NAME);
}

void DiscoveryManager::StartDiscovery(int protocols, std::vector<std::string> drmSchemes)
{
    HiSysEventWriteWrap(__func__, {
            {"BIZ_SCENE", static_cast<int32_t>(BIZSceneType::DEVICE_DISCOVERY)},
            {"BIZ_STATE", static_cast<int32_t>(BIZStateType::BIZ_STATE_BEGIN)},
            {"BIZ_STAGE", static_cast<int32_t>(BIZSceneStage::START_DISCOVERY)},
            {"STAGE_RES", static_cast<int32_t>(StageResType::STAGE_RES_IDLE)},
            {"ERROR_CODE", CAST_RADAR_SUCCESS}}, {
            {"TO_CALL_PKG", ""},
            {"LOCAL_SESS_NAME", ""},
            {"PEER_SESS_NAME", ""},
            {"PEER_UDID", ""}});

    CLOGI("StartDiscovery in");
    protocolType_ = protocols;
    drmSchemes_ = drmSchemes;
    CastLocalDevice localDevice;
    ConnectionManager::GetInstance().GetLocalDeviceInfo(localDevice);
    std::lock_guard<std::mutex> lock(mutex_);
    if (eventHandler_ == nullptr) {
        CLOGE("Event handler is null!");
        return;
    }
    std::thread([this]() {
        Utils::SetThreadName("DiscoveryEventRunner");
        if (eventRunner_ != nullptr) {
            eventRunner_->Run();
        }
    }).detach();
    scanCount_ = 0;
    for (auto it = remoteDeviceMap_.begin(); it != remoteDeviceMap_.end();) {
        std::string deviceId = it->first.deviceId;
        CastDeviceDataManager::GetInstance().SetDeviceNotFresh(deviceId);
        it++;
    }
    remoteDeviceMap_.clear();
    std::string connectDeviceId = ConnectionManager::GetInstance().GetConnectingDeviceId();
    if (!connectDeviceId.empty()) {
        auto device =  CastDeviceDataManager::GetInstance().GetDeviceByDeviceId(connectDeviceId);
        if (device != std::nullopt) {
            device->deviceName = "";
            remoteDeviceMap_[*device] = scanCount_ + 1;
        }
    }
    uid_ = IPCSkeleton::GetCallingUid();
    hasStartDiscovery_.store(true);
    eventHandler_->RemoveEvent(EVENT_START_DISCOVERY);
    eventHandler_->SendEvent(EVENT_START_DISCOVERY);
    CLOGI("StartDiscovery out");
}

void DiscoveryManager::StopDiscovery()
{
    CLOGI("StopDiscovery in");
    hasStartDiscovery_.store(false);
    SetDeviceNotFresh();
    eventHandler_->RemoveAllEvents();
    if (eventRunner_ != nullptr) {
        eventRunner_->Stop();
    }

    StopDmDiscovery();
}

void DiscoveryManager::StartDmDiscovery()
{
    CLOGI("StartDmDiscovery in");
    {
        std::lock_guard<std::mutex> lock(mutex_);
        UpdateDeviceStateLocked();
        reportTypeMap_.clear();
    }
    scanCount_++;
    std::map<std::string, std::string> discoverParam{
        { PARAM_KEY_META_TYPE, std::to_string(5) } };
    std::map<std::string, std::string> filterOptions{
        { PARAM_KEY_FILTER_OPTIONS, DISCOVERY_TRUST_VALUE } };
    int32_t ret = DeviceManager::GetInstance().StartDiscovering(PKG_NAME, discoverParam, filterOptions,
        std::make_shared<CastDiscoveryCallback>());
    if (ret != DM_OK && ret != ERR_DM_DISCOVERY_REPEATED) {
        CLOGE("Failed to start discovery, ret:%d", ret);
        CastEngineDfx::WriteErrorEvent(START_DISCOVERY_FAIL);
    }
}

bool DiscoveryManager::StartAdvertise()
{
    CLOGI("PublishDiscovery in");
    std::map<std::string, std::string> advertiseParam { { PARAM_KEY_DISC_CAPABILITY, DM_CAPABILITY_CASTPLUS } };
    int ret = DeviceManager::GetInstance().StartAdvertising(PKG_NAME, advertiseParam,
        std::make_shared<CastPublishDiscoveryCallback>());
    if (ret != DM_OK) {
        CLOGE("Failed to publish discovery, ret:%{public}d", ret);
        return false;
    }
    return true;
}

bool DiscoveryManager::StopAdvertise()
{
    CLOGI("UnPublishDiscoveryv in");
    std::map<std::string, std::string> advertiseParam = {};
    int ret = DeviceManager::GetInstance().StopAdvertising(PKG_NAME, advertiseParam);
    if (ret != DM_OK) {
        CLOGE("Failed to unpublish discovery, ret:%{public}d", ret);
        return false;
    }
    return true;
}

int DiscoveryManager::GetProtocolType() const
{
    return protocolType_;
}

// {"BLE_MAC":"74:41:3d:f3:3d:cd","BLE_UDID_HASH":"xxxxxx","CONN_ADDR_TYPE":"BLE_TYPE","CUSTOM_DATA":""}
// {"CONN_ADDR_TYPE":"WLAN_IP_TYPE","CUSTOM_DATA":"{\"castPlus\":\"\"}","WIFI_IP":"192.168.135.204","WIFI_PORT":36767}
void DiscoveryManager::OnDeviceInfoFound(uint16_t subscribeId, const DmDeviceInfo &dmDeviceInfo)
{
    CLOGD("OnDeviceInfoFound in deviceName: %{public}s, deviceId: %{public}s, extra: %{public}s",
          dmDeviceInfo.deviceName, dmDeviceInfo.deviceId, dmDeviceInfo.extraData.c_str());
    CastInnerRemoteDevice newDevice = CreateRemoteDevice(dmDeviceInfo);

    // If the map does not exist, the notification is sent.
    bool isDeviceExist = true;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = remoteDeviceMap_.find(newDevice);
        if (it == remoteDeviceMap_.end()) {
            // If the deviceId are the same, delete the old device.
            RemoveSameDeviceLocked(newDevice);
            isDeviceExist = false;
        }
    }

    if (!isDeviceExist) {
        // Set subDeviceType based on isTrusted.
        std::string networkId = dmDeviceInfo.networkId;
        bool isTrusted = ConnectionManager::GetInstance().IsDeviceTrusted(dmDeviceInfo.deviceId, networkId);
        if (!isTrusted) {
            newDevice.subDeviceType = SubDeviceType::SUB_DEVICE_MATEBOOK_PAD;
        }

        if (CastDeviceDataManager::GetInstance().AddDevice(newDevice, dmDeviceInfo)) {
            NotifyDeviceIsFound(newDevice);
        }
    }

    std::lock_guard<std::mutex> lock(mutex_);
    remoteDeviceMap_[newDevice] = scanCount_;
    RecordDeviceFoundType(dmDeviceInfo);
}

void DiscoveryManager::NotifyDeviceIsFound(const CastInnerRemoteDevice &newDevice)
{
    std::vector<CastInnerRemoteDevice> devices;
    std::vector<CastInnerRemoteDevice> device2In1;

    if (newDevice.deviceType == DeviceType::DEVICE_TYPE_2IN1) {
        CLOGI("device type is 2IN1");
        device2In1.push_back(newDevice);
    }
    devices.push_back(newDevice);

    isNotifyDevice_ = system::GetBoolParameter(NOTIFY_DEVICE_FOUND, false);
    auto listener = GetListener();
    if (listener == nullptr) {
        CLOGE("listener is null");
        return;
    }

    if (IsDrmMatch(newDevice)) {
        listener->OnDeviceFound(devices);
    }
    
    if (uid_ == AV_SESSION_UID) {
        if (IsDrmMatch(newDevice)) {
            listener->OnDeviceFound(device2In1);
        }

        return;
    }
}

void DiscoveryManager::NotifyDeviceIsOnline(const DmDeviceInfo &dmDeviceInfo)
{
    CastInnerRemoteDevice newDevice = CreateRemoteDevice(dmDeviceInfo);
    NotifyDeviceIsFound(newDevice);
}

void DiscoveryManager::StopDmDiscovery()
{
    CLOGI("StopDmDiscovery in");
    std::map<std::string, std::string> discoverParam = {};
    DeviceManager::GetInstance().StopDiscovering(PKG_NAME, discoverParam);
}

void DiscoveryManager::GetAndReportTrustedDevices()
{
    std::vector<DmDeviceInfo> dmDevices;
    auto result = DeviceManager::GetInstance().GetTrustedDeviceList(PKG_NAME, "", true, dmDevices);
    if (result != DM_OK || dmDevices.size() == 0) {
        CLOGW("No trusted devices, result:%d", result);
        return;
    }
    for (const auto &dmDevice : dmDevices) {
        CLOGI("GetAndReportTrustedDevices, device id is %{public}s, device name is %s",
            Mask(dmDevice.deviceId).c_str(), dmDevice.deviceName);
        CastInnerRemoteDevice newDevice = CreateRemoteDevice(dmDevice);
        NotifyDeviceIsFound(newDevice);
    }
}

void DiscoveryManager::ParseDeviceInfo(const DmDeviceInfo &dmDevice, CastInnerRemoteDevice &castDevice)
{
    CLOGD("dm device extraData parse, %s", dmDevice.extraData.c_str());

    std::pair<std::string, std::string> ret = CastDeviceDataManager::GetInstance()
        .GetDeviceNameByDeviceId(dmDevice.deviceId);
    std::string deviceName = ret.first.empty() ? "" : ret.first;
    std::string discoveryType = ret.second.empty() ? "" : ret.second;
    json jsonObj = json::parse(dmDevice.extraData, nullptr, false);
    if (jsonObj.is_discarded()) {
        CLOGE("dm device extraData parse error, %s", dmDevice.extraData.c_str());
        return;
    }

    // 获取解析的数据
    if (jsonObj.contains(PARAM_KEY_WIFI_IP) && jsonObj[PARAM_KEY_WIFI_IP].is_string()) {
        castDevice.wifiIp = jsonObj[PARAM_KEY_WIFI_IP];
        castDevice.deviceName = !castDevice.deviceName.empty() ? castDevice.deviceName : deviceName;
        castDevice.remoteIp = jsonObj[PARAM_KEY_WIFI_IP];
        castDevice.localWifiIp = Utils::GetWifiIp();
        castDevice.localIp = castDevice.localWifiIp;
        castDevice.isWifiFresh = true;
        castDevice.mediumTypes |= static_cast<uint32_t>(NotifyMediumType::COAP);
    }

    if (jsonObj.contains(PARAM_KEY_WIFI_PORT) && jsonObj[PARAM_KEY_WIFI_PORT].is_number()) {
        castDevice.wifiPort = jsonObj[PARAM_KEY_WIFI_PORT];
        castDevice.deviceName = !castDevice.deviceName.empty() ? castDevice.deviceName : deviceName;
    }

    if (jsonObj.contains(PARAM_KEY_BLE_MAC) && jsonObj[PARAM_KEY_BLE_MAC].is_string()) {
        castDevice.bleMac = jsonObj[PARAM_KEY_BLE_MAC];
        if (discoveryType == "WIFI") {
            castDevice.deviceName = !deviceName.empty() ? deviceName : castDevice.deviceName;
        }
        castDevice.mediumTypes |= static_cast<uint32_t>(NotifyMediumType::BLE);
        castDevice.isBleFresh = true;
    }

    CLOGI("name %{public}s, %{public}s, ip %{public}s, ble %{public}s, wifiFresh %{public}d, bleFresh %{public}d"
        " discoveryType:%{public}s, capability %{public}d, mediumTypes %{public}u",
        Mask(castDevice.deviceName).c_str(), Mask(castDevice.deviceId).c_str(), Mask(castDevice.wifiIp).c_str(),
        Mask(castDevice.bleMac).c_str(), castDevice.isWifiFresh, castDevice.isBleFresh, discoveryType.c_str(),
        castDevice.capability, castDevice.mediumTypes);
    
    ParseCustomData(jsonObj, castDevice);
}

void DiscoveryManager::ParseCustomData(const json &jsonObj, CastInnerRemoteDevice &castDevice)
{
    if (jsonObj.contains(PARAM_KEY_CUSTOM_DATA) && jsonObj[PARAM_KEY_CUSTOM_DATA].is_string()) {
        std::string customData = jsonObj[PARAM_KEY_CUSTOM_DATA];
        json softbusCustData = json::parse(customData, nullptr, false);
        if (!softbusCustData.is_discarded() && softbusCustData.contains("castPlus")
            && softbusCustData["castPlus"].is_string()) {
            std::string castData = softbusCustData["castPlus"];
            castDevice.customData = castData;
            ParseCapability(castDevice.customData, castDevice);
        }
        if (!softbusCustData.is_discarded() && softbusCustData.contains("castId")
            && softbusCustData["castId"].is_string()) {
            castDevice.udid = softbusCustData["castId"];
        }
    }
}

void DiscoveryManager::ParseCapability(const std::string castData, CastInnerRemoteDevice &newDevice)
{
    auto firstByte = static_cast<uint32_t>(Utils::StringToInt(castData.substr(0, 2), 16));
    if ((firstByte & MIRROR_CAPABILITY) != 0) {
        newDevice.capabilityInfo |= static_cast<uint32_t>(ProtocolType::CAST_PLUS_MIRROR);
    }

    if ((newDevice.capabilityInfo & static_cast<uint32_t>(ProtocolType::CAST_PLUS_STREAM)) != 0) {
        if (newDevice.capability == CapabilityType::DLNA) {
            newDevice.capability = CapabilityType::CAST_AND_DLNA;
        } else if (newDevice.capability != CapabilityType::CAST_AND_DLNA) {
            newDevice.capability = CapabilityType::CAST_PLUS;
        }
    }

    CLOGI("device %{public}s castData:%{public}s authVersion:%{public}s capabilityInfo:%{public}u drmCapa:%{public}d"
        "capability:0x%{public}x",
        Mask(newDevice.deviceName).c_str(), castData.c_str(), newDevice.authVersion.c_str(), newDevice.capabilityInfo,
        (newDevice.drmCapabilities.size() > 0), newDevice.capability);
}

void DiscoveryManager::SetListener(std::shared_ptr<IDiscoveryManagerListener> listener)
{
    std::lock_guard<std::mutex> lock(mutex_);
    listener_ = listener;
}

std::shared_ptr<IDiscoveryManagerListener> DiscoveryManager::GetListener()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return listener_;
}

bool DiscoveryManager::HasListener()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return listener_ != nullptr;
}

void DiscoveryManager::ResetListener()
{
    SetListener(nullptr);
}

void DiscoveryManager::RemoveSameDeviceLocked(const CastInnerRemoteDevice &newDevice)
{
    auto it = std::find_if(remoteDeviceMap_.begin(), remoteDeviceMap_.end(), [&](const auto& pair) {
        return pair.first.deviceId == newDevice.deviceId;
    });
    if (it != remoteDeviceMap_.end()) {
        remoteDeviceMap_.erase(it);
    }
}

CastInnerRemoteDevice DiscoveryManager::CreateRemoteDevice(const DmDeviceInfo &dmDeviceInfo)
{
    auto device = CastDeviceDataManager::GetInstance().GetDeviceByDeviceId(dmDeviceInfo.deviceId);
    CastInnerRemoteDevice newDevice;
    if (device != std::nullopt) {
        newDevice = *device;
    }
    newDevice.deviceId = dmDeviceInfo.deviceId;
    newDevice.deviceName = dmDeviceInfo.deviceName;
    newDevice.deviceType = ConvertDeviceType(dmDeviceInfo.deviceTypeId);
    newDevice.deviceTypeId = dmDeviceInfo.deviceTypeId;
    newDevice.subDeviceType = SubDeviceType::SUB_DEVICE_DEFAULT;
    newDevice.channelType = ChannelType::SOFT_BUS;
    newDevice.networkId = dmDeviceInfo.networkId;
    newDevice.authVersion = AUTH_VERSION_1;
    if (newDevice.deviceType == DeviceType::DEVICE_TYPE_2IN1) {
        newDevice.authVersion = AUTH_VERSION_3;
    }

    ParseDeviceInfo(dmDeviceInfo, newDevice);

    return newDevice;
}

void DiscoveryManager::UpdateDeviceStateLocked()
{
    for (auto it = remoteDeviceMap_.begin(); it != remoteDeviceMap_.end();) {
        if (scanCount_ == it->second) {
            std::string deviceId = it->first.deviceId;
            if (!reportTypeMap_[deviceId].first) {
                CastDeviceDataManager::GetInstance().RemoveDeviceInfo(deviceId, true);
            }
            if (!reportTypeMap_[deviceId].second) {
                CastDeviceDataManager::GetInstance().RemoveDeviceInfo(deviceId, false);
            }
        }
        if (scanCount_ - it->second > 1 &&
            it->first.deviceId != ConnectionManager::GetInstance().GetConnectingDeviceId()) {
            CastInnerRemoteDevice remoteDevice = it->first;
            CLOGE("StartDmDiscovery offline: %{public}s", Mask(remoteDevice.deviceName).c_str());
            ConnectionManager::GetInstance().NotifyDeviceIsOffline(remoteDevice.deviceId);
            CastDeviceDataManager::GetInstance().RemoveDevice(remoteDevice.deviceId);
            it = remoteDeviceMap_.erase(it);
        } else {
            it++;
        }
    }
}

void DiscoveryManager::SetDeviceNotFresh()
{
    CLOGI("in");
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = remoteDeviceMap_.begin(); it != remoteDeviceMap_.end();) {
        std::string deviceId = it->first.deviceId;
        CastDeviceDataManager::GetInstance().SetDeviceNotFresh(deviceId);
        it++;
    }
    CLOGI("out");
}

void DiscoveryManager::RecordDeviceFoundType(const DmDeviceInfo dmDevice)
{
    CLOGI("scanCount_ is %{public}d", scanCount_);
    std::string deviceId = dmDevice.deviceId;
    if (reportTypeMap_.find(deviceId) == reportTypeMap_.end()) {
        reportTypeMap_.insert({ deviceId, {false, false} });
    }
    json jsonObj = json::parse(dmDevice.extraData, nullptr, false);
    if (!jsonObj.is_discarded()) {
        if (jsonObj.contains(PARAM_KEY_WIFI_IP) && jsonObj[PARAM_KEY_WIFI_IP].is_string()) {
            reportTypeMap_[deviceId].first = true;
        }
        if (jsonObj.contains(PARAM_KEY_BLE_MAC) && jsonObj[PARAM_KEY_BLE_MAC].is_string()) {
            reportTypeMap_[deviceId].second = true;
        }
    }
}

std::string DiscoveryManager::Mask(const std::string &str)
{
    if (str.empty() || str.length() <= INT_THREE) {
        return str;
    } else if (str.length() < INT_SEVEN) {
        return str.substr(0, INT_THREE) + "***" + str.substr(str.length() - 1);
    } else {
        return str.substr(0, INT_FOUR) + "***" + str.substr(str.length() - INT_THREE);
    }
}

bool DiscoveryManager::IsDrmMatch(const CastInnerRemoteDevice &newDevice)
{
    if (drmSchemes_.empty()) {
        return true;
    }
    for (auto iter = drmSchemes_.begin(); iter != drmSchemes_.end(); ++iter) {
        if (find(newDevice.drmCapabilities.begin(), newDevice.drmCapabilities.end(), *iter) !=
            newDevice.drmCapabilities.end()) {
            return true;
        }
    }
    return false;
}

bool DiscoveryManager::IsNeedNotify(const CastInnerRemoteDevice &newDevice)
{
    bool isNotifyDevice = system::GetBoolParameter(NOTIFY_DEVICE_FOUND, false);
    if (isNotifyDevice) {
        CLOGD("prop is true, notify %{public}s ", Mask(newDevice.deviceName).c_str());
        return true;
    }

    if (newDevice.deviceType == DeviceType::DEVICE_HW_TV) {
        if (newDevice.customData.empty() || newDevice.customData.length() < CAST_DATA_LENGTH) {
            return false;
        }
    } else {
        CLOGI("%{public}s is not huawei device, notify device", Mask(newDevice.deviceName).c_str());
        return true;
    }

    return true;
}

void CastDiscoveryCallback::OnDiscoverySuccess(uint16_t subscribeId)
{
    CLOGI("OnDiscoverySuccess, subscribe id:%{public}u", subscribeId);
}

void CastDiscoveryCallback::OnDiscoveryFailed(uint16_t subscribeId, int32_t failedReason)
{
    auto errorCode = GetErrorCode(CAST_ENGINE_SYSTEM_ID, CAST_ENGINE_CAST_PLUS_MODULE_ID, subscribeId);
    HiSysEventWriteWrap(__func__, {
            {"BIZ_SCENE", static_cast<int32_t>(BIZSceneType::DEVICE_DISCOVERY)},
            {"BIZ_STATE", static_cast<int32_t>(BIZStateType::BIZ_STATE_END)},
            {"BIZ_STAGE", static_cast<int32_t>(BIZSceneStage::DEVICE_DISCOVERY)},
            {"STAGE_RES", static_cast<int32_t>(StageResType::STAGE_RES_FAILED)},
            {"ERROR_CODE", errorCode}}, {
            {"TO_CALL_PKG", DEVICE_MANAGER_NAME},
            {"LOCAL_SESS_NAME", ""},
            {"PEER_SESS_NAME", ""},
            {"PEER_UDID", ""}});

    CLOGI("OnDiscoveryFailed, subscribe id:%{public}u, reason:%{public}d", subscribeId, failedReason);
}

void CastDiscoveryCallback::OnDeviceFound(uint16_t subscribeId, const DmDeviceInfo &deviceInfo)
{
    CLOGI("OnDeviceInfoFound in, subscribe id:%{public}u, device id:%s, device name:%s", subscribeId,
        deviceInfo.deviceId, deviceInfo.deviceName);
    DiscoveryManager::GetInstance().OnDeviceInfoFound(subscribeId, deviceInfo);
}

void CastPublishDiscoveryCallback::OnPublishResult(int32_t publishId, int32_t publishResult)
{
    CLOGI("OnPublishResult publishId is %{public}d, publishResult is %{public}d", publishId, publishResult);
}

void DiscoveryEventHandler::ProcessEvent(const InnerEvent::Pointer &event)
{
    DiscoveryManager::GetInstance().StopDmDiscovery();
    if (!DiscoveryManager::GetInstance().hasStartDiscovery_.load()) {
        CLOGI("Discovery has been stopped, skip ProcessEvent");
        return;
    }
    auto eventId = event->GetInnerEventId();
    switch (eventId) {
        case EVENT_START_DISCOVERY:
            scanCount = 1;
            SendEvent(EVENT_CONTINUE_DISCOVERY, DISCOVERY_DELAY_TIME);
            break;
        case EVENT_CONTINUE_DISCOVERY:
            scanCount++;
            if (scanCount < TIMEOUT_COUNT) {
                SendEvent(EVENT_CONTINUE_DISCOVERY, DISCOVERY_DELAY_TIME);
            }
            break;
        default:
            break;
    }
    CLOGI("handler run, thread name: %{public}s, scanCount: %{public}d, eventId: %{public}d",
          GetEventRunner()->GetRunnerThreadName().c_str(), scanCount, eventId);
    DiscoveryManager::GetInstance().StartDmDiscovery();
}
} // namespace CastEngineService
} // namespace CastEngine
} // namespace OHOS
