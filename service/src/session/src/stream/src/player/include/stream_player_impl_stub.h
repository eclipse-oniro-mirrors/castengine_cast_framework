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
 * Description: supply stream player implement stub class.
 * Author: huangchanggui
 * Create: 2023-01-13
 */

#ifndef STREAM_PLAYER_IMPL_STUB_H
#define STREAM_PLAYER_IMPL_STUB_H

#include "cast_stub_helper.h"
#include "i_stream_player.h"
#include "i_stream_player_ipc.h"
#include "i_stream_player_impl.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineService {
class StreamPlayerImplStub : public IRemoteStub<IStreamPlayerIpc> {
public:
    explicit StreamPlayerImplStub(std::shared_ptr<IStreamPlayerImpl> streamPlayerImpl);

    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

private:
    DECLARE_STUB_TASK_MAP(StreamPlayerImplStub);

    int32_t DoRegisterListenerTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoUnregisterListenerTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoSetSurface(MessageParcel &data, MessageParcel &reply);
    int32_t DoLoadTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoStartTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoLocalPlayTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoPlayTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoPauseTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoStopTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoNextTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoPreviousTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoSeekTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoFastForwardTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoFastRewindTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoSetVolumeTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoSetLoopModeTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoSetAvailableCapabilityTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoSetSpeedTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoGetPlayerStatusTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoGetPositionTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoGetDurationTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoGetVolumeTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoGetMuteTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoGetPlaySpeedTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoSetMuteTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoGetLoopModeTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoGetAvailableCapabilityTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoGetMediaInfoHolderTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoProvideKeyResponseTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoReleaseTask(MessageParcel &data, MessageParcel &reply);
    int32_t RegisterListener(sptr<IStreamPlayerListenerImpl> listener) override;
    std::shared_ptr<IStreamPlayerImpl> PlayerImplGetter();
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
    int32_t GetDownloadRate(int64_t &totalAverageRate, int64_t &lastSecondRate) override;
    int32_t GetDuration(int &duration) override;
    int32_t GetVolume(int &volume, int &maxVolume) override;
    int32_t GetMute(bool &mute) override;
    int32_t GetLoopMode(LoopMode &loopMode) override;
    int32_t GetAvailableCapability(StreamCapability &streamCapability) override;
    int32_t GetPlaySpeed(PlaybackSpeed &playbackSpeed) override;
    int32_t GetMediaInfoHolder(MediaInfoHolder &mediaInfoHolder) override;
    int32_t ProvideKeyResponse(const std::string &mediaId, const std::vector<uint8_t> &response) override;
    int32_t Release() override;

    std::mutex dataMutex_;
    std::shared_ptr<IStreamPlayerImpl> streamPlayerImpl_;
};
} // namespace CastEngineService
} // namespace CastEngine
} // namespace OHOS

#endif
