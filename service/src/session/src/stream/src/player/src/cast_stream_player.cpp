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
 * Description: supply cast stream player.
 * Author: zhangjingnan
 * Create: 2023-1-11
 */

#include <chrono>
#include <unistd.h>
#include "image_source.h"
#include "cast_engine_dfx.h"
#include "cast_engine_log.h"
#include "cast_stream_player.h"
#include "cast_stream_player_utils.h"

using OHOS::Media::MSERR_OK;

namespace OHOS {
namespace CastEngine {
namespace CastEngineService {
DEFINE_CAST_ENGINE_LABEL("Cast-Stream-Player");

namespace {

void OnInfoWriteWrap(const std::string& funcName, int32_t extra)
{
    if (static_cast<PlayerStates>(extra) == PlayerStates::PLAYER_STARTED) {
        HiSysEventWriteWrap(funcName, {
                {"BIZ_SCENE", static_cast<int32_t>(BIZSceneType::ONLINE_CAST_STREAM)},
                {"BIZ_STATE", static_cast<int32_t>(BIZStateType::BIZ_STATE_END)},
                {"BIZ_STAGE", static_cast<int32_t>(BIZSceneStage::ONLINE_STREAM_PLAY_RESOURCE_SUCCESS)},
                {"STAGE_RES", static_cast<int32_t>(StageResType::STAGE_RES_SUCCESS)},
                {"ERROR_CODE", CAST_RADAR_SUCCESS}}, {
                {"TO_CALL_PKG", DSOFTBUS_NAME},
                {"LOCAL_SESS_NAME", ""},
                {"PEER_SESS_NAME", ""},
                {"PEER_UDID", ""}});
    } else {
        HiSysEventWriteWrap(funcName, {
                {"BIZ_SCENE", static_cast<int32_t>(BIZSceneType::DUAL_POINTS_CONTROL)},
                {"BIZ_STATE", static_cast<int32_t>(BIZStateType::BIZ_STATE_END)},
                {"BIZ_STAGE", static_cast<int32_t>(BIZSceneStage::SINK_RESPONSE)},
                {"STAGE_RES", static_cast<int32_t>(StageResType::STAGE_RES_SUCCESS)},
                {"ERROR_CODE", CAST_RADAR_SUCCESS}}, {
                {"TO_CALL_PKG", DSOFTBUS_NAME},
                {"LOCAL_SESS_NAME", ""},
                {"PEER_SESS_NAME", ""},
                {"PEER_UDID", ""}});
    }
}
}

CastStreamPlayerCallback::CastStreamPlayerCallback(std::shared_ptr<ICastStreamManagerServer> callback)
{
    CLOGD("CastStreamPlayerCallback in");
    callback_ = callback;
    timer_ = std::make_shared<CastTimer>();
}

CastStreamPlayerCallback::~CastStreamPlayerCallback()
{
    CLOGD("~CastStreamPlayerCallback in");
    timer_ = nullptr;
}

bool CastStreamPlayerCallback::RegisterListener(sptr<IStreamPlayerListenerImpl> listener)
{
    CLOGD("RegisterListener in");
    if (!listener) {
        CLOGE("listener is null");
        return false;
    }
    std::lock_guard<std::mutex> lock(listenerMutex_);
    listener_ = listener;
    CLOGD("RegisterListener out");
    return true;
}

bool CastStreamPlayerCallback::UnregisterListener()
{
    CLOGD("UnregisterListener in");
    std::lock_guard<std::mutex> lock(listenerMutex_);
    listener_ = nullptr;
    return true;
}

void CastStreamPlayerCallback::SetPlayer(std::shared_ptr<CastStreamPlayer> player)
{
    CLOGD("SetPlayer in");
    player_ = player;
}

void CastStreamPlayerCallback::SetState(PlayerStates state)
{
    std::lock_guard<std::mutex> lock(mutex_);
    state_ = state;
}

void CastStreamPlayerCallback::SetSpeedMode(Media::PlaybackRateMode speedMode)
{
    CLOGD("SetSpeed in");
    std::lock_guard<std::mutex> lock(mutex_);
    speedMode_ = speedMode;
}

void CastStreamPlayerCallback::SetSwitching()
{
    CLOGD("SetSwitching in");
    std::lock_guard<std::mutex> lock(mutex_);
    switchingCount_++;
}

PlayerStates CastStreamPlayerCallback::GetPlayerStatus()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return state_;
}

Media::PlaybackRateMode CastStreamPlayerCallback::GetSpeedMode()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return speedMode_;
}

bool CastStreamPlayerCallback::GetSwitching()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return switchingCount_;
}

bool CastStreamPlayerCallback::IsPaused()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return state_ == PlayerStates::PLAYER_PAUSED;
}

bool CastStreamPlayerCallback::IsNeededToReset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return state_ == PlayerStates::PLAYER_INITIALIZED ||
        state_ == PlayerStates::PLAYER_PREPARING ||
        state_ == PlayerStates::PLAYER_PREPARED ||
        state_ == PlayerStates::PLAYER_STARTED ||
        state_ == PlayerStates::PLAYER_PAUSED ||
        state_ == PlayerStates::PLAYER_STATE_ERROR ||
        state_ == PlayerStates::PLAYER_PLAYBACK_COMPLETE ||
        state_ == PlayerStates::PLAYER_STOPPED ||
        state_ == PlayerStates::PLAYER_RELEASED;
}

void CastStreamPlayerCallback::SetPosInfo(int startPos, int endPos)
{
    CLOGD("SetInitPosInfo in, startPos: %{public}d, endPos: %{public}d", startPos, endPos);
    std::lock_guard<std::mutex> lock(posMutex_);
    isRequestingResource_ = false;
    if (endPos > startPos) {
        endPosition_ = endPos;
    } else {
        endPosition_ = INT_MAX;
    }
}

void CastStreamPlayerCallback::CheckPosInfo(int curPos)
{
    std::lock_guard<std::mutex> lock(posMutex_);
    if (!isRequestingResource_ && curPos >= endPosition_) {
        CLOGD("Reach endPos, curPos: %{public}d", curPos);
        isRequestingResource_ = true;
        OnNextRequest();
    }
}

void CastStreamPlayerCallback::HandleInterruptEvent(const Media::Format &infoBody)
{
    int32_t hintTypeInt32 = 0;
    infoBody.GetIntValue(std::string(Media::PlayerKeys::AUDIO_INTERRUPT_HINT), hintTypeInt32);
    auto hintType = AudioStandard::InterruptHint(hintTypeInt32);
    CLOGI("HandleInterruptEvent, InterruptHintType = %{public}d", static_cast<int32_t>(hintType));
    auto player = player_.lock();
    if (!player) {
        CLOGE("Media player is null");
        return;
    }
    if (hintType == AudioStandard::InterruptHint::INTERRUPT_HINT_PAUSE) {
        if (player->IsPlaying()) {
            CLOGI("onAudioInterrupt pause music");
            player->Pause();
        }
    } else if (hintType == AudioStandard::InterruptHint::INTERRUPT_HINT_RESUME) {
        if (IsPaused()) {
            CLOGI("onAudioInterrupt resume music");
            player->Play();
        }
    } else if (hintType == AudioStandard::InterruptHint::INTERRUPT_HINT_STOP) {
        CLOGI("onAudioInterrupt stop music");
        if (player->IsPlaying()) {
            CLOGI("onAudioInterrupt pause music");
            player->Pause();
        }
    }
}

sptr<IStreamPlayerListenerImpl> CastStreamPlayerCallback::ListenerGetter()
{
    std::lock_guard<std::mutex> lock(listenerMutex_);
    return listener_;
}

void CastStreamPlayerCallback::OnInfo(Media::PlayerOnInfoType type, int32_t extra, const Media::Format &infoBody)
{
    CLOGI("OnInfo in, playerOnInfoType = %{public}d, extra = %{public}d", static_cast<int32_t>(type), extra);
    switch (type) {
        case Media::INFO_TYPE_SEEKDONE: {
            OnSeekDone(extra);
            break;
        }
        case Media::INFO_TYPE_SPEEDDONE: {
            OnPlaySpeedChanged();
            break;
        }
        case Media::INFO_TYPE_STATE_CHANGE: {
            OnInfoWriteWrap(__func__, extra);
            SetState(static_cast<PlayerStates>(extra));
            OnStateChanged(static_cast<PlayerStates>(extra), true);
            break;
        }
        case Media::INFO_TYPE_POSITION_UPDATE: {
            OnPositionChanged(extra, CAST_STREAM_INT_IGNORE, CAST_STREAM_INT_IGNORE);
            break;
        }
        case Media::INFO_TYPE_BUFFERING_UPDATE: {
            OnBufferChanged(infoBody);
            break;
        }
        case Media::INFO_TYPE_DURATION_UPDATE: {
            OnPositionChanged(CAST_STREAM_INT_IGNORE, CAST_STREAM_INT_IGNORE, extra);
            break;
        }
        case Media::INFO_TYPE_RESOLUTION_CHANGE: {
            OnVideoSizeChanged(infoBody);
            break;
        }
        case Media::INFO_TYPE_EOS: {
            OnEndOfStream(extra);
            break;
        }
        case Media::INFO_TYPE_INTERRUPT_EVENT: {
            HandleInterruptEvent(infoBody);
            break;
        }
        default: {
            break;
        }
    }
    CLOGD("OnInfo out");
}

void CastStreamPlayerCallback::OnError(int32_t errorCode, const std::string &errorMsg)
{
    CLOGE("Player OnError message %{public}s", errorMsg.c_str());
    OnPlayerError(errorCode, errorMsg);
}

void CastStreamPlayerCallback::OnStateChanged(const PlayerStates playbackState, bool isPlayWhenReady)
{
    CLOGD("OnStateChanged in");
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (switchingCount_ > 0 && state_ == PlayerStates::PLAYER_IDLE) {
            CLOGD("Is Switching");
            switchingCount_--;
        }
    }
    if (playbackState == PlayerStates::PLAYER_PLAYBACK_COMPLETE) {
        auto player = player_.lock();
        if (player) {
            player->NotifyPlayComplete();
        }
    }
    if (playbackState != PlayerStates::PLAYER_STARTED) {
        if (timer_ && !timer_->IsStopped()) {
            timer_ ->Stop();
        }
    }
    auto listener = ListenerGetter();
    if (!listener) {
        CLOGE("StreamPlayerListener is null");
        return;
    }
    listener->OnStateChanged(playbackState, isPlayWhenReady);

    auto targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerServer is null");
        return;
    }
    targetCallback->NotifyPeerPlayerStatusChanged(playbackState, isPlayWhenReady);

    CLOGD("OnStateChanged out");
}

void CastStreamPlayerCallback::OnPositionChanged(int position, int bufferPosition, int duration)
{
    CLOGD("OnPositionChanged in");
    auto player = player_.lock();
    if (!player) {
        CLOGE("player_ is null");
        return;
    }
    // Due to the bug of Media::PlayerCallback(don't report duration), need to do active acquirement now.
    duration = player->GetDuration();
    if (duration < 0) {
        CLOGE("GetDuration failed");
        return;
    }
    CheckPosInfo(position);
    auto listener = ListenerGetter();
    if (!listener) {
        CLOGE("StreamPlayerListener is null");
        return;
    }
    listener->OnPositionChanged(position, bufferPosition, duration);

    auto targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerServer is null");
        return;
    }
    if (position != CAST_STREAM_INT_IGNORE && GetPlayerStatus() == PlayerStates::PLAYER_STARTED) {
        if (timer_ && timer_->IsStopped()) {
            targetCallback->NotifyPeerPositionChanged(position, CAST_STREAM_INT_IGNORE, duration);
            timer_->Start([this]() { AutoSyncPosition();}, AUTO_POSITION_SYNC_INTERVAL);
            std::lock_guard<std::mutex> lock(posMutex_);
            curPosition_ = position;
            return;
        }
        if (!IsNeedToSyncPos(position)) {
            return;
        }
    }
    targetCallback->NotifyPeerPositionChanged(position, bufferPosition, duration);
    CLOGD("OnPositionChanged out");
}

void CastStreamPlayerCallback::OnBufferChanged(const Media::Format &infoBody)
{
    CLOGD("OnBufferChanged in");
    if (infoBody.ContainKey(std::string(Media::PlayerKeys::PLAYER_BUFFERING_START))) {
        CLOGI("PLAYER_BUFFERING_START");
        OnStateChanged(PlayerStates::PLAYER_BUFFERING, true);
    } else if (infoBody.ContainKey(std::string(Media::PlayerKeys::PLAYER_BUFFERING_END))) {
        CLOGI("PLAYER_BUFFERING_END");
        OnStateChanged(GetPlayerStatus(), true);
    } else if (infoBody.ContainKey(std::string(Media::PlayerKeys::PLAYER_CACHED_DURATION))) {
        int bufferCachedDuration = CAST_STREAM_INT_INVALID;
        infoBody.GetIntValue(std::string(Media::PlayerKeys::PLAYER_CACHED_DURATION), bufferCachedDuration);
        CLOGI("PLAYER_CACHED_DURATION: %{public}d", bufferCachedDuration);
        if (bufferCachedDuration != CAST_STREAM_INT_INVALID) {
            OnPositionChanged(CAST_STREAM_INT_IGNORE, bufferCachedDuration, CAST_STREAM_INT_IGNORE);
        }
    }
    CLOGD("OnBufferChanged out");
}

void CastStreamPlayerCallback::OnVideoSizeChanged(const Media::Format &infoBody)
{
    CLOGD("OnVideoSizeChanged in");
    int32_t width;
    int32_t height;
    if (!infoBody.GetIntValue(std::string(Media::PlayerKeys::PLAYER_WIDTH), width) ||
        !infoBody.GetIntValue(std::string(Media::PlayerKeys::PLAYER_HEIGHT), height)) {
        CLOGE("get width or height failed");
        return;
    }

    auto listener = ListenerGetter();
    if (!listener) {
        CLOGE("StreamPlayerListener is null");
        return;
    }
    listener->OnVideoSizeChanged(width, height);
    CLOGD("OnVideoSizeChanged: width = %{public}d, height = %{public}d", width, height);
}

void CastStreamPlayerCallback::OnMediaItemChanged(const MediaInfo &mediaInfo)
{
    CLOGD("OnMediaItemChanged in");
    auto listener = ListenerGetter();
    if (!listener) {
        CLOGE("StreamPlayerListener is null");
        return;
    }
    listener->OnMediaItemChanged(mediaInfo);

    auto targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerServer is null");
        return;
    }
    targetCallback->NotifyPeerMediaItemChanged(mediaInfo);
    CLOGD("OnMediaItemChanged out");
}

void CastStreamPlayerCallback::OnVolumeChanged(int volume, int maxVolume)
{
    CLOGD("OnVolumeChanged in");
    auto listener = ListenerGetter();
    if (!listener) {
        CLOGE("StreamPlayerListener is null");
        return;
    }
    listener->OnVolumeChanged(volume, maxVolume);

    auto targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerServer is null");
        return;
    }
    targetCallback->NotifyPeerVolumeChanged(volume, maxVolume);
    CLOGD("OnVolumeChanged out");
}

void CastStreamPlayerCallback::OnEvent(EventId eventId, const std::string &data)
{
    CLOGD("OnEvent in");
    auto targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerServer is null");
        return;
    }
    targetCallback->OnEvent(eventId, data);
    CLOGD("OnEvent out");
}

void CastStreamPlayerCallback::OnRenderReady(bool isReady)
{
    CLOGD("OnRenderReady in");
    auto targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerServer is null");
        return;
    }
    targetCallback->OnRenderReady(isReady);
    CLOGD("OnRenderReady out");
}

void CastStreamPlayerCallback::OnImageChanged(std::shared_ptr<Media::PixelMap> pixelMap)
{
    CLOGD("OnImageChanged in");
    auto listener = ListenerGetter();
    if (!listener) {
        CLOGE("StreamPlayerListener is null");
        return;
    }
    listener->OnImageChanged(pixelMap);
    CLOGD("OnImageChanged out");
}

void CastStreamPlayerCallback::OnPlayerError(int errorCode, const std::string &errorMsg)
{
    CLOGD("OnPlayerError in");
    auto listener = ListenerGetter();
    if (!listener) {
        CLOGE("StreamPlayerListener is null");
        return;
    }
    listener->OnPlayerError(errorCode, errorMsg);

    std::shared_ptr<ICastStreamManagerServer> targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerServer is null");
        return;
    }
    targetCallback->NotifyPeerPlayerError(errorCode, errorMsg);
    CLOGD("OnPlayerError out");
}

void CastStreamPlayerCallback::OnLoopModeChanged(const LoopMode loopMode)
{
    CLOGD("OnLoopModeChanged in");
    auto listener = ListenerGetter();
    if (!listener) {
        CLOGE("StreamPlayerListener is null");
        return;
    }
    listener->OnLoopModeChanged(loopMode);

    auto targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerServer is null");
        return;
    }
    targetCallback->NotifyPeerRepeatModeChanged(loopMode);
    CLOGD("OnLoopModeChanged out");
}

void CastStreamPlayerCallback::OnAvailableCapabilityChanged(const StreamCapability &streamCapability)
{
    CLOGI("OnAvailableCapabilityChanged in");
    auto listener = ListenerGetter();
    if (!listener) {
        CLOGE("StreamPlayerListener is null");
        return;
    }
    listener->OnAvailableCapabilityChanged(streamCapability);
    CLOGI("OnAvailableCapabilityChanged out");
}

void CastStreamPlayerCallback::OnPlayRequest(const MediaInfo &mediaInfo)
{
    CLOGD("OnPlayRequest in");
    auto targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerCallback is null");
        return;
    }
    targetCallback->NotifyPeerPlayRequest(mediaInfo);
    CLOGD("OnPlayRequest out");
}

void CastStreamPlayerCallback::OnAlbumCoverChanged(std::shared_ptr<Media::PixelMap> pixelMap)
{
    CLOGD("OnAlbumCoverChanged in");
    if (!pixelMap) {
        CLOGE("pixelMap is null");
        return;
    }
    auto listener = ListenerGetter();
    if (!listener) {
        CLOGE("StreamPlayerListener is null");
        return;
    }
    listener->OnAlbumCoverChanged(pixelMap);
    CLOGD("OnAlbumCoverChanged out");
}

void CastStreamPlayerCallback::OnPlaySpeedChanged()
{
    CLOGD("OnPlaySpeedChanged in");
    PlaybackSpeed curSpeed = ConvertMediaSpeedToPlaybackSpeed(speedMode_);
    auto listener = ListenerGetter();
    if (!listener) {
        CLOGE("StreamPlayerListener is null");
        return;
    }
    listener->OnPlaySpeedChanged(curSpeed);

    auto targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerServer is null");
        return;
    }
    targetCallback->NotifyPeerPlaySpeedChanged(curSpeed);
    CLOGD("OnPlaySpeedChanged out");
}

void CastStreamPlayerCallback::OnNextRequest()
{
    CLOGD("OnNextRequest in");
    auto targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerServer is null");
        return;
    }
    targetCallback->NotifyPeerNextRequest();
    CLOGD("OnNextRequest out");
}

void CastStreamPlayerCallback::OnPreviousRequest()
{
    CLOGD("OnPreviousRequest in");
    auto targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerServer is null");
        return;
    }
    targetCallback->NotifyPeerPreviousRequest();
    CLOGD("OnPreviousRequest out");
}

void CastStreamPlayerCallback::OnSeekDone(int position)
{
    CLOGD("OnSeekDone in");
    auto listener = ListenerGetter();
    if (!listener) {
        CLOGE("StreamPlayerListener is null");
        return;
    }
    listener->OnSeekDone(position);

    auto targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerServer is null");
        return;
    }
    targetCallback->NotifyPeerSeekDone(position);
    CLOGD("OnSeekDone out");
}

void CastStreamPlayerCallback::OnEndOfStream(int isLooping)
{
    CLOGD("OnEndOfStream in");
    auto listener = ListenerGetter();
    if (!listener) {
        CLOGE("StreamPlayerListener is null");
        return;
    }
    listener->OnEndOfStream(isLooping);

    auto targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerServer is null");
        return;
    }
    targetCallback->NotifyPeerEndOfStream(isLooping);
    CLOGD("OnEndOfStream out");
}

PlaybackSpeed CastStreamPlayerCallback::ConvertMediaSpeedToPlaybackSpeed(Media::PlaybackRateMode speedMode)
{
    auto iter = g_mediaSpeedToPlaybackSpeedMap.find(speedMode);
    if (iter == g_mediaSpeedToPlaybackSpeedMap.end()) {
        CLOGE("ConvertMediaSpeedToPlaybackSpeed, unknown speedMode keyCode");
        return PlaybackSpeed::SPEED_FORWARD_1_00_X;
    }
    return iter->second;
}

bool CastStreamPlayerCallback::IsNeedToSyncPos(int position)
{
    CLOGD("IsNeedToSyncPos in");
    std::lock_guard<std::mutex> lock(posMutex_);
    int timeLag = position - curPosition_;
    curPosition_ = position;
    if (timeLag >= POSITION_LAG_MINIMUM && timeLag <= POSITION_LAG_MAXIMUM) {
        return false;
    } else if (timeLag == 0) {
        return false;
    }
    return true;
}

bool CastStreamPlayerCallback::AutoSyncPosition()
{
    CLOGD("AutoSyncPosition in");
    int position;
    {
        std::lock_guard<std::mutex> lock(posMutex_);
        position = curPosition_;
    }
    auto targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerServer is null");
        return false;
    }
    targetCallback->NotifyPeerPositionChanged(position, CAST_STREAM_INT_IGNORE, CAST_STREAM_INT_IGNORE);
    return true;
}

CastStreamPlayer::CastStreamPlayer(std::shared_ptr<CastStreamPlayerCallback> callback,
    std::shared_ptr<CastLocalFileChannelClient> fileChannel)
{
    CLOGD("CastStreamPlayer in");
    player_ = Media::PlayerFactory::CreatePlayer();
    if (player_ == nullptr) {
        CLOGE("CreatePlayer failed");
        CastEngineDfx::WriteErrorEvent(PLAYER_INIT_FAIL);
        return;
    }
    audioSystemMgr_ = AudioStandard::AudioSystemManager::GetInstance();
    if (audioSystemMgr_ == nullptr) {
        CLOGE("Audio manager is null");
        Release();
        return;
    }
    // In current audioStandard structure, all type will be converted into STREAM_MUSIC.
    auto streamType = AudioStandard::AudioVolumeType::STREAM_MUSIC;
    castStreamVolumeCallback_ = std::make_shared<CastStreamVolumeCallback>(callback);
    if (castStreamVolumeCallback_ == nullptr) {
        CLOGE("Volume callback is null");
        Release();
        return;
    }
    castStreamVolumeCallback_->SetMaxVolume(audioSystemMgr_->GetMaxVolume(streamType));
    int32_t audioSystemRet = audioSystemMgr_->RegisterVolumeKeyEventCallback(getpid(), castStreamVolumeCallback_);
    if (audioSystemRet != MSERR_OK) {
        CLOGE("Register volume callback failed");
        Release();
        return;
    }
    SendInitSysVolume();
    callback_ = callback;
    if (callback_ == nullptr) {
        CLOGE("CreatePlayerCallback failed");
        Release();
        return;
    }
    fileChannelClient_ = fileChannel;
    int32_t playerRet = player_->SetPlayerCallback(std::static_pointer_cast<Media::PlayerCallback>(callback_));
    if (playerRet != MSERR_OK) {
        CLOGE("Media player SetPlayerCallback failed");
        Release();
        return;
    }
    avMetadataHelper_ = Media::AVMetadataHelperFactory::CreateAVMetadataHelper();
    if (!avMetadataHelper_) {
        CLOGE("CreateAVMetadataHelper failed");
    }
    CLOGD("CastStreamPlayer out");
}

CastStreamPlayer::~CastStreamPlayer()
{
    CLOGD("~CastStreamPlayer in");
    Release();
}

bool CastStreamPlayer::RegisterListener(sptr<IStreamPlayerListenerImpl> listener)
{
    CLOGD("RegisterListener in");
    if (!callback_) {
        CLOGE("callback_ is null");
        return false;
    }
    bool ret = callback_->RegisterListener(listener);
    if (!ret) {
        CLOGE("RegisterListener failed");
        return false;
    }
    CLOGD("RegisterListener out");
    return true;
}

bool CastStreamPlayer::UnregisterListener()
{
    CLOGD("UnregisterListener in");
    if (!callback_) {
        CLOGE("callback_ is null");
        return false;
    }
    bool ret = callback_->UnregisterListener();
    if (!ret) {
        CLOGE("UnregisterListener failed");
        return false;
    }
    CLOGD("UnregisterListener out");
    return true;
}

bool CastStreamPlayer::SetSource(const MediaInfo &mediaInfo)
{
    CLOGD("SetSource in");
    if (!player_) {
        CLOGE("Media player is null");
        return false;
    }
    if (dataSource_) {
        dataSource_->Stop();
        dataSource_ = nullptr;
    }
    int32_t ret;
    if (mediaInfo.mediaUrl.find("http") == 0) {
        // online source
        ret = player_->SetSource(mediaInfo.mediaUrl);
    } else {
        // local source
        if (!fileChannelClient_) {
            return false;
        }
        fileChannelClient_->NotifyCreateChannel();
        dataSource_ = std::make_shared<LocalDataSource>(mediaInfo.mediaUrl, mediaInfo.mediaSize, fileChannelClient_);
        dataSource_->Start();
        fileChannelClient_->WaitCreateChannel();
        if (mediaInfo.mediaType == "IMAGE") {
            CLOGI("Start to get image resource");
            if (!GetImageResource()) {
                return false;
            }
            CLOGI("Get image resource successfully");
            return true;
        } else if (mediaInfo.mediaType == "AUDIO" && avMetadataHelper_) {
            CLOGD("Start to get album cover");
            avMetadataHelper_->SetSource(dataSource_);
            auto albumCoverMem = avMetadataHelper_->FetchArtPicture();
            ProcessAlbumCover(albumCoverMem);
        }
        ret = player_->SetSource(dataSource_);
    }
    if (ret != MSERR_OK) {
        CLOGE("Media player setSource failed");
        return false;
    }
    if (!callback_) {
        CLOGE("callback_ is null");
        return false;
    }
    callback_->SetPosInfo(mediaInfo.startPosition, mediaInfo.closingCreditsPosition);
    return true;
}

bool CastStreamPlayer::SetPlayRangeWithMode(int32_t start)
{
    CLOGI("SetPlayRangeWithMode in, start %{public}d", start);

    if (start <= 0) {
        CLOGE("start position less than 0");
        return false;
    }
    if (!player_) {
        CLOGE("Media player is null");
        return false;
    }
    if (player_->SetPlayRangeWithMode(start, -1) != MSERR_OK) {
        CLOGE("SetPlayRangeWithMode fail");
        return false;
    }

    return true;
}

bool CastStreamPlayer::GetImageResource()
{
    int64_t imageSize;
    if (!dataSource_) {
        return false;
    }
    dataSource_->GetSize(imageSize);
    std::unique_ptr<uint8_t[]> buffer = std::make_unique<uint8_t[]>(imageSize);
    if (!buffer) {
        return false;
    }
    uint8_t *currDstPos = buffer.get();
    int32_t sumReadBytes = CAST_STREAM_INT_INIT;
    int32_t readSize = imageSize;
    int32_t tryTimes = CAST_STREAM_INT_INIT;
    while (tryTimes < CAST_STREAM_MAX_TIMES && sumReadBytes < imageSize) {
        int32_t curReadBytes = dataSource_->ReadBuffer(currDstPos, readSize, sumReadBytes);
        currDstPos += curReadBytes;
        sumReadBytes += curReadBytes;
        readSize -= curReadBytes;
        if (curReadBytes <= 0) {
            tryTimes++;
            std::this_thread::sleep_for(std::chrono::milliseconds(CAST_STREAM_WAIT_TIME));
        }
    }
    if (sumReadBytes != imageSize) {
        CLOGE("read image bytes failed");
        return false;
    }

    Media::SourceOptions options;
    uint32_t errCode;
    std::unique_ptr<Media::ImageSource> imageSource
        = Media::ImageSource::CreateImageSource(buffer.get(), imageSize, options, errCode);
    if (imageSource == nullptr) {
        CLOGE("imageSource is null, errCode = %{public}d", errCode);
        return false;
    }
    Media::DecodeOptions decodeParam;
    auto pixelMap = imageSource->CreatePixelMap(0, decodeParam, errCode);
    if (pixelMap == nullptr || errCode != 0) {
        CLOGE("pixelMap is null, errCode = %{public}d", errCode);
        return false;
    }
    return InvokeImageChanged(std::move(pixelMap));
}

bool CastStreamPlayer::InvokeImageChanged(std::shared_ptr<Media::PixelMap> sharedImg)
{
    if (!callback_) {
        CLOGE("callback_ is null");
        return false;
    }
    callback_->OnImageChanged(sharedImg);
    return true;
}

bool CastStreamPlayer::ProcessAlbumCover(std::shared_ptr<Media::AVSharedMemory> albumCoverMem)
{
    if (!albumCoverMem) {
        CLOGE("albumCoverMem is null");
        return false;
    }
    CLOGI("Get album cover successfully, album size: %d", albumCoverMem->GetSize());
    Media::SourceOptions options;
    uint32_t errCode;
    std::unique_ptr<Media::ImageSource> imageSource
        = Media::ImageSource::CreateImageSource(albumCoverMem->GetBase(), albumCoverMem->GetSize(), options, errCode);
    if (imageSource == nullptr) {
        CLOGE("imageSource is null, errCode = %{public}d", errCode);
        return false;
    }
    Media::DecodeOptions decodeParam;
    auto pixelMap = imageSource->CreatePixelMap(0, decodeParam, errCode);
    if (pixelMap == nullptr || errCode != 0) {
        CLOGE("pixelMap is null, errCode = %{public}d", errCode);
        return false;
    }
    std::shared_ptr<Media::PixelMap> sharedAlbumCover = std::move(pixelMap);
    if (!callback_) {
        CLOGE("callback_ is null");
        return false;
    }
    callback_->OnAlbumCoverChanged(sharedAlbumCover);
    return true;
}

bool CastStreamPlayer::SendInitSysVolume()
{
    int maxVolume = CastStreamPlayerUtils::GetMaxVolume();
    CLOGI("Init maxVolume is %{public}d", maxVolume);
    int initVolume = CastStreamPlayerUtils::GetVolume();
    CLOGI("Init initVolume is %{public}d", initVolume);
    if (!callback_) {
        CLOGE("callback_ is null");
        return false;
    }
    callback_->OnVolumeChanged(initVolume, maxVolume);
    return true;
}

bool CastStreamPlayer::Play()
{
    CLOGD("Play in");
    if (!player_) {
        CLOGE("Media player is null");
        return false;
    }
    int32_t ret = player_->Play();
    if (ret != MSERR_OK) {
        CLOGE("Media player Play failed");
        return false;
    }
    SendInitSysVolume();
    return true;
}

bool CastStreamPlayer::Prepare()
{
    CLOGD("Prepare in");
    if (!player_) {
        CLOGE("Media player is null");
        return false;
    }
    int32_t ret = player_->Prepare();
    if (ret != MSERR_OK) {
        CLOGE("Media player prepare failed");
        return false;
    }
    SendInitSysVolume();
    return true;
}

bool CastStreamPlayer::PrepareAsync()
{
    CLOGD("PrepareAsync in");
    if (!player_) {
        CLOGE("Media player is null");
        return false;
    }
    int ret = player_->PrepareAsync();
    if (ret != MSERR_OK) {
        CLOGE("Media player prepareAsync failed");
        return false;
    }
    return true;
}

bool CastStreamPlayer::Pause()
{
    CLOGD("Pause in");
    if (!player_) {
        CLOGE("Media player is null");
        return false;
    }
    int32_t ret = player_->Pause();
    if (ret != MSERR_OK) {
        CLOGE("Media player pause failed");
        return false;
    }
    return true;
}

bool CastStreamPlayer::Stop()
{
    CLOGD("Stop in");
    if (!player_) {
        CLOGE("Media player is null");
        return false;
    }
    int32_t ret = player_->Stop();
    if (ret != MSERR_OK) {
        CLOGE("Media player stop failed");
        return false;
    }
    return true;
}

bool CastStreamPlayer::SeekPrepare(int32_t &mseconds, Media::PlayerSeekMode &mode)
{
    CLOGD("SeekPrepare in");
    int32_t duration = CAST_STREAM_INT_INVALID;
    [[maybe_unused]] int32_t seekPosition = CAST_STREAM_INT_INVALID;
    if (!player_) {
        CLOGE("Media player is null");
        return false;
    }
    int32_t ret = player_->GetDuration(duration);
    if (ret != MSERR_OK) {
        CLOGE("Media player getDuration failed");
        return false;
    }

    if (mseconds < 0) {
        seekPosition = 0;
    } else if (mseconds > duration) {
        seekPosition = duration;
    } else {
        seekPosition = mseconds;
    }
    return true;
}

bool CastStreamPlayer::Seek(int32_t mseconds, Media::PlayerSeekMode mode)
{
    CLOGD("Seek in");
    if (!SeekPrepare(mseconds, mode)) {
        return false;
    }
    CLOGI("Seek mseconds is %{public}d", mseconds);
    if (!player_) {
        CLOGE("Media player is null");
        return false;
    }
    int32_t ret = player_->Seek(mseconds, mode);
    if (ret != MSERR_OK) {
        CLOGE("Seek failed");
        return false;
    }
    return true;
}

bool CastStreamPlayer::Reset()
{
    CLOGD("Reset in");
    if (!player_) {
        CLOGE("Media player is null");
        return false;
    }
    int32_t ret = player_->Reset();
    if (ret != MSERR_OK) {
        CLOGE("Media player reset failed");
        return false;
    }
    return true;
}

bool CastStreamPlayer::Release()
{
    CLOGD("Release in");
    if (audioSystemMgr_) {
        audioSystemMgr_->UnregisterVolumeKeyEventCallback(getpid());
        audioSystemMgr_ = nullptr;
    }
    callback_ = nullptr;
    castStreamVolumeCallback_ = nullptr;
    if (!player_) {
        CLOGE("Media player is null");
        return false;
    }
    int32_t ret = player_->Release();
    if (ret != MSERR_OK) {
        CLOGE("Media player Release failed");
        return false;
    }
    player_ = nullptr;
    return true;
}

bool CastStreamPlayer::SetVolume(int32_t volume)
{
    CLOGD("SetVolume in");
    if (!audioSystemMgr_) {
        CLOGE("Audio manager is null");
        return false;
    }
    // In current audioStandard structure, all type will be converted into STREAM_MUSIC.
    auto streamType = AudioStandard::AudioVolumeType::STREAM_MUSIC;
    // maxVolume is the max value current system supports. For example, phone is 15.
    int maxVolume = audioSystemMgr_->GetMaxVolume(streamType);
    int ret = audioSystemMgr_->SetVolume(streamType, (volume * maxVolume) / CAST_STREAM_FULL_VOLUME);
    if (ret != MSERR_OK) {
        CLOGE("Media player SetVolume failed");
        return false;
    }
    return true;
}

bool CastStreamPlayer::GetVolume(int &volume, int &curMaxVolume)
{
    int maxVolume = CastStreamPlayerUtils::GetMaxVolume();
    CLOGI("Init maxVolume is %{public}d", maxVolume);
    int initVolume = CastStreamPlayerUtils::GetVolume();
    CLOGI("Init initVolume is %{public}d", initVolume);
    volume = initVolume;
    curMaxVolume = maxVolume;
    return true;
}

bool CastStreamPlayer::SetMute(bool mute)
{
    CLOGD("SetMute in");
    if (!audioSystemMgr_) {
        CLOGE("Audio manager is null");
        return false;
    }
    // In current audioStandard structure, all type will be converted into STREAM_MUSIC.
    auto streamType = AudioStandard::AudioVolumeType::STREAM_MUSIC;
    int ret = audioSystemMgr_->SetMute(streamType, mute);
    if (ret != MSERR_OK) {
        CLOGE("Media player SetMute failed");
        return false;
    }
    return true;
}

bool CastStreamPlayer::SetLooping(bool loop)
{
    CLOGD("SetLooping in");
    if (!player_) {
        CLOGE("Media player is null");
        return false;
    }
    int32_t ret = player_->SetLooping(loop);
    if (ret != MSERR_OK) {
        CLOGE("Media player SetLooping failed");
        return false;
    }
    return true;
}

bool CastStreamPlayer::GetCurrentTime(int32_t &currentTime)
{
    CLOGD("GetCurrentTime in");
    if (!player_) {
        CLOGE("Media player is null");
        return false;
    }
    int32_t ret = player_->GetCurrentTime(currentTime);
    if (ret != MSERR_OK) {
        CLOGE("Media player GetCurrentTime failed");
        return false;
    }
    return true;
}

bool CastStreamPlayer::GetVideoTrackInfo(std::vector<Media::Format> &videoTrack)
{
    CLOGD("GetVideoTrackInfo in");
    if (!player_) {
        CLOGE("Media player is null");
        return false;
    }
    int32_t ret = player_->GetVideoTrackInfo(videoTrack);
    if (ret != MSERR_OK) {
        CLOGE("Media player GetVideoTrackInfo failed");
        return false;
    }
    return true;
}

bool CastStreamPlayer::GetAudioTrackInfo(std::vector<Media::Format> &audioTrack)
{
    CLOGD("GetAudioTrackInfo in");
    if (!player_) {
        CLOGE("Media player is null");
        return false;
    }
    int32_t ret = player_->GetAudioTrackInfo(audioTrack);
    if (ret != MSERR_OK) {
        CLOGE("Media player GetAudioTrackInfo failed");
        return false;
    }
    return true;
}

bool CastStreamPlayer::GetVideoWidth()
{
    CLOGD("GetVideoWidth in");
    if (!player_) {
        CLOGE("Media player is null");
        return false;
    }
    int32_t ret = player_->GetVideoWidth();
    if (ret != MSERR_OK) {
        CLOGE("Media player GetVideoWidth failed");
        return false;
    }
    return true;
}

bool CastStreamPlayer::GetVideoHeight()
{
    CLOGD("GetVideoHeight in");
    if (!player_) {
        CLOGE("Media player is null");
        return false;
    }
    int32_t ret = player_->GetVideoHeight();
    if (ret != MSERR_OK) {
        CLOGE("Media player GetVideoHeight failed");
        return false;
    }
    return true;
}

int32_t CastStreamPlayer::GetDuration()
{
    CLOGD("GetDuration in");
    if (!player_) {
        CLOGE("Media player is null");
        return CAST_STREAM_INT_INVALID;
    }
    int duration = CAST_STREAM_INT_INVALID;
    int32_t ret = player_->GetDuration(duration);
    if (ret != MSERR_OK) {
        CLOGE("Media player GetDuration failed");
        return CAST_STREAM_INT_INVALID;
    }
    return duration;
}

bool CastStreamPlayer::GetMute()
{
    // In current audioStandard structure, all type will be converted into STREAM_MUSIC.
    auto streamType = AudioStandard::AudioVolumeType::STREAM_MUSIC;
    if (audioSystemMgr_ == nullptr) {
        CLOGE("Audio manager is null");
        return false;
    }
    return audioSystemMgr_->IsStreamMute(streamType);
}

bool CastStreamPlayer::SetLoopMode(const LoopMode mode)
{
    CLOGD("SetLoopMode in");
    if (!player_) {
        CLOGE("Media player is null");
        return false;
    }
    if (!callback_) {
        CLOGE("Media player callback is null");
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (mode == LoopMode::LOOP_MODE_SINGLE) {
        player_->SetLooping(true);
    } else {
        player_->SetLooping(false);
    }
    callback_->OnLoopModeChanged(mode);
    loopMode_ = mode;
    return true;
}

bool CastStreamPlayer::SetPlaybackSpeed(Media::PlaybackRateMode mode)
{
    CLOGD("SetPlaybackSpeed in");
    if (!player_) {
        CLOGE("Media player is null");
        return false;
    }
    int32_t ret = player_->SetPlaybackSpeed(mode);
    if (ret != MSERR_OK) {
        CLOGE("Media player SetPlaybackSpeed failed");
        return false;
    }
    if (!callback_) {
        CLOGE("Media player callback is null");
        return false;
    }
    callback_->SetSpeedMode(mode);
    return true;
}

LoopMode CastStreamPlayer::GetLoopMode()
{
    CLOGD("GetLoopMode in");
    std::lock_guard<std::mutex> lock(mutex_);
    return loopMode_;
}

bool CastStreamPlayer::GetPlaybackSpeed(Media::PlaybackRateMode &mode)
{
    CLOGD("GetPlaybackSpeed in");
    if (!player_) {
        CLOGE("Media player is null");
        return false;
    }
    int32_t ret = player_->GetPlaybackSpeed(mode);
    if (ret != MSERR_OK) {
        CLOGE("Media player GetPlaybackSpeed failed");
        return false;
    }
    return true;
}

bool CastStreamPlayer::SelectBitRate(uint32_t bitRate)
{
    CLOGD("SelectBitRate in");
    if (!player_) {
        CLOGE("Media player is null");
        return false;
    }
    int32_t ret = player_->SelectBitRate(bitRate);
    if (ret != MSERR_OK) {
        CLOGE("Media player SelectBitRate failed");
        return false;
    }
    return true;
}

bool CastStreamPlayer::IsPlaying()
{
    CLOGD("IsPlaying in");
    if (!player_) {
        CLOGE("Media player is null");
        return false;
    }
    if (player_->IsPlaying()) {
        return true;
    }
    if (!callback_) {
        CLOGE("callback_ is null");
        return false;
    }
    return callback_->IsPaused();
}

bool CastStreamPlayer::IsLooping()
{
    CLOGD("IsLooping in");
    if (!player_) {
        CLOGE("Media player is null");
        return false;
    }
    return player_->IsLooping();
}

void CastStreamPlayer::NotifyPlayComplete()
{
    CLOGD("NotifyPlayComplete in");
    if (dataSource_) {
        dataSource_->Stop();
        dataSource_ = nullptr;
    }
}

bool CastStreamPlayer::SetParameter(const Media::Format &param)
{
    CLOGD("SetParameter in");
    if (!player_) {
        CLOGE("Media player is null");
        return false;
    }
    int32_t ret = player_->SetParameter(param);
    if (ret != MSERR_OK) {
        CLOGE("Media player SetParameter failed");
        return false;
    }
    return true;
}

bool CastStreamPlayer::SetVideoSurface(sptr<Surface> surface)
{
    CLOGD("SetVideoSurface in");
    if (surface == nullptr) {
        CLOGE("Surface input is null");
        return false;
    }
    if (!player_) {
        CLOGE("Media player is null");
        return false;
    }
    int32_t ret = player_->SetVideoSurface(surface);
    if (ret != MSERR_OK) {
        CLOGE("Media player SetVideoSurface failed");
        return false;
    }
    return true;
}

CastStreamVolumeCallback::~CastStreamVolumeCallback()
{
    CLOGI("~CastStreamVolumeCallback in");
}

void CastStreamVolumeCallback::SetMaxVolume(int maxVolume)
{
    CLOGI("SetMaxVolume in");
    std::lock_guard<std::mutex> lock(mutex_);
    maxVolume_ = maxVolume;
    CLOGI("SetMaxVolume out");
}

void CastStreamVolumeCallback::OnVolumeKeyEvent(AudioStandard::VolumeEvent volumeEvent)
{
    if (volumeEvent.volumeType != AudioStandard::AudioStreamType::STREAM_MUSIC) {
        CLOGD("OnVolumeKeyEvent is filtered, when it's not STREAM_MUSIC type");
        return;
    }
    CLOGI("OnVolumeKeyEvent input %{public}d", volumeEvent.volume);
    if (!callback_) {
        CLOGE("StreamPlayerCallback is null");
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (maxVolume_ <= 0) {
        CLOGE("maxVolume_ <= 0");
        return;
    }
    int curVolume = (volumeEvent.volume * CAST_STREAM_FULL_VOLUME) / maxVolume_;
    callback_->OnVolumeChanged(curVolume, maxVolume_);
    CLOGD("OnVolumeKeyEvent out");
}
} // namespace CastEngineService
} // namespace CastEngine
} // namespace OHOS
