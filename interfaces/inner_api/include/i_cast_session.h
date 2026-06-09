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
 * Description: Cast session interface.
 * Author: zhangge
 * Create: 2022-06-15
 */

#ifndef I_CAST_SESSION_H
#define I_CAST_SESSION_H

#include "cast_engine_common.h"
#include "oh_remote_control_event.h"
#include "i_stream_player.h"
#include "i_mirror_player.h"

namespace OHOS {
namespace CastEngine {
class EXPORT ICastSessionListener {
public:
    ICastSessionListener() = default;
    ICastSessionListener(const ICastSessionListener &) = delete;
    ICastSessionListener &operator = (const ICastSessionListener &) = delete;
    ICastSessionListener(ICastSessionListener &&) = delete;
    ICastSessionListener &operator = (ICastSessionListener &&) = delete;
    virtual ~ICastSessionListener() = default;

    virtual void OnDeviceState(const DeviceStateInfo &stateInfo) = 0;
    virtual void OnEvent(const EventId &eventId, const std::string &jsonParam) = 0;
    virtual void OnRemoteCtrlEvent(int eventType, const uint8_t *data, uint32_t len) {}
};

class EXPORT ICastSession {
public:
    ICastSession() = default;
    ICastSession(const ICastSession &) = delete;
    ICastSession &operator = (const ICastSession &) = delete;
    ICastSession(ICastSession &&) = delete;
    ICastSession &operator = (ICastSession &&) = delete;
    virtual ~ICastSession() = default;

    virtual int32_t RegisterListener(std::shared_ptr<ICastSessionListener> listener) = 0;
    virtual int32_t UnregisterListener() = 0;
    virtual int32_t AddDevice(const CastRemoteDevice &remoteDevice,
                              const ConnectionConfig &connectionConfig = ConnectionConfig{}) = 0;
    virtual int32_t RemoveDevice(const std::string &deviceId,
        const DeviceRemoveAction &actionType = DeviceRemoveAction::ACTION_DISCONNECT) = 0;
    virtual int32_t StartAuth(const AuthInfo &authInfo) = 0;
    virtual int32_t GetSessionId(std::string &sessionId) = 0;
    virtual int32_t SetSessionProperty(const CastSessionProperty &property) = 0;
    virtual int32_t CreateMirrorPlayer(std::shared_ptr<IMirrorPlayer> &mirrorPlayer) = 0;
    virtual int32_t CreateStreamPlayer(std::shared_ptr<IStreamPlayer> &streamPlayer) = 0;
    virtual int32_t NotifyEvent(EventId eventId, std::string &jsonParam) = 0;
    virtual int32_t SetCastMode(CastMode mode, std::string &jsonParam) = 0;
    virtual int32_t Release() = 0;
    virtual int32_t GetRemoteDeviceInfo(std::string deviceId, CastRemoteDevice &remoteDevice) = 0;
};
} // namespace CastEngine
} // namespace OHOS

#endif
