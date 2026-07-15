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
 * Description: supply cast session manager service stub class.
 * Author: zhangge
 * Create: 2022-5-29
 */

#include "cast_session_manager_service_stub.h"

#include "cast_engine_common_helper.h"
#include "cast_engine_log.h"
#include "cast_service_listener_impl_proxy.h"
#include "permission.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineService {
DEFINE_CAST_ENGINE_LABEL("Cast-Service");
constexpr uint32_t MAX_CYCLES_NUM = 10;

int CastSessionManagerServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    RETURN_IF_WRONG_TASK(code, data, reply, option);
    return EXECUTE_SINGLE_STUB_TASK(code, data, reply);
}

CastSessionManagerServiceStub::CastSessionManagerServiceStub()
{
    FILL_SINGLE_STUB_TASK(REGISTER_LISTENER, &CastSessionManagerServiceStub::DoRegisterListenerTask);
    FILL_SINGLE_STUB_TASK(UNREGISTER_LISTENER, &CastSessionManagerServiceStub::DoUnregisterListenerTask);
    FILL_SINGLE_STUB_TASK(RELEASE, &CastSessionManagerServiceStub::DoReleaseTask);
    FILL_SINGLE_STUB_TASK(SET_LOCAL_DEVICE, &CastSessionManagerServiceStub::DoSetLocalDeviceTask);
    FILL_SINGLE_STUB_TASK(CREATE_CAST_SESSION, &CastSessionManagerServiceStub::DoCreateCastSessionTask);
    FILL_SINGLE_STUB_TASK(SET_SINK_SESSION_CAPACITY, &CastSessionManagerServiceStub::DoSetSinkSessionCapacityTask);
    FILL_SINGLE_STUB_TASK(START_DISCOVERY, &CastSessionManagerServiceStub::DoStartDiscoveryTask);
    FILL_SINGLE_STUB_TASK(SET_DISCOVERABLE, &CastSessionManagerServiceStub::DoSetDiscoverableTask);
    FILL_SINGLE_STUB_TASK(STOP_DISCOVERY, &CastSessionManagerServiceStub::DoStopDiscoveryTask);
    FILL_SINGLE_STUB_TASK(START_DEVICE_LOGGING, &CastSessionManagerServiceStub::DoStartDeviceLoggingTask);
    FILL_SINGLE_STUB_TASK(GET_CAST_SESSION, &CastSessionManagerServiceStub::DoGetCastSessionTask);
}

CastSessionManagerServiceStub::~CastSessionManagerServiceStub()
{
    CLOGD("destructor in");
}

int32_t CastSessionManagerServiceStub::DoRegisterListenerTask(MessageParcel &data, MessageParcel &reply)
{
    if (!Permission::CheckMirrorPermission() && !Permission::CheckStreamPermission() &&
        !Permission::CheckPidPermission()) {
        return ERR_NO_PERMISSION;
    }
    sptr<IRemoteObject> obj = data.ReadRemoteObject();
    if (obj == nullptr) {
        return ERR_NULL_OBJECT;
    }

    sptr<ICastServiceListenerImpl> listener{ new (std::nothrow) CastServiceListenerImplProxy(obj) };
    if (listener == nullptr) {
        CLOGE("RegisterListener failed, listener is null");
    }

    if (!reply.WriteInt32(RegisterListener(listener))) {
        CLOGE("Failed to write int value");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t CastSessionManagerServiceStub::DoUnregisterListenerTask(MessageParcel &data, MessageParcel &reply)
{
    if (!Permission::CheckMirrorPermission() && !Permission::CheckStreamPermission() &&
        !Permission::CheckPidPermission()) {
        return ERR_NO_PERMISSION;
    }
    if (!reply.WriteInt32(UnregisterListener())) {
        CLOGE("Failed to write int value");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t CastSessionManagerServiceStub::DoReleaseTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(data);

    if ((!Permission::CheckMirrorPermission() &&
        !Permission::CheckStreamPermission()) ||
        !Permission::CheckPidPermission()) {
        return ERR_NO_PERMISSION;
    }

    if (!reply.WriteInt32(Release())) {
        CLOGE("Failed to write int value");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t CastSessionManagerServiceStub::DoSetLocalDeviceTask(MessageParcel &data, MessageParcel &reply)
{
    if (!Permission::CheckMirrorPermission() || !Permission::CheckPidPermission()) {
        return ERR_NO_PERMISSION;
    }

    auto device = ReadCastLocalDevice(data);
    if (device == nullptr) {
        CLOGE("Invalid device object comes");
        return ERR_INVALID_DATA;
    }

    if (!reply.WriteInt32(SetLocalDevice(*device))) {
        CLOGE("Failed to write int value");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t CastSessionManagerServiceStub::DoCreateCastSessionTask(MessageParcel &data, MessageParcel &reply)
{
    if ((!Permission::CheckMirrorPermission() &&
        !Permission::CheckStreamPermission()) ||
        !Permission::CheckPidPermission()) {
        return ERR_NO_PERMISSION;
    }

    auto property = ReadCastSessionProperty(data);
    if (property == nullptr) {
        CLOGE("Invalid property object comes");
        return ERR_INVALID_DATA;
    }

    sptr<ICastSessionImpl> sessionStub;
    int32_t ret = CreateCastSession(*property, sessionStub);
    if (sessionStub == nullptr) {
        return IPC_STUB_ERR;
    }
    if (!reply.WriteInt32(ret)) {
        CLOGE("Failed to write ret:%d", ret);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    if (!reply.WriteRemoteObject(sessionStub->AsObject())) {
        CLOGE("Failed to write cast session");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t CastSessionManagerServiceStub::DoSetSinkSessionCapacityTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(reply);

    if (!Permission::CheckMirrorPermission() || !Permission::CheckPidPermission()) {
        return ERR_NO_PERMISSION;
    }

    int32_t capacity = data.ReadInt32();
    int32_t ret = SetSinkSessionCapacity(capacity);
    if (!reply.WriteInt32(ret)) {
        CLOGE("Failed to write ret:%d", ret);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t CastSessionManagerServiceStub::DoStartDiscoveryTask(MessageParcel &data, MessageParcel &reply)
{
    if ((!Permission::CheckMirrorPermission() &&
        !Permission::CheckStreamPermission()) ||
        !Permission::CheckPidPermission()) {
        return ERR_NO_PERMISSION;
    }

    int32_t type = data.ReadInt32();
    if (!IsProtocolType(type)) {
        CLOGE("Invalid protocol type comes, %{public}d", type);
        return ERR_INVALID_DATA;
    }
    std::vector<std::string> drmSchemes;
    uint32_t drmSchemeSize = data.ReadUint32();
    for (uint32_t i = 0; (i < drmSchemeSize) && (i < MAX_CYCLES_NUM); i++) {
        drmSchemes.push_back(data.ReadString());
    }
    if (!reply.WriteInt32(StartDiscovery(type, drmSchemes))) {
        CLOGE("Failed to write int value");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t CastSessionManagerServiceStub::DoSetDiscoverableTask(MessageParcel &data, MessageParcel &reply)
{
    if ((!Permission::CheckMirrorPermission() &&
        !Permission::CheckStreamPermission()) ||
        !Permission::CheckPidPermission()) {
        return ERR_NO_PERMISSION;
    }

    bool enable = data.ReadBool();
    if (!reply.WriteInt32(SetDiscoverable(enable))) {
        CLOGE("Failed to write int value");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t CastSessionManagerServiceStub::DoStopDiscoveryTask(MessageParcel &data, MessageParcel &reply)
{
    if ((!Permission::CheckMirrorPermission() &&
        !Permission::CheckStreamPermission()) ||
        !Permission::CheckPidPermission()) {
        return ERR_NO_PERMISSION;
    }

    if (!reply.WriteInt32(StopDiscovery())) {
        CLOGE("Failed to write int value");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t CastSessionManagerServiceStub::DoStartDeviceLoggingTask(MessageParcel &data, MessageParcel &reply)
{
    if ((!Permission::CheckMirrorPermission() && !Permission::CheckStreamPermission()) ||
        !Permission::CheckPidPermission()) {
        return ERR_NO_PERMISSION;
    }
    int32_t fd = data.ReadFileDescriptor();
    uint32_t maxSize = data.ReadUint32();
    if (!reply.WriteInt32(StartDeviceLogging(fd, maxSize))) {
        CLOGE("Failed to write int value");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t CastSessionManagerServiceStub::DoGetCastSessionTask(MessageParcel &data, MessageParcel &reply)
{
    sptr<ICastSessionImpl> sessionStub;
    if (!Permission::CheckPidPermission()) {
        return ERR_NO_PERMISSION;
    }
    int32_t ret = GetCastSession(data.ReadString(), sessionStub);
    if (sessionStub == nullptr) {
        return IPC_STUB_ERR;
    }
    if (!reply.WriteInt32(ret)) {
        CLOGE("Failed to write ret:%d", ret);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    if (!reply.WriteRemoteObject(sessionStub->AsObject())) {
        CLOGE("Failed to write cast session");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}
} // namespace CastEngineService
} // namespace CastEngine
} // namespace OHOS
