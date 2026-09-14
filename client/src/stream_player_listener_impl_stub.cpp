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
 * Description: supply stream player listener implement stub realization.
 * Author: huangchanggui
 * Create: 2023-01-12
 */

#include "stream_player_listener_impl_stub.h"
#include "cast_engine_common_helper.h"
#include "cast_stub_helper.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineClient {
DEFINE_CAST_ENGINE_LABEL("Cast-Client-StreamPlayerListener");

int StreamPlayerListenerImplStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    RETURN_IF_WRONG_TASK(code, data, reply, option);
    if (userListener_ == nullptr) {
        CLOGE("userListener_ is null, code:%{public}d", code);
        return ERR_NULL_OBJECT;
    }
    return EXECUTE_SINGLE_STUB_TASK(code, data, reply);
}

StreamPlayerListenerImplStub::StreamPlayerListenerImplStub(std::shared_ptr<IStreamPlayerListener> userListener)
    : userListener_(userListener)
{
    FILL_SINGLE_STUB_TASK(ON_PLAYER_STATUS_CHANGED, &StreamPlayerListenerImplStub::DoOnStateChangedTask);
    FILL_SINGLE_STUB_TASK(ON_POSITION_CHANGED, &StreamPlayerListenerImplStub::DoOnPositionChangedTask);
    FILL_SINGLE_STUB_TASK(ON_MEDIA_ITEM_CHANGED, &StreamPlayerListenerImplStub::DoOnMediaItemChangedTask);
    FILL_SINGLE_STUB_TASK(ON_VOLUME_CHANGED, &StreamPlayerListenerImplStub::DoOnVolumeChangedTask);
    FILL_SINGLE_STUB_TASK(ON_REPEAT_MODE_CHANGED, &StreamPlayerListenerImplStub::DoOnLoopModeChangedTask);
    FILL_SINGLE_STUB_TASK(ON_PLAY_SPEED_CHANGED, &StreamPlayerListenerImplStub::DoOnPlaySpeedChangedTask);
    FILL_SINGLE_STUB_TASK(ON_PLAYER_ERROR, &StreamPlayerListenerImplStub::DoOnPlayerErrorTask);
    FILL_SINGLE_STUB_TASK(ON_VIDEO_SIZE_CHANGED, &StreamPlayerListenerImplStub::DoOnVideoSizeChangedTask);
    FILL_SINGLE_STUB_TASK(ON_NEXT_REQUEST, &StreamPlayerListenerImplStub::DoOnNextRequestTask);
    FILL_SINGLE_STUB_TASK(ON_PREVIOUS_REQUEST, &StreamPlayerListenerImplStub::DoOnPreviousRequestTask);
    FILL_SINGLE_STUB_TASK(ON_SEEK_DONE, &StreamPlayerListenerImplStub::DoOnSeekDoneTask);
    FILL_SINGLE_STUB_TASK(ON_END_OF_STREAM, &StreamPlayerListenerImplStub::DoOnEndOfStreamTask);
    FILL_SINGLE_STUB_TASK(ON_PLAY_REQUEST, &StreamPlayerListenerImplStub::DoOnPlayRequestTask);
    FILL_SINGLE_STUB_TASK(ON_IMAGE_CHANGED, &StreamPlayerListenerImplStub::DoOnImageChangedTask);
    FILL_SINGLE_STUB_TASK(ON_ALBUM_COVER_CHANGED, &StreamPlayerListenerImplStub::DoOnAlbumCoverChangedTask);
    FILL_SINGLE_STUB_TASK(ON_KEY_REQUEST, &StreamPlayerListenerImplStub::DoOnKeyRequestTask);
    FILL_SINGLE_STUB_TASK(ON_AVAILABLE_CAPABILITY_CHANGED,
        &StreamPlayerListenerImplStub::DoOnAvailableCapabilityChangedTask);
    FILL_SINGLE_STUB_TASK(ON_MEDIA_INFO_CHANGED, &StreamPlayerListenerImplStub::DoOnMediaInfoChangedTask);
}

StreamPlayerListenerImplStub::~StreamPlayerListenerImplStub()
{
    userListener_.reset();
    CLOGE("destructor in");
}

int32_t StreamPlayerListenerImplStub::DoOnStateChangedTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(reply);
    int32_t state = data.ReadInt32();
    bool isPlayWhenReady = data.ReadBool();
    PlayerStates playbackState = static_cast<PlayerStates>(state);
    userListener_->OnStateChanged(playbackState, isPlayWhenReady);

    return ERR_NONE;
}

int32_t StreamPlayerListenerImplStub::DoOnPositionChangedTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(reply);
    int32_t position = data.ReadInt32();
    int32_t bufferPosition = data.ReadInt32();
    int32_t duration = data.ReadInt32();
    userListener_->OnPositionChanged(position, bufferPosition, duration);

    return ERR_NONE;
}

int32_t StreamPlayerListenerImplStub::DoOnMediaItemChangedTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(reply);
    auto mediaInfo = ReadMediaInfo(data);
    if (mediaInfo == nullptr) {
        CLOGE("DoOnMediaItemChangedTask,mediaInfo is null");
        return ERR_NULL_OBJECT;
    }
    userListener_->OnMediaItemChanged(*mediaInfo);

    return ERR_NONE;
}

int32_t StreamPlayerListenerImplStub::DoOnVolumeChangedTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(reply);
    int32_t volume = data.ReadInt32();
    int32_t maxVolume = data.ReadInt32();
    userListener_->OnVolumeChanged(volume, maxVolume);

    return ERR_NONE;
}

int32_t StreamPlayerListenerImplStub::DoOnLoopModeChangedTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(reply);
    int32_t mode = data.ReadInt32();
    LoopMode loopMode = static_cast<LoopMode>(mode);
    userListener_->OnLoopModeChanged(loopMode);

    return ERR_NONE;
}

int32_t StreamPlayerListenerImplStub::DoOnPlaySpeedChangedTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(reply);
    int32_t speed = data.ReadInt32();
    PlaybackSpeed speedMode = static_cast<PlaybackSpeed>(speed);
    userListener_->OnPlaySpeedChanged(speedMode);

    return ERR_NONE;
}

int32_t StreamPlayerListenerImplStub::DoOnPlayerErrorTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(reply);
    int32_t errorCode = data.ReadInt32();
    std::string errorMsg = data.ReadString();
    userListener_->OnPlayerError(errorCode, errorMsg);

    return ERR_NONE;
}
int32_t StreamPlayerListenerImplStub::DoOnVideoSizeChangedTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(reply);
    int32_t width = data.ReadInt32();
    int32_t height = data.ReadInt32();
    userListener_->OnVideoSizeChanged(width, height);

    return ERR_NONE;
}

int32_t StreamPlayerListenerImplStub::DoOnNextRequestTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(data);
    static_cast<void>(reply);
    userListener_->OnNextRequest();

    return ERR_NONE;
}

int32_t StreamPlayerListenerImplStub::DoOnPreviousRequestTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(data);
    static_cast<void>(reply);
    userListener_->OnPreviousRequest();

    return ERR_NONE;
}

int32_t StreamPlayerListenerImplStub::DoOnSeekDoneTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(reply);
    int32_t position = data.ReadInt32();
    userListener_->OnSeekDone(position);

    return ERR_NONE;
}

int32_t StreamPlayerListenerImplStub::DoOnEndOfStreamTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(reply);
    int32_t isLooping = data.ReadInt32();
    userListener_->OnEndOfStream(isLooping);

    return ERR_NONE;
}

int32_t StreamPlayerListenerImplStub::DoOnPlayRequestTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(reply);
    auto mediaInfo = ReadMediaInfo(data);
    if (mediaInfo == nullptr) {
        CLOGE("DoOnPlayRequestTask, mediaInfo is null");
        return ERR_NULL_OBJECT;
    }
    userListener_->OnPlayRequest(*mediaInfo);

    return ERR_NONE;
}

int32_t StreamPlayerListenerImplStub::DoOnImageChangedTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(reply);
    Media::PixelMap *pixelMap = Media::PixelMap::Unmarshalling(data);
    if (pixelMap == nullptr) {
        CLOGE("DoOnImageChangedTask, pixelMap is null");
        return ERR_NULL_OBJECT;
    }
    std::shared_ptr<Media::PixelMap> pixelMapShared(pixelMap);
    userListener_->OnImageChanged(pixelMapShared);
 
    return ERR_NONE;
}

int32_t StreamPlayerListenerImplStub::DoOnAlbumCoverChangedTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(reply);
    Media::PixelMap *pixelMap = Media::PixelMap::Unmarshalling(data);
    if (pixelMap == nullptr) {
        CLOGE("DoOnAlbumCoverChangedTask, pixelMap is null");
        return ERR_NULL_OBJECT;
    }
    std::shared_ptr<Media::PixelMap> pixelMapShared(pixelMap);
    userListener_->OnAlbumCoverChanged(pixelMapShared);
 
    return ERR_NONE;
}

int32_t StreamPlayerListenerImplStub::DoOnKeyRequestTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(reply);
    std::string mediaId = data.ReadString();
    std::vector<uint8_t> request;
    int32_t requestSize = data.ReadInt32();
    const uint8_t *requestBuf = data.ReadBuffer(static_cast<size_t>(requestSize));
    if (requestSize == 0 || requestBuf == nullptr) {
        CLOGE("invalid buffer, len = %{public}d", requestSize);
        return ERR_NULL_OBJECT;
    }
    request.assign(requestBuf, requestBuf + requestSize);
    userListener_->OnKeyRequest(mediaId, request);
 
    return ERR_NONE;
}

int32_t StreamPlayerListenerImplStub::DoOnAvailableCapabilityChangedTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(reply);
    auto streamCapability = ReadStreamCapability(data);
    userListener_->OnAvailableCapabilityChanged(streamCapability);

    return ERR_NONE;
}

int32_t StreamPlayerListenerImplStub::DoOnMediaInfoChangedTask(MessageParcel &data, MessageParcel &reply)
{
    static_cast<void>(reply);
    auto mediaInfo = ReadMediaInfo(data);
    if (mediaInfo == nullptr) {
        CLOGE("DoOnMediaInfoChangedTask, mediaInfo is null");
        return ERR_NULL_OBJECT;
    }
    if (userListener_ == nullptr) {
        CLOGE("userListener_ is null");
        CheckAndCloseFd(*mediaInfo);
        return ERR_NULL_OBJECT;
    }
    userListener_->OnMediaInfoChanged(*mediaInfo);
    CheckAndCloseFd(*mediaInfo);
    return ERR_NONE;
}

void StreamPlayerListenerImplStub::OnStateChanged(const PlayerStates playbackState, bool isPlayWhenReady)
{
    static_cast<void>(playbackState);
    static_cast<void>(isPlayWhenReady);
}

void StreamPlayerListenerImplStub::OnPositionChanged(int position, int bufferPosition, int duration)
{
    static_cast<void>(position);
    static_cast<void>(bufferPosition);
    static_cast<void>(duration);
}

void StreamPlayerListenerImplStub::OnMediaItemChanged(const MediaInfo &mediaInfo)
{
    static_cast<void>(mediaInfo);
}

void StreamPlayerListenerImplStub::OnVolumeChanged(int volume, int maxVolume)
{
    static_cast<void>(volume);
    static_cast<void>(maxVolume);
}

void StreamPlayerListenerImplStub::OnLoopModeChanged(const LoopMode loopMode)
{
    static_cast<void>(loopMode);
}

void StreamPlayerListenerImplStub::OnPlaySpeedChanged(const PlaybackSpeed speed)
{
    static_cast<void>(speed);
}

void StreamPlayerListenerImplStub::OnPlayerError(int errorCode, const std::string &errorMsg)
{
    static_cast<void>(errorCode);
    static_cast<void>(errorMsg);
}

void StreamPlayerListenerImplStub::OnVideoSizeChanged(int width, int height)
{
    static_cast<void>(width);
    static_cast<void>(height);
}

void StreamPlayerListenerImplStub::OnNextRequest()
{
}

void StreamPlayerListenerImplStub::OnPreviousRequest()
{
}

void StreamPlayerListenerImplStub::OnSeekDone(int position)
{
    static_cast<void>(position);
}

void StreamPlayerListenerImplStub::OnEndOfStream(int isLooping)
{
    static_cast<void>(isLooping);
}

void StreamPlayerListenerImplStub::OnPlayRequest(const MediaInfo &mediaInfo)
{
    static_cast<void>(mediaInfo);
}

void StreamPlayerListenerImplStub::OnImageChanged(std::shared_ptr<Media::PixelMap> pixelMap)
{
    static_cast<void>(pixelMap);
}

void StreamPlayerListenerImplStub::OnAlbumCoverChanged(std::shared_ptr<Media::PixelMap> pixelMap)
{
    static_cast<void>(pixelMap);
}

void StreamPlayerListenerImplStub::OnKeyRequest(const std::string &mediaId, const std::vector<uint8_t> &keyRequestData)
{
    static_cast<void>(mediaId);
    static_cast<void>(keyRequestData);
}

void StreamPlayerListenerImplStub::OnAvailableCapabilityChanged(const StreamCapability &streamCapability)
{
    static_cast<void>(streamCapability);
}

void StreamPlayerListenerImplStub::OnMediaInfoChanged(const MediaInfo &mediaInfo)
{
    static_cast<void>(mediaInfo);
}
} // namespace CastEngineClient
} // namespace CastEngine
} // namespace OHOS
