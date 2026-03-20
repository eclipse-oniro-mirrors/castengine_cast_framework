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
 * Description: supply stream player implement proxy
 * Author: huangchanggui
 * Create: 2023-01-12
 */

#ifndef STREAM_PLAYER_IMPL_PROXY_H
#define STREAM_PLAYER_IMPL_PROXY_H

#include "i_stream_player_ipc.h"
#include "cast_engine_common.h"
#include "iremote_proxy.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineClient {
class StreamPlayerImplProxy : public IRemoteProxy<IStreamPlayerIpc> {
public:
    explicit StreamPlayerImplProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<IStreamPlayerIpc>(impl) {}
    ~StreamPlayerImplProxy() override;

    int32_t RegisterListener(sptr<IStreamPlayerListenerImpl> listener) override;
    int32_t UnregisterListener() override;

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
    int32_t Release() override;

    int32_t SetSurface(sptr<IBufferProducer> producer) override;
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

private:
    static const int GET_FAILED = -1;
    static constexpr size_t MAX_KEY_RESPONSE_SIZE = 2 * 1024 * 1024;
    static inline BrokerDelegator<StreamPlayerImplProxy> delegator_;
};
} // namespace CastEngineClient
} // namespace CastEngine
} // namespace OHOS

#endif // CAST_ENGINE_PROXY_H