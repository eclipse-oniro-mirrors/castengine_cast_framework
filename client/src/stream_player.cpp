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
 * Description: Stream Player function realization.
 * Author: huangchanggui
 * Create: 2023-01-12
 */

#include "stream_player.h"

#include "cast_engine_errors.h"
#include "cast_engine_log.h"
#include "stream_player_listener_impl_stub.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineClient {
DEFINE_CAST_ENGINE_LABEL("Cast-Client-StreamPlayer");

StreamPlayer::~StreamPlayer()
{
    CLOGD("destructor in");
}

int32_t StreamPlayer::RegisterListener(std::shared_ptr<IStreamPlayerListener> listener)
{
    if (listener == nullptr) {
        CLOGE("listener is null");
        return ERR_INVALID_PARAM;
    }
    sptr<IStreamPlayerListenerImpl> listenerStub = new (std::nothrow) StreamPlayerListenerImplStub(listener);
    if (listenerStub == nullptr) {
        CLOGE("Failed to new a stream player listener");
        return CAST_ENGINE_ERROR;
    }

    return proxy_ ? proxy_->RegisterListener(listenerStub) : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::UnregisterListener()
{
    return proxy_ ? proxy_->UnregisterListener() : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::SetSurface(const std::string &surfaceId)
{
    errno = 0;
    char *end = nullptr;

    uint64_t surfaceUniqueId = static_cast<uint64_t>(std::strtoll(surfaceId.c_str(), &end, 10));
    if (errno == ERANGE || (surfaceUniqueId == 0 && *end != '\0')) {
        return ERR_INVALID_PARAM;
    }

    sptr<Surface> surface = SurfaceUtils::GetInstance()->GetSurface(surfaceUniqueId);
    if (!surface) {
        CLOGE("surface is null, surface uniqueId %" PRIu64, surfaceUniqueId);
        return CAST_ENGINE_ERROR;
    }

    sptr<IBufferProducer> producer = surface->GetProducer();
    if (!producer) {
        CLOGE("producer is null");
        return CAST_ENGINE_ERROR;
    }

    return proxy_ ? proxy_->SetSurface(producer) : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::Load(const MediaInfo &mediaInfo)
{
    return proxy_ ? proxy_->Load(mediaInfo) : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::Play(const MediaInfo &mediaInfo)
{
    return proxy_ ? proxy_->Play(mediaInfo) : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::Play(int index)
{
    return proxy_ ? proxy_->Play(index) : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::Play()
{
    return proxy_ ? proxy_->Play() : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::Pause()
{
    return proxy_ ? proxy_->Pause() : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::Stop()
{
    return proxy_ ? proxy_->Stop() : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::Next()
{
    return proxy_ ? proxy_->Next() : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::Previous()
{
    return proxy_ ? proxy_->Previous() : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::Seek(int position)
{
    return proxy_ ? proxy_->Seek(position) : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::FastForward(int delta)
{
    return proxy_ ? proxy_->FastForward(delta) : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::FastRewind(int delta)
{
    return proxy_ ? proxy_->FastRewind(delta) : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::SetVolume(int volume)
{
    return proxy_ ? proxy_->SetVolume(volume) : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::SetMute(bool mute)
{
    return proxy_ ? proxy_->SetMute(mute) : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::SetLoopMode(const LoopMode mode)
{
    return proxy_ ? proxy_->SetLoopMode(mode) : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::SetAvailableCapability(const StreamCapability &streamCapability)
{
    return proxy_ ? proxy_->SetAvailableCapability(streamCapability) : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::SetSpeed(const PlaybackSpeed speed)
{
    return proxy_ ? proxy_->SetSpeed(speed) : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::SendData(const DataType dataType, const std::string &dataStr)
{
    return CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::GetPlayerStatus(PlayerStates &playerStates)
{
    return proxy_ ? proxy_->GetPlayerStatus(playerStates) : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::GetPosition(int &position)
{
    return proxy_ ? proxy_->GetPosition(position) : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::GetDownloadRate(int64_t &totalAverageRate, int64_t &lastSecondRate)
{
    return proxy_ ? proxy_->GetDownloadRate(totalAverageRate, lastSecondRate) : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::GetDuration(int &duration)
{
    return proxy_ ? proxy_->GetDuration(duration) : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::GetVolume(int &volume, int &maxVolume)
{
    return proxy_ ? proxy_->GetVolume(volume, maxVolume) : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::GetMute(bool &mute)
{
    return proxy_ ? proxy_->GetMute(mute) : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::GetLoopMode(LoopMode &loopMode)
{
    return proxy_ ? proxy_->GetLoopMode(loopMode) : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::GetAvailableCapability(StreamCapability &streamCapability)
{
    return proxy_ ? proxy_->GetAvailableCapability(streamCapability) : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::GetPlaySpeed(PlaybackSpeed &playbackSpeed)
{
    return proxy_ ? proxy_->GetPlaySpeed(playbackSpeed) : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::GetMediaInfoHolder(MediaInfoHolder &mediaInfoHolder)
{
    return proxy_ ? proxy_->GetMediaInfoHolder(mediaInfoHolder) : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::ProvideKeyResponse(const std::string &mediaId, const std::vector<uint8_t> &response)
{
    return proxy_ ? proxy_->ProvideKeyResponse(mediaId, response) : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::Release()
{
    return proxy_ ? proxy_->Release() : CAST_ENGINE_ERROR;
}

int32_t StreamPlayer::GetMediaCapabilities(std::string &jsonCapabilities)
{
    return CAST_ENGINE_ERROR;
}

} // namespace CastEngineClient
} // namespace CastEngine
} // namespace OHOS