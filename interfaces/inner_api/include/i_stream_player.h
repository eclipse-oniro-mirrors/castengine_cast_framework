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
 * Description: Stream Player interface.
 * Author: huangchanggui
 * Create: 2023-01-11
 */

#ifndef I_STREAM_PLAYER_H
#define I_STREAM_PLAYER_H

#include "cast_engine_common.h"
#include "pixel_map.h"

namespace OHOS {
namespace CastEngine {
class EXPORT IStreamPlayerListener {
public:
    IStreamPlayerListener() = default;
    IStreamPlayerListener(const IStreamPlayerListener &) = delete;
    IStreamPlayerListener &operator=(const IStreamPlayerListener &) = delete;
    IStreamPlayerListener(IStreamPlayerListener &&) = delete;
    IStreamPlayerListener &operator=(IStreamPlayerListener &&) = delete;
    virtual ~IStreamPlayerListener() = default;

    virtual void OnStateChanged(const PlayerStates playbackState, bool isPlayWhenReady) = 0;
    virtual void OnPositionChanged(int position, int bufferPosition, int duration) = 0;
    virtual void OnMediaItemChanged(const MediaInfo &mediaInfo) = 0;
    virtual void OnVolumeChanged(int volume, int maxVolume) = 0;
    virtual void OnLoopModeChanged(const LoopMode loopMode) = 0;
    virtual void OnPlaySpeedChanged(const PlaybackSpeed speed) = 0;
    virtual void OnPlayerError(int errorCode, const std::string &errorMsg) = 0;
    virtual void OnVideoSizeChanged(int width, int height) = 0;
    virtual void OnNextRequest() = 0;
    virtual void OnPreviousRequest() = 0;
    virtual void OnSeekDone(int position) = 0;
    virtual void OnEndOfStream(int isLooping) = 0;
    virtual void OnPlayRequest(const MediaInfo &mediaInfo) = 0;
    virtual void OnImageChanged(std::shared_ptr<Media::PixelMap> pixelMap) = 0;
    virtual void OnAlbumCoverChanged(std::shared_ptr<Media::PixelMap> pixelMap) = 0;
};

class EXPORT IStreamPlayer {
public:
    IStreamPlayer() = default;
    IStreamPlayer(const IStreamPlayer &) = delete;
    IStreamPlayer &operator=(const IStreamPlayer &) = delete;
    IStreamPlayer(IStreamPlayer &&) = delete;
    IStreamPlayer &operator=(IStreamPlayer &&) = delete;
    virtual ~IStreamPlayer() = default;

    virtual int32_t RegisterListener(std::shared_ptr<IStreamPlayerListener> listener) = 0;
    virtual int32_t UnregisterListener() = 0;
    virtual int32_t SetSurface(const std::string &surfaceId) = 0;
    virtual int32_t Load(const MediaInfo &mediaInfo) = 0;
    virtual int32_t Play(const MediaInfo &mediaInfo) = 0;
    virtual int32_t Play(int index) = 0;
    virtual int32_t Play() = 0;
    virtual int32_t Pause() = 0;
    virtual int32_t Stop() = 0;
    virtual int32_t Next() = 0;
    virtual int32_t Previous() = 0;
    virtual int32_t Seek(int position) = 0;
    virtual int32_t FastForward(int delta) = 0;
    virtual int32_t FastRewind(int delta) = 0;
    virtual int32_t SetVolume(int volume) = 0;
    virtual int32_t SetMute(bool mute) = 0;
    virtual int32_t SetLoopMode(const LoopMode mode) = 0;
    virtual int32_t SetSpeed(const PlaybackSpeed speed) = 0;
    virtual int32_t GetPlayerStatus(PlayerStates &playerStates) = 0;
    virtual int32_t GetPosition(int &position) = 0;
    virtual int32_t GetDuration(int &duration) = 0;
    virtual int32_t GetVolume(int &volume, int &maxVolume) = 0;
    virtual int32_t GetMute(bool &mute) = 0;
    virtual int32_t GetLoopMode(LoopMode &loopMode) = 0;
    virtual int32_t GetPlaySpeed(PlaybackSpeed &playbackSpeed) = 0;
    virtual int32_t GetMediaInfoHolder(MediaInfoHolder &mediaInfoHolder) = 0;
    virtual int32_t Release() = 0;
};
} // namespace CastEngine
} // namespace OHOS

#endif
