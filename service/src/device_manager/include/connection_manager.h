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

#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include <map>
#include <mutex>
#include <optional>
#include <string>

#include "cast_device_data_manager.h"
#include "cast_engine_common.h"
#include "cast_service_common.h"
#include "connection_manager_listener.h"
#include "dm_device_info.h"
#include "device_manager_callback.h"
#include "json.hpp"
#include "socket.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineService {
using OHOS::DistributedHardware::BindTargetCallback;
using OHOS::DistributedHardware::UnbindTargetCallback;
using OHOS::DistributedHardware::DeviceStateCallback;
using OHOS::DistributedHardware::DmDeviceInfo;
using OHOS::DistributedHardware::PeerTargetId;
using nlohmann::json;

class CastBindTargetCallback : public BindTargetCallback {
public:
    void OnBindResult(const PeerTargetId &targetId, int32_t result, int32_t status, std::string content) override;
private:
    void HandleBindAction(const CastInnerRemoteDevice &device, int action, const json &authInfo);
    void HandleConnectDeviceAction(const CastInnerRemoteDevice &device, const json &authInfo);
    bool GetSessionKey(const json &authInfo, uint8_t *sessionkey);
    void HandleQueryIpAction(const CastInnerRemoteDevice &remoteDevice, const json &authInfo);
    static const std::map<int32_t, int32_t> RESULT_REASON_MAP;
    static const std::map<int32_t, int32_t> STATUS_REASON_MAP;
};

class CastUnBindTargetCallback : public UnbindTargetCallback {
public:
    void OnUnbindResult(const PeerTargetId &targetId, int32_t result, std::string content) override;
};

class ConnectionManager {
public:
    static ConnectionManager &GetInstance();

    void Init(std::shared_ptr<IConnectionManagerListener> listener);
    void Deinit();

    bool IsDeviceTrusted(const std::string &deviceId, std::string &networkId);
    DmDeviceInfo GetDmDeviceInfo(const std::string &deviceId);
    bool EnableDiscoverable();
    bool DisableDiscoverable();
    void GrabDevice();

    bool OpenConsultSession(const CastInnerRemoteDevice &device);
    void OnConsultDataReceived(int transportId, const void *data, unsigned int dataLen);
    bool OnConsultSessionOpened(int transportId, bool isSource);
    void OnConsultDataReceivedFromSink(int transportId, const void *data, unsigned int dataLen);
    bool handleConsultData(const json &body, int transportId);

    bool ConnectDevice(const CastInnerRemoteDevice &dev, const ProtocolType &protocolType);
    void DisconnectDevice(const std::string &deviceId);

    bool UpdateDeviceState(const std::string &deviceId, RemoteDeviceState state);
    void UpdateGrabState(bool changeState, int32_t sessionId);

    int GetProtocolType() const;
    void SetProtocolType(ProtocolType protocol);

    int32_t GetLocalDeviceInfo(CastLocalDevice &device);

    void NotifySessionIsReady(int transportId);
    void NotifyDeviceIsOffline(const std::string &deviceId);
    bool NotifyConnectStage(const CastInnerRemoteDevice &device, int result, int32_t reasonCode = REASON_DEFAULT);
    bool NotifyListenerToLoadSinkSA(const std::string &networkId);

    void AddSessionListener(int castSessionId, std::shared_ptr<IConnectManagerSessionListener> listener);
    void RemoveSessionListener(int castSessionId);

    int32_t GetSessionProtocolType(int sessionId, ProtocolType &protocolType);
    int32_t SetSessionProtocolType(int sessionId, ProtocolType protocolType);

    void SetRTSPPort(int port);
    void SendConsultInfo(const std::string &deviceId, int port);

    std::string GetConnectingDeviceId();
    void SetConnectingDeviceId(std::string deviceId);
    bool IsSingle(const CastInnerRemoteDevice &device);
    bool IsHuaweiDevice(const CastInnerRemoteDevice &device);
    bool IsThirdDevice(const CastInnerRemoteDevice &device);
    bool IsBindTarget(std::string deviceId);

    bool SourceCheckConnectAccess(std::string &peerNetworkId);
    bool SinkCheckConnectAccess(json &data, std::string &peerDeviceId);

    std::map<std::string, bool> isBindTargetMap_;
    std::string connectingDeviceId_{};
    time_t openSessionTime_;
    std::string authTimeString_ = "";
    long totalAuthTime_ = 0;

private:
    bool BindTarget(const CastInnerRemoteDevice &dev);
    bool BuildBindParam(const CastInnerRemoteDevice &device, std::map<std::string, std::string> &bindParam);
    std::string GetAuthVersion(const CastInnerRemoteDevice &device);

    bool QueryP2PIp(const CastInnerRemoteDevice &device);

    void SendConsultData(const CastInnerRemoteDevice &device, int port);
    std::string GetConsultationData(const CastInnerRemoteDevice &device, int port, json &body);

    void EncryptPort(int port, const uint8_t *sessionKey, json &body);
    void EncryptIp(const std::string &ip, const std::string &key, const uint8_t *sessionKey, json &body);
    std::unique_ptr<uint8_t[]> intToByteArray(int32_t num);
    std::string convLatin1ToUTF8(std::string &latin1);

    void DestroyConsulationSession(const std::string &deviceId);
    void RemoveTransIdMapping(int transportId);
    int GetCastSessionId(int transportId);

    bool ParseAndCheckJsonData(const std::string &data, json &jsonData);
    std::unique_ptr<CastInnerRemoteDevice> GetRemoteFromJsonData(const std::string &Data);
    int OpenSoftBusSocket(
        const std::optional<std::string> &networkId, const CastInnerRemoteDevice &device, int32_t &errorCode);
    void WaitAndConnectTargetDevice(const CastInnerRemoteDevice &dev);

    bool IsWifiChannelFirst(const std::string &deviceId);
    bool IsNeedDiscoveryDevice(const CastInnerRemoteDevice &dev);
    bool IsDeviceConnectStateChange(const CastInnerRemoteDevice &device);

    int GetRTSPPort();
    std::shared_ptr<IConnectManagerSessionListener> GetSessionListener(int castSessionId);
    void SetListener(std::shared_ptr<IConnectionManagerListener> listener);
    std::shared_ptr<IConnectionManagerListener> GetListener();
    bool HasListener();
    void ResetListener();

    std::mutex mutex_;
    ProtocolType protocolType_ = ProtocolType::CAST_PLUS_MIRROR;
    std::shared_ptr<IConnectionManagerListener> listener_;
    std::map<int, std::shared_ptr<IConnectManagerSessionListener>> sessionListeners_;
    std::map<int, int> transIdToCastSessionIdMap_;
    bool isDiscoverable_{ false };
    DeviceGrabState grabState_{ DeviceGrabState::NO_GRAB };
    int32_t sessionId_{ -1 };
    int rtspPort_{ INVALID_PORT };
    bool isWifiFresh_ = false;

    // For synchronizing result of openSession between OpenConsultSession and OnOpenSession.
    std::mutex openConsultingSessionMutex_;
};

class CastDeviceStateCallback : public DeviceStateCallback {
public:
    void OnDeviceOnline(const DmDeviceInfo &deviceInfo) override;
    void OnDeviceOffline(const DmDeviceInfo &deviceInfo) override;
    void OnDeviceChanged(const DmDeviceInfo &deviceInfo) override;
    void OnDeviceReady(const DmDeviceInfo &deviceInfo) override;
};
} // namespace CastEngineService
} // namespace CastEngine
} // namespace OHOS

#endif // CONNECTION_MANAGER_H