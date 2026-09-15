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
 * Description: stream player remote controller realization.
 * Author: huangchanggui
 * Create: 2023-02-01
 */

#include "remote_player_controller.h"

#include "cast_engine_dfx.h"
#include "cast_engine_errors.h"
#include "cast_engine_log.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineService {
DEFINE_CAST_ENGINE_LABEL("Cast-Player-Controller");

RemotePlayerController::~RemotePlayerController()
{
    CLOGD("~RemotePlayerController in");
}

void RemotePlayerController::SetSessionCallbackForRelease(const std::function<void(void)>& callback)
{
    std::lock_guard<std::mutex> lock(sessionCallbackMutex_);
    sessionCallback_ = callback;
}

int32_t RemotePlayerController::RegisterListener(sptr<IStreamPlayerListenerImpl> listener)
{
    CLOGI("RegisterListener in");
    if (!listener) {
        CLOGE("listener is null");
        return CAST_ENGINE_ERROR;
    }
    std::shared_ptr<ICastStreamManagerClient> targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerClient is null");
        return CAST_ENGINE_ERROR;
    }
    if (!targetCallback->RegisterListener(listener)) {
        CLOGE("RegisterListener failed");
        return CAST_ENGINE_ERROR;
    }
    std::lock_guard<std::mutex> lock(listenerMutex_);
    listener_ = listener;
    return CAST_ENGINE_SUCCESS;
}

int32_t RemotePlayerController::UnregisterListener()
{
    CLOGI("UnregisterListener in");
    std::shared_ptr<ICastStreamManagerClient> targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerClient is null");
        return CAST_ENGINE_ERROR;
    }
    if (!targetCallback->UnregisterListener()) {
        CLOGE("UnregisterListener failed");
        return CAST_ENGINE_ERROR;
    }
    std::lock_guard<std::mutex> lock(listenerMutex_);
    listener_ = nullptr;
    return CAST_ENGINE_SUCCESS;
}

int32_t RemotePlayerController::SetSurface(sptr<IBufferProducer> producer)
{
    CLOGE("Don't support SetSurface");
    static_cast<void>(producer);
    return CAST_ENGINE_ERROR;
}

int32_t RemotePlayerController::Load(const MediaInfo &mediaInfo)
{
    HiSysEventWriteWrap(__func__, {
            {"BIZ_SCENE", static_cast<int32_t>(BIZSceneType::DUAL_POINTS_CONTROL)},
            {"BIZ_STAGE", static_cast<int32_t>(BIZSceneStage::SOURCE_CONTROL)},
            {"STAGE_RES", static_cast<int32_t>(StageResType::STAGE_RES_SUCCESS)},
            {"ERROR_CODE", CAST_RADAR_SUCCESS}}, {
            {"TO_CALL_PKG", DSOFTBUS_NAME},
            {"LOCAL_SESS_NAME", ""},
            {"PEER_SESS_NAME", ""},
            {"PEER_UDID", ""}});

    CLOGI("Load in");
    if (!fileChannelServer_) {
        CLOGE("fileChannelServer_ is nullptr");
        return CAST_ENGINE_ERROR;
    }
    auto mediaInfoToPlay = mediaInfo;
    if (mediaInfo.mediaUrl != "http:") {
        fileChannelServer_->ClearAllLocalFileInfo();
        fileChannelServer_->AddLocalFileInfo(mediaInfoToPlay);
    }

    std::shared_ptr<ICastStreamManagerClient> targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerClient is null");
        return CAST_ENGINE_ERROR;
    }

    if (targetCallback->TransferToStreamMode(mediaInfoToPlay)) {
        return CAST_ENGINE_SUCCESS;
    }

    if (targetCallback->IsDoubleFrame()) {
        if (mediaInfo.mediaUrl == "http:") {
            CLOGD("No need for double frame to load");
            return CAST_ENGINE_SUCCESS;
        }
        if (!targetCallback->NotifyPeerPlay(mediaInfoToPlay)) {
            CLOGE("NotifyPeerPlay failed");
            return CAST_ENGINE_ERROR;
        }
    } else {
        if (!targetCallback->NotifyPeerLoad(mediaInfoToPlay)) {
            CLOGE("NotifyPeerLoad failed");
            return CAST_ENGINE_ERROR;
        }
    }

    return CAST_ENGINE_SUCCESS;
}

int32_t RemotePlayerController::Play(const MediaInfo &mediaInfo)
{
    HiSysEventWriteWrap(__func__, {
            {"BIZ_SCENE", static_cast<int32_t>(BIZSceneType::DUAL_POINTS_CONTROL)},
            {"BIZ_STATE", static_cast<int32_t>(BIZStateType::BIZ_STATE_BEGIN)},
            {"BIZ_STAGE", static_cast<int32_t>(BIZSceneStage::SOURCE_CONTROL)},
            {"STAGE_RES", static_cast<int32_t>(StageResType::STAGE_RES_SUCCESS)},
            {"ERROR_CODE", CAST_RADAR_SUCCESS}}, {
            {"TO_CALL_PKG", DSOFTBUS_NAME},
            {"LOCAL_SESS_NAME", ""},
            {"PEER_SESS_NAME", ""},
            {"PEER_UDID", ""}});

    CLOGI("Start in");
    if (!fileChannelServer_) {
        CLOGE("fileChannelServer_ is nullptr");
        return CAST_ENGINE_ERROR;
    }
    fileChannelServer_->ClearAllLocalFileInfo();
    auto mediaInfoToPlay = mediaInfo;
    fileChannelServer_->AddLocalFileInfo(mediaInfoToPlay);
    CastEngineDfx::SetStreamInfo("mediaType", mediaInfo.mediaType);
    std::shared_ptr<ICastStreamManagerClient> targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerClient is null");
        return CAST_ENGINE_ERROR;
    }
    if (!targetCallback->NotifyPeerPlay(mediaInfoToPlay)) {
        CLOGE("NotifyPeerPlay failed");
        return CAST_ENGINE_ERROR;
    }
    return CAST_ENGINE_SUCCESS;
}

int32_t RemotePlayerController::Play(int index)
{
    CLOGE("Don't support Play index remotely");
    static_cast<void>(index);
    return CAST_ENGINE_ERROR;
}

int32_t RemotePlayerController::Play()
{
    HiSysEventWriteWrap(__func__, {
            {"BIZ_SCENE", static_cast<int32_t>(BIZSceneType::DUAL_POINTS_CONTROL)},
            {"BIZ_STATE", static_cast<int32_t>(BIZStateType::BIZ_STATE_BEGIN)},
            {"BIZ_STAGE", static_cast<int32_t>(BIZSceneStage::SOURCE_CONTROL)},
            {"STAGE_RES", static_cast<int32_t>(StageResType::STAGE_RES_SUCCESS)},
            {"ERROR_CODE", CAST_RADAR_SUCCESS}}, {
            {"TO_CALL_PKG", DSOFTBUS_NAME},
            {"LOCAL_SESS_NAME", ""},
            {"PEER_SESS_NAME", ""},
            {"PEER_UDID", ""}});

    CLOGI("Play in");
    std::shared_ptr<ICastStreamManagerClient> targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerClient is null");
        return CAST_ENGINE_ERROR;
    }
    if (!targetCallback->NotifyPeerResume()) {
        CLOGE("NotifyPeerResume failed");
        return CAST_ENGINE_ERROR;
    }
    return CAST_ENGINE_SUCCESS;
}

int32_t RemotePlayerController::Pause()
{
    HiSysEventWriteWrap(__func__, {
            {"BIZ_SCENE", static_cast<int32_t>(BIZSceneType::DUAL_POINTS_CONTROL)},
            {"BIZ_STATE", static_cast<int32_t>(BIZStateType::BIZ_STATE_BEGIN)},
            {"BIZ_STAGE", static_cast<int32_t>(BIZSceneStage::SOURCE_CONTROL)},
            {"STAGE_RES", static_cast<int32_t>(StageResType::STAGE_RES_SUCCESS)},
            {"ERROR_CODE", CAST_RADAR_SUCCESS}}, {
            {"TO_CALL_PKG", DSOFTBUS_NAME},
            {"LOCAL_SESS_NAME", ""},
            {"PEER_SESS_NAME", ""},
            {"PEER_UDID", ""}});

    CLOGI("Pause in");
    std::shared_ptr<ICastStreamManagerClient> targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerClient is null");
        return CAST_ENGINE_ERROR;
    }
    if (!targetCallback->NotifyPeerPause()) {
        CLOGE("NotifyPeerPause failed");
        return CAST_ENGINE_ERROR;
    }
    return CAST_ENGINE_SUCCESS;
}

int32_t RemotePlayerController::Stop()
{
    HiSysEventWriteWrap(__func__, {
            {"BIZ_SCENE", static_cast<int32_t>(BIZSceneType::DUAL_POINTS_CONTROL)},
            {"BIZ_STAGE", static_cast<int32_t>(BIZSceneStage::SOURCE_CONTROL)},
            {"STAGE_RES", static_cast<int32_t>(StageResType::STAGE_RES_SUCCESS)},
            {"ERROR_CODE", CAST_RADAR_SUCCESS}}, {
            {"TO_CALL_PKG", DSOFTBUS_NAME},
            {"LOCAL_SESS_NAME", ""},
            {"PEER_SESS_NAME", ""},
            {"PEER_UDID", ""}});

    CLOGI("Stop in");
    std::shared_ptr<ICastStreamManagerClient> targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerClient is null");
        return CAST_ENGINE_ERROR;
    }
    if (!targetCallback->NotifyPeerStop()) {
        CLOGE("NotifyPeerStop failed");
        return CAST_ENGINE_ERROR;
    }
    return CAST_ENGINE_SUCCESS;
}

sptr<IStreamPlayerListenerImpl> RemotePlayerController::PlayerListenerGetter()
{
    std::lock_guard<std::mutex> lock(listenerMutex_);
    return listener_;
}

int32_t RemotePlayerController::Next()
{
    HiSysEventWriteWrap(__func__, {
            {"BIZ_SCENE", static_cast<int32_t>(BIZSceneType::DUAL_POINTS_CONTROL)},
            {"BIZ_STAGE", static_cast<int32_t>(BIZSceneStage::SOURCE_CONTROL)},
            {"STAGE_RES", static_cast<int32_t>(StageResType::STAGE_RES_SUCCESS)},
            {"ERROR_CODE", CAST_RADAR_SUCCESS}}, {
            {"TO_CALL_PKG", DSOFTBUS_NAME},
            {"LOCAL_SESS_NAME", ""},
            {"PEER_SESS_NAME", ""},
            {"PEER_UDID", ""}});

    CLOGI("Next in");
    auto playerListener = PlayerListenerGetter();
    if (!playerListener) {
        CLOGE("playerListener is nullptr");
        return CAST_ENGINE_ERROR;
    }
    playerListener->OnNextRequest();
    return CAST_ENGINE_SUCCESS;
}

int32_t RemotePlayerController::Previous()
{
    HiSysEventWriteWrap(__func__, {
            {"BIZ_SCENE", static_cast<int32_t>(BIZSceneType::DUAL_POINTS_CONTROL)},
            {"BIZ_STAGE", static_cast<int32_t>(BIZSceneStage::SOURCE_CONTROL)},
            {"STAGE_RES", static_cast<int32_t>(StageResType::STAGE_RES_SUCCESS)},
            {"ERROR_CODE", CAST_RADAR_SUCCESS}}, {
            {"TO_CALL_PKG", DSOFTBUS_NAME},
            {"LOCAL_SESS_NAME", ""},
            {"PEER_SESS_NAME", ""},
            {"PEER_UDID", ""}});

    CLOGI("Previous in");
    auto playerListener = PlayerListenerGetter();
    if (!playerListener) {
        CLOGE("playerListener is nullptr");
        return CAST_ENGINE_ERROR;
    }
    playerListener->OnPreviousRequest();
    return CAST_ENGINE_SUCCESS;
}

int32_t RemotePlayerController::Seek(int position)
{
    HiSysEventWriteWrap(__func__, {
            {"BIZ_SCENE", static_cast<int32_t>(BIZSceneType::DUAL_POINTS_CONTROL)},
            {"BIZ_STAGE", static_cast<int32_t>(BIZSceneStage::SOURCE_CONTROL)},
            {"STAGE_RES", static_cast<int32_t>(StageResType::STAGE_RES_SUCCESS)},
            {"ERROR_CODE", CAST_RADAR_SUCCESS}}, {
            {"TO_CALL_PKG", DSOFTBUS_NAME},
            {"LOCAL_SESS_NAME", ""},
            {"PEER_SESS_NAME", ""},
            {"PEER_UDID", ""}});

    CLOGI("Seek in position:%{public}d", position);
    if (position < 0) {
        CLOGE("position is null");
        return ERR_INVALID_PARAM;
    }
    std::shared_ptr<ICastStreamManagerClient> targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerClient is null");
        return CAST_ENGINE_ERROR;
    }
    if (!targetCallback->NotifyPeerSeek(position)) {
        CLOGE("NotifyPeerSeek failed");
        return CAST_ENGINE_ERROR;
    }
    return CAST_ENGINE_SUCCESS;
}

int32_t RemotePlayerController::FastForward(int delta)
{
    HiSysEventWriteWrap(__func__, {
            {"BIZ_SCENE", static_cast<int32_t>(BIZSceneType::DUAL_POINTS_CONTROL)},
            {"BIZ_STAGE", static_cast<int32_t>(BIZSceneStage::SOURCE_CONTROL)},
            {"STAGE_RES", static_cast<int32_t>(StageResType::STAGE_RES_SUCCESS)},
            {"ERROR_CODE", CAST_RADAR_SUCCESS}}, {
            {"TO_CALL_PKG", DSOFTBUS_NAME},
            {"LOCAL_SESS_NAME", ""},
            {"PEER_SESS_NAME", ""},
            {"PEER_UDID", ""}});

    CLOGI("FastForward with delta:%{public}d", delta);
    if (delta <= 0) {
        CLOGE("delta is invalid");
        return ERR_INVALID_PARAM;
    }
    std::shared_ptr<ICastStreamManagerClient> targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerClient is null");
        return CAST_ENGINE_ERROR;
    }
    if (!targetCallback->NotifyPeerFastForward(delta)) {
        CLOGE("NotifyFastForward failed");
        return CAST_ENGINE_ERROR;
    }
    return CAST_ENGINE_SUCCESS;
}

int32_t RemotePlayerController::FastRewind(int delta)
{
    HiSysEventWriteWrap(__func__, {
            {"BIZ_SCENE", static_cast<int32_t>(BIZSceneType::DUAL_POINTS_CONTROL)},
            {"BIZ_STAGE", static_cast<int32_t>(BIZSceneStage::SOURCE_CONTROL)},
            {"STAGE_RES", static_cast<int32_t>(StageResType::STAGE_RES_SUCCESS)},
            {"ERROR_CODE", CAST_RADAR_SUCCESS}}, {
            {"TO_CALL_PKG", DSOFTBUS_NAME},
            {"LOCAL_SESS_NAME", ""},
            {"PEER_SESS_NAME", ""},
            {"PEER_UDID", ""}});

    CLOGI("FastRewind with delta:%{public}d", delta);
    if (delta <= 0) {
        CLOGE("delta is invalid");
        return ERR_INVALID_PARAM;
    }
    std::shared_ptr<ICastStreamManagerClient> targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerClient is null");
        return CAST_ENGINE_ERROR;
    }
    if (!targetCallback->NotifyPeerFastRewind(delta)) {
        CLOGE("NotifyPeerFastRewind failed");
        return CAST_ENGINE_ERROR;
    }
    return CAST_ENGINE_SUCCESS;
}

int32_t RemotePlayerController::SetVolume(int volume)
{
    HiSysEventWriteWrap(__func__, {
            {"BIZ_SCENE", static_cast<int32_t>(BIZSceneType::DUAL_POINTS_CONTROL)},
            {"BIZ_STAGE", static_cast<int32_t>(BIZSceneStage::SOURCE_CONTROL)},
            {"STAGE_RES", static_cast<int32_t>(StageResType::STAGE_RES_SUCCESS)},
            {"ERROR_CODE", CAST_RADAR_SUCCESS}}, {
            {"TO_CALL_PKG", DSOFTBUS_NAME},
            {"LOCAL_SESS_NAME", ""},
            {"PEER_SESS_NAME", ""},
            {"PEER_UDID", ""}});

    CLOGI("SetVolume in volume:%{public}d", volume);
    if (volume < 0 || volume > CAST_STREAM_FULL_VOLUME) {
        CLOGE("volume is invalid");
        return ERR_INVALID_PARAM;
    }
    std::shared_ptr<ICastStreamManagerClient> targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerClient is null");
        return CAST_ENGINE_ERROR;
    }
    if (!targetCallback->NotifyPeerSetVolume(volume)) {
        CLOGE("NotifyPeerSetVolume failed");
        return CAST_ENGINE_ERROR;
    }
    return CAST_ENGINE_SUCCESS;
}

int32_t RemotePlayerController::SetMute(bool mute)
{
    HiSysEventWriteWrap(__func__, {
            {"BIZ_SCENE", static_cast<int32_t>(BIZSceneType::DUAL_POINTS_CONTROL)},
            {"BIZ_STAGE", static_cast<int32_t>(BIZSceneStage::SOURCE_CONTROL)},
            {"STAGE_RES", static_cast<int32_t>(StageResType::STAGE_RES_SUCCESS)},
            {"ERROR_CODE", CAST_RADAR_SUCCESS}}, {
            {"TO_CALL_PKG", DSOFTBUS_NAME},
            {"LOCAL_SESS_NAME", ""},
            {"PEER_SESS_NAME", ""},
            {"PEER_UDID", ""}});

    CLOGI("SetMute in: %{public}d", mute);
    std::shared_ptr<ICastStreamManagerClient> targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerClient is null");
        return CAST_ENGINE_ERROR;
    }
    if (!targetCallback->NotifyPeerSetMute(mute)) {
        CLOGE("NotifyPeerSetMute failed");
        return CAST_ENGINE_ERROR;
    }
    return CAST_ENGINE_SUCCESS;
}

int32_t RemotePlayerController::SetLoopMode(const LoopMode mode)
{
    HiSysEventWriteWrap(__func__, {
            {"BIZ_SCENE", static_cast<int32_t>(BIZSceneType::DUAL_POINTS_CONTROL)},
            {"BIZ_STAGE", static_cast<int32_t>(BIZSceneStage::SOURCE_CONTROL)},
            {"STAGE_RES", static_cast<int32_t>(StageResType::STAGE_RES_SUCCESS)},
            {"ERROR_CODE", CAST_RADAR_SUCCESS}}, {
            {"TO_CALL_PKG", DSOFTBUS_NAME},
            {"LOCAL_SESS_NAME", ""},
            {"PEER_SESS_NAME", ""},
            {"PEER_UDID", ""}});

    auto loopMode = static_cast<int>(mode);
    CLOGI("SetLoopMode in mode:%{public}d", loopMode);
    std::shared_ptr<ICastStreamManagerClient> targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerClient is null");
        return CAST_ENGINE_ERROR;
    }
    if (!targetCallback->NotifyPeerSetRepeatMode(loopMode)) {
        CLOGE("NotifyPeerSetRepeatMode failed");
        return CAST_ENGINE_ERROR;
    }
    return CAST_ENGINE_SUCCESS;
}

int32_t RemotePlayerController::SetAvailableCapability(const StreamCapability &streamCapability)
{
    CLOGI("SetAvailableCapability in");
    std::shared_ptr<ICastStreamManagerClient> targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerClient is null");
        return CAST_ENGINE_ERROR;
    }
    if (!targetCallback->NotifyPeerSetAvailableCapability(streamCapability)) {
        CLOGE("NotifyPeerSetAvailableCapability failed");
        return CAST_ENGINE_ERROR;
    }
    return CAST_ENGINE_SUCCESS;
}

int32_t RemotePlayerController::SetSpeed(const PlaybackSpeed speed)
{
    HiSysEventWriteWrap(__func__, {
            {"BIZ_SCENE", static_cast<int32_t>(BIZSceneType::DUAL_POINTS_CONTROL)},
            {"BIZ_STAGE", static_cast<int32_t>(BIZSceneStage::SOURCE_CONTROL)},
            {"STAGE_RES", static_cast<int32_t>(StageResType::STAGE_RES_SUCCESS)},
            {"ERROR_CODE", CAST_RADAR_SUCCESS}}, {
            {"TO_CALL_PKG", DSOFTBUS_NAME},
            {"LOCAL_SESS_NAME", ""},
            {"PEER_SESS_NAME", ""},
            {"PEER_UDID", ""}});

    auto playbackSpeed = static_cast<int>(speed);
    CLOGI("SetSpeed in speed:%{public}d", playbackSpeed);
    std::shared_ptr<ICastStreamManagerClient> targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerClient is null");
        return CAST_ENGINE_ERROR;
    }
    if (!targetCallback->NotifyPeerSetSpeed(playbackSpeed)) {
        CLOGE("NotifyPeerSetSpeed failed");
        return CAST_ENGINE_ERROR;
    }
    return CAST_ENGINE_SUCCESS;
}

int32_t RemotePlayerController::GetPlayerStatus(PlayerStates &playerStates)
{
    CLOGI("GetPlayerStatus in");
    std::shared_ptr<ICastStreamManagerClient> targetCallback = callback_.lock();
    playerStates = PlayerStates::PLAYER_STATE_ERROR;
    if (!targetCallback) {
        CLOGE("ICastStreamManagerClient is null");
        return CAST_ENGINE_ERROR;
    }
    playerStates = targetCallback->GetPlayerStatus();
    return CAST_ENGINE_SUCCESS;
}

int32_t RemotePlayerController::GetPosition(int &position)
{
    CLOGI("GetPosition in");
    std::shared_ptr<ICastStreamManagerClient> targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerClient is null");
        return CAST_ENGINE_ERROR;
    }
    position = targetCallback->GetPosition();
    return CAST_ENGINE_SUCCESS;
}

int32_t RemotePlayerController::GetDuration(int &duration)
{
    CLOGI("GetDuration in");
    std::shared_ptr<ICastStreamManagerClient> targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerClient is null");
        return CAST_ENGINE_ERROR;
    }
    duration = targetCallback->GetDuration();
    return CAST_ENGINE_SUCCESS;
}

int32_t RemotePlayerController::GetVolume(int &volume, int &maxVolume)
{
    CLOGI("GetVolume in");
    std::shared_ptr<ICastStreamManagerClient> targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerClient is null");
        return CAST_ENGINE_ERROR;
    }
    volume = targetCallback->GetVolume();
    maxVolume = targetCallback->GetMaxVolume();
    return CAST_ENGINE_SUCCESS;
}

int32_t RemotePlayerController::GetMute(bool &mute)
{
    CLOGI("GetMute in");
    std::shared_ptr<ICastStreamManagerClient> targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerClient is null");
        return CAST_ENGINE_ERROR;
    }
    mute = targetCallback->GetMute();
    return CAST_ENGINE_SUCCESS;
}

int32_t RemotePlayerController::GetLoopMode(LoopMode &loopMode)
{
    CLOGI("GetLoopMode in");
    std::shared_ptr<ICastStreamManagerClient> targetCallback = callback_.lock();
    loopMode = LoopMode::LOOP_MODE_LIST;
    if (!targetCallback) {
        CLOGE("ICastStreamManagerClient is null");
        return CAST_ENGINE_ERROR;
    }
    loopMode = targetCallback->GetLoopMode();
    return CAST_ENGINE_SUCCESS;
}

int32_t RemotePlayerController::GetAvailableCapability(StreamCapability &streamCapability)
{
    CLOGI("GetAvailableCapability in");
    std::shared_ptr<ICastStreamManagerClient> targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerClient is null");
        return CAST_ENGINE_ERROR;
    }
    streamCapability = targetCallback->GetAvailableCapability();
    return CAST_ENGINE_SUCCESS;
}

int32_t RemotePlayerController::GetPlaySpeed(PlaybackSpeed &playbackSpeed)
{
    CLOGI("GetPlaySpeed in");
    std::shared_ptr<ICastStreamManagerClient> targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerClient is null");
        playbackSpeed = PlaybackSpeed::SPEED_FORWARD_1_00_X;
        return CAST_ENGINE_ERROR;
    }

    playbackSpeed = targetCallback->GetPlaySpeed();
    return CAST_ENGINE_SUCCESS;
}

int32_t RemotePlayerController::GetMediaInfoHolder(MediaInfoHolder &mediaInfoHolder)
{
    CLOGE("Don't support GetMediaInfoHolder");
    return CAST_ENGINE_ERROR;
}

int32_t RemotePlayerController::ProvideKeyResponse(const std::string &mediaId, const std::vector<uint8_t> &response)
{
    CLOGI("ProvideKeyResponse in");
    std::shared_ptr<ICastStreamManagerClient> targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerClient is null");
        return CAST_ENGINE_ERROR;
    }
    if (!targetCallback->NotifyPeerKeyResponse(mediaId, response)) {
        CLOGE("NotifyPeerKeyResponse failed");
        return CAST_ENGINE_ERROR;
    }
    return CAST_ENGINE_SUCCESS;
}

int32_t RemotePlayerController::UpdateMediaInfo(const MediaInfo &mediaInfo)
{
    CLOGI("UpdateMediaInfo in");
    auto playerListener = PlayerListenerGetter();
    if (!playerListener) {
        CLOGE("playerListener is nullptr");
        return CAST_ENGINE_ERROR;
    }
    playerListener->OnMediaInfoChanged(mediaInfo);
    std::shared_ptr<ICastStreamManagerClient> targetCallback = callback_.lock();
    if (!targetCallback) {
        CLOGE("ICastStreamManagerClient is null");
        return CAST_ENGINE_ERROR;
    }
    if (!targetCallback->NotifyPeerMediaInfoChanged(mediaInfo)) {
        CLOGE("NotifyPeerMediaInfoChanged failed");
        return CAST_ENGINE_ERROR;
    }
    return CAST_ENGINE_SUCCESS;
}

int32_t RemotePlayerController::Release()
{
    std::lock_guard<std::mutex> lock(sessionCallbackMutex_);
    if (!sessionCallback_) {
        CLOGE("sessionCallback is null");
        return CAST_ENGINE_ERROR;
    }
    sessionCallback_();
    return CAST_ENGINE_SUCCESS;
}
} // namespace CastEngineService
} // namespace CastEngine
} // namespace OHOS