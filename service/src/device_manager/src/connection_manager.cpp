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
 * Description: implement the cast source connect
 * Author: zhangge
 * Create: 2022-08-23
 */

#include "connection_manager.h"

#include <thread>

#include "dm_device_info.h"
#include "dm_constants.h"
#include "json.hpp"
#include "securec.h"

#include "cast_engine_dfx.h"
#include "cast_engine_errors.h"
#include "cast_engine_log.h"
#include "discovery_manager.h"
#include "session.h"
#include "softbus_common.h"
#include "utils.h"
#include "openssl/rand.h"
#include "encrypt_decrypt.h"
#include "utils.h"
#include <iconv.h>
#include "radar_constants.h"

using nlohmann::json;
using namespace OHOS::DistributedHardware;
using OHOS::DistributedHardware::DeviceManager;
using OHOS::DistributedHardware::PeerTargetId;

namespace OHOS {
namespace CastEngine {
namespace CastEngineService {
DEFINE_CAST_ENGINE_LABEL("Cast-Connection-Manager");
namespace {
using namespace OHOS::DistributedHardware;

constexpr int SOFTBUS_OK = 0;

const std::string AUTH_WITH_PIN = "1";

const std::string VERSION_KEY = "version";
const std::string OPERATION_TYPE_KEY = "operType";
const std::string SEQUENCE_NUMBER = "sequenceNumber";
const std::string DATA_KEY = "data";

const std::string DEVICE_ID_KEY = "deviceId";
const std::string DEVICE_NAME_KEY = "deviceName";
const std::string KEY_SESSION_ID = "sessionId";
const std::string PROTOCOL_TYPE_KEY = "protocolType";
const std::string KEY_TRANSFER_MODE = "transferMode";
const std::string DEVICE_CAST_SOURCE = "deviceCastSource";
const std::string PORT_KEY = "port";
const std::string SOURCE_IP_KEY = "sourceIp";
const std::string SINK_IP_KEY = "sinkIp";
const std::string TYPE_SESSION_KEY = "sessionKey";

constexpr int TRANSFER_MODE_SOFTBUS_SINGLE = 2;
constexpr int SESSION_KEY_LENGTH = 16;

const std::string VERSION = "OH1.0";
constexpr int OPERATION_CONSULT = 3;

const std::map<uint16_t, DeviceType> DEVICE_TYPE_CONVERT_MAP = {
    { DEVICE_TYPE_TV, DeviceType::DEVICE_HW_TV },
    { DEVICE_TYPE_CAR, DeviceType::DEVICE_HICAR },
    { DEVICE_TYPE_PAD, DeviceType::DEVICE_MATEBOOK },
    { DEVICE_TYPE_PC, DeviceType::DEVICE_MATEBOOK },
    { DEVICE_TYPE_2IN1, DeviceType::DEVICE_MATEBOOK },
};

const std::map<uint16_t, SubDeviceType> SUB_DEVICE_TYPE_CONVERT_MAP = {
    { DEVICE_TYPE_PAD, SubDeviceType::SUB_DEVICE_MATEBOOK_PAD },
};

DeviceType ConvertDeviceType(uint16_t deviceTypeId)
{
    return DEVICE_TYPE_CONVERT_MAP.count(deviceTypeId) ?
        DEVICE_TYPE_CONVERT_MAP.at(deviceTypeId) : DeviceType::DEVICE_CAST_PLUS;
}

SubDeviceType ConvertSubDeviceType(uint16_t deviceTypeId)
{
    return SUB_DEVICE_TYPE_CONVERT_MAP.count(deviceTypeId) ?
        SUB_DEVICE_TYPE_CONVERT_MAP.at(deviceTypeId) : SubDeviceType::SUB_DEVICE_DEFAULT;
}

/*
 * send to json key auth version, hichain 1.0 or 2.0
 */
const std::string AUTH_VERSION_KEY = "authVersion";
const std::string AUTH_VERSION_1 = "1.0";
const std::string AUTH_VERSION_2 = "2.0";
const std::string AUTH_VERSION_3 = "DM";

const std::string KEY_BIND_TARGET_ACTION = "action";
constexpr int ACTION_CONNECT_DEVICE = 0;
constexpr int ACTION_QUERY_P2P_IP = 1;
constexpr int ACTION_SEND_MESSAGE = 2;

const std::string KEY_LOCAL_P2P_IP = "localP2PIp";
const std::string KEY_REMOTE_P2P_IP = "remoteP2PIp";
const std::string NETWORK_ID = "networkId";

const std::string TIME_RECORD = "TimeRecord";
const std::string TOTAL_AUTH_TIME = "totalAuthTime";
const std::string TIME_RECORD_STRING = "timeRecordString";

void DeviceDiscoveryWriteWrap(const std::string& funcName, const std::string& puid)
{
    HiSysEventWriteWrap(funcName, {
        {"BIZ_SCENE", static_cast<int32_t>(BIZSceneType::DEVICE_DISCOVERY)},
        {"BIZ_STATE", static_cast<int32_t>(BIZStateType::BIZ_STATE_END)},
        {"BIZ_STAGE", static_cast<int32_t>(BIZSceneStage::DEVICE_DISCOVERY)},
        {"STAGE_RES", static_cast<int32_t>(StageResType::STAGE_RES_SUCCESS)},
        {"ERROR_CODE", CAST_RADAR_SUCCESS}}, {
        {"TO_CALL_PKG", DEVICE_MANAGER_NAME},
        {"LOCAL_SESS_NAME", ""},
        {"PEER_SESS_NAME", ""},
        {"PEER_UDID", puid}});
}

void EstablishConsultWriteWrap(const std::string& funcName, int sceneType, const std::string& puid)
{
    HiSysEventWriteWrap(funcName, {
            {"BIZ_SCENE", sceneType},
            {"BIZ_STAGE", static_cast<int32_t>(BIZSceneStage::ESTABLISH_CONSULT_SESSION)},
            {"STAGE_RES", static_cast<int32_t>(StageResType::STAGE_RES_IDLE)},
            {"ERROR_CODE", CAST_RADAR_SUCCESS}}, {
            {"TO_CALL_PKG", DEVICE_MANAGER_NAME},
            {"LOCAL_SESS_NAME", ""},
            {"PEER_SESS_NAME", ""},
            {"PEER_UDID", puid}});
}

} // namespace

namespace SoftBus {
constexpr char PEER_NAME[] = "CastPlusSessionName";
int32_t g_bindSocketId = INVALID_ID;

static void OnShutdown(int32_t socket, ShutdownReason reason)
{
    CLOGI("OnShutdown, socket id = %{public}d", socket);
    auto device = CastDeviceDataManager::GetInstance().GetDeviceByTransId(socket);
    if (device == std::nullopt) {
        CLOGE("Failed to get device by socketId.");
        return;
    }
    CLOGI("notify disconnect %{public}d, deviceId = %{public}s", socket, Utils::Mask(device->deviceId).c_str());
    ConnectionManager::GetInstance().DestroyConsulationSession(device->deviceId);
    ConnectionManager::GetInstance().RemoveTransIdMapping(socket);
}

static void OnBytes(int32_t socket, const void *data, uint32_t dataLen)
{
    CLOGI("OnBytes, socket id = %{public}d", socket);
    ConnectionManager::GetInstance().OnConsultDataReceivedFromSink(socket, data, dataLen);
}

QosTV mirrorQos1st[] = {
    { .qos = QOS_TYPE_MIN_BW,            .value = 80 * 1024 * 1024 }, // 最小带宽80M
    { .qos = QOS_TYPE_MAX_LATENCY,      .value = 16000 }, // 最大建链时延16s
    { .qos = QOS_TYPE_RTT_LEVEL,        .value = QosRttLevel::RTT_LEVEL_LOW },
};

QosTV mirrorQos2nd[] = {
    { .qos = QOS_TYPE_MIN_BW,            .value = 80 * 1024 * 1024 }, // 最小带宽80M
    { .qos = QOS_TYPE_MAX_LATENCY,      .value = 6000 }, // 最大建链时延6s
    { .qos = QOS_TYPE_RTT_LEVEL,        .value = QosRttLevel::RTT_LEVEL_LOW },
};

QosTV singleMirrorQos[] = {
    { .qos = QOS_TYPE_MIN_BW,            .value = 80 * 1024 * 1024 }, // 最小带宽80M
    { .qos = QOS_TYPE_MAX_LATENCY,      .value = 5000 }, // 最大建链时延5s
    { .qos = QOS_TYPE_RTT_LEVEL,        .value = QosRttLevel::RTT_LEVEL_LOW },
};

QosTV streamQos1st[] = {
    { .qos = QOS_TYPE_MIN_BW,            .value = 4 * 1024 * 1024 }, // 最小带宽4M, Wifi优先
    { .qos = QOS_TYPE_MAX_LATENCY,      .value = 16000 }, // 最大建链时延16s
};

QosTV streamQos2nd[] = {
    { .qos = QOS_TYPE_MIN_BW,            .value = 4 * 1024 * 1024 }, // 最小带宽4M, Wifi优先
    { .qos = QOS_TYPE_MAX_LATENCY,      .value = 6000 }, // 最大建链时延6s
};

QosTV singleStreamQos[] = {
    { .qos = QOS_TYPE_MIN_BW,            .value = 80 * 1024 * 1024 }, // 最小带宽80M
    { .qos = QOS_TYPE_MAX_LATENCY,      .value = 5000 }, // 最大建链时延5s
};

ISocketListener listener = {
    .OnShutdown = OnShutdown,
    .OnBytes = OnBytes,
};

int32_t CreateSocket(const std::optional<std::string> &networkId, const ProtocolType &protocolType)
{
    CLOGI("CreateSocket in, protocol type is %{public}d", protocolType);
    SocketInfo messageSessionInfo = {
        .name = (char *)PEER_NAME,
        .peerName = (char *)PEER_NAME,
        .peerNetworkId = (char *)networkId->c_str(),
        .pkgName = (char *)PKG_NAME,
        .dataType = DATA_TYPE_BYTES,
    };

    int32_t socketId = Socket(messageSessionInfo);
    if (socketId <= 0) {
        CLOGE("Create fail socket = %{public}d", socketId);
        return socketId;
    }

    CLOGI("Create socket successfully");
    return socketId;
}

int BindSocket(int32_t socketId, const ProtocolType &protocolType, bool isSingle, int32_t times)
{
    CLOGI("BindSocket in, protocol type is %{public}d isSingle:%{public}d", protocolType, isSingle);
    Utils::SetFirstTokenID();

    if (socketId <= INVALID_ID) {
        CLOGE("bind socket = %{public}d error", socketId);
        return socketId;
    }

    int32_t result = INVALID_ID;
    // Synchronization method. A failure message is returned after timeout.
    if (protocolType == ProtocolType::CAST_PLUS_STREAM) {
        if (isSingle) {
            result = Bind(socketId, singleStreamQos, sizeof(singleStreamQos) / sizeof(QosTV), &SoftBus::listener);
        } else {
            if (times == 1) {
                result = Bind(socketId, streamQos1st, sizeof(streamQos1st) / sizeof(QosTV), &SoftBus::listener);
            } else {
                result = Bind(socketId, streamQos2nd, sizeof(streamQos2nd) / sizeof(QosTV), &SoftBus::listener);
            }
        }
    } else {
        if (isSingle) {
            result = Bind(socketId, singleMirrorQos, sizeof(singleMirrorQos) / sizeof(QosTV), &SoftBus::listener);
        } else {
            if (times == 1) {
                result = Bind(socketId, mirrorQos1st, sizeof(mirrorQos1st) / sizeof(QosTV), &SoftBus::listener);
            } else {
                result = Bind(socketId, mirrorQos2nd, sizeof(mirrorQos2nd) / sizeof(QosTV), &SoftBus::listener);
            }
        }
    }
    if (result != SOFTBUS_OK) {
        CLOGE("Bind fail result = %{public}d", result);
        return result;
    }

    CLOGI("Bind socket successfully");
    return SOFTBUS_OK;
}
} // namespace SoftBus
constexpr static int SECOND_BYTE_OFFSET = 8;
constexpr static int THIRD_BYTE_OFFSET = 16;
constexpr static int FOURTH_BYTE_OFFSET = 24;

constexpr static int INT_FOUR = 4;

/*
 * auth success
 */
constexpr int AUTH_SUCCESS_FROM_SINK = 0;
/*
 * connection failed
 */
constexpr int CONNECTION_FAILED = 9;
 
/*
 * send to json key consultResult
 */
const std::string CONSULT_RESULT = "consultResult";

// consult key
const std::string ACCOUNT_ID_KEY = "accountId";
const std::string USER_ID_KEY = "userId";

/*
* User's unusual action or other event scenarios could cause changing of STATE or RESULT which delivered
* by DM.
*/
const std::map<int, int32_t> CastBindTargetCallback::RESULT_REASON_MAP = {
    // SINK peer click distrust button during 3-state authentication.
    { ERR_DM_AUTH_PEER_REJECT, REASON_DISTRUST_BY_SINK },
    // SINK peer click cancel button during pin code inputting.
    { ERR_DM_BIND_USER_CANCEL_PIN_CODE_DISPLAY, REASON_CANCEL_BY_SINK },
    // SOURCE peer input wrong pin code up to 3 times
    { ERR_DM_BIND_PIN_CODE_ERROR, REASON_PIN_CODE_OVER_RETRY },
    // SOURCE peer click cancel button during pin code inputting.
    { ERR_DM_BIND_USER_CANCEL_ERROR, REASON_CANCEL_BY_SOURCE },
    // User interupt binding.
    { STOP_BIND, REASON_STOP_BIND_BY_SOURCE }
};

const std::map<int, int32_t> CastBindTargetCallback::STATUS_REASON_MAP = {
    // DEFAULT event
    { DmAuthStatus::STATUS_DM_AUTH_DEFAULT, REASON_DEFAULT },
    // Sink peer click trust during 3-state authentication.
    { DmAuthStatus::STATUS_DM_SHOW_PIN_INPUT_UI, REASON_TRUST_BY_SINK },
    // input right pin code, so close input dialog and show connecting dialog
    { DmAuthStatus::STATUS_DM_CLOSE_PIN_INPUT_UI, REASON_BIND_COMPLETED },
    // Waiting for user to click confirm
    { DmAuthStatus::STATUS_DM_SHOW_AUTHORIZE_UI, REASON_SHOW_TRUST_SELECT_UI }
};

ConnectionManager &ConnectionManager::GetInstance()
{
    static ConnectionManager instance{};
    return instance;
}

void ConnectionManager::Init(std::shared_ptr<IConnectionManagerListener> listener)
{
    CLOGI("ConnectionManager init start");
    if (listener == nullptr) {
        CLOGE("The input listener is null!");
        return;
    }

    if (HasListener()) {
        CLOGE("Already inited");
        return;
    }

    if (DeviceManager::GetInstance().RegisterDevStateCallback(PKG_NAME, "",
        std::make_shared<CastDeviceStateCallback>()) != DM_OK) {
        CLOGE("Failed to register device state callback");
        return;
    }

    SetListener(listener);
    CLOGD("ConnectionManager init done");
}

void ConnectionManager::Deinit()
{
    CLOGI("Deinit start");
    ResetListener();
    DisableDiscoverable();
    DeviceManager::GetInstance().UnRegisterDevStateCallback(PKG_NAME);
}

bool ConnectionManager::IsDeviceTrusted(const std::string &deviceId, std::string &networkId)
{
    std::vector<DmDeviceInfo> trustedDevices;
    if (DeviceManager::GetInstance().GetTrustedDeviceList(PKG_NAME, "", trustedDevices) != DM_OK) {
        CLOGE("Fail to get TrustedDeviceList from DM.");
        return false;
    }

    for (const auto &device : trustedDevices) {
        if (device.deviceId == deviceId) {
            networkId = device.networkId;
            CLOGI("Device: %{public}s has been trusted.", Utils::Mask(deviceId).c_str());
            return true;
        }
    }
    CLOGI("Device: %{public}s has not been trusted yet.", Utils::Mask(deviceId).c_str());

    return false;
}

DmDeviceInfo ConnectionManager::GetDmDeviceInfo(const std::string &deviceId)
{
    std::vector<DmDeviceInfo> trustedDevices;
    if (DeviceManager::GetInstance().GetTrustedDeviceList(PKG_NAME, "", trustedDevices) != DM_OK) {
        CLOGE("GetTrustedDeviceList fail");
        return {};
    }

    for (const auto &device : trustedDevices) {
        if (device.deviceId == deviceId) {
            return device;
        }
    }
    CLOGW("Can't find device(%{public}s)", Utils::Mask(deviceId).c_str());

    return {};
}

bool ConnectionManager::EnableDiscoverable()
{
    std::lock_guard lock(mutex_);
    if (isDiscoverable_) {
        CLOGW("service has been set discoverable");
        return true;
    }

    isDiscoverable_ = true;
    return true;
}

bool ConnectionManager::DisableDiscoverable()
{
    std::lock_guard lock(mutex_);
    if (!isDiscoverable_) {
        return true;
    }

    isDiscoverable_ = false;
    return true;
}

void ConnectionManager::GrabDevice()
{
    CLOGI("GrabDevice in");
    if (grabState_ == DeviceGrabState::NO_GRAB) {
        CLOGE("DeviceGrabState: NO_GRAB");
        return;
    }
    auto listener = GetListener();
    if (!listener) {
        CLOGE("listener_ nullptr.");
        return;
    }
    listener->GrabDevice(sessionId_);
}

bool ConnectionManager::OpenConsultSession(const CastInnerRemoteDevice &device)
{
    CLOGI("start open consult session");
    // The session can only be opened using a network ID instead of a UDID in OH system
    auto networkId = CastDeviceDataManager::GetInstance().GetDeviceNetworkId(device.deviceId);
    if (networkId == std::nullopt) {
        CLOGE("networkId is null");
        return false;
    }

    EstablishConsultWriteWrap(__func__, GetBIZSceneType(GetProtocolType()), GetAnonymousDeviceID(device.deviceId));
    int32_t errorCode = SUCCESS;
    int socketId = OpenSoftBusSocket(networkId, device, errorCode);
    if (socketId <= INVALID_ID) {
        CLOGE("session id invalid");
        NotifyConnectStage(device, ConnectStageResult::DISCONNECT_START, REASON_DEFAULT);
        return false;
    }

    OnConsultSessionOpened(socketId, true);
    HiSysEventWriteWrap(__func__, {
            {"BIZ_SCENE", GetBIZSceneType(GetProtocolType())},
            {"BIZ_STAGE", static_cast<int32_t>(BIZSceneStage::ESTABLISH_CONSULT_SESSION)},
            {"STAGE_RES", static_cast<int32_t>(StageResType::STAGE_RES_SUCCESS)},
            {"ERROR_CODE", CAST_RADAR_SUCCESS}}, {
            {"TO_CALL_PKG", DEVICE_MANAGER_NAME},
            {"LOCAL_SESS_NAME", ""},
            {"PEER_SESS_NAME", ""},
            {"PEER_UDID", GetAnonymousDeviceID(device.deviceId)}});

    UpdateDeviceState(device.deviceId, RemoteDeviceState::CONNECTED);
    CLOGI("Out, socketId = %{public}d", socketId);
    return true;
}

void ConnectionManager::OnConsultDataReceived(int transportId, const void *data, unsigned int dataLen)
{
    std::string dataStr(static_cast<const char *>(data), dataLen);
    CLOGD("Received data: len:%{public}u, data:%s", dataLen, dataStr.c_str());

    auto device = GetRemoteFromJsonData(dataStr);
    if (device == nullptr) {
        return;
    }
    const std::string &deviceId = device->deviceId;
    auto dmDevice = GetDmDeviceInfo(deviceId);
    if (deviceId.compare(dmDevice.deviceId) != 0) {
        CLOGE("Failed to get DmDeviceInfo");
        return;
    }

    constexpr int32_t sleepTimeMs = 50;
    constexpr int32_t retryTimes = 20;
    int castSessionId = GetCastSessionId(transportId);
    for (int32_t retryTime = 1; castSessionId == INVALID_ID && retryTime < retryTimes; retryTime++) {
        CLOGD("Retry for the %d(th) time after sleeping %dms", retryTime, sleepTimeMs);
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepTimeMs));
        castSessionId = GetCastSessionId(transportId);
    }

    if (castSessionId == INVALID_ID) {
        CLOGE("Invalid CastSessionId!");
        return;
    }
    CLOGI("protocolType is %d", device->protocolType);
    if (device->protocolType == ProtocolType::CAST_PLUS_STREAM) {
        SetSessionProtocolType(castSessionId, device->protocolType);
    }
    auto listener = GetListener();
    if (!listener) {
        CLOGE("Detect absence of listener_.");
        return;
    }
    listener->ReportSessionCreate(castSessionId);
    device->localCastSessionId = castSessionId;
    if (!CastDeviceDataManager::GetInstance().AddDevice(*device, dmDevice)) {
        return;
    }
    if (!CastDeviceDataManager::GetInstance().SetDeviceRole(deviceId, true) ||
        !UpdateDeviceState(deviceId, RemoteDeviceState::CONNECTED)) {
        CastDeviceDataManager::GetInstance().RemoveDevice(deviceId);
        return;
    }
    if (!listener->NotifyRemoteDeviceIsReady(castSessionId, *device)) {
        CastDeviceDataManager::GetInstance().RemoveDevice(deviceId);
    }

    DestroyConsulationSession(deviceId);
}

int ConnectionManager::GetCastSessionId(int transportId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (transIdToCastSessionIdMap_.count(transportId) == 1) {
        return transIdToCastSessionIdMap_[transportId];
    } else {
        CLOGE("Invalid transport id:%{public}d", transportId);
        return INVALID_ID;
    }
}

bool ConnectionManager::OnConsultSessionOpened(int transportId, bool isSource)
{
    auto time = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    auto openSessionCost = time.time_since_epoch().count() - openSessionTime_;
    CLOGI("%{public}s, openSession:%{public}lld, total %{public}lld, unit:ms", authTimeString_.c_str(),
        openSessionCost, totalAuthTime_ + openSessionCost);
    std::thread([transportId, isSource]() {
        Utils::SetThreadName("OnConsultSessionOpened");
        if (isSource) {
            auto device = CastDeviceDataManager::GetInstance().GetDeviceByTransId(transportId);
            if (device == std::nullopt) {
                CLOGE("Failed to get device by sessionId.");
                return;
            }

            bool isWifiChannelFirst = ConnectionManager::GetInstance().IsWifiChannelFirst(device->deviceId);
            if (isWifiChannelFirst) {
                CLOGE("select wifi channel localip %s, remoteIp %s", (device->localWifiIp).c_str(),
                    (device->wifiIp).c_str());
                CastDeviceDataManager::GetInstance().SetDeviceIp(device->deviceId, device->localWifiIp, device->wifiIp);
            }
            if (ConnectionManager::GetInstance().IsHuaweiDevice(*device) && !isWifiChannelFirst) {
                ConnectionManager::GetInstance().QueryP2PIp(*device);
            } else {
                ConnectionManager::GetInstance().NotifyConnectStage(*device, ConnectStageResult::AUTH_SUCCESS);
            }
            return;
        }
        ConnectionManager::GetInstance().GrabDevice();
        ConnectionManager::GetInstance().NotifySessionIsReady(transportId);
    }).detach();

    return true;
}

void ConnectionManager::OnConsultDataReceivedFromSink(int transportId, const void *data, unsigned int dataLen)
{
    std::string dataString(reinterpret_cast<const char *>(data), dataLen);
    CLOGI("Received data: %{public}s", dataString.c_str());

    if (!json::accept(dataString)) {
        CLOGE("received data string does not conform to JSON format");
        return;
    }
    json jsonObject = json::parse(dataString, nullptr, false);
    if (jsonObject.contains(OPERATION_TYPE_KEY)) {
        if (!jsonObject[OPERATION_TYPE_KEY].is_number()) {
            CLOGE("OPERATION_TYPE_KEY json data is not number");
            return;
        }
        int operType = jsonObject[OPERATION_TYPE_KEY];
        if (operType != OPERATION_CONSULT) {
            CLOGE("cast operation type %d, return", operType);
            return;
        }
    }

    if (jsonObject.contains(DATA_KEY)) {
        if (!jsonObject[DATA_KEY].is_string()) {
            CLOGE("data key is empty, get body string fail");
            return;
        }

        std::string bodyString = jsonObject[DATA_KEY];
        if (!json::accept(bodyString)) {
            CLOGE("received body string does not conform to JSON format");
            return;
        }
        json body = json::parse(bodyString, nullptr, false);
        handleConsultData(body, transportId);
    }
}

bool ConnectionManager::handleConsultData(const json &body, int transportId)
{
    CLOGI("handleConsultData data from sink %{public}s", body.dump().c_str());
    if (!body.contains(CONSULT_RESULT) || !body[CONSULT_RESULT].is_number()) {
        CLOGE("consultResult is missing or not a number");
        return false;
    }

    auto device = CastDeviceDataManager::GetInstance().GetDeviceByTransId(transportId);
    if (device == std::nullopt) {
        CLOGE("handleConsultData device is null");
        return false;
    }

    int consultResult = body[CONSULT_RESULT];
    switch (consultResult) {
        case AUTH_SUCCESS_FROM_SINK: {
            CLOGI("handleConsultData auth success, consult result is %{public}d", consultResult);
            break;
        }
        case CONNECTION_FAILED: {
            CLOGI("handleConsultData connect fail, consult result is %{public}d", consultResult);
            break;
        }
        default: {
            CLOGE("unhandled message, consult result is %d", consultResult);
            break;
        }
    }

    return true;
}

bool ConnectionManager::ConnectDevice(const CastInnerRemoteDevice &dev, const ProtocolType &protocolType)
{
    DeviceDiscoveryWriteWrap(__func__, GetAnonymousDeviceID(dev.deviceId));

    auto &deviceId = dev.deviceId;
    CLOGI("deviceId %{public}s, protocolType %{public}d, capabilityInfo %{public}d, wifiIp %{public}s, "
          "bleMac %{public}s, isLegacy %{public}d, isFresh wifi %{public}d, ble %{public}d",
          Utils::Mask(deviceId).c_str(), protocolType, dev.capabilityInfo, Utils::Mask(dev.wifiIp).c_str(),
          Utils::Mask(dev.bleMac).c_str(), dev.isLegacy, dev.isWifiFresh, dev.isBleFresh);

    if (!UpdateDeviceState(deviceId, RemoteDeviceState::CONNECTING)) {
        CLOGE("Device(%s) is missing", deviceId.c_str());
        return false;
    }

    protocolType_ = protocolType;
    SetConnectingDeviceId(deviceId);

    if (IsNeedDiscoveryDevice(dev)) {
        CLOGI("need discovery device");
        DiscoveryManager::GetInstance().StartDiscovery(static_cast<int>(protocolType), {});
        std::thread([this, dev]() {
            Utils::SetThreadName("ConnectTargetDevice");
            WaitAndConnectTargetDevice(dev);
        }).detach();
        return true;
    }
    isWifiFresh_ = dev.isWifiFresh;
    DiscoveryManager::GetInstance().StopDiscovery();

    std::string networkId;
    if (IsDeviceTrusted(dev.deviceId, networkId) && IsSingle(dev) && SourceCheckConnectAccess(networkId)) {
        NotifyListenerToLoadSinkSA(networkId);
        if (!CastDeviceDataManager::GetInstance().SetDeviceNetworkId(deviceId, networkId) ||
            !OpenConsultSession(dev)) {
            (void)UpdateDeviceState(deviceId, RemoteDeviceState::FOUND);
            SetConnectingDeviceId("");
            return false;
        }
        (void)UpdateDeviceState(deviceId, RemoteDeviceState::CONNECTED);
        return true;
    }
    if (!BindTarget(dev)) {
        (void)UpdateDeviceState(deviceId, RemoteDeviceState::FOUND);
        SetConnectingDeviceId("");
        return false;
    }
    std::unique_lock<std::mutex> lock(mutex_);
    if (isBindTargetMap_.find(deviceId) != isBindTargetMap_.end()) {
        isBindTargetMap_[deviceId] = true;
    } else {
        isBindTargetMap_.insert({ deviceId, true });
    }
    CLOGI("ConnectDevice out, %{public}s", Utils::Mask(deviceId).c_str());
    return true;
}

void ConnectionManager::DisconnectDevice(const std::string &deviceId)
{
    CLOGI("DisconnectDevice in, deviceId %{public}s", Utils::Mask(deviceId).c_str());

    std::unique_lock<std::mutex> lock(mutex_);
    connectingDeviceId_ = "";
    DiscoveryManager::GetInstance().StopDiscovery();
    if (!CastDeviceDataManager::GetInstance().IsDeviceUsed(deviceId)) {
        CLOGE("Device(%s) is not used, remove it", deviceId.c_str());
        CastDeviceDataManager::GetInstance().UpdateDeviceByDeviceId(deviceId);
        return;
    }

    protocolType_ = ProtocolType::CAST_PLUS_MIRROR;
    lock.unlock();
    UpdateDeviceState(deviceId, RemoteDeviceState::FOUND);
    SetConnectingDeviceId("");
    DestroyConsulationSession(deviceId);
    CastDeviceDataManager::GetInstance().GetDeviceByDeviceId(deviceId);
    auto isActiveAuth = CastDeviceDataManager::GetInstance().GetDeviceIsActiveAuth(deviceId);
    if (isActiveAuth == std::nullopt) {
        return;
    }
    auto networkId = CastDeviceDataManager::GetInstance().GetDeviceNetworkId(deviceId);
    if (networkId == std::nullopt) {
        return;
    }

    CastDeviceDataManager::GetInstance().UpdateDeviceByDeviceId(deviceId);
}

bool ConnectionManager::UpdateDeviceState(const std::string &deviceId, RemoteDeviceState state)
{
    CLOGI("UpdateDeviceState device: %{public}s state: %{public}s", Utils::Mask(deviceId).c_str(),
        REMOTE_DEVICE_STATE_STRING[static_cast<size_t>(state)].c_str());
    return CastDeviceDataManager::GetInstance().SetDeviceState(deviceId, state);
}

void ConnectionManager::UpdateGrabState(bool changeState, int32_t sessionId)
{
    CLOGI("GrabDevice in");
    std::lock_guard<std::mutex> lock(mutex_);
    sessionId_ = sessionId;
    if (changeState) {
        grabState_ = DeviceGrabState::GRAB_ALLOWED;
        return;
    }
    grabState_ = DeviceGrabState::NO_GRAB;
}

int ConnectionManager::GetProtocolType() const
{
    return static_cast<int>(protocolType_);
}

void ConnectionManager::SetProtocolType(ProtocolType protocol)
{
    protocolType_ = protocol;
}

int32_t ConnectionManager::GetLocalDeviceInfo(CastLocalDevice &device)
{
    CLOGI("GetLocalDeviceInfo in");
    DmDeviceInfo local;
    int32_t ret = DeviceManager::GetInstance().GetLocalDeviceInfo(PKG_NAME, local);
    if (ret != DM_OK) {
        CLOGE("Cannot get the local device info from DM");
        return ret;
    };
    device.deviceId = local.deviceId;
    device.deviceType = ConvertDeviceType(local.deviceTypeId);
    device.subDeviceType = ConvertSubDeviceType(local.deviceTypeId);
    device.deviceName = local.deviceName;
    CLOGI("GetLocalDeviceInfo out, dmTypeId:%{public}d deviceType:%{public}d subDeviceType:%{public}d",
        local.deviceTypeId, device.deviceType, device.subDeviceType);
    return ret;
}

void ConnectionManager::NotifySessionIsReady(int transportId)
{
    auto listener = GetListener();
    if (!listener) {
        CLOGE("Detect absence of listener_.");
        return;
    }
    int castSessionId = listener->NotifySessionIsReady();
    if (castSessionId == INVALID_ID) {
        CLOGE("sessionId is invalid");
        return;
    }

    CLOGD("Update cast session id map: %d: %d", transportId, castSessionId);
    std::lock_guard<std::mutex> lock(mutex_);
    transIdToCastSessionIdMap_.insert({ transportId, castSessionId });
}

void ConnectionManager::NotifyDeviceIsOffline(const std::string &deviceId)
{
    CLOGI("NotifyDeviceIsOffline in");
    auto listener = GetListener();
    if (!listener) {
        return;
    }
    listener->NotifyDeviceIsOffline(deviceId);
}

bool ConnectionManager::NotifyConnectStage(const CastInnerRemoteDevice &device, int result, int32_t reasonCode)
{
    CLOGI("result %{public}d, reasonCode %{public}d", result, reasonCode);

    auto sessionListener = GetSessionListener(device.localCastSessionId);
    if (sessionListener == nullptr) {
        CLOGE("sessionListener is NULL");
        return false;
    }

    if (result == ConnectStageResult::AUTH_FAILED || result == ConnectStageResult::CONNECT_FAIL ||
        result == ConnectStageResult::DISCONNECT_START) {
        UpdateDeviceState(device.deviceId, RemoteDeviceState::FOUND);
    }

    sessionListener->NotifyConnectStage(device.deviceId, result, reasonCode);
    return true;
}

bool ConnectionManager::NotifyListenerToLoadSinkSA(const std::string& networkId)
{
    auto listener = GetListener();
    if (!listener) {
        return false;
    }
    return listener->LoadSinkSA(networkId);
}

bool ConnectionManager::SourceCheckConnectAccess(std::string &peerNetworkId)
{
    std::string localNetworkId = "";
    if (DeviceManager::GetInstance().GetLocalDeviceNetWorkId(PKG_NAME, localNetworkId) != 0) {
        CLOGI("GetLocalDeviceNetWorkId fail %s", localNetworkId.c_str());
        return false;
    }
    DmAccessCaller dmSrcCaller = {
        .accountId = Utils::GetOhosAccountId(),
        .pkgName = PKG_NAME,
        .networkId = localNetworkId,
        .userId = Utils::GetCurrentActiveAccountUserId(),
        .tokenId = 0,
    };
    DmAccessCallee dmDstCallee = {
        .networkId = peerNetworkId,
        .peerId = "",
    };
    bool ret = DeviceManager::GetInstance().CheckAccessControl(dmSrcCaller, dmDstCallee);
    CLOGI("peerNetworkId:%{public}s, has connect Access:%{public}d", Utils::Mask(peerNetworkId).c_str(), ret);
    return ret;
}

bool ConnectionManager::SinkCheckConnectAccess(json &data, std::string &peerDeviceId)
{
    if (!data.contains(ACCOUNT_ID_KEY)) {
        CLOGI("ACCOUNT_ID_KEY is not exit, no need to check access");
        return true;
    }
    if (!data[ACCOUNT_ID_KEY].is_string()) {
        CLOGE("ACCOUNT_ID_KEY json data is not string");
        return false;
    }
    if (!data.contains(USER_ID_KEY) || !data[USER_ID_KEY].is_number()) {
        CLOGE("ACCOUNT_ID_KEY json data is not exit or is not number");
        return false;
    }
    std::string accountId = data[ACCOUNT_ID_KEY];
    int userId = data[USER_ID_KEY];
    auto dmDevice = GetDmDeviceInfo(peerDeviceId);
    if (peerDeviceId.compare(dmDevice.deviceId) != 0) {
        CLOGE("Failed to get DmDeviceInfo");
        return false;
    }
    std::string localNetworkId = "";
    if (DeviceManager::GetInstance().GetLocalDeviceNetWorkId(PKG_NAME, localNetworkId) != 0) {
        CLOGI("GetLocalDeviceNetWorkId fail %s", localNetworkId.c_str());
        return false;
    }
    DmAccessCaller dmSrcCaller = {
        .accountId = accountId,
        .pkgName = PKG_NAME,
        .networkId = dmDevice.networkId,
        .userId = userId,
        .tokenId = 0,
    };
    DmAccessCallee dmDstCallee = {
        .accountId = Utils::GetOhosAccountId(),
        .networkId = localNetworkId,
        .peerId = "",
        .userId = Utils::GetCurrentActiveAccountUserId(),
    };
    bool ret = DeviceManager::GetInstance().CheckAccessControl(dmSrcCaller, dmDstCallee);
    CLOGI("peerDeviceId:%{public}s, has connect Access:%{public}d", Utils::Mask(peerDeviceId).c_str(), ret);
    return ret;
}

bool ConnectionManager::IsWifiChannelFirst(const std::string &deviceId)
{
    if (deviceId.empty()) {
        return false;
    }
    if (protocolType_ != ProtocolType::CAST_PLUS_STREAM) {
        CLOGI("protocal type is %{public}d, not stream", protocolType_);
        return false;
    }

    if (!isWifiFresh_) {
        CLOGI("deviceId %s, wifi is not fresh", deviceId.c_str());
        return false;
    }
    auto remote = CastDeviceDataManager::GetInstance().GetDeviceByDeviceId(deviceId);
    if (remote == std::nullopt) {
        CLOGE("Get remote device is empty");
        return false;
    }
    CastInnerRemoteDevice device = *remote;
    CLOGI("deviceId %s, wifi ip local %s, remote %s, link ip local %s, remote %s", deviceId.c_str(),
        device.localWifiIp.c_str(), device.wifiIp.c_str(), device.localIp.c_str(), device.remoteIp.c_str());
    return !device.localWifiIp.empty() && !device.wifiIp.empty();
}

bool ConnectionManager::IsNeedDiscoveryDevice(const CastInnerRemoteDevice &dev)
{
    return IsHuaweiDevice(dev) && !dev.isWifiFresh && !dev.isBleFresh;
}

void ConnectionManager::WaitAndConnectTargetDevice(const CastInnerRemoteDevice &dev)
{
    CLOGI("In");
    constexpr int maxCount = 100;
    constexpr int sleepTime = 50;
    bool result = false;
    for (int index = 0; index <= maxCount; index++) {
        if (GetConnectingDeviceId().empty()) {
            CLOGI("connecting deviceId is empty, return");
            result = true;
            break;
        }
        auto temp = CastDeviceDataManager::GetInstance().GetDeviceByDeviceId(dev.deviceId);
        if (temp == std::nullopt || IsNeedDiscoveryDevice(*temp)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
            continue;
        } else {
            CastInnerRemoteDevice device = *temp;
            CLOGI("find target device %{public}s wifiFresh is %{public}d", Utils::Mask(dev.deviceId).c_str(),
                device.isWifiFresh);
            if (!device.isWifiFresh) {
                device.wifiIp = "";
                device.wifiPort = 0;
            }
            result = ConnectDevice(device, protocolType_);
            break;
        }
    }
    // auto end = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    CLOGI("wait target device time (endTime - startTime ms), result %{public}d", result);
    if (!result) {
        NotifyConnectStage(dev, ConnectStageResult::DISCONNECT_START, REASON_DEFAULT);
    }
}

bool ConnectionManager::BindTarget(const CastInnerRemoteDevice &dev)
{
    CLOGD("device info is %s, device name %s, customData %s", dev.deviceId.c_str(), dev.deviceName.c_str(),
        dev.customData.c_str());
    PeerTargetId targetId = {
        .deviceId = dev.deviceId,
        .bleMac = dev.bleMac,
        .wifiIp = dev.wifiIp,
        .wifiPort = dev.wifiPort,
    };
    std::map<std::string, std::string> bindParam;
    BuildBindParam(dev, bindParam);
    int ret = DeviceManager::GetInstance().BindTarget(PKG_NAME, targetId, bindParam,
        std::make_shared<CastBindTargetCallback>());
    if (ret != DM_OK) {
        CLOGE("ConnectDevice BindTarget fail, ret = %{public}d)", ret);
        if (ret == ERR_DM_AUTH_BUSINESS_BUSY) {
            DeviceManager::GetInstance().UnbindTarget(
                PKG_NAME, targetId, bindParam, std::make_shared<CastUnBindTargetCallback>());
        }
        return false;
    }

    CastDeviceDataManager::GetInstance().SetDeviceIsActiveAuth(dev.deviceId, true);
    CLOGI("Finish to BindTarget device!");
    return true;
}

bool ConnectionManager::BuildBindParam(const CastInnerRemoteDevice &device,
    std::map<std::string, std::string> &bindParam)
{
    CLOGI("start BuildBindParam");
    if (IsSingle(device)) { // bind target by dm
        bindParam[PARAM_KEY_AUTH_TYPE] = AUTH_WITH_PIN;
    } else { // bind target by meta node
        CastLocalDevice local;
        int32_t ret = GetLocalDeviceInfo(local);
        if (ret != DM_OK) {
            CLOGE("CastLocalDevice get failed");
            return false;
        }
        bindParam[DistributedHardware::PARAM_KEY_META_TYPE] = "5";
        bindParam[KEY_TRANSFER_MODE] = std::to_string(TRANSFER_MODE_SOFTBUS_SINGLE);
        bindParam[DEVICE_NAME_KEY] = local.deviceName;
        bindParam[AUTH_VERSION_KEY] = GetAuthVersion(device);
        bindParam[KEY_SESSION_ID] = std::to_string(device.sessionId);
        bindParam["udid"] = device.udid;
    }
    return true;
}

std::string ConnectionManager::GetAuthVersion(const CastInnerRemoteDevice &device)
{
    if (!device.bleMac.empty() && !device.customData.empty()) {
        return AUTH_VERSION_2;
    }
    return device.wifiPort == 0 ? AUTH_VERSION_1 : AUTH_VERSION_2;
}

bool ConnectionManager::QueryP2PIp(const CastInnerRemoteDevice &dev)
{
    CLOGI("query p2p ip method in ");
    auto temp = CastDeviceDataManager::GetInstance().GetDeviceByDeviceId(dev.deviceId);
    if (temp == std::nullopt) {
        CLOGE("GetDeviceByDeviceId the device(%s) is missing", dev.deviceId.c_str());
        return false;
    }

    CastInnerRemoteDevice device = *temp;

    PeerTargetId targetId;
    targetId.deviceId = device.deviceId;
    targetId.wifiIp = device.wifiIp;
    targetId.wifiPort = device.wifiPort;
    targetId.bleMac = device.bleMac;

    std::string localNetworkId = "";
    if (DeviceManager::GetInstance().GetLocalDeviceNetWorkId(PKG_NAME, localNetworkId) != 0) {
        CLOGI("GetLocalDeviceNetWorkId fail %s", localNetworkId.c_str());
        return false;
    }

    // The session can only be opened using a network ID instead of a UDID in OH system
    auto remoteNetworkId = CastDeviceDataManager::GetInstance().GetDeviceNetworkId(dev.deviceId);
    if (remoteNetworkId == std::nullopt) {
        return false;
    }
    std::map<std::string, std::string> bindParam;
    bindParam[DistributedHardware::PARAM_KEY_META_TYPE] = "5";
    bindParam[KEY_BIND_TARGET_ACTION] = std::to_string(ACTION_QUERY_P2P_IP);
    bindParam["localNetworkId"] = localNetworkId;
    bindParam["remoteNetworkId"] = remoteNetworkId.value();
    CLOGI("QueryP2PIp localNetworkId=%s, remoteNetworkId=%s", localNetworkId.c_str(), remoteNetworkId.value().c_str());
    DeviceManager::GetInstance().BindTarget(PKG_NAME, targetId, bindParam,
        std::make_shared<CastBindTargetCallback>());
    return true;
}

void ConnectionManager::SendConsultData(const CastInnerRemoteDevice &device, int port)
{
    CLOGI("In");
    int transportId = CastDeviceDataManager::GetInstance().GetDeviceTransId(device.deviceId);
    if (transportId == INVALID_ID) {
        CLOGE("transport id is invalid");
        return;
    }

    json data;
    data[VERSION_KEY] = VERSION;
    data[OPERATION_TYPE_KEY] = OPERATION_CONSULT;
    data[SEQUENCE_NUMBER] = rand();
    json body;
    GetConsultationData(device, port, body);
    data[DATA_KEY] = body;

    std::string dataStr = data.dump();
    int ret = SendBytes(transportId, dataStr.c_str(), dataStr.size());
    if (ret != SOFTBUS_OK) {
        CLOGE("failed to send consultion data, return:%{public}d", ret);
        CastEngineDfx::WriteErrorEvent(SEND_CONSULTION_DATA_FAIL);
        return;
    }
    CLOGD("return:%{public}d, data:%s", ret, dataStr.c_str());
}

std::string ConnectionManager::GetConsultationData(const CastInnerRemoteDevice &device, int port, json &body)
{
    CastLocalDevice local;
    auto ret = GetLocalDeviceInfo(local);
    if (ret != DM_OK) {
        CLOGE("local device info get failed");
        return "";
    }

    body[DEVICE_ID_KEY] = local.deviceId;
    body[DEVICE_NAME_KEY] = local.deviceName;
    body[KEY_SESSION_ID] = device.sessionId;
    body[KEY_TRANSFER_MODE] = TRANSFER_MODE_SOFTBUS_SINGLE;

    ProtocolType protocolType;
    GetSessionProtocolType(device.sessionId, protocolType);
    body[PROTOCOL_TYPE_KEY] = protocolType;

    if (GetAuthVersion(device) == AUTH_VERSION_2) {
        body[TYPE_SESSION_KEY] = device.sessionKey;
    }

    CLOGI("Encrypt data localIp %s, remoteIp %s, port %d", device.localIp.c_str(), device.remoteIp.c_str(), port);
    EncryptPort(port, device.sessionKey, body);
    EncryptIp(device.localIp, SOURCE_IP_KEY, device.sessionKey, body);
    EncryptIp(device.remoteIp, SINK_IP_KEY, device.sessionKey, body);
    return body.dump();
}

void ConnectionManager::EncryptPort(int port, const uint8_t *sessionKey, json &body)
{
    std::unique_ptr<uint8_t[]> portArray = intToByteArray(port);
    int portArraySize = 4;
    ConstPacketData inputData = { portArray.get(), portArraySize };

    int encryptedDataLen = 0;
    auto encryptedData = EncryptDecrypt::GetInstance().EncryptData(EncryptDecrypt::CTR_CODE, { sessionKey,
        SESSION_KEY_LENGTH }, inputData, encryptedDataLen);
    if (!encryptedData) {
        CLOGE("encrypt error");
        return;
    }
    CLOGD("encrypt result is %d ", encryptedDataLen);
    std::string encryptedPortLatin1(reinterpret_cast<const char *>(encryptedData.get()), encryptedDataLen);
    std::string encryptedPortUtf8 = convLatin1ToUTF8(encryptedPortLatin1);
    body[PORT_KEY] = encryptedPortUtf8;
}

void ConnectionManager::EncryptIp(const std::string &ip, const std::string &key, const uint8_t *sessionKey, json &body)
{
    if (ip.empty()) {
        return;
    }
    ConstPacketData inputData = { reinterpret_cast<const uint8_t *>(ip.c_str()), ip.size() };
    int encryptedDataLen = 0;
    auto encryptedData = EncryptDecrypt::GetInstance().EncryptData(EncryptDecrypt::CTR_CODE, { sessionKey,
        SESSION_KEY_LENGTH }, inputData, encryptedDataLen);
    if (!encryptedData) {
        CLOGE("encrypt error");
        return;
    }
    uint8_t *encrypted = encryptedData.get();
    for (int i = 0; i < encryptedDataLen; i++) {
        body[key].push_back(encrypted[i]);
    }
    CLOGI("encrypt %s finish", key.c_str());
}

std::unique_ptr<uint8_t[]> ConnectionManager::intToByteArray(int32_t num)
{
    unsigned int number = static_cast<unsigned int>(num);
    std::unique_ptr<uint8_t[]> result = std::make_unique<uint8_t[]>(INT_FOUR);
    unsigned int i = 0;
    result[i] = (number >> FOURTH_BYTE_OFFSET) & 0xFF;
    result[++i] = (number >> THIRD_BYTE_OFFSET) & 0xFF;
    result[++i] = (number >> SECOND_BYTE_OFFSET) & 0xFF;
    result[++i] = number & 0xFF;
    return result;
}

std::string ConnectionManager::convLatin1ToUTF8(std::string &latin1)
{
    iconv_t cd = iconv_open("utf8", "iso88591");
    if (cd == (iconv_t)-1) {
        CLOGD("andy Failed to open iconv conversion descriptor");
        return "";
    }

    size_t inSize = latin1.size();
    size_t outSize = inSize * 2;
    std::string utf8(outSize, 0);

    char *inbuf = &latin1[0];
    char *outbuf = &utf8[0];
    size_t result = iconv(cd, &inbuf, &inSize, &outbuf, &outSize);
    if (result == (size_t)-1) {
        CLOGD("Failed to convert encoding");
        iconv_close(cd);
        return "";
    }

    iconv_close(cd);
    utf8.resize(outbuf - &utf8[0]);
    return utf8;
}

void ConnectionManager::DestroyConsulationSession(const std::string &deviceId)
{
    int transportId = CastDeviceDataManager::GetInstance().ResetDeviceTransId(deviceId);
    CLOGI("DestroyConsulationSession in tranId = %{public}d", transportId);
    if (transportId != INVALID_ID) {
        CloseSession(transportId);
        RemoveTransIdMapping(transportId);
    }

    auto isSink = CastDeviceDataManager::GetInstance().GetDeviceRole(deviceId);
    if (isSink == std::nullopt || (*isSink)) {
        // The sink's Server is only removed when DisableDiscoverable or Deinit is performed.
        return;
    }
}

void ConnectionManager::RemoveTransIdMapping(int transportId)
{
    if (transportId <= INVALID_ID) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    transIdToCastSessionIdMap_.erase(transportId);
}

bool ConnectionManager::ParseAndCheckJsonData(const std::string &data, json &jsonData)
{
    if (!json::accept(data)) {
        CLOGE("something wrong for the json data!");
        return false;
    }
    json jsonObject = json::parse(data, nullptr, false);
    if (!jsonObject.contains(DATA_KEY)) {
        CLOGE("json object have no data!");
        return false;
    }

    if (jsonObject[DATA_KEY].is_string()) {
        std::string dataString = jsonObject[DATA_KEY];
        jsonData = json::parse(dataString, nullptr, false);
    } else if (jsonObject[DATA_KEY].is_object()) {
        jsonData = jsonObject[DATA_KEY];
    } else {
        CLOGE("data key in json object is invalid!");
        return false;
    }
    if (jsonData.is_discarded()) {
        CLOGE("json object discarded!");
        return false;
    }

    if (jsonData.contains(DEVICE_ID_KEY) && !jsonData[DEVICE_ID_KEY].is_string()) {
        CLOGE("DEVICE_ID_KEY json data is not string");
        return false;
    }

    if (jsonData.contains(DEVICE_NAME_KEY) && !jsonData[DEVICE_NAME_KEY].is_string()) {
        CLOGE("DEVICE_NAME_KEY json data is not string");
        return false;
    }

    if (jsonData.contains(KEY_SESSION_ID) && !jsonData[KEY_SESSION_ID].is_number()) {
        CLOGE("KEY_SESSION_ID json data is not number");
        return false;
    }
    return true;
}

std::unique_ptr<CastInnerRemoteDevice> ConnectionManager::GetRemoteFromJsonData(const std::string &data)
{
    json jsonData;
    if (!ParseAndCheckJsonData(data, jsonData)) {
        return nullptr;
    }

    auto device = std::make_unique<CastInnerRemoteDevice>();
    if (!device) {
        CLOGE("make unique failed");
        return nullptr;
    }
    device->deviceId = jsonData.contains(DEVICE_ID_KEY) ? jsonData[DEVICE_ID_KEY] : "";
    device->deviceName = jsonData.contains(DEVICE_NAME_KEY) ? jsonData[DEVICE_NAME_KEY] : "";
    device->deviceType = DeviceType::DEVICE_CAST_PLUS;
    if (jsonData.contains(KEY_SESSION_ID)) {
        device->sessionId = jsonData[KEY_SESSION_ID];
    }
    if (jsonData.contains(PROTOCOL_TYPE_KEY) && jsonData[PROTOCOL_TYPE_KEY].is_number()) {
        device->protocolType = jsonData[PROTOCOL_TYPE_KEY];
    }
    if (!SinkCheckConnectAccess(jsonData, device->deviceId)) {
        return nullptr;
    }
    
    return device;
}

int ConnectionManager::OpenSoftBusSocket(const std::optional<std::string> &networkId,
                                         const CastInnerRemoteDevice &device, int32_t &errorCode)
{
    // Only transortId > INVALID_ID does NOT meaning openSession successfully,
    // result from OnOpenSession also count.
    CLOGI("OpenSoftBusSocket in");
    constexpr int32_t attemptCountMax = 2;
    int socketId = INVALID_ID;
    int bindResult = INVALID_ID;
    bool isSingle = IsSingle(device);
    auto time = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    openSessionTime_ = time.time_since_epoch().count();
    std::lock_guard<std::mutex> lock(openConsultingSessionMutex_);
    for (int attemptCount = 1; attemptCount <= attemptCountMax; ++attemptCount) {
        CLOGI("Attemp to OpenSession in %{public}d times.", attemptCount);
        if (IsDeviceConnectStateChange(device)) {
            CLOGE("device state is not connecting, do nothting");
            socketId = INVALID_ID;
            errorCode = CONNECTION_DEVICE_IS_MISSING;
            break;
        }
        socketId = SoftBus::CreateSocket(networkId, protocolType_);
        if (socketId <= INVALID_ID) {
            CLOGE("Failed to open session, and try again, socketId: %{public}d", socketId);
            errorCode = socketId;
            continue;
        }

        bindResult = SoftBus::BindSocket(socketId, protocolType_, isSingle, attemptCount);
        if (bindResult != SOFTBUS_OK) {
            CLOGE("Failed to bind socket, result %{public}d", bindResult);
            Shutdown(socketId);
            socketId = INVALID_ID;
            errorCode = bindResult;
            continue;
        }
        if (IsDeviceConnectStateChange(device) ||
            !CastDeviceDataManager::GetInstance().SetDeviceTransId(device.deviceId, socketId)) {
            CLOGE("deviceState is %{public}d",
                  static_cast<int>(CastDeviceDataManager::GetInstance().GetDeviceState(device.deviceId)));
            Shutdown(socketId);
            socketId = INVALID_ID;
            errorCode = CONNECTION_DEVICE_IS_MISSING;
            break;
        }
        break;
    }
    return socketId;
}

bool ConnectionManager::IsDeviceConnectStateChange(const CastInnerRemoteDevice &device)
{
    auto newDevice = CastDeviceDataManager::GetInstance().GetDeviceByDeviceId(device.deviceId);
    if (!CastDeviceDataManager::GetInstance().IsDeviceConnecting(device.deviceId) ||
        (newDevice && newDevice->localCastSessionId != device.localCastSessionId)) {
        return true;
    }

    return false;
}

void ConnectionManager::AddSessionListener(int castSessionId, std::shared_ptr<IConnectManagerSessionListener> listener)
{
    std::lock_guard<std::mutex> lock(mutex_);
    sessionListeners_[castSessionId] = listener;
}

void ConnectionManager::RemoveSessionListener(int castSessionId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (castSessionId == INVALID_ID) {
        sessionListeners_.clear();
        return;
    }
    auto it = sessionListeners_.find(castSessionId);
    if (it == sessionListeners_.end()) {
        CLOGE("Cast session listener(%{public}d) has gone.", castSessionId);
        return;
    }
    sessionListeners_.erase(it);
}

std::shared_ptr<IConnectManagerSessionListener> ConnectionManager::GetSessionListener(int castSessionId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessionListeners_.find(castSessionId);
    if (it == sessionListeners_.end()) {
        CLOGE("Cast session listener(%{public}d) has gone.", castSessionId);
        return nullptr;
    }
    return it->second;
}

void ConnectionManager::SetListener(std::shared_ptr<IConnectionManagerListener> listener)
{
    std::lock_guard<std::mutex> lock(mutex_);
    listener_ = listener;
}

std::shared_ptr<IConnectionManagerListener> ConnectionManager::GetListener()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return listener_;
}

bool ConnectionManager::HasListener()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return listener_ != nullptr;
}

void ConnectionManager::ResetListener()
{
    SetListener(nullptr);
    RemoveSessionListener(INVALID_ID);
}

int32_t ConnectionManager::GetSessionProtocolType(int sessionId, ProtocolType &protocolType)
{
    auto listener = GetListener();
    if (!listener) {
        return CAST_ENGINE_ERROR;
    }
    return listener->GetSessionProtocolType(sessionId, protocolType);
}

int32_t ConnectionManager::SetSessionProtocolType(int sessionId, ProtocolType protocolType)
{
    auto listener = GetListener();
    if (!listener) {
        return CAST_ENGINE_ERROR;
    }
    return listener->SetSessionProtocolType(sessionId, protocolType);
}

void ConnectionManager::SendConsultInfo(const std::string &deviceId, int port)
{
    CLOGI("SendConsultInfo In");
    auto remote = CastDeviceDataManager::GetInstance().GetDeviceByDeviceId(deviceId);
    if (remote == std::nullopt) {
        CLOGE("Get remote device is empty");
        return;
    }

    SendConsultData(*remote, port);
}

std::string ConnectionManager::GetConnectingDeviceId()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return connectingDeviceId_;
}

void ConnectionManager::SetConnectingDeviceId(std::string deviceId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    connectingDeviceId_ = deviceId;
}

void CastBindTargetCallback::OnBindResult(const PeerTargetId &targetId, int32_t result, int32_t status,
    std::string content)
{
    CLOGI("device id: %{public}s, status: %{public}d, result: %{public}d", Utils::Mask(targetId.deviceId).c_str(),
          status, result);
    auto remote = CastDeviceDataManager::GetInstance().GetDeviceByDeviceId(targetId.deviceId);
    if (remote == std::nullopt) {
        CLOGE("Get remote device is empty");
        return;
    }

    json jsonInfo{};
    if (json::accept(content)) {
        jsonInfo = json::parse(content, nullptr, false);
    }

    int action = -1;
    switch (status) {
        case DmAuthStatus::STATUS_DM_AUTH_FINISH:
            if (jsonInfo.contains(KEY_BIND_TARGET_ACTION) && jsonInfo[KEY_BIND_TARGET_ACTION].is_number()) {
                action = jsonInfo[KEY_BIND_TARGET_ACTION];
            }
            return HandleBindAction(*remote, action, jsonInfo);
        case DmAuthStatus::STATUS_DM_SHOW_PIN_INPUT_UI:
        case DmAuthStatus::STATUS_DM_CLOSE_PIN_INPUT_UI:
        case DmAuthStatus::STATUS_DM_SHOW_AUTHORIZE_UI:
            return;
        case DmAuthStatus::STATUS_DM_AUTH_DEFAULT:
            return;
        default:
            CLOGW("unknow status %{public}d", status);
            return;
    }
}

void CastBindTargetCallback::HandleBindAction(const CastInnerRemoteDevice &device, int action, const json &authInfo)
{
    CLOGI("action is %{public}d", action);

    switch (action) {
        case ACTION_CONNECT_DEVICE: {
            HandleConnectDeviceAction(device, authInfo);
            return;
        }
        case ACTION_QUERY_P2P_IP: {
            HandleQueryIpAction(device, authInfo);
            return;
        }
        case ACTION_SEND_MESSAGE: {
            CLOGI("action operate 3 send message");
            return;
        }
        default: {
            CLOGW("unknow action %{public}d", action);
            return;
        }
    }
}

void CastBindTargetCallback::HandleConnectDeviceAction(const CastInnerRemoteDevice &device, const json &authInfo)
{
    CLOGI("handle connect device action");
    if (!authInfo.contains(NETWORK_ID) || !authInfo[NETWORK_ID].is_string()) {
        CLOGE("networkId json data is not string");
        return;
    }

    if (authInfo.contains(TIME_RECORD_STRING) && authInfo[TIME_RECORD_STRING].is_string()) {
        ConnectionManager::GetInstance().authTimeString_ = authInfo[TIME_RECORD_STRING];
    }

    if (authInfo.contains(TOTAL_AUTH_TIME) && authInfo[TOTAL_AUTH_TIME].is_number()) {
        ConnectionManager::GetInstance().totalAuthTime_ = authInfo[TOTAL_AUTH_TIME];
    }

    const std::string networkId = authInfo[NETWORK_ID];
    const std::string deviceId = device.deviceId;
    if (!CastDeviceDataManager::GetInstance().SetDeviceNetworkId(device.deviceId, networkId)) {
        return;
    }

    if (authInfo.contains(KEY_TRANSFER_MODE) && authInfo[KEY_TRANSFER_MODE].is_number()) {
        int mode = authInfo[KEY_TRANSFER_MODE];
        ChannelType type = mode == TRANSFER_MODE_SOFTBUS_SINGLE ? ChannelType::SOFT_BUS : ChannelType::LEGACY_CHANNEL;
        CastDeviceDataManager::GetInstance().SetDeviceChannleType(deviceId, type);
    }

    if (authInfo.contains(AUTH_VERSION_KEY) && authInfo[AUTH_VERSION_KEY].is_string()) {
        std::string authVersion = authInfo[AUTH_VERSION_KEY];
        uint8_t sessionKey[SESSION_KEY_LENGTH] = {0};
        if (authVersion == AUTH_VERSION_1) {
            // 获取sessionKey
            if (!GetSessionKey(authInfo, sessionKey)) {
                CLOGE("authVersion is 1.0, get sessionkey fail");
                return;
            }
            bool result = CastDeviceDataManager::GetInstance().SetDeviceSessionKey(deviceId, sessionKey);
            CLOGI("auth version 1.0, set sessionkey result is %{public}d", result);
            ConnectionManager::GetInstance().NotifyConnectStage(device, ConnectStageResult::AUTH_SUCCESS);
        } else {
            RAND_bytes(sessionKey, SESSION_KEY_LENGTH);
            bool result = CastDeviceDataManager::GetInstance().SetDeviceSessionKey(deviceId, sessionKey);
            CLOGI("authVersion is 2.0, set sessionkey result is %{public}d", result);
            std::thread([device]() {
                Utils::SetThreadName("HandleConnectDeviceAction");
                ConnectionManager::GetInstance().OpenConsultSession(device);
            }).detach();
        }
    }
}

bool CastBindTargetCallback::GetSessionKey(const json &authInfo, uint8_t *sessionKey)
{
    if (!authInfo.contains(TYPE_SESSION_KEY) || !authInfo[TYPE_SESSION_KEY].is_array()) {
        CLOGE("get sessionkey from json data fail");
        return false;
    }
    const auto &keyArray = authInfo[TYPE_SESSION_KEY];
    if (static_cast<int>(keyArray.size()) < SESSION_KEY_LENGTH) {
        CLOGE("sessionKey array length %{public}zu is shorter than SESSION_KEY_LENGTH %{public}d",
            keyArray.size(), SESSION_KEY_LENGTH);
        return false;
    }
    for (int i = 0; i < SESSION_KEY_LENGTH; i++) {
        if (!keyArray[i].is_number()) {
            CLOGE("sessionKey[%{public}d] is not a number", i);
            return false;
        }
        sessionKey[i] = keyArray[i];
    }
    return true;
}

void CastBindTargetCallback::HandleQueryIpAction(const CastInnerRemoteDevice &remoteDevice, const json &authInfo)
{
    CLOGI("query p2p finish, notify session auth success");
    std::string localIp;
    std::string remoteIp;
    if (authInfo.contains(KEY_LOCAL_P2P_IP) && authInfo[KEY_LOCAL_P2P_IP].is_string()) {
        localIp = authInfo[KEY_LOCAL_P2P_IP];
    }

    if (authInfo.contains(KEY_REMOTE_P2P_IP) && authInfo[KEY_REMOTE_P2P_IP].is_string()) {
        remoteIp = authInfo[KEY_REMOTE_P2P_IP];
    }

    if (localIp.empty() || remoteIp.empty()) {
        CLOGI("Get remote device p2p ip is empty");
        if (remoteDevice.localWifiIp.empty() || remoteDevice.wifiIp.empty()) {
            CLOGI("device p2p ip is empty start disconnect");
            return;
        } else {
            localIp = remoteDevice.localWifiIp;
            remoteIp = remoteDevice.wifiIp;
        }
    }

    CastDeviceDataManager::GetInstance().SetDeviceIp(remoteDevice.deviceId, localIp, remoteIp);
    ConnectionManager::GetInstance().NotifyConnectStage(remoteDevice, ConnectStageResult::AUTH_SUCCESS);
}

void CastUnBindTargetCallback::OnUnbindResult(const PeerTargetId &targetId, int32_t result, std::string content)
{
    CLOGI("OnUnbindResult,device id:%s, result: %d, content: %s", targetId.deviceId.c_str(), result, content.c_str());
}

void CastDeviceStateCallback::OnDeviceOnline(const DmDeviceInfo &deviceInfo)
{
    CLOGI("device(%{public}s) is online", Utils::Mask(deviceInfo.deviceId).c_str());
    std::string deviceId = std::string(deviceInfo.deviceId);
    if (!ConnectionManager::GetInstance().IsBindTarget(deviceId)) {
        return;
    }

    CLOGD("Online for bind target, networkId:%s", deviceInfo.networkId);
    if (!CastDeviceDataManager::GetInstance().SetDeviceNetworkId(deviceId, deviceInfo.networkId)) {
        return;
    }
    auto isActiveAuth = CastDeviceDataManager::GetInstance().GetDeviceIsActiveAuth(deviceId);
    if (isActiveAuth == std::nullopt || !(*isActiveAuth)) {
        return;
    }
    auto remote = CastDeviceDataManager::GetInstance().GetDeviceByDeviceId(deviceId);
    if (remote == std::nullopt) {
        CLOGE("Get remote device is empty");
        return;
    }
    if (!ConnectionManager::GetInstance().IsSingle(*remote)) {
        CLOGI("current device is not single device %s ", deviceId.c_str());
        return;
    }
    ConnectionManager::GetInstance().NotifyListenerToLoadSinkSA(deviceInfo.networkId);
    ConnectionManager::GetInstance().OpenConsultSession(*remote);
}

void CastDeviceStateCallback::OnDeviceOffline(const DmDeviceInfo &deviceInfo)
{
    CLOGI("device(%{public}s) is offline", Utils::Mask(deviceInfo.deviceId).c_str());
}

void CastDeviceStateCallback::OnDeviceChanged(const DmDeviceInfo &deviceInfo)
{
    CLOGI("device(%{public}s) is changed", Utils::Mask(deviceInfo.deviceId).c_str());
}

void CastDeviceStateCallback::OnDeviceReady(const DmDeviceInfo &deviceInfo)
{
    CLOGI("device(%{public}s) is ready", Utils::Mask(deviceInfo.deviceId).c_str());
}

bool ConnectionManager::IsSingle(const CastInnerRemoteDevice &device)
{
    if (device.authVersion == AUTH_VERSION_3) {
        CLOGI("Is hw single device");
        return true;
    }
    return false;
}

bool ConnectionManager::IsHuaweiDevice(const CastInnerRemoteDevice &device)
{
    if (device.authVersion == AUTH_VERSION_2) {
        CLOGI("Is hw device");
        return true;
    }
    return false;
}

bool ConnectionManager::IsThirdDevice(const CastInnerRemoteDevice &device)
{
    if (device.authVersion == AUTH_VERSION_1) {
        CLOGI("Is third device");
        return true;
    }

    return false;
}

bool ConnectionManager::IsBindTarget(std::string deviceId)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (isBindTargetMap_.find(deviceId) == isBindTargetMap_.end()) {
        return false;
    }

    isBindTargetMap_.erase(deviceId);
    return true;
}

int ConnectionManager::GetRTSPPort()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return rtspPort_;
}

void ConnectionManager::SetRTSPPort(int port)
{
    std::lock_guard<std::mutex> lock(mutex_);
    rtspPort_ = port;
}

} // namespace CastEngineService
} // namespace CastEngine
} // namespace OHOS
