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
 * Description: Stream player listener implement interface.
 * Author: huangchanggui
 * Create: 2023-01-12
 */

#ifndef I_STREAM_PLAYER_LISTENER_IMPL_H
#define I_STREAM_PLAYER_LISTENER_IMPL_H

#include "cast_engine_common.h"
#include "iremote_broker.h"
#include "pixel_map.h"

namespace OHOS {
namespace CastEngine {
class IStreamPlayerListenerImpl : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.CastEngine.IStreamPlayerListenerImpl");
    IStreamPlayerListenerImpl() = default;
    IStreamPlayerListenerImpl(const IStreamPlayerListenerImpl &) = delete;
    IStreamPlayerListenerImpl &operator=(const IStreamPlayerListenerImpl &) = delete;
    IStreamPlayerListenerImpl(IStreamPlayerListenerImpl &&) = delete;
    IStreamPlayerListenerImpl &operator=(IStreamPlayerListenerImpl &&) = delete;
    virtual ~IStreamPlayerListenerImpl() override = default;

    virtual void OnStateChanged(const PlayerStates playbackState, bool isPlayWhenReady) = 0;
    virtual void OnPositionChanged(int position, int bufferPosition, int duration) = 0;
    virtual void OnMediaItemChanged(const MediaInfo &mediaInfo) = 0;
    virtual void OnVolumeChanged(int volume, int maxVolume) = 0;
    virtual void OnPlayerError(int errorCode, const std::string &errorMsg) = 0;
    virtual void OnVideoSizeChanged(int width, int height) = 0;
    virtual void OnLoopModeChanged(const LoopMode loopMode) = 0;
    virtual void OnPlaySpeedChanged(const PlaybackSpeed speed) = 0;
    virtual void OnNextRequest() = 0;
    virtual void OnPreviousRequest() = 0;
    virtual void OnSeekDone(int position) = 0;
    virtual void OnEndOfStream(int isLooping) = 0;
    virtual void OnPlayRequest(const MediaInfo &mediaInfo) = 0;
    virtual void OnImageChanged(std::shared_ptr<Media::PixelMap> pixelMap) = 0;
    virtual void OnAlbumCoverChanged(std::shared_ptr<Media::PixelMap> pixelMap) = 0;
    virtual void OnKeyRequest(const std::string &mediaId, const std::vector<uint8_t> &keyRequestData) = 0;
    virtual void OnAvailableCapabilityChanged(const StreamCapability &streamCapability) = 0;
    virtual void OnMediaInfoChanged(const MediaInfo &mediaInfo) = 0;

protected:
    enum {
        ON_PLAYER_STATUS_CHANGED = 1,
        ON_POSITION_CHANGED,
        ON_MEDIA_ITEM_CHANGED,
        ON_VOLUME_CHANGED,
        ON_REPEAT_MODE_CHANGED,
        ON_PLAY_SPEED_CHANGED,
        ON_PLAYER_ERROR,
        ON_VIDEO_SIZE_CHANGED,
        ON_NEXT_REQUEST,
        ON_PREVIOUS_REQUEST,
        ON_SEEK_DONE,
        ON_END_OF_STREAM,
        ON_PLAY_REQUEST,
        ON_IMAGE_CHANGED,
        ON_ALBUM_COVER_CHANGED,
        ON_KEY_REQUEST,
        ON_AVAILABLE_CAPABILITY_CHANGED,
        ON_MEDIA_INFO_CHANGED,
    };
};
} // namespace CastEngine
} // namespace OHOS

#endif