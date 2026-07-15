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
 * Description: Cast stream manager server definition, which processes and sends server's instructions.
 * Author: zhangjingnan
 * Create: 2023-08-31
 */

#include "cast_stream_manager_server.h"
#include "cast_engine_log.h"
#include "cast_stream_player_manager.h"
#include "cast_local_file_channel_client.h"
#include "i_stream_player_ipc.h"
#include "stream_player_impl_stub.h"
#include "cast_stream_common.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineService {
DEFINE_CAST_ENGINE_LABEL("Cast-Stream-Manager-Server");

CastStreamManagerServer::CastStreamManagerServer(std::shared_ptr<ICastStreamListener> listener)
{
    CLOGD("CastStreamManagerServer in");
    streamActionProcessor_ = {
        { ACTION_LOAD, [this](const json &data) { return ProcessActionLoad(data); } },
        { ACTION_PLAY, [this](const json &data) { return ProcessActionPlay(data); } },
        { ACTION_PAUSE, [this](const json &data) { return ProcessActionPause(data); } },
        { ACTION_RESUME, [this](const json &data) { return ProcessActionResume(data); } },
        { ACTION_STOP, [this](const json &data) { return ProcessActionStop(data); } },
        { ACTION_NEXT, [this](const json &data) { return ProcessActionNext(data); } },
        { ACTION_PREVIOUS, [this](const json &data) { return ProcessActionPrevious(data); } },
        { ACTION_SEEK, [this](const json &data) { return ProcessActionSeek(data); } },
        { ACTION_FAST_FORWARD, [this](const json &data) { return ProcessActionFastForward(data); } },
        { ACTION_FAST_REWIND, [this](const json &data) { return ProcessActionFastRewind(data); } },
        { ACTION_SET_VOLUME, [this](const json &data) { return ProcessActionSetVolume(data); } },
        { ACTION_SET_REPEAT_MODE, [this](const json &data) { return ProcessActionSetRepeatMode(data); } },
        { ACTION_SET_AVAILABLE_CAPABILITY,
        [this](const json &data) { return ProcessActionSetAvailableCapability(data); } },
        { ACTION_SET_SPEED, [this](const json &data) { return ProcessActionSetSpeed(data); } }
    };
    streamListener_ = listener;
}

CastStreamManagerServer::~CastStreamManagerServer()
{
    CLOGD("~CastStreamManagerServer in");
    player_ = nullptr;
}

sptr<IStreamPlayerIpc> CastStreamManagerServer::CreateStreamPlayer(const std::function<void(void)> &releaseCallback)
{
    std::lock_guard<std::mutex> lock(dataMutex_);
    if (playerIpc_) {
        return playerIpc_;
    }
    auto fileChannel = std::make_shared<CastLocalFileChannelClient>(shared_from_this());
    auto player = std::make_shared<CastStreamPlayerManager>(shared_from_this(), fileChannel);
    if (player == nullptr) {
        CLOGE("CastStreamPlayerManager is null");
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

bool CastStreamManagerServer::NotifyPeerNextRequest()
{
    CLOGD("NotifyPeerNextRequest in");
    return SendCallbackAction(ACTION_NEXT_REQUEST);
}

bool CastStreamManagerServer::NotifyPeerPreviousRequest()
{
    CLOGD("NotifyPeerPreviousRequest in");
    return SendCallbackAction(ACTION_PREVIOUS_REQUEST);
}

bool CastStreamManagerServer::NotifyPeerSeekDone(int position)
{
    CLOGD("NotifyPeerSeekDone in");
    json body;
    body[KEY_POSITION] = position;
    return SendCallbackAction(ACTION_SEEK_DONE, body);
}

bool CastStreamManagerServer::NotifyPeerEndOfStream(int isLooping)
{
    CLOGD("NotifyPeerEndOfStream in");
    json body;
    body[KEY_IS_LOOPING] = isLooping;
    return SendCallbackAction(ACTION_END_OF_STREAM, body);
}

bool CastStreamManagerServer::NotifyPeerPlayerStatusChanged(const PlayerStates playbackState, bool isPlayWhenReady)
{
    CLOGD("NotifyPeerPlayerStatusChanged in");
    json body;
    body[KEY_PLAY_BACK_STATE] = static_cast<int>(playbackState);
    body[KEY_IS_PLAY_WHEN_READY] = isPlayWhenReady;
    return SendCallbackAction(ACTION_PLAYER_STATUS_CHANGED, body);
}

bool CastStreamManagerServer::NotifyPeerPositionChanged(int position, int bufferPosition, int duration)
{
    CLOGD("NotifyPeerPositionChanged in");
    json body;
    body[KEY_POSITION] = position;
    body[KEY_BUFFER_POSITION] = bufferPosition;
    body[KEY_DURATION] = duration;
    return SendCallbackAction(ACTION_POSITION_CHANGED, body);
}

bool CastStreamManagerServer::NotifyPeerMediaItemChanged(const MediaInfo &mediaInfo)
{
    CLOGD("NotifyPeerMediaItemChanged in");
    json body;
    EncapMediaInfo(mediaInfo, body, false);
    return SendCallbackAction(ACTION_MEDIA_ITEM_CHANGED, body);
}

bool CastStreamManagerServer::NotifyPeerVolumeChanged(int volume, int maxVolume)
{
    CLOGD("NotifyPeerVolumeChanged in");
    json body;
    body[KEY_VOLUME] = volume;
    body[KEY_MAX_VOLUME] = maxVolume;
    return SendCallbackAction(ACTION_VOLUME_CHANGED, body);
}

bool CastStreamManagerServer::NotifyPeerRepeatModeChanged(const LoopMode mode)
{
    CLOGD("NotifyPeerRepeatModeChanged in");
    json body;
    body[KEY_REPEAT_MODE] = mode;
    return SendCallbackAction(ACTION_REPEAT_MODE_CHANGED, body);
}

bool CastStreamManagerServer::NotifyPeerPlaySpeedChanged(const PlaybackSpeed speed)
{
    CLOGD("NotifyPeerPlaySpeedChanged in");
    json body;
    body[KEY_SPEED] = static_cast<int>(speed);
    return SendCallbackAction(ACTION_SPEED_CHANGED, body);
}

bool CastStreamManagerServer::NotifyPeerPlayerError(int errorCode, const std::string &errorMsg)
{
    CLOGD("NotifyPeerPlayerError in");
    json body;
    body[KEY_ERROR_CODE] = errorCode;
    body[KEY_ERROR_MSG] = errorMsg;
    return SendCallbackAction(ACTION_PLAYER_ERROR, body);
}

bool CastStreamManagerServer::NotifyPeerPlayRequest(const MediaInfo &mediaInfo)
{
    CLOGD("NotifyPeerPlayRequest in");
    json body;
    json list;
    body[KEY_CURRENT_INDEX] = 0;
    body[KEY_PROGRESS_INTERVAL] = 0;
    size_t mediaInfoListSize = 1;
    for (size_t i = 0; i < mediaInfoListSize; i++) {
        json info;
        EncapMediaInfo(mediaInfo, info, false);
        list[i] = info;
    }
    body[KEY_LIST] = list;
    CLOGD("list size:%{public}zu ", list.size());
    return SendControlAction(ACTION_PLAY_REQUEST, body);
}

bool CastStreamManagerServer::NotifyPeerCreateChannel()
{
    CLOGD("NotifyPeerCreateChannel in");
    if (!streamListener_) {
        CLOGE("streamListener_ is nullptr");
        return false;
    }
    return streamListener_->SendActionToPeers(MODULE_EVENT_ID_CHANNEL_CREATE, "");
}

void CastStreamManagerServer::OnEvent(EventId eventId, const std::string &data)
{
    CLOGD("OnEvent in");
    if (!streamListener_) {
        CLOGE("streamListener_ is nullptr");
        return;
    }
    streamListener_->OnEvent(eventId, data);
}

void CastStreamManagerServer::OnRenderReady(bool isReady)
{
    CLOGD("OnRenderReady in, isReady:%{public}d", isReady);
    if (!streamListener_) {
        CLOGE("streamListener_ is nullptr");
        return;
    }
    streamListener_->OnRenderReady(isReady);
}

bool CastStreamManagerServer::PlayAfterSwitchToStream()
{
    CLOGE("Don't support PlayAfterSwitchToStream in CastStreamManagerServer");
    return false;
}

std::shared_ptr<CastStreamPlayerManager> CastStreamManagerServer::PlayerGetter()
{
    std::lock_guard<std::mutex> lock(dataMutex_);
    return player_;
}

bool CastStreamManagerServer::ParseMediaInfoHolder(const json &data, MediaInfoHolder &mediaInfoHolder)
{
    RETURN_FALSE_IF_PARSE_NUMBER_WRONG(mediaInfoHolder.currentIndex, data, KEY_CURRENT_INDEX);
    RETURN_FALSE_IF_PARSE_NUMBER_WRONG(mediaInfoHolder.progressRefreshInterval, data, KEY_PROGRESS_INTERVAL);
    if (!data.contains(KEY_LIST)) {
        CLOGE("json object have no mediaInfo list");
        return false;
    }
    json list = data[KEY_LIST];
    if (list.empty()) {
        CLOGE("mediaInfo list is empty");
        return false;
    }
    for (size_t i = 0; i < list.size(); i++) {
        MediaInfo mediaInfo = MediaInfo();
        json info = list[i];
        if (!ParseMediaInfo(info, mediaInfo, false)) {
            return false;
        }
        mediaInfoHolder.mediaInfoList.push_back(mediaInfo);
    }
    return true;
}

bool CastStreamManagerServer::ProcessActionLoad(const json &data)
{
    CLOGI("in");
    auto player = PlayerGetter();
    if (!player) {
        CLOGE("player is nullptr");
        return false;
    }
    MediaInfoHolder mediaInfoHolder = MediaInfoHolder{};
    if (!ParseMediaInfoHolder(data, mediaInfoHolder)) {
        CLOGE("ParseMediaInfoHolder failed");
        return false;
    }
    return player->Load(mediaInfoHolder.mediaInfoList.front());
}

bool CastStreamManagerServer::ProcessActionPlay(const json &data)
{
    CLOGI("in");
    auto player = PlayerGetter();
    if (!player) {
        CLOGE("player is nullptr");
        return false;
    }
    MediaInfoHolder mediaInfoHolder = MediaInfoHolder{};
    if (!ParseMediaInfoHolder(data, mediaInfoHolder)) {
        CLOGE("ParseMediaInfoHolder failed");
        return false;
    }
    return player->InnerPlay(mediaInfoHolder.mediaInfoList.front());
}

bool CastStreamManagerServer::ProcessActionPause(const json &data)
{
    CLOGI("in");
    static_cast<void>(data);
    auto player = PlayerGetter();
    if (!player) {
        CLOGE("player is nullptr");
        return false;
    }
    return player->Pause();
}

bool CastStreamManagerServer::ProcessActionResume(const json &data)
{
    CLOGI("in");
    static_cast<void>(data);
    auto player = PlayerGetter();
    if (!player) {
        CLOGE("player is nullptr");
        return false;
    }
    return player->Play();
}

bool CastStreamManagerServer::ProcessActionStop(const json &data)
{
    CLOGI("in");
    static_cast<void>(data);
    auto player = PlayerGetter();
    if (!player) {
        CLOGE("player is nullptr");
        return false;
    }
    return player->Stop();
}

bool CastStreamManagerServer::ProcessActionNext(const json &data)
{
    CLOGI("in");
    static_cast<void>(data);
    auto player = PlayerGetter();
    if (!player) {
        CLOGE("player is nullptr");
        return false;
    }
    return player->Next();
}

bool CastStreamManagerServer::ProcessActionPrevious(const json &data)
{
    CLOGI("in");
    static_cast<void>(data);
    auto player = PlayerGetter();
    if (!player) {
        CLOGE("player is nullptr");
        return false;
    }
    return player->Previous();
}

bool CastStreamManagerServer::ProcessActionSeek(const json &data)
{
    auto player = PlayerGetter();
    if (!player) {
        CLOGE("player is nullptr");
        return false;
    }
    int position;
    RETURN_FALSE_IF_PARSE_NUMBER_WRONG(position, data, KEY_POSITION);
    CLOGI("position:%{public}d", position);
    return player->Seek(position);
}

bool CastStreamManagerServer::ProcessActionFastForward(const json &data)
{
    auto player = PlayerGetter();
    if (!player) {
        CLOGE("player is nullptr");
        return false;
    }
    int delta;
    RETURN_FALSE_IF_PARSE_NUMBER_WRONG(delta, data, KEY_DELTA);
    CLOGI("delta:%{public}d", delta);
    return player->FastForward(delta);
}

bool CastStreamManagerServer::ProcessActionFastRewind(const json &data)
{
    auto player = PlayerGetter();
    if (!player) {
        CLOGE("player is nullptr");
        return false;
    }
    int delta;
    RETURN_FALSE_IF_PARSE_NUMBER_WRONG(delta, data, KEY_DELTA);
    CLOGI("delta:%{public}d", delta);
    return player->FastRewind(delta);
}

bool CastStreamManagerServer::ProcessActionSetVolume(const json &data)
{
    auto player = PlayerGetter();
    if (!player) {
        CLOGE("player is nullptr");
        return false;
    }
    int volume;
    RETURN_FALSE_IF_PARSE_NUMBER_WRONG(volume, data, KEY_VOLUME);
    CLOGI("volume:%{public}d", volume);
    return player->SetVolume(volume);
}

bool CastStreamManagerServer::ProcessActionSetMute(const json &data)
{
    auto player = PlayerGetter();
    if (!player) {
        CLOGE("player is nullptr");
        return false;
    }
    bool mute = false;
    RETURN_FALSE_IF_PARSE_BOOL_WRONG(mute, data, KEY_MUTE);
    CLOGI("mute:%{public}d", mute);
    return player->SetMute(mute);
}

bool CastStreamManagerServer::ProcessActionSetRepeatMode(const json &data)
{
    auto player = PlayerGetter();
    if (!player) {
        CLOGE("player is nullptr");
        return false;
    }
    int mode;
    RETURN_FALSE_IF_PARSE_NUMBER_WRONG(mode, data, KEY_MODE);
    CLOGI("mode:%{public}d", mode);
    return player->SetLoopMode(static_cast<LoopMode>(mode));
}

bool CastStreamManagerServer::ProcessActionSetAvailableCapability(const json &data)
{
    auto player = PlayerGetter();
    if (!player) {
        CLOGE("player is nullptr");
        return false;
    }
    StreamCapability streamCapability{};
    if (!ParseStreamCapability(data, streamCapability)) {
        CLOGE("ParseStreamCapability failed");
        return false;
    }
    return player->InnerSetAvailableCapability(streamCapability);
}

bool CastStreamManagerServer::ProcessActionSetSpeed(const json &data)
{
    auto player = PlayerGetter();
    if (!player) {
        CLOGE("player is nullptr");
        return false;
    }
    int speed;
    RETURN_FALSE_IF_PARSE_NUMBER_WRONG(speed, data, KEY_SPEED);
    CLOGI("speed:%{public}d", speed);
    return player->SetSpeed(static_cast<PlaybackSpeed>(speed));
}
} // namespace CastEngineService
} // namespace CastEngine
} // namespace OHOS