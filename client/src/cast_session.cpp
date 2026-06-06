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
 * Description: Cast Session function realization.
 * Author: zhangge
 * Create: 2022-06-15
 */

#include "cast_session.h"

#include "cast_engine_errors.h"
#include "cast_engine_log.h"
#include "cast_session_listener_impl_stub.h"
#include "surface_utils.h"
#include "stream_player.h"
#include "mirror_player.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineClient {
DEFINE_CAST_ENGINE_LABEL("Cast-Client-Session");

CastSession::~CastSession()
{
    CLOGI("Stop the client cast session.");
}

int32_t CastSession::RegisterListener(std::shared_ptr<ICastSessionListener> listener)
{
    if (listener == nullptr) {
        CLOGE("The listener is null");
        return ERR_INVALID_PARAM;
    }
    sptr<ICastSessionListenerImpl> listenerStub = new (std::nothrow) CastSessionListenerImplStub(listener);
    if (listenerStub == nullptr) {
        CLOGE("Failed to new a session listener");
        return ERR_NO_MEMORY;
    }

    return proxy_ ? proxy_->RegisterListener(listenerStub) : CAST_ENGINE_ERROR;
}

int32_t CastSession::SetSessionId(std::string sessionId)
{
    sessionId_ = sessionId;
    return CAST_ENGINE_SUCCESS;
}

int32_t CastSession::UnregisterListener()
{
    return proxy_ ? proxy_->UnregisterListener() : CAST_ENGINE_ERROR;
}

int32_t CastSession::AddDevice(const CastRemoteDevice &remoteDevice, const ConnectionConfig &connectionConfig)
{
    if (remoteDevice.deviceId.empty()) {
        CLOGE("The remote device id is null");
        return ERR_INVALID_PARAM;
    }
    return proxy_ ? proxy_->AddDevice(remoteDevice, connectionConfig) : CAST_ENGINE_ERROR;
}

int32_t CastSession::RemoveDevice(const std::string &deviceId, const DeviceRemoveAction &actionType)
{
    if (deviceId.empty()) {
        CLOGE("The device id is null");
        return ERR_INVALID_PARAM;
    }
    return proxy_ ? proxy_->RemoveDevice(deviceId) : CAST_ENGINE_ERROR;
}

int32_t CastSession::StartAuth(const AuthInfo &authInfo)
{
    if (authInfo.deviceId.empty()) {
        CLOGE("The device id is null");
        return ERR_INVALID_PARAM;
    }
    return proxy_ ? proxy_->StartAuth(authInfo) : CAST_ENGINE_ERROR;
}

int32_t CastSession::GetSessionId(std::string &sessionId)
{
    if (!proxy_) {
        CLOGE("proxy is null");
        return CAST_ENGINE_ERROR;
    }
    int32_t ret = proxy_->GetSessionId(sessionId_);
    sessionId = sessionId_;
    return ret;
}

int32_t CastSession::SetSessionProperty(const CastSessionProperty &property)
{
    return proxy_ ? proxy_->SetSessionProperty(property) : CAST_ENGINE_ERROR;
}

int32_t CastSession::CreateMirrorPlayer(std::shared_ptr<IMirrorPlayer> &mirrorPlayer)
{
    if (!proxy_) {
        CLOGE("proxy_ is null");
        return CAST_ENGINE_ERROR;
    }
    sptr<IMirrorPlayerImpl> impl;
    int32_t ret = proxy_->CreateMirrorPlayer(impl);
    CHECK_AND_RETURN_RET_LOG(ret != CAST_ENGINE_SUCCESS, ret, "CastEngine Errors");
    if (!impl) {
        return CAST_ENGINE_ERROR;
    }

    auto player = std::make_shared<MirrorPlayer>(impl);
    if (!player) {
        CLOGE("Failed to malloc mirror player");
        return ERR_NO_MEMORY;
    }
    mirrorPlayer = player;
    return ret;
}

int32_t CastSession::CreateStreamPlayer(std::shared_ptr<IStreamPlayer> &streamPlayer)
{
    if (!proxy_) {
        CLOGE("proxy_ is null");
        return CAST_ENGINE_ERROR;
    }
    sptr<IStreamPlayerIpc> streamPlayerIpc;
    int32_t ret = proxy_->CreateStreamPlayer(streamPlayerIpc);
    CHECK_AND_RETURN_RET_LOG(ret != CAST_ENGINE_SUCCESS, ret, "CastEngine Errors");
    if (!streamPlayerIpc) {
        return CAST_ENGINE_ERROR;
    }

    auto player = std::make_shared<StreamPlayer>(streamPlayerIpc);
    if (!player) {
        CLOGE("Failed to malloc stream player");
        return ERR_NO_MEMORY;
    }
    streamPlayer = player;
    return ret;
}

int32_t CastSession::Release()
{
    return proxy_ ? proxy_->Release() : CAST_ENGINE_ERROR;
}

int32_t CastSession::SetCastMode(CastMode mode, std::string &jsonParam)
{
    return proxy_ ? proxy_->SetCastMode(mode, jsonParam) : false;
}

int32_t CastSession::NotifyEvent(EventId eventId, std::string &jsonParam)
{
    if (proxy_ != nullptr) {
        proxy_->NotifyEvent(eventId, jsonParam);
        return CAST_ENGINE_SUCCESS;
    }
    return CAST_ENGINE_ERROR;
}

int32_t CastSession::GetRemoteDeviceInfo(std::string deviceId, CastRemoteDevice &remoteDevice)
{
    if (!proxy_) {
        CLOGE("proxy_ is null");
        return CAST_ENGINE_ERROR;
    }
    return proxy_->GetRemoteDeviceInfo(deviceId, remoteDevice);
}
} // namespace CastEngineClient
} // namespace CastEngine
} // namespace OHOS