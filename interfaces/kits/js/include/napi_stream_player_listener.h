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
 * Description: supply stream player listener for napi interface.
 * Author: huangchanggui
 * Create: 2023-1-11
 */

#ifndef NAPI_STREAM_PLAYER_LISTENER_H
#define NAPI_STREAM_PLAYER_LISTENER_H

#include <memory>
#include <list>
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "cast_engine_common.h"
#include "napi_callback.h"
#include "napi_castengine_utils.h"
#include "i_stream_player.h"
#include "pixel_map.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineClient {
class NapiStreamPlayerListener : public IStreamPlayerListener {
public:
    using NapiArgsGetter = std::function<void(napi_env env, int &argc, napi_value *argv)>;
    enum {
        EVENT_PLAYER_STATUS_CHANGED,
        EVENT_POSITION_CHANGED,
        EVENT_MEDIA_ITEM_CHANGED,
        EVENT_VOLUME_CHANGED,
        EVENT_VIDEO_SIZE_CHANGED,
        EVENT_LOOP_MODE_CHANGED,
        EVENT_PLAY_SPEED_CHANGED,
        EVENT_PLAYER_ERROR,
        EVENT_NEXT_REQUEST,
        EVENT_PREVIOUS_REQUEST,
        EVENT_SEEK_DONE,
        EVENT_END_OF_STREAM,
        EVENT_PLAY_REQUEST,
        EVENT_IMAGE_CHANGED,
        EVENT_ALBUM_COVER_CHANGED,
        EVENT_KEY_REQUEST,
        EVENT_AVAILABLE_CAPABILITY_CHANGED,
        EVENT_MEDIA_INFO_CHANGED,
        EVENT_TYPE_MAX
    };

    NapiStreamPlayerListener() {};
    ~NapiStreamPlayerListener() override;

    void OnStateChanged(const PlayerStates playbackState, bool isPlayWhenReady) override;
    void OnPositionChanged(int position, int bufferPosition, int duration) override;
    void OnMediaItemChanged(const MediaInfo &mediaInfo) override;
    void OnMediaInfoChanged(const MediaInfo &mediaInfo) override;
    void OnVolumeChanged(int volume, int maxVolume) override;
    void OnVideoSizeChanged(int width, int height) override;
    void OnLoopModeChanged(const LoopMode loopMode) override;
    void OnPlaySpeedChanged(const PlaybackSpeed speed) override;
    void OnPlayerError(int errorCode, const std::string &errorMsg) override;
    void OnNextRequest() override;
    void OnPreviousRequest() override;
    void OnSeekDone(int position) override;
    void OnEndOfStream(int isLooping) override;
    void OnPlayRequest(const MediaInfo &mediaInfo) override;
    void OnImageChanged(std::shared_ptr<Media::PixelMap> pixelMap) override;
    void OnAlbumCoverChanged(std::shared_ptr<Media::PixelMap> pixelMap) override;
    void OnKeyRequest(const std::string &mediaId, const std::vector<uint8_t> &keyRequestData) override;
    void OnAvailableCapabilityChanged(const StreamCapability &streamCapability) override;
    void OnData(const DataType dataType, const std::string &dataStr) override;

    napi_status AddCallback(napi_env env, int32_t event, napi_value callback);
    napi_status RemoveCallback(napi_env env, int32_t event, napi_value callback);

private:
    void HandleEvent(int32_t event, NapiArgsGetter &getter);

    std::mutex lock_;
    std::shared_ptr<NapiCallback> callback_;
};
} // namespace CastEngineClient
} // namespace CastEngine
} // namespace OHOS
#endif