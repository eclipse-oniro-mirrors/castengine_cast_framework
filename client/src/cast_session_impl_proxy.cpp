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
 * Description: supply cast session implement proxy
 * Author: zhangge
 * Create: 2022-5-29
 */

#include "cast_session_impl_proxy.h"

#include "cast_engine_common_helper.h"
#include "cast_engine_errors.h"
#include "cast_engine_log.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineClient {
DEFINE_CAST_ENGINE_LABEL("Cast-Client-Session");

CastSessionImplProxy::~CastSessionImplProxy()
{
    CLOGI("Stop the client cast session proxy.");
}

int32_t CastSessionImplProxy::RegisterListener(sptr<ICastSessionListenerImpl> listener)
{
    if (!listener) {
        CLOGE("listener is nullptr!");
        return CAST_ENGINE_ERROR;
    }
    MessageParcel data, reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (!data.WriteRemoteObject(listener->AsObject())) {
        CLOGE("Failed to write cast session listener");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(REGISTER_LISTENER, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when registering listener");
        return CAST_ENGINE_ERROR;
    }

    return reply.ReadInt32();
}

int32_t CastSessionImplProxy::UnregisterListener()
{
    MessageParcel data, reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(UNREGISTER_LISTENER, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when unregistering listener");
        return CAST_ENGINE_ERROR;
    }

    return reply.ReadInt32();
}

int32_t CastSessionImplProxy::AddDevice(const CastRemoteDevice &remoteDevice)
{
    MessageParcel data, reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (!WriteCastRemoteDevice(data, remoteDevice)) {
        CLOGE("Failed to write the remote device");
        return CAST_ENGINE_ERROR;
    }

    int32_t ret = Remote()->SendRequest(ADD_DEVICE, data, reply, option);
    if (ret == ERR_UNKNOWN_TRANSACTION) {
        CLOGE("No permission when adding device");
        return ERR_NO_PERMISSION;
    } else if (ret == ERR_INVALID_DATA) {
        CLOGE("Invalid parameter when adding device");
        return ERR_INVALID_PARAM;
    } else if (ret != ERR_NONE) {
        CLOGE("Failed to send ipc request when adding device");
        return CAST_ENGINE_ERROR;
    }

    return reply.ReadInt32();
}

int32_t CastSessionImplProxy::RemoveDevice(const std::string &deviceId)
{
    MessageParcel data, reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (!data.WriteString(deviceId)) {
        CLOGE("Failed to write the device id");
        return CAST_ENGINE_ERROR;
    }

    int32_t ret = Remote()->SendRequest(REMOVE_DEVICE, data, reply, option);
    if (ret == ERR_UNKNOWN_TRANSACTION) {
        CLOGE("No permission when removing device");
        return ERR_NO_PERMISSION;
    } else if (ret == ERR_INVALID_DATA) {
        CLOGE("Invalid parameter when removing device");
        return ERR_INVALID_PARAM;
    } else if (ret != ERR_NONE) {
        CLOGE("Failed to send ipc request when removing device");
        return CAST_ENGINE_ERROR;
    }

    return reply.ReadInt32();
}

int32_t CastSessionImplProxy::StartAuth(const AuthInfo &authInfo)
{
    MessageParcel data, reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return false;
    }
    if (!WriteAuthInfo(data, authInfo)) {
        CLOGE("Failed to write auth info");
        return false;
    }

    int32_t ret = Remote()->SendRequest(START_AUTH, data, reply, option);
    if (ret == ERR_UNKNOWN_TRANSACTION) {
        CLOGE("No permission when starting auth");
        return ERR_NO_PERMISSION;
    } else if (ret == ERR_INVALID_DATA) {
        CLOGE("Invalid parameter when starting auth");
        return ERR_INVALID_PARAM;
    } else if (ret != ERR_NONE) {
        CLOGE("Failed to send ipc request when starting auth");
        return CAST_ENGINE_ERROR;
    }

    return reply.ReadInt32();
}

int32_t CastSessionImplProxy::Release()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }

    int32_t ret = Remote()->SendRequest(RELEASE, data, reply, option);
    if (ret == ERR_UNKNOWN_TRANSACTION) {
        CLOGE("No permission when releasing the cast session");
        return ERR_NO_PERMISSION;
    } else if (ret != ERR_NONE) {
        CLOGE("Failed to send ipc request when releasing the cast session");
        return CAST_ENGINE_ERROR;
    }

    return reply.ReadInt32();
}

int32_t CastSessionImplProxy::GetSessionId(std::string &sessionId)
{
    MessageParcel data, reply;
    MessageOption option;

    sessionId = std::string{};
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }

    int32_t ret = Remote()->SendRequest(GET_SESSION_ID, data, reply, option);
    if (ret == ERR_UNKNOWN_TRANSACTION) {
        CLOGE("No permission when getting session id");
        return ERR_NO_PERMISSION;
    } else if (ret != ERR_NONE) {
        CLOGE("Failed to send ipc request when getting session id");
        return CAST_ENGINE_ERROR;
    }

    int32_t errorCode = reply.ReadInt32();
    CHECK_AND_RETURN_RET_LOG(errorCode != CAST_ENGINE_SUCCESS, errorCode, "CastEngine Errors");
    sessionId = reply.ReadString();

    return errorCode;
}

int32_t CastSessionImplProxy::GetDeviceState(const std::string &deviceId, DeviceState &deviceState)
{
    MessageParcel data, reply;
    MessageOption option;

    deviceState = DeviceState::DISCONNECTED;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (!data.WriteString(deviceId)) {
        CLOGE("Failed to write the the device id");
        return CAST_ENGINE_ERROR;
    }

    int32_t ret = Remote()->SendRequest(GET_DEVICE_STATE, data, reply, option);
    if (ret == ERR_UNKNOWN_TRANSACTION) {
        CLOGE("No permission when getting device state");
        return ERR_NO_PERMISSION;
    } else if (ret == ERR_INVALID_DATA) {
        CLOGE("Invalid parameter when getting device state");
        return ERR_INVALID_PARAM;
    } else if (ret != ERR_NONE) {
        CLOGE("Failed to send ipc request when getting device state");
        return CAST_ENGINE_ERROR;
    }

    int32_t errorCode = reply.ReadInt32();
    CHECK_AND_RETURN_RET_LOG(errorCode != CAST_ENGINE_SUCCESS, errorCode, "CastEngine Errors");
    int state = reply.ReadInt32();
    if (IsDeviceState(state)) {
        deviceState = static_cast<DeviceState>(state);
    }

    return errorCode;
}

int32_t CastSessionImplProxy::GetRemoteDeviceInfo(std::string deviceId, CastRemoteDevice &remoteDevice)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (!data.WriteString(deviceId)) {
        CLOGE("Failed to Write remote deviceId");
        return CAST_ENGINE_ERROR;
    }

    int32_t ret = Remote()->SendRequest(GET_REMOTE_DEVICE_INFO, data, reply, option);
    if (ret == ERR_NO_PERMISSION) {
        CLOGE("No permission when getting remote device");
        return ERR_NO_PERMISSION;
    } else if (ret == ERR_INVALID_DATA) {
        CLOGE("Invalid parameter when getting remote device");
        return ERR_INVALID_PARAM;
    } else if (ret != ERR_NONE) {
        CLOGE("Failed to send ipc request when getting remote device");
        return CAST_ENGINE_ERROR;
    }

    int32_t errorCode = reply.ReadInt32();
    CHECK_AND_RETURN_RET_LOG(errorCode != CAST_ENGINE_SUCCESS, errorCode, "CastEngine Errors");

    if (!ReadCastRemoteDevice(reply, remoteDevice)) {
        CLOGE("Failed to get the remote device");
        return CAST_ENGINE_ERROR;
    }
    return errorCode;
}

int32_t CastSessionImplProxy::SetSessionProperty(const CastSessionProperty &property)
{
    MessageParcel data, reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (!WriteCastSessionProperty(data, property)) {
        CLOGE("Failed to write the property");
        return CAST_ENGINE_ERROR;
    }

    int32_t ret = Remote()->SendRequest(SET_SESSION_PROPERTY, data, reply, option);
    if (ret == ERR_UNKNOWN_TRANSACTION) {
        CLOGE("No permission when setting session property");
        return ERR_NO_PERMISSION;
    } else if (ret != ERR_NONE) {
        CLOGE("Failed to send ipc request when setting session property");
        return CAST_ENGINE_ERROR;
    }

    return reply.ReadInt32();
}

int32_t CastSessionImplProxy::SetCastMode(CastMode mode, std::string &jsonParam)
{
    MessageParcel data, reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (!data.WriteInt32(static_cast<int32_t>(mode))) {
        CLOGE("Failed to write cast mode");
        return CAST_ENGINE_ERROR;
    }
    if (!data.WriteString(jsonParam)) {
        CLOGE("Failed to write json param");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(SET_CAST_MODE, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when set cast mode");
        return CAST_ENGINE_ERROR;
    }

    return reply.ReadInt32();
}

int32_t CastSessionImplProxy::CreateMirrorPlayer(sptr<IMirrorPlayerImpl> &mirrorPlayer)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }

    int32_t ret = Remote()->SendRequest(CREATE_MIRROR_PLAYER, data, reply, option);
    if (ret == ERR_UNKNOWN_TRANSACTION) {
        CLOGE("No permission when creating mirror player");
        return ERR_NO_PERMISSION;
    } else if (ret != ERR_NONE) {
        CLOGE("Failed to send ipc request when creating mirror player");
        return CAST_ENGINE_ERROR;
    }

    int32_t errorCode = reply.ReadInt32();
    CHECK_AND_RETURN_RET_LOG(errorCode != CAST_ENGINE_SUCCESS, errorCode, "CastEngine Errors");
    auto object = reply.ReadRemoteObject();
    if (!object) {
        CLOGE("Failed to get the mirror player object");
        return CAST_ENGINE_ERROR;
    }
    mirrorPlayer = iface_cast<IMirrorPlayerImpl>(object);

    return errorCode;
}

int32_t CastSessionImplProxy::CreateStreamPlayer(sptr<IStreamPlayerIpc> &streamPlayer)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }

    int32_t ret = Remote()->SendRequest(CREAT_STREAM_PLAYER, data, reply, option);
    if (ret == ERR_UNKNOWN_TRANSACTION) {
        CLOGE("No permission when creating stream player");
        return ERR_NO_PERMISSION;
    } else if (ret != ERR_NONE) {
        CLOGE("Failed to send ipc request when creating stream player");
        return CAST_ENGINE_ERROR;
    }

    int32_t errorCode = reply.ReadInt32();
    CHECK_AND_RETURN_RET_LOG(errorCode != CAST_ENGINE_SUCCESS, errorCode, "CastEngine Errors");
    auto object = reply.ReadRemoteObject();
    if (!object) {
        CLOGE("Failed to get the cast session object");
        return CAST_ENGINE_ERROR;
    }
    streamPlayer = iface_cast<IStreamPlayerIpc>(object);

    return errorCode;
}

int32_t CastSessionImplProxy::NotifyEvent(EventId eventId, std::string &jsonParam)
{
    MessageParcel data, reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (!data.WriteInt32(static_cast<int32_t>(eventId))) {
        CLOGE("Failed to write event id");
        return CAST_ENGINE_ERROR;
    }
    if (!data.WriteString(jsonParam)) {
        CLOGE("Failed to write json param");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(NOTIFY_EVENT, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when notify event");
        return CAST_ENGINE_ERROR;
    }
    return CAST_ENGINE_SUCCESS;
}

} // namespace CastEngineClient
} // namespace CastEngine
} // namespace OHOS
