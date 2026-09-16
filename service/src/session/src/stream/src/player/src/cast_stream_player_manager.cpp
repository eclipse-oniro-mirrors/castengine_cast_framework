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
 * Description: supply cast stream player manager.
 * Author: zhangjingnan
 * Create: 2023-1-11
 */

#include "cast_engine_errors.h"
#include "cast_engine_log.h"
#include "cast_stream_player_manager.h"
#include "cast_engine_dfx.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineService {
DEFINE_CAST_ENGINE_LABEL("Cast-Stream-Player-Manager");

CastStreamPlayerManager::CastStreamPlayerManager(std::shared_ptr<ICastStreamManagerServer> callback,
    std::shared_ptr<CastLocalFileChannelClient> fileChannel)
{
    CLOGD("CastStreamPlayerManager in");
    callback_ = std::make_shared<CastStreamPlayerCallback>(callback);
    if (!callback_) {
        CLOGE("callback_ is null");
        return;
    }

    player_ = std::make_shared<CastStreamPlayer>(callback_, fileChannel);
    if (!player_) {
        CLOGE("CastStreamPlayerManager init failed");
        return;
    }
    callback_->SetPlayer(player_);
    MockErrorCode_ = OHOS::system::GetParameter("debug.cast.stream.error", "");
    CLOGD("CastStreamPlayerManager out");
}

CastStreamPlayerManager::~CastStreamPlayerManager()
{
    CLOGD("~CastStreamPlayerManager in");
}

void CastStreamPlayerManager::SetSessionCallbackForRelease(const std::function<void(void)> &callback)
{
    std::lock_guard<std::mutex> lock(sessionCallbackMutex_);
    sessionCallback_ = callback;
}

int32_t CastStreamPlayerManager::RegisterListener(sptr<IStreamPlayerListenerImpl> listener)
{
    CLOGD("RegisterListener in");
    if (!player_) {
        CLOGE("player_ is null");
        return CAST_ENGINE_ERROR;
    }
    if (!player_->RegisterListener(listener)) {
        CLOGE("Register listener failed");
        return CAST_ENGINE_ERROR;
    }
    CLOGD("RegisterListener out");
    return CAST_ENGINE_SUCCESS;
}

int32_t CastStreamPlayerManager::UnregisterListener()
{
    CLOGD("UnregisterListener in");
    if (!player_) {
        CLOGE("player_ is null");
        return CAST_ENGINE_ERROR;
    }
    if (!player_->UnregisterListener()) {
        return CAST_ENGINE_ERROR;
    }
    return CAST_ENGINE_SUCCESS;
}

int32_t CastStreamPlayerManager::SetSurface(sptr<IBufferProducer> producer)
{
    CLOGD("SetSurface in");
    sptr<Surface> surface = Surface::CreateSurfaceAsProducer(producer);
    if (surface == nullptr) {
        CLOGE("surface is null.");
        return CAST_ENGINE_ERROR;
    }
    if (!callback_) {
        CLOGE("callback_ is null");
        return CAST_ENGINE_ERROR;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    surface_ = surface;
    if (!isReady_) {
        callback_->OnRenderReady(true);
        isReady_ = true;
    }
    if (isReceivePlayCommand_) {
        if (!InnerPlayLocked(false)) {
            callback_->OnPlayerError(ERR_CODE_PLAY_FAILED, PLAYER_ERROR);
            return CAST_ENGINE_ERROR;
        }
    } else if (isReceiveLoadCommand_) {
        if (!InnerPlayLocked(true)) {
            callback_->OnPlayerError(ERR_CODE_PLAY_FAILED, PLAYER_ERROR);
            return CAST_ENGINE_ERROR;
        }
    }
    CLOGD("SetSurface out");
    return CAST_ENGINE_SUCCESS;
}

int32_t CastStreamPlayerManager::Load(const MediaInfo &mediaInfo)
{
    CLOGI("Load in");
    if (!player_) {
        CLOGE("player_ is null");
        return CAST_ENGINE_ERROR;
    }
    if (!callback_) {
        CLOGE("callback_ is null");
        return CAST_ENGINE_ERROR;
    }
    MOCK_TEST_PLAYER_ERROR(static_cast<int>(StreamActionId::LOAD));
    std::lock_guard<std::mutex> lock(mutex_);
    if (callback_->IsNeededToReset()) {
        callback_->SetSwitching();
        StopLocked();
    }
    mediaInfo_ = mediaInfo;
    if (surface_ != nullptr) {
        if (mediaInfo_.mediaUrl == "http:") {
            CLOGD("Load out: mediaUrl is null");
            callback_->OnMediaItemChanged(mediaInfo_);
            return CAST_ENGINE_SUCCESS;
        }
        if (!InnerPlayLocked(true)) {
            callback_->OnPlayerError(ERR_CODE_PLAY_FAILED, PLAYER_ERROR);
            return CAST_ENGINE_ERROR;
        }
    }
    isReceivePlayCommand_ = false;
    isReceiveLoadCommand_ = true;
    CLOGD("Load out");
    return CAST_ENGINE_SUCCESS;
}

int32_t CastStreamPlayerManager::Play(int index)
{
    CLOGW("Don't support play index");
    return CAST_ENGINE_SUCCESS;
}

// Play request from sink.
int32_t CastStreamPlayerManager::Play(const MediaInfo &mediaInfo)
{
    CLOGD("Play in");
    if (!callback_) {
        CLOGE("callback_ is null");
        return CAST_ENGINE_ERROR;
    }
    callback_->OnPlayRequest(mediaInfo);
    return CAST_ENGINE_SUCCESS;
}

bool CastStreamPlayerManager::InnerPlayLocked(bool isLoading)
{
    CLOGD("InnerPlayLocked in");

    if (!player_) {
        CLOGE("player_ is null");
        return false;
    }
    if (!callback_) {
        CLOGE("callback_ is null");
        return false;
    }
    callback_->OnMediaItemChanged(mediaInfo_);
    if (!player_->SetSource(mediaInfo_)) {
        CLOGE("StreamPlayer SetSource failed");
        return false;
    }
    if (mediaInfo_.mediaType == "IMAGE") {
        return true;
    }
    if (!player_->SetVideoSurface(surface_)) {
        CLOGE("StreamPlayer SetVideoSurface failed");
        return false;
    }
    if (mediaInfo_.startPosition > 0) {
        auto ret = player_->SetPlayRangeWithMode(mediaInfo_.startPosition);
        CLOGI("ret %{public}d", ret);
    }
    if (!player_->Prepare()) {
        CLOGE("StreamPlayer Prepare failed");
        return false;
    }
    if (!isLoading) {
        if (!player_->Play()) {
            CLOGE("StreamPlayer Play failed");
            return false;
        }
    }

    CLOGD("InnerPlayLocked out");
    return true;
}

// Start media, when play media first time.
int32_t CastStreamPlayerManager::InnerPlay(const MediaInfo &mediaInfo)
{
    CLOGI("InnerPlay in");
    if (!player_) {
        CLOGE("player_ is null");
        return CAST_ENGINE_ERROR;
    }
    if (!callback_) {
        CLOGE("callback_ is null");
        return CAST_ENGINE_ERROR;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (callback_->IsNeededToReset()) {
        callback_->SetSwitching();
        StopLocked();
    }
    mediaInfo_ = mediaInfo;
    if (surface_ != nullptr) {
        if (!InnerPlayLocked(false)) {
            callback_->OnPlayerError(ERR_CODE_PLAY_FAILED, PLAYER_ERROR);
            return CAST_ENGINE_ERROR;
        }
    }
    isReceiveLoadCommand_ = false;
    isReceivePlayCommand_ = true;
    CLOGD("InnerPlay out");
    return CAST_ENGINE_SUCCESS;
}

PlaybackSpeed CastStreamPlayerManager::ConvertMediaSpeedToPlaybackSpeed(Media::PlaybackRateMode speedMode)
{
    auto iter = g_mediaSpeedToPlaybackSpeedMap.find(speedMode);
    if (iter == g_mediaSpeedToPlaybackSpeedMap.end()) {
        CLOGE("ConvertMediaSpeedToPlaybackSpeed, unknown event keyCode");
        return PlaybackSpeed::SPEED_FORWARD_1_00_X;
    }
    return iter->second;
}

int32_t CastStreamPlayerManager::Pause()
{
    CLOGD("Pause in");
    if (!player_) {
        CLOGE("player_ is null");
        return CAST_ENGINE_ERROR;
    }
    MOCK_TEST_PLAYER_ERROR(static_cast<int>(StreamActionId::PAUSE));
    if (!player_->Pause()) {
        CLOGE("StreamPlayer Pause failed");
        return CAST_ENGINE_ERROR;
    }
    return CAST_ENGINE_SUCCESS;
}

int32_t CastStreamPlayerManager::Play()
{
    CLOGD("Play in");
    if (!player_) {
        CLOGE("player_ is null");
        return CAST_ENGINE_ERROR;
    }
    MOCK_TEST_PLAYER_ERROR(static_cast<int>(StreamActionId::PLAY));
    if (!player_->Play()) {
        CLOGE("StreamPlayer Play failed");
        return CAST_ENGINE_ERROR;
    }
    return CAST_ENGINE_SUCCESS;
}

int32_t CastStreamPlayerManager::Stop()
{
    CLOGD("Stop in");
    if (!callback_) {
        CLOGE("callback_ is null");
        return CAST_ENGINE_ERROR;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (!StopLocked()) {
        CLOGE("Stop locked failed");
        return CAST_ENGINE_ERROR;
    }
    CLOGD("Stop out");
    return CAST_ENGINE_SUCCESS;
}

bool CastStreamPlayerManager::StopLocked()
{
    CLOGD("StopLocked in");
    if (!player_) {
        CLOGE("player_ is null");
        return false;
    }
    if (!callback_) {
        CLOGE("callback_ is null");
        return false;
    }
    isReceiveLoadCommand_ = false;
    isReceivePlayCommand_ = false;
    if (!player_->Stop()) {
        CLOGE("StreamPlayer Stop failed");
    }
    if (!player_->Reset()) {
        CLOGE("StreamPlayer Reset failed");
        return false;
    }
    CLOGD("StopLocked out");
    return true;
}

int32_t CastStreamPlayerManager::Next()
{
    CLOGD("Next in");
    if (!callback_) {
        CLOGE("callback_ is null");
        return CAST_ENGINE_ERROR;
    }
    MOCK_TEST_PLAYER_ERROR(static_cast<int>(StreamActionId::NEXT));
    callback_->OnNextRequest();
    CLOGD("Next out");
    return CAST_ENGINE_SUCCESS;
}

int32_t CastStreamPlayerManager::Previous()
{
    CLOGD("Previous in");
    if (!callback_) {
        CLOGE("callback_ is null");
        return CAST_ENGINE_ERROR;
    }
    MOCK_TEST_PLAYER_ERROR(static_cast<int>(StreamActionId::PREVIOUS));
    callback_->OnPreviousRequest();
    CLOGD("Previous out");
    return CAST_ENGINE_SUCCESS;
}

int32_t CastStreamPlayerManager::Seek(int position)
{
    CLOGD("Seek in");
    if (!player_) {
        CLOGE("player_ is null");
        return CAST_ENGINE_ERROR;
    }
    MOCK_TEST_PLAYER_ERROR(static_cast<int>(StreamActionId::SEEK));
    if (!player_->Seek(position, Media::SEEK_PREVIOUS_SYNC)) {
        CLOGE("StreamPlayer Seek failed");
        return CAST_ENGINE_ERROR;
    }
    CLOGD("Seek out");
    return CAST_ENGINE_SUCCESS;
}

int32_t CastStreamPlayerManager::FastForward(int delta)
{
    CLOGD("FastForWard in");
    if (!player_) {
        CLOGE("player_ is null");
        return CAST_ENGINE_ERROR;
    }
    MOCK_TEST_PLAYER_ERROR(static_cast<int>(StreamActionId::FASTFORWARD));
    int curPosition;
    int ret = GetPosition(curPosition);
    if (ret != CAST_ENGINE_SUCCESS) {
        CLOGE("GetPosition failed");
        return ret;
    }
    if (!player_->Seek(curPosition + delta, Media::SEEK_PREVIOUS_SYNC)) {
        CLOGE("StreamPlayer Seek failed");
        return CAST_ENGINE_ERROR;
    }
    CLOGD("FastForWard out");
    return CAST_ENGINE_SUCCESS;
}

int32_t CastStreamPlayerManager::FastRewind(int delta)
{
    CLOGD("FastForRewind in");
    if (!player_) {
        CLOGE("player_ is null");
        return CAST_ENGINE_ERROR;
    }
    MOCK_TEST_PLAYER_ERROR(static_cast<int>(StreamActionId::FASTREWIND));
    int curPosition;
    int ret = GetPosition(curPosition);
    if (ret != CAST_ENGINE_SUCCESS) {
        CLOGE("GetPosition failed");
        return ret;
    }
    if (!player_->Seek(curPosition - delta, Media::SEEK_PREVIOUS_SYNC)) {
        CLOGE("StreamPlayer Seek failed");
        return CAST_ENGINE_ERROR;
    }
    CLOGD("FastForRewind out");
    return CAST_ENGINE_SUCCESS;
}

int32_t CastStreamPlayerManager::SetVolume(int volume)
{
    CLOGD("SetVolume in");
    if (!player_) {
        CLOGE("player_ is null");
        return CAST_ENGINE_ERROR;
    }
    MOCK_TEST_PLAYER_ERROR(static_cast<int>(StreamActionId::SET_VOLUME));
    if (!player_->SetVolume(volume)) {
        CLOGE("StreamPlayer SetVolume failed");
        return CAST_ENGINE_ERROR;
    }
    CLOGD("SetVolume out");
    return CAST_ENGINE_SUCCESS;
}

int32_t CastStreamPlayerManager::SetMute(bool mute)
{
    CLOGD("SetMute in");
    if (!player_) {
        CLOGE("player_ is null");
        return CAST_ENGINE_ERROR;
    }
    MOCK_TEST_PLAYER_ERROR(static_cast<int>(StreamActionId::SET_MUTE));
    if (!player_->SetMute(mute)) {
        CLOGE("StreamPlayer SetMute failed");
        return CAST_ENGINE_ERROR;
    }
    CLOGD("SetMute out");
    return CAST_ENGINE_SUCCESS;
}

int32_t CastStreamPlayerManager::SetLoopMode(const LoopMode mode)
{
    CLOGD("SetLoopMode in");
    if (!player_) {
        CLOGE("player_ is null");
        return CAST_ENGINE_ERROR;
    }
    MOCK_TEST_PLAYER_ERROR(static_cast<int>(StreamActionId::SET_LOOP_MODE));
    if (!player_->SetLoopMode(mode)) {
        CLOGE("StreamPlayer SetLoopMode failed");
        return CAST_ENGINE_ERROR;
    }
    CLOGD("SetLoopMode out");
    return CAST_ENGINE_SUCCESS;
}

int32_t CastStreamPlayerManager::InnerSetAvailableCapability(const StreamCapability &streamCapability)
{
    CLOGD("InnerSetAvailableCapability in");
    if (!callback_) {
        CLOGE("callback_ is null");
        return CAST_ENGINE_ERROR;
    }
    callback_->OnAvailableCapabilityChanged(streamCapability);
    std::lock_guard<std::mutex> lock(mutex_);
    availableCapability_ = streamCapability;
    CLOGD("InnerSetAvailableCapability out");
    return CAST_ENGINE_SUCCESS;
}

int32_t CastStreamPlayerManager::SetAvailableCapability(const StreamCapability &streamCapability)
{
    CLOGW("Don't support SetAvailableCapability");
    return CAST_ENGINE_ERROR;
}

int32_t CastStreamPlayerManager::SetSpeed(const PlaybackSpeed speed)
{
    CLOGD("SetSpeed in");
    auto iter = g_doubleToModeTypeMap.find(speed);
    if (iter == g_doubleToModeTypeMap.end()) {
        CLOGE("ConvertDoubleToSpeedMode, unknown event keyCode");
        return CAST_ENGINE_ERROR;
    }
    auto mode = static_cast<Media::PlaybackRateMode>(iter->second);
    if (!player_) {
        CLOGE("player_ is null");
        return CAST_ENGINE_ERROR;
    }
    if (!player_->SetPlaybackSpeed(mode)) {
        CLOGE("StreamPlayer SetSpeed failed");
        return CAST_ENGINE_ERROR;
    }
    CLOGD("SetSpeed out");
    return CAST_ENGINE_SUCCESS;
}

int32_t CastStreamPlayerManager::GetPlayerStatus(PlayerStates &playerStates)
{
    CLOGD("GetPlayerStatus in");
    playerStates = PlayerStates::PLAYER_STATE_ERROR;
    if (!player_) {
        CLOGE("player_ is null");
        return CAST_ENGINE_ERROR;
    }
    CLOGD("GetPlayerStatus out");
    playerStates = callback_->GetPlayerStatus();
    return CAST_ENGINE_SUCCESS;
}

int32_t CastStreamPlayerManager::GetPosition(int &position)
{
    CLOGD("GetPosition in");
    position = CAST_STREAM_INT_INVALID;
    if (!player_) {
        CLOGE("player_ is null");
        return CAST_ENGINE_ERROR;
    }
    if (!player_->GetCurrentTime(position)) {
        CLOGE("StreamPlayer GetPosition failed");
        return CAST_ENGINE_ERROR;
    }
    CLOGD("GetPosition out");
    return CAST_ENGINE_SUCCESS;
}

int32_t CastStreamPlayerManager::GetDuration(int &duration)
{
    CLOGD("GetDuration in");
    duration = CAST_STREAM_INT_INVALID;
    if (!player_) {
        CLOGE("player_ is null");
        return CAST_ENGINE_ERROR;
    }
    duration = player_->GetDuration();
    CLOGD("GetDuration out");
    return CAST_ENGINE_SUCCESS;
}

int32_t CastStreamPlayerManager::GetVolume(int &volume, int &maxVolume)
{
    CLOGD("GetVolume in");
    volume = CAST_STREAM_INT_INVALID;
    maxVolume = CAST_STREAM_INT_INVALID;
    if (!player_) {
        CLOGE("player_ is null");
        return CAST_ENGINE_ERROR;
    }
    if (!player_->GetVolume(volume, maxVolume)) {
        return CAST_ENGINE_ERROR;
    }
    return CAST_ENGINE_SUCCESS;
}

int32_t CastStreamPlayerManager::GetMute(bool &mute)
{
    CLOGD("GetMute in");
    if (!player_) {
        CLOGE("player_ is null");
        return CAST_ENGINE_ERROR;
    }
    mute = player_->GetMute();
    return CAST_ENGINE_SUCCESS;
}

int32_t CastStreamPlayerManager::GetLoopMode(LoopMode &loopMode)
{
    CLOGD("GetLoopMode in");
    loopMode = LoopMode::LOOP_MODE_LIST;
    if (!player_) {
        CLOGE("player_ is null");
        return CAST_ENGINE_ERROR;
    }
    loopMode = player_->GetLoopMode();
    return CAST_ENGINE_SUCCESS;
}

int32_t CastStreamPlayerManager::GetAvailableCapability(StreamCapability &streamCapability)
{
    CLOGD("GetAvailableCapability in");
    if (!player_) {
        CLOGE("player_ is null");
        return CAST_ENGINE_ERROR;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    streamCapability = availableCapability_;
    return CAST_ENGINE_SUCCESS;
}

int32_t CastStreamPlayerManager::GetPlaySpeed(PlaybackSpeed &playbackSpeed)
{
    CLOGD("GetPlaySpeed in");
    playbackSpeed = PlaybackSpeed::SPEED_FORWARD_1_00_X;
    if (!player_) {
        CLOGE("player_ is null");
        return CAST_ENGINE_ERROR;
    }
    Media::PlaybackRateMode ret = Media::SPEED_FORWARD_1_00_X;
    if (!player_->GetPlaybackSpeed(ret)) {
        CLOGE("StreamPlayer GetPlaybackSpeed failed");
        return CAST_ENGINE_ERROR;
    }
    playbackSpeed = ConvertMediaSpeedToPlaybackSpeed(ret);
    CLOGD("GetPlaySpeed out");
    return CAST_ENGINE_SUCCESS;
}

int32_t CastStreamPlayerManager::GetMediaInfoHolder(MediaInfoHolder &mediaInfoHolder)
{
    CLOGW("Don't support GetMediaInfoHolder");
    return CAST_ENGINE_ERROR;
}

int32_t CastStreamPlayerManager::ProvideKeyResponse(const std::string &mediaId, const std::vector<uint8_t> &response)
{
    CLOGW("Don't support GetMediaInfoHolder");
    return CAST_ENGINE_ERROR;
}

int32_t CastStreamPlayerManager::UpdateMediaInfo(const MediaInfo &mediaInfo)
{
    CLOGE("Don't support UpdateMediaInfo");
    return CAST_ENGINE_ERROR;
}

int32_t CastStreamPlayerManager::Release()
{
    Stop();
    std::lock_guard<std::mutex> lock(sessionCallbackMutex_);
    if (!sessionCallback_) {
        CLOGE("sessionCallback is null");
        return CAST_ENGINE_ERROR;
    }
    sessionCallback_();
    return CAST_ENGINE_SUCCESS;
}

// Used for mock tests
bool CastStreamPlayerManager::MockPlayerError(int32_t action)
{
    CLOGI("MockPlayerError MockErrorCode_%{public}s, action %{public}d", MockErrorCode_.c_str(), action);
    if (MockErrorCode_.empty() || !callback_) {
        return false;
    }
    std::string param = OHOS::system::GetParameter("debug.cast.stream.error", "");
    std::string errormsg = std::to_string(action);
    json jsonObj = json::parse(param, nullptr, false);
    if (!jsonObj.is_discarded()) {
        if (jsonObj.contains(errormsg) && jsonObj[errormsg].is_string()) {
            int32_t errorCode = std::atoi(jsonObj[errormsg].get<std::string>().c_str());
            callback_->OnPlayerError(errorCode, errormsg);
            return true;
        }
    }
    return false;
}

} // namespace CastEngineService
} // namespace CastEngine
} // namespace OHOS