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
 * Description: supply Cast session definition.
 * Author: zhangge
 * Create: 2022-06-15
 */

#ifndef CAST_SESSION_H
#define CAST_SESSION_H

#include "cast_engine_common.h"
#include "i_cast_session.h"
#include "i_cast_session_impl.h"
#include "oh_remote_control_event.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineClient {
class CastSession : public ICastSession {
public:
    CastSession(sptr<ICastSessionImpl> proxy) : proxy_(proxy) {};
    ~CastSession() override;

    int32_t RegisterListener(std::shared_ptr<ICastSessionListener> listener) override;
    int32_t UnregisterListener() override;

    int32_t AddDevice(const CastRemoteDevice &remoteDevice,
                      const ConnectionConfig &connectionConfig = ConnectionConfig{}) override;
    int32_t RemoveDevice(const std::string &deviceId,
        const DeviceRemoveAction &actionType = DeviceRemoveAction::ACTION_DISCONNECT) override;

    int32_t StartAuth(const AuthInfo &authInfo) override;
    int32_t Release() override;
    int32_t NotifyEvent(EventId eventId, std::string &jsonParam) override;

    int32_t GetSessionId(std::string &sessionId) override;
    int32_t GetRemoteDeviceInfo(std::string deviceId, CastRemoteDevice &remoteDevice) override;

    int32_t SetSessionProperty(const CastSessionProperty &property) override;
    int32_t SetSessionId(std::string sessionId);
    int32_t SetCastMode(CastMode mode, std::string &jsonParam) override;

    int32_t CreateMirrorPlayer(std::shared_ptr<IMirrorPlayer> &mirrorPlayer) override;
    int32_t CreateStreamPlayer(std::shared_ptr<IStreamPlayer> &streamPlayer) override;
private:
    sptr<ICastSessionImpl> proxy_;
    std::string sessionId_{};
};
} // namespace CastEngineClient
} // namespace CastEngine
} // namespace OHOS

#endif
