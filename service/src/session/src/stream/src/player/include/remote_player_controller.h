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

#ifndef CAST_STREAM_CONTROLLER_H
#define CAST_STREAM_CONTROLLER_H

#include <mutex>
#include "i_stream_player_impl.h"
#include "i_stream_player_listener_impl.h"
#include "i_cast_stream_manager_client.h"
#include "cast_stream_common.h"
#include "cast_local_file_channel_server.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineService {
class RemotePlayerController : public IStreamPlayerImpl {
public:
    RemotePlayerController(std::shared_ptr<ICastStreamManagerClient> callback,
        std::shared_ptr<CastLocalFileChannelServer> fileChannel)
        : callback_(callback), fileChannelServer_(fileChannel) {}
    ~RemotePlayerController() override;

    void SetSessionCallbackForRelease(const std::function<void(void)>& callback);
    int32_t RegisterListener(sptr<IStreamPlayerListenerImpl> listener) override;
    int32_t UnregisterListener() override;
    int32_t SetSurface(sptr<IBufferProducer> producer) override;
    int32_t Load(const MediaInfo &mediaInfo) override;
    int32_t Play(const MediaInfo &mediaInfo) override;
    int32_t Play(int index) override;
    int32_t Play() override;
    int32_t Pause() override;
    int32_t Stop() override;
    int32_t Next() override;
    int32_t Previous() override;
    int32_t Seek(int position) override;
    int32_t FastForward(int delta) override;
    int32_t FastRewind(int delta) override;
    int32_t SetVolume(int volume) override;
    int32_t SetMute(bool mute) override;
    int32_t SetLoopMode(const LoopMode mode) override;
    int32_t SetAvailableCapability(const StreamCapability &streamCapability) override;
    int32_t SetSpeed(const PlaybackSpeed speed) override;
    int32_t GetPlayerStatus(PlayerStates &playerStates) override;
    int32_t GetPosition(int &position) override;
    int32_t GetDuration(int &duration) override;
    int32_t GetVolume(int &volume, int &maxVolume) override;
    int32_t GetMute(bool &mute) override;
    int32_t GetLoopMode(LoopMode &loopMode) override;
    int32_t GetAvailableCapability(StreamCapability &streamCapability) override;
    int32_t GetPlaySpeed(PlaybackSpeed &playbackSpeed) override;
    int32_t GetMediaInfoHolder(MediaInfoHolder &mediaInfoHolder) override;
    int32_t ProvideKeyResponse(const std::string &mediaId, const std::vector<uint8_t> &response) override;
    int32_t UpdateMediaInfo(const MediaInfo &mediaInfo) override;
    int32_t Release() override;

protected:
    sptr<IStreamPlayerListenerImpl> PlayerListenerGetter();
    std::mutex sessionCallbackMutex_;
    std::mutex listenerMutex_;
    std::function<void(void)> sessionCallback_;
    std::weak_ptr<ICastStreamManagerClient> callback_;
    std::shared_ptr<CastLocalFileChannelServer> fileChannelServer_;
    sptr<IStreamPlayerListenerImpl> listener_;
};
} // namespace CastEngineService
} // namespace CastEngine
} // namespace OHOS
#endif // CAST_STREAM_PLAYER_H