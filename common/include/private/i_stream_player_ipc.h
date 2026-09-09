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
 * Description: supply stream player implement ipc interface.
 * Author: zhangjingnan
 * Create: 2023-08-29
 */

#ifndef I_STREAM_PLAYER_IPC_H
#define I_STREAM_PLAYER_IPC_H

#include <string>
#include "surface_utils.h"
#include "cast_engine_common.h"
#include "cast_service_common.h"
#include "i_stream_player_listener_impl.h"
#include "iremote_broker.h"

namespace OHOS {
namespace CastEngine {
class IStreamPlayerIpc : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.CastEngine.IStreamPlayerIpc");

    IStreamPlayerIpc() = default;
    IStreamPlayerIpc(const IStreamPlayerIpc &) = delete;
    IStreamPlayerIpc &operator=(const IStreamPlayerIpc &) = delete;
    IStreamPlayerIpc(IStreamPlayerIpc &&) = delete;
    IStreamPlayerIpc &operator=(IStreamPlayerIpc &&) = delete;
    virtual ~IStreamPlayerIpc() override = default;

    virtual int32_t RegisterListener(sptr<IStreamPlayerListenerImpl> listener) = 0;
    virtual int32_t UnregisterListener() = 0;
    virtual int32_t SetSurface(sptr<IBufferProducer> producer) = 0;
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
    virtual int32_t SetAvailableCapability(const StreamCapability &streamCapability) = 0;
    virtual int32_t SetSpeed(const PlaybackSpeed speed) = 0;
    virtual int32_t GetPlayerStatus(PlayerStates &playerStates) = 0;
    virtual int32_t GetPosition(int &position) = 0;
    virtual int32_t GetDownloadRate(int64_t &totalAverageRate, int64_t &lastSecondRate) = 0;
    virtual int32_t GetDuration(int &duration) = 0;
    virtual int32_t GetVolume(int &volume, int &maxVolume) = 0;
    virtual int32_t GetMute(bool &mute) = 0;
    virtual int32_t GetLoopMode(LoopMode &loopMode) = 0;
    virtual int32_t GetAvailableCapability(StreamCapability &streamCapability) = 0;
    virtual int32_t GetPlaySpeed(PlaybackSpeed &playbackSpeed) = 0;
    virtual int32_t GetMediaInfoHolder(MediaInfoHolder &mediaInfoHolder) = 0;
    virtual int32_t ProvideKeyResponse(const std::string &mediaId, const std::vector<uint8_t> &response) = 0;
    virtual int32_t Release() = 0;

protected:
    enum {
        REGISTER_LISTENER = 1,
        UNREGISTER_LISTENER,
        SET_SURFACE,
        PLAY_INDEX,
        LOAD,
        START,
        PAUSE,
        PLAY,
        STOP,
        NEXT,
        PREVIOUS,
        SEEK,
        FAST_FORWARD,
        FAST_REWIND,
        SET_VOLUME,
        SET_MUTE,
        SET_LOOP_MODE,
        SET_AVAILABLE_CAPABILITY,
        SET_SPEED,
        GET_PLAYER_STATUS,
        GET_POSITION,
        GET_DOWNLOAD_RATE,
        GET_DURATION,
        GET_VOLUME,
        GET_MUTE,
        GET_LOOP_MODE,
        GET_AVAILABLE_CAPABILITY,
        GET_PLAY_SPEED,
        GET_MEDIA_INFO_HOLDER,
        PROVIDE_KEY_RESPONSE,
        RELEASE
    };
    static const size_t MAX_PLAY_LIST_SIZE = 100;
};
} // namespace CastEngine
} // namespace OHOS

#endif
