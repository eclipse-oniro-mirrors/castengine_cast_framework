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
 * Description: supply stream player implement stub class.
 * Author: huangchanggui
 * Create: 2023-01-13
 */

#include "stream_player_impl_stub.h"

#include "ipc_skeleton.h"
#include "cast_engine_common_helper.h"
#include "permission.h"
#include "stream_player_listener_impl_proxy.h"
#include "cast_engine_errors.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineService {
DEFINE_CAST_ENGINE_LABEL("Cast-StreamPlayerImpl");

int StreamPlayerImplStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    if (!Permission::CheckStreamPermission()) {
        return ERR_NO_PERMISSION;
    }

    if (!Permission::CheckPidPermission()) {
        return ERR_NO_PERMISSION;
    }
    RETURN_IF_WRONG_TASK(code, data, reply, option);
    return EXECUTE_SINGLE_STUB_TASK(code, data, reply);
}

StreamPlayerImplStub::StreamPlayerImplStub(std::shared_ptr<IStreamPlayerImpl> streamPlayerImpl)
{
    streamPlayerImpl_ = streamPlayerImpl;
    FILL_SINGLE_STUB_TASK(REGISTER_LISTENER, &StreamPlayerImplStub::DoRegisterListenerTask);
    FILL_SINGLE_STUB_TASK(UNREGISTER_LISTENER, &StreamPlayerImplStub::DoUnregisterListenerTask);
    FILL_SINGLE_STUB_TASK(SET_SURFACE, &StreamPlayerImplStub::DoSetSurface);
    FILL_SINGLE_STUB_TASK(LOAD, &StreamPlayerImplStub::DoLoadTask);
    FILL_SINGLE_STUB_TASK(START, &StreamPlayerImplStub::DoStartTask);
    FILL_SINGLE_STUB_TASK(PLAY_INDEX, &StreamPlayerImplStub::DoLocalPlayTask);
    FILL_SINGLE_STUB_TASK(PLAY, &StreamPlayerImplStub::DoPlayTask);
    FILL_SINGLE_STUB_TASK(PAUSE, &StreamPlayerImplStub::DoPauseTask);
    FILL_SINGLE_STUB_TASK(STOP, &StreamPlayerImplStub::DoStopTask);
    FILL_SINGLE_STUB_TASK(NEXT, &StreamPlayerImplStub::DoNextTask);
    FILL_SINGLE_STUB_TASK(PREVIOUS, &StreamPlayerImplStub::DoPreviousTask);
    FILL_SINGLE_STUB_TASK(SEEK, &StreamPlayerImplStub::DoSeekTask);
    FILL_SINGLE_STUB_TASK(FAST_FORWARD, &StreamPlayerImplStub::DoFastForwardTask);
    FILL_SINGLE_STUB_TASK(FAST_REWIND, &StreamPlayerImplStub::DoFastRewindTask);
    FILL_SINGLE_STUB_TASK(SET_VOLUME, &StreamPlayerImplStub::DoSetVolumeTask);
    FILL_SINGLE_STUB_TASK(SET_MUTE, &StreamPlayerImplStub::DoSetMuteTask);
    FILL_SINGLE_STUB_TASK(SET_LOOP_MODE, &StreamPlayerImplStub::DoSetLoopModeTask);
    FILL_SINGLE_STUB_TASK(SET_AVAILABLE_CAPABILITY, &StreamPlayerImplStub::DoSetAvailableCapabilityTask);
    FILL_SINGLE_STUB_TASK(SET_SPEED, &StreamPlayerImplStub::DoSetSpeedTask);
    FILL_SINGLE_STUB_TASK(GET_PLAYER_STATUS, &StreamPlayerImplStub::DoGetPlayerStatusTask);
    FILL_SINGLE_STUB_TASK(GET_POSITION, &StreamPlayerImplStub::DoGetPositionTask);
    FILL_SINGLE_STUB_TASK(GET_DURATION, &StreamPlayerImplStub::DoGetDurationTask);
    FILL_SINGLE_STUB_TASK(GET_VOLUME, &StreamPlayerImplStub::DoGetVolumeTask);
    FILL_SINGLE_STUB_TASK(GET_LOOP_MODE, &StreamPlayerImplStub::DoGetLoopModeTask);
    FILL_SINGLE_STUB_TASK(GET_AVAILABLE_CAPABILITY, &StreamPlayerImplStub::DoGetAvailableCapabilityTask);
    FILL_SINGLE_STUB_TASK(GET_PLAY_SPEED, &StreamPlayerImplStub::DoGetPlaySpeedTask);
    FILL_SINGLE_STUB_TASK(GET_MEDIA_INFO_HOLDER, &StreamPlayerImplStub::DoGetMediaInfoHolderTask);
    FILL_SINGLE_STUB_TASK(RELEASE, &StreamPlayerImplStub::DoReleaseTask);
}

int32_t StreamPlayerImplStub::DoRegisterListenerTask(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> obj = data.ReadRemoteObject();
    if (obj == nullptr) {
        return ERR_NULL_OBJECT;
    }

    sptr<IStreamPlayerListenerImpl> listener = new (std::nothrow) StreamPlayerListenerImplProxy(obj);
    if (!reply.WriteInt32(RegisterListener(listener))) {
        CLOGE("Failed to write int value");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoUnregisterListenerTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(data);
    if (!reply.WriteInt32(UnregisterListener())) {
        CLOGE("Failed to write bool value");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoSetSurface(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> remoteObj = data.ReadRemoteObject();
    if (remoteObj == nullptr) {
        CLOGE("BufferProducer is null");
        return ERR_NULL_OBJECT;
    }

    sptr<IBufferProducer> producer = iface_cast<IBufferProducer>(remoteObj);
    if (!reply.WriteInt32(SetSurface(producer))) {
        CLOGE("Failed to write int SetSurface");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoLoadTask(MessageParcel &data, MessageParcel &reply)
{
    auto mediaInfo = ReadMediaInfo(data);
    if (mediaInfo == nullptr) {
        CLOGE("Invalid remote device object comes");
        return ERR_INVALID_DATA;
    }

    if (!reply.WriteInt32(Load(*mediaInfo))) {
        CLOGE("Failed to write int value");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoStartTask(MessageParcel &data, MessageParcel &reply)
{
    auto mediaInfo = ReadMediaInfo(data);
    if (mediaInfo == nullptr) {
        CLOGE("Invalid remote device object comes");
        return ERR_INVALID_DATA;
    }

    if (!reply.WriteInt32(Play(*mediaInfo))) {
        CLOGE("Failed to write int value");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoLocalPlayTask(MessageParcel &data, MessageParcel &reply)
{
    int32_t index = data.ReadInt32();
    if (!reply.WriteInt32(Play(index))) {
        CLOGE("Failed to write int value");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoPlayTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(data);
    if (!reply.WriteInt32(Play())) {
        CLOGE("Failed to write int value");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoPauseTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(data);
    if (!reply.WriteInt32(Pause())) {
        CLOGE("Failed to write int value");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoStopTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(data);
    if (!reply.WriteInt32(Stop())) {
        CLOGE("Failed to write int value");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoNextTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(data);
    if (!reply.WriteInt32(Next())) {
        CLOGE("Failed to write int value");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoPreviousTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(data);
    if (!reply.WriteInt32(Previous())) {
        CLOGE("Failed to write int value");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoSeekTask(MessageParcel &data, MessageParcel &reply)
{
    int32_t position = data.ReadInt32();
    if (!reply.WriteInt32(Seek(position))) {
        CLOGE("Failed to write int value");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoFastForwardTask(MessageParcel &data, MessageParcel &reply)
{
    int32_t delta = data.ReadInt32();
    if (!reply.WriteInt32(FastForward(delta))) {
        CLOGE("Failed to write int value");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoFastRewindTask(MessageParcel &data, MessageParcel &reply)
{
    int32_t delta = data.ReadInt32();
    if (!reply.WriteInt32(FastRewind(delta))) {
        CLOGE("Failed to write int value");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoSetVolumeTask(MessageParcel &data, MessageParcel &reply)
{
    int32_t volume = data.ReadInt32();
    if (!reply.WriteInt32(SetVolume(volume))) {
        CLOGE("Failed to write int value");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoSetMuteTask(MessageParcel &data, MessageParcel &reply)
{
    bool mute = data.ReadBool();
    if (!reply.WriteInt32(SetMute(mute))) {
        CLOGE("Failed to write int value");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoSetLoopModeTask(MessageParcel &data, MessageParcel &reply)
{
    int32_t mode = data.ReadInt32();
    auto loopMode = static_cast<LoopMode>(mode);
    if (!reply.WriteInt32(SetLoopMode(loopMode))) {
        CLOGE("Failed to write int value");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoSetAvailableCapabilityTask(MessageParcel &data, MessageParcel &reply)
{
    StreamCapability streamCapability = ReadStreamCapability(data);
    if (!reply.WriteInt32(SetAvailableCapability(streamCapability))) {
        CLOGE("Failed to write streamCapability value");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoSetSpeedTask(MessageParcel &data, MessageParcel &reply)
{
    int speed = data.ReadInt32();
    auto playbackSpeed = static_cast<PlaybackSpeed>(speed);
    if (!reply.WriteInt32(SetSpeed(playbackSpeed))) {
        CLOGE("Failed to write int value");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoGetPlayerStatusTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(data);
    PlayerStates playerStatus = PlayerStates::PLAYER_STATE_ERROR;
    int32_t ret = GetPlayerStatus(playerStatus);
    if (!reply.WriteInt32(ret)) {
        CLOGE("Failed to write ret:%{public}d", ret);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    int32_t status = static_cast<int32_t>(playerStatus);
    if (!reply.WriteInt32(status)) {
        CLOGE("Failed to write status:%{public}d", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoGetPositionTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(data);
    int32_t position = 0;
    int32_t ret = GetPosition(position);
    if (!reply.WriteInt32(ret)) {
        CLOGE("Failed to write ret:%{public}d", ret);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    if (!reply.WriteInt32(position)) {
        CLOGE("Failed to write position:%{public}d", position);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoGetDurationTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(data);
    int32_t duration = 0;
    int32_t ret = GetDuration(duration);
    if (!reply.WriteInt32(ret)) {
        CLOGE("Failed to write ret:%{public}d", ret);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    if (!reply.WriteInt32(duration)) {
        CLOGE("Failed to write duration:%{public}d", duration);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoGetVolumeTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(data);
    int32_t volume = 0;
    int32_t maxVolume = 0;
    int32_t ret = GetVolume(volume, maxVolume);
    if (!reply.WriteInt32(ret)) {
        CLOGE("Failed to write ret:%{public}d", ret);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    if (!reply.WriteInt32(volume)) {
        CLOGE("Failed to write volume:%{public}d", volume);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    if (!reply.WriteInt32(maxVolume)) {
        CLOGE("Failed to write maxVolume:%{public}d", maxVolume);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoGetMuteTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(data);
    bool mute = false;
    int32_t ret = GetMute(mute);
    if (!reply.WriteInt32(ret)) {
        CLOGE("Failed to write ret:%{public}d", ret);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    if (!reply.WriteBool(mute)) {
        CLOGE("Failed to write mute:%{public}d", mute);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoGetPlaySpeedTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(data);
    PlaybackSpeed speedMode = PlaybackSpeed::SPEED_FORWARD_1_00_X;
    int32_t ret = GetPlaySpeed(speedMode);
    if (!reply.WriteInt32(ret)) {
        CLOGE("Failed to write ret:%{public}d", ret);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    int32_t speed = static_cast<int32_t>(speedMode);
    if (!reply.WriteInt32(speed)) {
        CLOGE("Failed to write speed:%{public}d", speed);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoGetLoopModeTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(data);
    LoopMode mode = LoopMode::LOOP_MODE_LIST;
    int32_t ret = GetLoopMode(mode);
    if (!reply.WriteInt32(ret)) {
        CLOGE("Failed to write ret:%{public}d", ret);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    int32_t loopMode = static_cast<int32_t>(mode);
    if (!reply.WriteInt32(loopMode)) {
        CLOGE("Failed to write mode:%{public}d", loopMode);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoGetAvailableCapabilityTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(data);
    StreamCapability streamCapability{};
    int32_t ret = GetAvailableCapability(streamCapability);
    if (!reply.WriteInt32(ret)) {
        CLOGE("Failed to write ret:%{public}d", ret);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    if (!WriteStreamCapability(reply, streamCapability)) {
        CLOGE("Failed to write streamCapability");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoGetMediaInfoHolderTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(data);
    MediaInfoHolder mediaInfoHolder{};
    int32_t ret = GetMediaInfoHolder(mediaInfoHolder);
    if (!reply.WriteInt32(ret)) {
        CLOGE("Failed to write ret:%{public}d", ret);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    if (!WriteMediaInfoHolder(reply, mediaInfoHolder)) {
        CLOGE("Failed to write mediaInfoHolder");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoProvideKeyResponseTask(MessageParcel &data, MessageParcel &reply)
{
    std::string mediaId = data.ReadString();
    std::vector<uint8_t> response;
    int32_t responseSize = data.ReadInt32();
    const uint8_t *responseBuf = data.ReadBuffer(static_cast<size_t>(responseSize));
    if (responseSize == 0 || responseBuf == nullptr) {
        CLOGE("invalid buffer, len = %{public}d", responseSize);
        return ERR_NULL_OBJECT;
    }
    response.assign(responseBuf, responseBuf + responseSize);

    if (!reply.WriteInt32(ProvideKeyResponse(mediaId, response))) {
        CLOGE("Failed to write int value");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

int32_t StreamPlayerImplStub::DoReleaseTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(data);
    if (!reply.WriteInt32(Release())) {
        CLOGE("Failed to write int value");
        return IPC_STUB_WRITE_PARCEL_ERR;
    }

    return ERR_NONE;
}

std::shared_ptr<IStreamPlayerImpl> StreamPlayerImplStub::PlayerImplGetter()
{
    std::lock_guard<std::mutex> lock(dataMutex_);
    return streamPlayerImpl_;
}

int32_t StreamPlayerImplStub::RegisterListener(sptr<IStreamPlayerListenerImpl> listener)
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->RegisterListener(listener);
}

int32_t StreamPlayerImplStub::UnregisterListener()
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->UnregisterListener();
}

int32_t StreamPlayerImplStub::SetSurface(sptr<IBufferProducer> producer)
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->SetSurface(producer);
}

int32_t StreamPlayerImplStub::Load(const MediaInfo &mediaInfo)
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->Load(mediaInfo);
}

int32_t StreamPlayerImplStub::Play(int index)
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->Play(index);
}

int32_t StreamPlayerImplStub::Play(const MediaInfo &mediaInfo)
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->Play(mediaInfo);
}

int32_t StreamPlayerImplStub::Pause()
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->Pause();
}

int32_t StreamPlayerImplStub::Play()
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->Play();
}

int32_t StreamPlayerImplStub::Stop()
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->Stop();
}

int32_t StreamPlayerImplStub::Next()
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->Next();
}

int32_t StreamPlayerImplStub::Previous()
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->Previous();
}

int32_t StreamPlayerImplStub::Seek(int position)
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->Seek(position);
}

int32_t StreamPlayerImplStub::FastForward(int delta)
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->FastForward(delta);
}

int32_t StreamPlayerImplStub::FastRewind(int delta)
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->FastRewind(delta);
}

int32_t StreamPlayerImplStub::SetVolume(int volume)
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->SetVolume(volume);
}

int32_t StreamPlayerImplStub::SetMute(bool mute)
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->SetMute(mute);
}

int32_t StreamPlayerImplStub::SetLoopMode(const LoopMode mode)
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->SetLoopMode(mode);
}

int32_t StreamPlayerImplStub::SetAvailableCapability(const StreamCapability &streamCapability)
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->SetAvailableCapability(streamCapability);
}

int32_t StreamPlayerImplStub::SetSpeed(const PlaybackSpeed speed)
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->SetSpeed(speed);
}

int32_t StreamPlayerImplStub::GetPlayerStatus(PlayerStates &playerStates)
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->GetPlayerStatus(playerStates);
}

int32_t StreamPlayerImplStub::GetPosition(int &position)
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->GetPosition(position);
}

int32_t StreamPlayerImplStub::GetDuration(int &duration)
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->GetDuration(duration);
}

int32_t StreamPlayerImplStub::GetVolume(int &volume, int &maxVolume)
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->GetVolume(volume, maxVolume);
}

int32_t StreamPlayerImplStub::GetMute(bool &mute)
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->GetMute(mute);
}

int32_t StreamPlayerImplStub::GetLoopMode(LoopMode &loopMode)
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->GetLoopMode(loopMode);
}

int32_t StreamPlayerImplStub::GetAvailableCapability(StreamCapability &streamCapability)
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->GetAvailableCapability(streamCapability);
}

int32_t StreamPlayerImplStub::GetPlaySpeed(PlaybackSpeed &playbackSpeed)
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->GetPlaySpeed(playbackSpeed);
}

int32_t StreamPlayerImplStub::GetMediaInfoHolder(MediaInfoHolder &mediaInfoHolder)
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->GetMediaInfoHolder(mediaInfoHolder);
}

int32_t StreamPlayerImplStub::ProvideKeyResponse(const std::string &mediaId, const std::vector<uint8_t> &response)
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    return streamPlayerImpl->ProvideKeyResponse(mediaId, response);
}

int32_t StreamPlayerImplStub::Release()
{
    auto streamPlayerImpl = PlayerImplGetter();
    if (!streamPlayerImpl) {
        CLOGE("playerImpl is nullptr");
        return CAST_ENGINE_ERROR;
    }
    int ret = streamPlayerImpl->Release();
    std::lock_guard<std::mutex> lock(dataMutex_);
    streamPlayerImpl_ = nullptr;
    return ret;
}
} // namespace CastEngineService
} // namespace CastEngine
} // namespace OHOS
