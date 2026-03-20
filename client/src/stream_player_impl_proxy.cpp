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
 * Description: supply stream player implement proxy realization.
 * Author: huangchanggui
 * Create: 2023-01-12
 */

#include "stream_player_impl_proxy.h"
#include "cast_engine_common_helper.h"
#include "cast_engine_errors.h"
#include "cast_engine_log.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineClient {
DEFINE_CAST_ENGINE_LABEL("Cast-Client-StreamPlayer");

StreamPlayerImplProxy::~StreamPlayerImplProxy()
{
    CLOGD("destructor in");
}

int32_t StreamPlayerImplProxy::RegisterListener(sptr<IStreamPlayerListenerImpl> listener)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (!data.WriteRemoteObject(listener->AsObject())) {
        CLOGE("Failed to write stream player listener");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(REGISTER_LISTENER, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when registering listener");
        return CAST_ENGINE_ERROR;
    }

    return reply.ReadInt32();
}

int32_t StreamPlayerImplProxy::UnregisterListener()
{
    MessageParcel data;
    MessageParcel reply;
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

int32_t StreamPlayerImplProxy::SetSurface(sptr<IBufferProducer> producer)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (!data.WriteRemoteObject(producer->AsObject())) {
        CLOGE("Failed to write surface producer");
        return CAST_ENGINE_ERROR;
    }

    int32_t ret = Remote()->SendRequest(SET_SURFACE, data, reply, option);
    if (ret == ERR_INVALID_DATA) {
        CLOGE("Invalid parameter when setting surface");
        return ERR_INVALID_PARAM;
    } else if (ret != ERR_NONE) {
        CLOGE("Failed to send ipc request when setting surface");
        return CAST_ENGINE_ERROR;
    }

    return reply.ReadInt32();
}

int32_t StreamPlayerImplProxy::Load(const MediaInfo &mediaInfo)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (!WriteMediaInfo(data, mediaInfo)) {
        CLOGE("Failed to write the mediaInfo");
        return CAST_ENGINE_ERROR;
    }

    int32_t ret = Remote()->SendRequest(LOAD, data, reply, option);
    if (ret == ERR_INVALID_DATA) {
        CLOGE("Invalid parameter when load");
        return ERR_INVALID_PARAM;
    } else if (ret != ERR_NONE) {
        CLOGE("Failed to send ipc request when load");
        return CAST_ENGINE_ERROR;
    }

    return reply.ReadInt32();
}

int32_t StreamPlayerImplProxy::Play(const MediaInfo &mediaInfo)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (!WriteMediaInfo(data, mediaInfo)) {
        CLOGE("Failed to write the mediaInfo");
        return CAST_ENGINE_ERROR;
    }

    int32_t ret = Remote()->SendRequest(START, data, reply, option);
    if (ret == ERR_INVALID_DATA) {
        CLOGE("Invalid parameter when play");
        return ERR_INVALID_PARAM;
    } else if (ret != ERR_NONE) {
        CLOGE("Failed to send ipc request when play");
        return CAST_ENGINE_ERROR;
    }

    return reply.ReadInt32();
}

int32_t StreamPlayerImplProxy::Play(int index)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (!data.WriteInt32(index)) {
        CLOGE("Failed to write the index");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(PLAY_INDEX, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when play");
        return CAST_ENGINE_ERROR;
    }

    return reply.ReadInt32();
}

int32_t StreamPlayerImplProxy::Pause()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(PAUSE, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when pause");
        return CAST_ENGINE_ERROR;
    }

    return reply.ReadInt32();
}

int32_t StreamPlayerImplProxy::Play()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(PLAY, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when resume");
        return CAST_ENGINE_ERROR;
    }

    return reply.ReadInt32();
}

int32_t StreamPlayerImplProxy::Stop()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(STOP, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when stop");
        return CAST_ENGINE_ERROR;
    }

    return reply.ReadInt32();
}

int32_t StreamPlayerImplProxy::Next()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(NEXT, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when stop");
        return CAST_ENGINE_ERROR;
    }

    return reply.ReadInt32();
}

int32_t StreamPlayerImplProxy::Previous()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(PREVIOUS, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when stop");
        return CAST_ENGINE_ERROR;
    }

    return reply.ReadInt32();
}

int32_t StreamPlayerImplProxy::Seek(int position)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (!data.WriteInt32(position)) {
        CLOGE("Failed to write the position");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(SEEK, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when seek");
        return CAST_ENGINE_ERROR;
    }

    return reply.ReadInt32();
}

int32_t StreamPlayerImplProxy::FastForward(int delta)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (!data.WriteInt32(delta)) {
        CLOGE("Failed to write the delta");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(FAST_FORWARD, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when fastForward");
        return CAST_ENGINE_ERROR;
    }

    return reply.ReadInt32();
}

int32_t StreamPlayerImplProxy::FastRewind(int delta)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (!data.WriteInt32(delta)) {
        CLOGE("Failed to write the delta");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(FAST_REWIND, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when fastRewind");
        return CAST_ENGINE_ERROR;
    }

    return reply.ReadInt32();
}

int32_t StreamPlayerImplProxy::SetVolume(int volume)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (!data.WriteInt32(volume)) {
        CLOGE("Failed to write the volume");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(SET_VOLUME, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when set volume");
        return CAST_ENGINE_ERROR;
    }

    return reply.ReadInt32();
}

int32_t StreamPlayerImplProxy::SetMute(bool mute)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (!data.WriteBool(mute)) {
        CLOGE("Failed to write the mute");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(SET_MUTE, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when set mute");
        return CAST_ENGINE_ERROR;
    }

    return reply.ReadInt32();
}

int32_t StreamPlayerImplProxy::SetLoopMode(const LoopMode mode)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (!data.WriteInt32(static_cast<int32_t>(mode))) {
        CLOGE("Failed to write the mode");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(SET_LOOP_MODE, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when set volume");
        return CAST_ENGINE_ERROR;
    }

    return reply.ReadInt32();
}

int32_t StreamPlayerImplProxy::SetAvailableCapability(const StreamCapability &streamCapability)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (!WriteStreamCapability(data, streamCapability)) {
        CLOGE("Failed to write the modeScope");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(SET_AVAILABLE_CAPABILITY, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when set available capability");
        return CAST_ENGINE_ERROR;
    }

    return reply.ReadInt32();
}

int32_t StreamPlayerImplProxy::SetSpeed(const PlaybackSpeed speed)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (!data.WriteInt32(static_cast<int32_t>(speed))) {
        CLOGE("Failed to write the position");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(SET_SPEED, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when set volume");
        return CAST_ENGINE_ERROR;
    }

    return reply.ReadInt32();
}

int32_t StreamPlayerImplProxy::GetPlayerStatus(PlayerStates &playerStates)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    playerStates = PlayerStates::PLAYER_STATE_ERROR;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(GET_PLAYER_STATUS, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when get player status");
        return CAST_ENGINE_ERROR;
    }

    int32_t errorCode = reply.ReadInt32();
    CHECK_AND_RETURN_RET_LOG(errorCode != CAST_ENGINE_SUCCESS, errorCode, "CastEngine Errors");
    playerStates = static_cast<PlayerStates>(reply.ReadInt32());

    return errorCode;
}

int32_t StreamPlayerImplProxy::GetPosition(int &position)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(GET_POSITION, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when get position");
        return CAST_ENGINE_ERROR;
    }

    int32_t errorCode = reply.ReadInt32();
    CHECK_AND_RETURN_RET_LOG(errorCode != CAST_ENGINE_SUCCESS, errorCode, "CastEngine Errors");
    position = reply.ReadInt32();

    return errorCode;
}

int32_t StreamPlayerImplProxy::GetDuration(int &duration)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(GET_DURATION, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when get duration");
        return CAST_ENGINE_ERROR;
    }

    int32_t errorCode = reply.ReadInt32();
    CHECK_AND_RETURN_RET_LOG(errorCode != CAST_ENGINE_SUCCESS, errorCode, "CastEngine Errors");
    duration = reply.ReadInt32();

    return errorCode;
}

int32_t StreamPlayerImplProxy::GetVolume(int &volume, int &maxVolume)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(GET_VOLUME, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when get duration");
        return CAST_ENGINE_ERROR;
    }

    int32_t errorCode = reply.ReadInt32();
    CHECK_AND_RETURN_RET_LOG(errorCode != CAST_ENGINE_SUCCESS, errorCode, "CastEngine Errors");
    volume = reply.ReadInt32();
    maxVolume = reply.ReadInt32();

    return errorCode;
}

int32_t StreamPlayerImplProxy::GetMute(bool &mute)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(GET_MUTE, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when query is mute");
        return CAST_ENGINE_ERROR;
    }

    int32_t errorCode = reply.ReadInt32();
    CHECK_AND_RETURN_RET_LOG(errorCode != CAST_ENGINE_SUCCESS, errorCode, "CastEngine Errors");
    mute = reply.ReadBool();

    return errorCode;
}

int32_t StreamPlayerImplProxy::GetLoopMode(LoopMode &loopMode)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    loopMode = LoopMode::LOOP_MODE_LIST;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(GET_LOOP_MODE, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when get duration");
        return CAST_ENGINE_ERROR;
    }

    int32_t errorCode = reply.ReadInt32();
    CHECK_AND_RETURN_RET_LOG(errorCode != CAST_ENGINE_SUCCESS, errorCode, "CastEngine Errors");
    loopMode = static_cast<LoopMode>(reply.ReadInt32());

    return errorCode;
}

int32_t StreamPlayerImplProxy::GetAvailableCapability(StreamCapability &streamCapability)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(GET_AVAILABLE_CAPABILITY, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when get available capability");
        return CAST_ENGINE_ERROR;
    }

    int32_t errorCode = reply.ReadInt32();
    CHECK_AND_RETURN_RET_LOG(errorCode != CAST_ENGINE_SUCCESS, errorCode, "CastEngine Errors");
    streamCapability = ReadStreamCapability(reply);

    return errorCode;
}

int32_t StreamPlayerImplProxy::GetPlaySpeed(PlaybackSpeed &playbackSpeed)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    playbackSpeed = PlaybackSpeed::SPEED_FORWARD_1_00_X;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(GET_PLAY_SPEED, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when get duration");
        return CAST_ENGINE_ERROR;
    }

    int32_t errorCode = reply.ReadInt32();
    CHECK_AND_RETURN_RET_LOG(errorCode != CAST_ENGINE_SUCCESS, errorCode, "CastEngine Errors");
    playbackSpeed = static_cast<PlaybackSpeed>(reply.ReadInt32());

    return errorCode;
}

int32_t StreamPlayerImplProxy::GetMediaInfoHolder(MediaInfoHolder &mediaInfoHolder)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(GET_MEDIA_INFO_HOLDER, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when get duration");
        return CAST_ENGINE_ERROR;
    }

    int32_t errorCode = reply.ReadInt32();
    CHECK_AND_RETURN_RET_LOG(errorCode != CAST_ENGINE_SUCCESS, errorCode, "CastEngine Errors");
    auto mediaInfos = ReadMediaInfoHolder(reply);
    if (mediaInfos == nullptr) {
        CLOGE("GetMediaInfoHolder, mediaInfoHolder is null");
        return CAST_ENGINE_ERROR;
    }
    mediaInfoHolder = *mediaInfos;

    return errorCode;
}

int32_t StreamPlayerImplProxy::ProvideKeyResponse(const std::string &mediaId, const std::vector<uint8_t> &response)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    auto len = response.size();
    if (len > MAX_KEY_RESPONSE_SIZE) {
        CLOGE("Key response size exceeds the maximum");
        return CAST_ENGINE_ERROR;
    }
    if (len > data.GetDataCapacity()) {
        CLOGD("ProvideKeyResponse SetDataCapacity totalSize: %zu", len);

        data.SetMaxCapacity(len + len);
        data.SetDataCapacity(len);
    }

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }

    if (!data.WriteString(mediaId)) {
        CLOGE("Failed to write mediaId");
        return CAST_ENGINE_ERROR;
    }

    if (!data.WriteInt32(response.size())) {
        CLOGE("StreamPlayerImplProxy ProvideKeyResponse Write response size failed");
        return IPC_PROXY_ERR;
    }

    if (len != 0) {
        if (!data.WriteBuffer(response.data(), len)) {
            CLOGE("StreamPlayerImplProxy ProvideKeyResponse write response failed");
            return IPC_PROXY_ERR;
        }
    }

    if (Remote()->SendRequest(PROVIDE_KEY_RESPONSE, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when provide key response");
        return CAST_ENGINE_ERROR;
    }

    int32_t errorCode = reply.ReadInt32();
    CHECK_AND_RETURN_RET_LOG(errorCode != CAST_ENGINE_SUCCESS, errorCode, "CastEngine Errors");
    return errorCode;
}

int32_t StreamPlayerImplProxy::Release()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return CAST_ENGINE_ERROR;
    }
    if (Remote()->SendRequest(RELEASE, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when Releasing stream player");
        return CAST_ENGINE_ERROR;
    }

    return reply.ReadInt32();
}
} // namespace CastEngineClient
} // namespace CastEngine
} // namespace OHOS
