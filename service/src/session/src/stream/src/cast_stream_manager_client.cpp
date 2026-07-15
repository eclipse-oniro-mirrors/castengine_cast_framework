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
 * Description: Cast stream manager client definition, which processes and sends client's instructions.
 * Author: zhangjingnan
 * Create: 2023-08-30
 */

#include "cast_stream_manager_client.h"
#include "cast_engine_log.h"
#include "remote_player_controller.h"
#include "cast_local_file_channel_server.h"
#include "i_stream_player_ipc.h"
#include "stream_player_impl_stub.h"
#include "cast_engine_common.h"
#include "cast_engine_dfx.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineService {
DEFINE_CAST_ENGINE_LABEL("Cast-Stream-Manager-Client");

namespace {
void ProcessActionWriteWrap(const std::string& funcName)
{
    HiSysEventWriteWrap(funcName, {
        {"BIZ_SCENE", static_cast<int32_t>(BIZSceneType::DUAL_POINTS_CONTROL)},
        {"BIZ_STATE", static_cast<int32_t>(BIZStateType::BIZ_STATE_END)},
        {"BIZ_STAGE", static_cast<int32_t>(BIZSceneStage::SOURCE_RESPONSE)},
        {"STAGE_RES", static_cast<int32_t>(StageResType::STAGE_RES_SUCCESS)},
        {"ERROR_CODE", CAST_RADAR_SUCCESS}}, {
        {"TO_CALL_PKG", DSOFTBUS_NAME},
        {"LOCAL_SESS_NAME", ""},
        {"PEER_SESS_NAME", ""},
        {"PEER_UDID", ""}});
}
}

CastStreamManagerClient::CastStreamManagerClient(std::shared_ptr<ICastStreamListener> listener, bool isDoubleFrame)
{
    CLOGD("CastStreamManagerClient in");
    streamActionProcessor_ = {
        { ACTION_PLAYER_STATUS_CHANGED, [this](const json &data) { return ProcessActionPlayerStatusChanged(data); } },
        { ACTION_POSITION_CHANGED, [this](const json &data) { return ProcessActionPositionChanged(data); } },
        { ACTION_MEDIA_ITEM_CHANGED, [this](const json &data) { return ProcessActionMediaItemChanged(data); } },
        { ACTION_VOLUME_CHANGED, [this](const json &data) { return ProcessActionVolumeChanged(data); } },
        { ACTION_REPEAT_MODE_CHANGED, [this](const json &data) { return ProcessActionRepeatModeChanged(data); } },
        { ACTION_SPEED_CHANGED, [this](const json &data) { return ProcessActionSpeedChanged(data); } },
        { ACTION_PLAYER_ERROR, [this](const json &data) { return ProcessActionPlayerError(data); } },
        { ACTION_NEXT_REQUEST, [this](const json &data) { return ProcessActionNextRequest(data); } },
        { ACTION_PREVIOUS_REQUEST, [this](const json &data) { return ProcessActionPreviousRequest(data); } },
        { ACTION_SEEK_DONE, [this](const json &data) { return ProcessActionSeekDone(data); } },
        { ACTION_END_OF_STREAM, [this](const json &data) { return ProcessActionEndOfStream(data); } },
        { ACTION_PLAY_REQUEST, [this](const json &data) { return ProcessActionPlayRequest(data); } },
        { ACTION_KEY_REQUEST, [this](const json &data) { return ProcessActionKeyRequest(data); } }
    };
    streamListener_ = listener;
    timer_ = std::make_shared<CastTimer>();
    isDoubleFrame_ = isDoubleFrame;
}

CastStreamManagerClient::~CastStreamManagerClient()
{
    CLOGD("~CastStreamManagerClient in");
    player_ = nullptr;
    timer_ = nullptr;
}

bool CastStreamManagerClient::IsDoubleFrame()
{
    std::lock_guard<std::mutex> lock(eventMutex_);
    return isDoubleFrame_;
}

sptr<IStreamPlayerIpc> CastStreamManagerClient::CreateStreamPlayer(const std::function<void(void)> &releaseCallback)
{
    std::lock_guard<std::mutex> lock(dataMutex_);
    if (playerIpc_) {
        return playerIpc_;
    }
    auto fileChannel = std::make_shared<CastLocalFileChannelServer>();
    auto player = std::make_shared<RemotePlayerController>(shared_from_this(), fileChannel);
    if (player == nullptr) {
        CLOGE("streamController is null");
        return nullptr;
    }
    player->SetSessionCallbackForRelease(releaseCallback);
    auto streamPlayer = new (std::nothrow) StreamPlayerImplStub(player);
    if (streamPlayer == nullptr) {
        CLOGE("streamPlayer is null");
        return nullptr;
    }
    localFileChannel_ = fileChannel;
    player_ = player;
    playerIpc_ = streamPlayer;
    return streamPlayer;
}

bool CastStreamManagerClient::RegisterListener(sptr<IStreamPlayerListenerImpl> listener)
{
    std::lock_guard<std::mutex> lock(listenerMutex_);
    playerListener_ = listener;
    return true;
}

bool CastStreamManagerClient::UnregisterListener()
{
    std::lock_guard<std::mutex> lock(listenerMutex_);
    playerListener_ = nullptr;
    return true;
}

bool CastStreamManagerClient::NotifyPeerLoad(const MediaInfo &mediaInfo)
{
    CLOGD("NotifyPeerLoad in");
    json body;
    json list;
    body[KEY_CURRENT_INDEX] = 0;
    body[KEY_PROGRESS_INTERVAL] = 0;
    size_t mediaInfoListSize = 1;
    {
        std::lock_guard<std::mutex> lock(eventMutex_);
        startPosition_ = mediaInfo.startPosition;
        currentPosition_ = CAST_STREAM_INT_INVALID;
        currentDuration_ = CAST_STREAM_INT_INVALID;
        currentBuffer_ = CAST_STREAM_INT_INVALID;
    }
    for (size_t i = 0; i < mediaInfoListSize; i++) {
        json info;
        EncapMediaInfo(mediaInfo, info, IsDoubleFrame());
        list[i] = info;
    }
    body[KEY_LIST] = list;
    CLOGD("list size:%{public}zu ", list.size());
    return SendControlAction(ACTION_LOAD, body);
}

bool CastStreamManagerClient::NotifyPeerPlay(const MediaInfo &mediaInfo)
{
    CLOGD("NotifyPeerPlay in");
    json body;
    json list;
    body[KEY_CURRENT_INDEX] = 0;
    body[KEY_PROGRESS_INTERVAL] = 0;
    size_t mediaInfoListSize = 1;
    {
        std::lock_guard<std::mutex> lock(eventMutex_);
        startPosition_ = mediaInfo.startPosition;
        currentPosition_ = CAST_STREAM_INT_INVALID;
        currentDuration_ = CAST_STREAM_INT_INVALID;
        currentBuffer_ = CAST_STREAM_INT_INVALID;
    }
    for (size_t i = 0; i < mediaInfoListSize; i++) {
        json info;
        EncapMediaInfo(mediaInfo, info, IsDoubleFrame());
        list[i] = info;
    }
    body[KEY_LIST] = list;
    CLOGD("list size:%{public}zu ", list.size());
    return SendControlAction(ACTION_PLAY, body);
}

bool CastStreamManagerClient::NotifyPeerPause()
{
    CLOGD("NotifyPeerPause in");
    return SendControlAction(ACTION_PAUSE);
}

bool CastStreamManagerClient::NotifyPeerResume()
{
    CLOGD("NotifyPeerResume in");
    return SendControlAction(ACTION_RESUME);
}

bool CastStreamManagerClient::NotifyPeerStop()
{
    CLOGD("NotifyPeerStop in");
    return SendControlAction(ACTION_STOP);
}

bool CastStreamManagerClient::NotifyPeerNext()
{
    CLOGD("NotifyPeerNext in");
    return SendControlAction(ACTION_NEXT);
}

bool CastStreamManagerClient::NotifyPeerPrevious()
{
    CLOGD("NotifyPeerPrevious in");
    return SendControlAction(ACTION_PREVIOUS);
}

bool CastStreamManagerClient::NotifyPeerSeek(int position)
{
    CLOGD("NotifyPeerSeek in");
    isSeeking_ = true;
    {
        std::lock_guard<std::mutex> lock(eventMutex_);
        seekPosition_ = position;
    }
    json body;
    body[KEY_POSITION] = position;
    return SendControlAction(ACTION_SEEK, body);
}

bool CastStreamManagerClient::NotifyPeerFastForward(int delta)
{
    CLOGD("NotifyPeerFastForward in");
    json body;
    body[KEY_DELTA] = delta;
    return SendControlAction(ACTION_FAST_FORWARD, body);
}

bool CastStreamManagerClient::NotifyPeerFastRewind(int delta)
{
    CLOGD("NotifyPeerFastRewind in");
    json body;
    body[KEY_DELTA] = delta;
    return SendControlAction(ACTION_FAST_REWIND, body);
}

bool CastStreamManagerClient::NotifyPeerSetVolume(int volume)
{
    CLOGD("NotifyPeerSetVolume in");
    json body;
    body[KEY_VOLUME] = volume;
    return SendControlAction(ACTION_SET_VOLUME, body);
}

bool CastStreamManagerClient::NotifyPeerSetMute(bool mute)
{
    CLOGD("NotifyPeerSetMute in");
    json body;
    body[KEY_MUTE] = mute;
    return SendControlAction(ACTION_SET_MUTE, body);
}

bool CastStreamManagerClient::NotifyPeerSetRepeatMode(int mode)
{
    CLOGD("NotifyPeerSetRepeatMode in");
    json body;
    body[KEY_MODE] = mode;
    return SendControlAction(ACTION_SET_REPEAT_MODE, body);
}

bool CastStreamManagerClient::NotifyPeerSetAvailableCapability(const StreamCapability &streamCapability)
{
    CLOGD("NotifyPeerSetAvailableCapability in");
    {
        std::lock_guard<std::mutex> lock(eventMutex_);
        availableCapability_ = streamCapability;
    }
    json body;
    EncapStreamCapability(streamCapability, body);
    return SendControlAction(ACTION_SET_AVAILABLE_CAPABILITY, body);
}

bool CastStreamManagerClient::NotifyPeerSetSpeed(int speed)
{
    CLOGD("NotifyPeerSetSpeed in");
    json body;
    body[KEY_SPEED] = speed;
    return SendControlAction(ACTION_SET_SPEED, body);
}

bool CastStreamManagerClient::NotifyPeerKeyResponse(const std::string &mediaId, const std::vector<uint8_t> &response)
{
    CLOGD("NotifyPeerKeyResponse in");
    json body;
    body[KEY_MEDIA_ID] = mediaId;
    char charArray[response.size()];
    for (size_t i = 0; i < response.size(); i++) {
        charArray[i] = static_cast<char>(response[i]);
    }
    std::string str(charArray, sizeof(charArray));
    body[KEY_RESPONSE_KEY] = str;
    return SendControlAction(ACTION_PROVIDE_KEY_RESPONSE, body);
}

PlayerStates CastStreamManagerClient::GetPlayerStatus()
{
    CLOGD("GetPlayerStatus in");
    std::lock_guard<std::mutex> lock(eventMutex_);
    return currentState_;
}

int CastStreamManagerClient::GetPosition()
{
    CLOGD("GetPosition in");
    std::lock_guard<std::mutex> lock(eventMutex_);
    return currentPosition_;
}

int CastStreamManagerClient::GetDuration()
{
    CLOGD("GetDuration in");
    std::lock_guard<std::mutex> lock(eventMutex_);
    return currentDuration_;
}

int CastStreamManagerClient::GetVolume()
{
    CLOGD("GetVolume in");
    std::lock_guard<std::mutex> lock(eventMutex_);
    return currentVolume_;
}

int CastStreamManagerClient::GetMaxVolume()
{
    CLOGD("GetMaxVolume in");
    std::lock_guard<std::mutex> lock(eventMutex_);
    return maxVolume_;
}

bool CastStreamManagerClient::GetMute()
{
    CLOGD("GetMute in");
    std::lock_guard<std::mutex> lock(eventMutex_);
    isMute_ = currentVolume_ == 0;
    return isMute_;
}

LoopMode CastStreamManagerClient::GetLoopMode()
{
    CLOGD("GetLoopMode in");
    std::lock_guard<std::mutex> lock(eventMutex_);
    return currentMode_;
}

StreamCapability CastStreamManagerClient::GetAvailableCapability()
{
    CLOGD("GetAvailableCapability in");
    std::lock_guard<std::mutex> lock(eventMutex_);
    return availableCapability_;
}

PlaybackSpeed CastStreamManagerClient::GetPlaySpeed()
{
    CLOGD("GetPlaySpeed in");
    std::lock_guard<std::mutex> lock(eventMutex_);
    return currentSpeed_;
}

void CastStreamManagerClient::OnEvent(EventId eventId, const std::string &data)
{
    CLOGD("OnEvent in");
    if (!streamListener_) {
        CLOGE("streamListener_ is nullptr");
        return;
    }
    streamListener_->OnEvent(eventId, data);
}

bool CastStreamManagerClient::TransferToStreamMode(const MediaInfo &mediaInfo)
{
    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        mediaInfo_ = mediaInfo;
    }
    CLOGI("TransferToStreamMode in");
    if (!streamListener_) {
        CLOGE("streamListener_ is nullptr");
        return false;
    }

    return streamListener_->TransferToStreamMode();
}

bool CastStreamManagerClient::DisconnectSession(std::string deviceId)
{
    CLOGI("DisconnectSession in");
    if (!streamListener_) {
        CLOGE("streamListener_ is nullptr");
        return false;
    }

    return streamListener_->DisconnectSession(deviceId);
}

bool CastStreamManagerClient::PlayAfterSwitchToStream()
{
    std::lock_guard<std::mutex> lock(dataMutex_);
    if (!player_) {
        CLOGE("player_ is nullptr");
        return false;
    }

    player_->Play(mediaInfo_);
    return true;
}

sptr<IStreamPlayerListenerImpl> CastStreamManagerClient::PlayerListenerGetter()
{
    std::lock_guard<std::mutex> lock(listenerMutex_);
    return playerListener_;
}

bool CastStreamManagerClient::AutoUpdateCurPosition()
{
    auto playerListener = PlayerListenerGetter();
    if (!playerListener) {
        CLOGE("playerListener is nullptr");
        return false;
    }
    int position;
    {
        std::lock_guard<std::mutex> lock(eventMutex_);
        currentPosition_ = currentPosition_ + AUTO_POSITION_UPDATE_INTERVAL;
        position = currentPosition_;
    }
    CLOGD("AutoUpdateCurPosition:%{public}d", position);
    playerListener->OnPositionChanged(position, CAST_STREAM_INT_IGNORE, CAST_STREAM_INT_IGNORE);
    return true;
}

bool CastStreamManagerClient::ProcessActionPlayerStatusChanged(const json &data)
{
    auto funcName = __func__;
    ProcessActionWriteWrap(funcName);

    auto playerListener = PlayerListenerGetter();
    if (!playerListener) {
        CLOGE("playerListener is nullptr");
        return false;
    }
    int state;
    bool isPlayWhenReady = false;
    RETURN_FALSE_IF_PARSE_NUMBER_WRONG(state, data, KEY_PLAY_BACK_STATE);
    RETURN_FALSE_IF_PARSE_BOOL_WRONG(isPlayWhenReady, data, KEY_IS_PLAY_WHEN_READY);
    PlayerStates playbackState = PlayerStates::PLAYER_IDLE;
    if (IsDoubleFrame()) {
        auto hmosPlaybackState = static_cast<HmosPlayerStates>(state);
        if (hmosPlaybackState == HmosPlayerStates::STATE_BUFFERING) {
            return true;
        }
        playbackState = ProcessHmosPlayerStatus(hmosPlaybackState, isPlayWhenReady);
    } else {
        playbackState = static_cast<PlayerStates>(state);
    }
    {
        std::lock_guard<std::mutex> lock(eventMutex_);
        currentState_ = playbackState;
    }
    CLOGI("playbackState:%{public}d  isPlayWhenReady:%{public}d", playbackState, isPlayWhenReady);
    playerListener->OnStateChanged(playbackState, isPlayWhenReady);

    if (IsDoubleFrame()) {
        return true;
    }
    if (!timer_) {
        CLOGE("timer_ is nullptr");
        return false;
    }
    if (playbackState == PlayerStates::PLAYER_STARTED) {
        if (timer_->IsStopped()) {
            playerListener->OnPositionChanged(GetPosition(), CAST_STREAM_INT_IGNORE, CAST_STREAM_INT_IGNORE);
            timer_->Start([this]() { AutoUpdateCurPosition();}, AUTO_POSITION_UPDATE_INTERVAL);
        }
    } else {
        if (!timer_->IsStopped()) {
            playerListener->OnPositionChanged(GetPosition(), CAST_STREAM_INT_IGNORE, CAST_STREAM_INT_IGNORE);
            timer_->Stop();
        }
    }
    return true;
}

PlayerStates CastStreamManagerClient::ProcessHmosPlayerStatus(HmosPlayerStates hmosPlaybackState, bool isPlayWhenReady)
{
    auto playerListener = PlayerListenerGetter();
    if (!playerListener) {
        CLOGE("playerListener is nullptr");
        return PlayerStates::PLAYER_STATE_ERROR;
    }
    PlayerStates playbackState = PlayerStates::PLAYER_IDLE;
    if (hmosPlaybackState == HmosPlayerStates::STATE_READY) {
        if (currentState_ == PlayerStates::PLAYER_IDLE) {
            playerListener->OnStateChanged(PlayerStates::PLAYER_PREPARED, isPlayWhenReady);
        }
        if (isPlayWhenReady) {
            playbackState = PlayerStates::PLAYER_STARTED;
        } else {
            playbackState = PlayerStates::PLAYER_PAUSED;
        }
    } else if (hmosPlaybackState == HmosPlayerStates::STATE_ENDED) {
        playerListener->OnEndOfStream(false);
        playbackState = PlayerStates::PLAYER_PLAYBACK_COMPLETE;
    } else {
        auto iter = hmosStreamStateConvertor_.find(hmosPlaybackState);
        if (iter == hmosStreamStateConvertor_.end()) {
            CLOGE("unsupport hmosPlaybackState");
            return PlayerStates::PLAYER_STATE_ERROR;
        }
        playbackState = static_cast<PlayerStates>(iter->second);
    }
    return playbackState;
}

bool CastStreamManagerClient::ProcessActionPositionChanged(const json &data)
{
    auto playerListener = PlayerListenerGetter();
    if (!playerListener) {
        CLOGE("playerListener is nullptr");
        return false;
    }
    int position;
    int bufferPosition;
    int duration;
    RETURN_FALSE_IF_PARSE_NUMBER_WRONG(position, data, KEY_POSITION);
    RETURN_FALSE_IF_PARSE_NUMBER_WRONG(bufferPosition, data, KEY_BUFFER_POSITION);
    RETURN_FALSE_IF_PARSE_NUMBER_WRONG(duration, data, KEY_DURATION);

    bool isDoubleFrame = IsDoubleFrame();
    PlayerStates currentState;
    {
        std::lock_guard<std::mutex> lock(eventMutex_);
        if (duration != CAST_STREAM_INT_IGNORE) {
            currentDuration_ = duration;
        }
        if (bufferPosition != CAST_STREAM_INT_IGNORE) {
            currentBuffer_ = bufferPosition;
        }
        if (position != CAST_STREAM_INT_IGNORE) {
            currentPosition_ = position;
        }
        currentState = currentState_;
    }
    if (position != CAST_STREAM_INT_IGNORE) {
        if (!isDoubleFrame && timer_ && currentState == PlayerStates::PLAYER_STARTED) {
            if (!timer_->IsStopped()) {
                timer_->Stop();
            }
            timer_->Start([this]() { AutoUpdateCurPosition();}, AUTO_POSITION_UPDATE_INTERVAL);
        }
    }
    if (isDoubleFrame) {
        ProcessHmosPlayerPosition(position);
    }
    CLOGI("position:%{public}d  bufferPosition:%{public}d duration:%{public}d", position, bufferPosition, duration);
    playerListener->OnPositionChanged(position, bufferPosition, duration);
    CLOGI("OnPositionChanged out");
    return true;
}

void CastStreamManagerClient::ProcessHmosPlayerPosition(int position)
{
    auto playerListener = PlayerListenerGetter();
    if (!playerListener) {
        CLOGE("playerListener is nullptr");
        return;
    }
    if (isNewResourceLoaded_ && startPosition_ == position) {
        isNewResourceLoaded_ = false;
        playerListener->OnSeekDone(position);
        return;
    }
    if (isSeeking_ && seekPosition_ == position) {
        isSeeking_ = false;
        playerListener->OnSeekDone(position);
    }
    return;
}

bool CastStreamManagerClient::ProcessActionMediaItemChanged(const json &data)
{
    auto playerListener = PlayerListenerGetter();
    if (!playerListener) {
        CLOGE("playerListener is nullptr");
        return false;
    }
    MediaInfo mediaInfo = MediaInfo();
    if (!ParseMediaInfo(data, mediaInfo, IsDoubleFrame())) {
        return false;
    }
    playerListener->OnMediaItemChanged(mediaInfo);
    isNewResourceLoaded_ = true;
    currentState_ = PlayerStates::PLAYER_IDLE;
    CLOGI("ProcessActionMediaItemChanged out");
    return true;
}

bool CastStreamManagerClient::ProcessActionVolumeChanged(const json &data)
{
    auto playerListener = PlayerListenerGetter();
    if (!playerListener) {
        CLOGE("playerListener is nullptr");
        return false;
    }
    int volume;
    int maxVolume = 15;
    RETURN_FALSE_IF_PARSE_NUMBER_WRONG(volume, data, KEY_VOLUME);
    if (!IsDoubleFrame()) {
        RETURN_FALSE_IF_PARSE_NUMBER_WRONG(maxVolume, data, KEY_MAX_VOLUME);
    }
    {
        std::lock_guard<std::mutex> lock(eventMutex_);
        currentVolume_ = volume;
        maxVolume_ = maxVolume;
        isMute_ = currentVolume_ == 0;
    }
    CLOGI("volume:%{public}d, maxVolume:%{public}d", volume, maxVolume);
    playerListener->OnVolumeChanged(volume, maxVolume);
    CLOGI("ProcessActionVolumeChanged out");
    return true;
}

bool CastStreamManagerClient::ProcessActionRepeatModeChanged(const json &data)
{
    auto playerListener = PlayerListenerGetter();
    if (!playerListener) {
        CLOGE("playerListener is nullptr");
        return false;
    }
    int mode;
    RETURN_FALSE_IF_PARSE_NUMBER_WRONG(mode, data, KEY_REPEAT_MODE);
    {
        std::lock_guard<std::mutex> lock(eventMutex_);
        currentMode_ = static_cast<LoopMode>(mode);
    }
    CLOGI("mode:%{public}d", mode);
    playerListener->OnLoopModeChanged(static_cast<LoopMode>(mode));
    CLOGI("ProcessActionRepeatModeChanged out");
    return true;
}

bool CastStreamManagerClient::ProcessActionSpeedChanged(const json &data)
{
    auto playerListener = PlayerListenerGetter();
    if (!playerListener) {
        CLOGE("playerListener is nullptr");
        return false;
    }
    int speed;
    RETURN_FALSE_IF_PARSE_NUMBER_WRONG(speed, data, KEY_SPEED);
    {
        std::lock_guard<std::mutex> lock(eventMutex_);
        currentSpeed_ = static_cast<PlaybackSpeed>(speed);
    }
    CLOGI("speed:%{public}d", speed);
    playerListener->OnPlaySpeedChanged(static_cast<PlaybackSpeed>(speed));
    CLOGI("ProcessActionSpeedChanged out");
    return true;
}

bool CastStreamManagerClient::ProcessActionPlayerError(const json &data)
{
    auto playerListener = PlayerListenerGetter();
    if (!playerListener) {
        CLOGE("playerListener is nullptr");
        return false;
    }
    int errorCode;
    std::string errorMsg;
    RETURN_FALSE_IF_PARSE_NUMBER_WRONG(errorCode, data, KEY_ERROR_CODE);
    RETURN_FALSE_IF_PARSE_STRING_WRONG(errorMsg, data, KEY_ERROR_MSG);
    CLOGI("errorCode:%{public}d errorMsg:%{public}s", errorCode, errorMsg.c_str());
    playerListener->OnPlayerError(errorCode, errorMsg);
    CLOGI("ProcessActionPlayerError out");
    return true;
}

bool CastStreamManagerClient::ProcessActionNextRequest(const json &data)
{
    auto playerListener = PlayerListenerGetter();
    if (!playerListener) {
        CLOGE("playerListener is nullptr");
        return false;
    }
    playerListener->OnNextRequest();
    CLOGI("ProcessActionNextRequest out");
    return true;
}

bool CastStreamManagerClient::ProcessActionPreviousRequest(const json &data)
{
    auto playerListener = PlayerListenerGetter();
    if (!playerListener) {
        CLOGE("playerListener is nullptr");
        return false;
    }
    playerListener->OnPreviousRequest();
    CLOGI("ProcessActionPreviousRequest out");
    return true;
}

bool CastStreamManagerClient::ProcessActionSeekDone(const json &data)
{
    auto playerListener = PlayerListenerGetter();
    if (!playerListener) {
        CLOGE("playerListener is nullptr");
        return false;
    }
    int position;
    RETURN_FALSE_IF_PARSE_NUMBER_WRONG(position, data, KEY_POSITION);
    CLOGI("position:%{public}d", position);
    playerListener->OnSeekDone(position);
    CLOGI("ProcessActionSeekDone out");
    return true;
}

bool CastStreamManagerClient::ProcessActionEndOfStream(const json &data)
{
    auto playerListener = PlayerListenerGetter();
    if (!playerListener) {
        CLOGE("playerListener is nullptr");
        return false;
    }
    int isLooping;
    RETURN_FALSE_IF_PARSE_NUMBER_WRONG(isLooping, data, KEY_IS_LOOPING);
    CLOGI("isLooping:%{public}d", isLooping);
    playerListener->OnEndOfStream(isLooping);
    CLOGI("ProcessActionEndOfStream out");
    return true;
}

bool CastStreamManagerClient::ProcessActionPlayRequest(const json &data)
{
    MediaInfoHolder mediaInfoHolder = MediaInfoHolder();
    RETURN_FALSE_IF_PARSE_NUMBER_WRONG(mediaInfoHolder.currentIndex, data, KEY_CURRENT_INDEX);
    RETURN_FALSE_IF_PARSE_NUMBER_WRONG(mediaInfoHolder.progressRefreshInterval, data, KEY_PROGRESS_INTERVAL);
    if (!data.contains(KEY_LIST)) {
        CLOGE("json object have no mediaInfo list");
        return false;
    }
    json list = data[KEY_LIST];
    for (size_t i = 0; i < list.size(); i++) {
        MediaInfo mediaInfo = MediaInfo();
        json info = list[i];
        if (!ParseMediaInfo(info, mediaInfo, false)) {
            return false;
        }
        mediaInfoHolder.mediaInfoList.push_back(mediaInfo);
    }

    auto playerListener = PlayerListenerGetter();
    if (!playerListener) {
        CLOGE("playerListener is nullptr");
        return false;
    }
    playerListener->OnPlayRequest(mediaInfoHolder.mediaInfoList.front());
    CLOGI("ProcessActionPlayRequest out");
    return true;
}

bool CastStreamManagerClient::ProcessActionKeyRequest(const json &data)
{
    auto playerListener = PlayerListenerGetter();
    if (!playerListener) {
        CLOGE("playerListener is nullptr");
        return false;
    }
    std::string mediaId;
    RETURN_FALSE_IF_PARSE_STRING_WRONG(mediaId, data, KEY_MEDIA_ID);
    std::string keyRequestDataStr;
    RETURN_FALSE_IF_PARSE_STRING_WRONG(keyRequestDataStr, data, KEY_REQUEST_KEY);
    uint32_t requestSize = static_cast<uint32_t>(keyRequestDataStr.length());
    if (requestSize == 0) {
        CLOGE("invalid buffer, requestSize = %{public}u", requestSize);
        return false;
    }
    constexpr uint32_t MAX_KEY_REQUEST_SIZE = 64 * 1024; // 64KB, sufficient for DRM key requests
    if (requestSize > MAX_KEY_REQUEST_SIZE) {
        CLOGE("key request size %{public}u exceeds max limit %{public}u", requestSize, MAX_KEY_REQUEST_SIZE);
        return false;
    }
    std::vector<uint8_t> keyRequest(requestSize);
    for (uint32_t i = 0; i < requestSize; i++) {
        keyRequest[i] = static_cast<uint8_t>(keyRequestDataStr[i]);
    }
    playerListener->OnKeyRequest(mediaId, keyRequest);
    CLOGI("ProcessActionKeyRequest out");
    return true;
}
} // namespace CastEngineService
} // namespace CastEngine
} // namespace OHOS