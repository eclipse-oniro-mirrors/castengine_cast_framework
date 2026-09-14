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
 * Description: supply stream player listener implement proxy apis.
 * Author: huangchanggui
 * Create: 2023-01-12
 */

#ifndef STREAM_PLAYER_LISTENER_IMPL_STUB_H
#define STREAM_PLAYER_LISTENER_IMPL_STUB_H

#include "i_stream_player_listener_impl.h"
#include "i_stream_player.h"
#include "cast_stub_helper.h"
#include "iremote_stub.h"
#include "pixel_map.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineClient {
class StreamPlayerListenerImplStub : public IRemoteStub<IStreamPlayerListenerImpl> {
public:
    explicit StreamPlayerListenerImplStub(std::shared_ptr<IStreamPlayerListener> userListener);
    ~StreamPlayerListenerImplStub() override;

    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

private:
    DECLARE_STUB_TASK_MAP(StreamPlayerListenerImplStub);

    int32_t DoOnStateChangedTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoOnPositionChangedTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoOnMediaItemChangedTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoOnVolumeChangedTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoOnLoopModeChangedTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoOnPlaySpeedChangedTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoOnPlayerErrorTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoOnVideoSizeChangedTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoOnNextRequestTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoOnPreviousRequestTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoOnSeekDoneTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoOnEndOfStreamTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoOnPlayRequestTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoOnImageChangedTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoOnAlbumCoverChangedTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoOnKeyRequestTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoOnAvailableCapabilityChangedTask(MessageParcel &data, MessageParcel &reply);
    int32_t DoOnMediaInfoChangedTask(MessageParcel &data, MessageParcel &reply);

    void OnStateChanged(const PlayerStates playbackState, bool isPlayWhenReady) override;
    void OnPositionChanged(int position, int bufferPosition, int duration) override;
    void OnMediaItemChanged(const MediaInfo &mediaInfo) override;
    void OnVolumeChanged(int volume, int maxVolume) override;
    void OnPlayerError(int errorCode, const std::string &errorMsg) override;
    void OnVideoSizeChanged(int width, int height) override;
    void OnLoopModeChanged(const LoopMode loopMode) override;
    void OnPlaySpeedChanged(const PlaybackSpeed speed)  override;
    void OnNextRequest() override;
    void OnPreviousRequest() override;
    void OnSeekDone(int position) override;
    void OnEndOfStream(int isLooping) override;
    void OnPlayRequest(const MediaInfo &mediaInfo) override;
    void OnImageChanged(std::shared_ptr<Media::PixelMap> pixelMap) override;
    void OnAlbumCoverChanged(std::shared_ptr<Media::PixelMap> pixelMap) override;
    void OnKeyRequest(const std::string &mediaId, const std::vector<uint8_t> &keyRequestData) override;
    void OnAvailableCapabilityChanged(const StreamCapability &streamCapability) override;
    void OnMediaInfoChanged(const MediaInfo &mediaInfo) override;

    std::shared_ptr<IStreamPlayerListener> userListener_;
};
} // namespace CastEngineClient
} // namespace CastEngine
} // namespace OHOS

#endif