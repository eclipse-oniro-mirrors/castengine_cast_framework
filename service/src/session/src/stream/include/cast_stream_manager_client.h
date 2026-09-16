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
 * Description: Cast stream manager client class.
 * Author: zhangjingnan
 * Create: 2023-08-30
 */

#ifndef CAST_STREAM_MANAGER_CLIENT_H
#define CAST_STREAM_MANAGER_CLIENT_H


#include <mutex>
#include "json.hpp"
#include "cast_stream_common.h"
#include "i_cast_stream_manager.h"
#include "i_cast_stream_manager_client.h"
#include "remote_player_controller.h"
#include "cast_timer.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineService {
using nlohmann::json;

class CastStreamManagerClient : public ICastStreamManager,
    public ICastStreamManagerClient,
    public std::enable_shared_from_this<CastStreamManagerClient> {
public:
    explicit CastStreamManagerClient(std::shared_ptr<ICastStreamListener> listener, bool isDoubleFrame);
    ~CastStreamManagerClient() override;

    sptr<IStreamPlayerIpc> CreateStreamPlayer(const std::function<void(void)> &releaseCallback) override;

    bool RegisterListener(sptr<IStreamPlayerListenerImpl> listener) override;
    bool UnregisterListener() override;
    void OnEvent(EventId eventId, const std::string &data) override;
    bool NotifyPeerLoad(const MediaInfo &mediaInfo) override;
    bool NotifyPeerPlay(const MediaInfo &mediaInfo) override;
    bool NotifyPeerPause() override;
    bool NotifyPeerResume() override;
    bool NotifyPeerStop() override;
    bool NotifyPeerNext() override;
    bool NotifyPeerPrevious() override;
    bool NotifyPeerSeek(int position) override;
    bool NotifyPeerFastForward(int delta) override;
    bool NotifyPeerFastRewind(int delta) override;
    bool NotifyPeerSetVolume(int volume) override;
    bool NotifyPeerSetMute(bool mute) override;
    bool NotifyPeerSetRepeatMode(int mode) override;
    bool NotifyPeerSetAvailableCapability(const StreamCapability &streamCapability) override;
    bool NotifyPeerSetSpeed(int speed) override;
    bool NotifyPeerKeyResponse(const std::string &mediaId, const std::vector<uint8_t> &response) override;
    bool NotifyPeerMediaInfoChanged(const MediaInfo &mediaInfo) override;
    PlayerStates GetPlayerStatus() override;
    int GetPosition() override;
    int GetDuration() override;
    int GetVolume() override;
    int GetMaxVolume() override;
    bool GetMute() override;
    LoopMode GetLoopMode() override;
    StreamCapability GetAvailableCapability() override;
    PlaybackSpeed GetPlaySpeed() override;
    bool IsDoubleFrame() override;
    bool TransferToStreamMode(const MediaInfo &mediaInfo) override;
    bool DisconnectSession(std::string deviceId) override;
    bool PlayAfterSwitchToStream() override;

private:
    bool ProcessActionPlayerStatusChanged(const json &data);
    bool ProcessActionPositionChanged(const json &data);
    bool ProcessActionMediaItemChanged(const json &data);
    bool ProcessActionVolumeChanged(const json &data);
    bool ProcessActionRepeatModeChanged(const json &data);
    bool ProcessActionSpeedChanged(const json &data);
    bool ProcessActionPlayerError(const json &data);
    bool ProcessActionNextRequest(const json &data);
    bool ProcessActionPreviousRequest(const json &data);
    bool ProcessActionSeekDone(const json &data);
    bool ProcessActionEndOfStream(const json &data);
    bool ProcessActionPlayRequest(const json &data);
    bool ProcessActionKeyRequest(const json &data);

    sptr<IStreamPlayerListenerImpl> PlayerListenerGetter();
    bool AutoUpdateCurPosition();
    PlayerStates ProcessHmosPlayerStatus(HmosPlayerStates hmosPlaybackState, bool isPlayWhenReady);
    void ProcessHmosPlayerPosition(int position);

    std::map<HmosPlayerStates, PlayerStates> hmosStreamStateConvertor_{
        {HmosPlayerStates::STATE_IDLE, PlayerStates::PLAYER_IDLE},
        {HmosPlayerStates::STATE_BUFFERING, PlayerStates::PLAYER_BUFFERING},
        {HmosPlayerStates::STATE_READY, PlayerStates::PLAYER_PREPARED},
        {HmosPlayerStates::STATE_ENDED, PlayerStates::PLAYER_PLAYBACK_COMPLETE},
    };

    std::map<PlaybackSpeed, float> doubleSupportSpeed_ {
        { PlaybackSpeed::SPEED_FORWARD_0_50_X, 0.5 },
        { PlaybackSpeed::SPEED_FORWARD_0_75_X, 0.75 },
        { PlaybackSpeed::SPEED_FORWARD_1_00_X, 1.0 },
        { PlaybackSpeed::SPEED_FORWARD_1_25_X, 1.25 },
        { PlaybackSpeed::SPEED_FORWARD_1_50_X, 1.5 },
        { PlaybackSpeed::SPEED_FORWARD_1_75_X, 1.75 },
        { PlaybackSpeed::SPEED_FORWARD_2_00_X, 2.0 },
    };

    std::shared_ptr<RemotePlayerController> player_;
    std::mutex eventMutex_;
    std::shared_ptr<CastTimer> timer_;
    PlayerStates currentState_ = PlayerStates::PLAYER_IDLE;
    int currentPosition_{ CAST_STREAM_INT_INVALID };
    int currentDuration_{ CAST_STREAM_INT_INVALID };
    int currentBuffer_{ CAST_STREAM_INT_INVALID };
    LoopMode currentMode_ = LoopMode::LOOP_MODE_LIST;
    PlaybackSpeed currentSpeed_ = PlaybackSpeed::SPEED_FORWARD_1_00_X;
    int seekPosition_ = CAST_STREAM_INT_INIT;
    int startPosition_ = CAST_STREAM_INT_INIT;
    std::atomic<bool> isSeeking_{ false };
    std::atomic<bool> isNewResourceLoaded_{ false };
    std::atomic<bool> isDoubleFrame_{ false };
    MediaInfo mediaInfo_{};
    StreamCapability availableCapability_{};
};
} // namespace CastEngineService
} // namespace CastEngine
} // namespace OHOS
#endif // CAST_STREAM_PLAYER_H