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
 * Description: supply napi interface for stream player.
 * Author: huangchanggui
 * Create: 2023-1-11
 */

#ifndef NAPI_STREAM_PLAYER_H
#define NAPI_STREAM_PLAYER_H

#include <list>
#include <map>
#include <memory>
#include <mutex>

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "cast_engine_log.h"
#include "cast_engine_common.h"
#include "i_stream_player.h"
#include "napi_stream_player_listener.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineClient {
class EXPORT NapiStreamPlayer {
public:
    using OnEventHandlerType = std::function<napi_status(napi_env, napi_value, NapiStreamPlayer *)>;
    using OffEventHandlerType = std::function<napi_status(napi_env, napi_value, NapiStreamPlayer *)>;

    static void DefineStreamPlayerJSClass(napi_env env);
    static napi_status CreateNapiStreamPlayer(napi_env env, std::shared_ptr<IStreamPlayer> player, napi_value &out);
    std::shared_ptr<NapiStreamPlayerListener> NapiListenerGetter();
    void NapiListenerSetter(std::shared_ptr<NapiStreamPlayerListener> listener);

    explicit NapiStreamPlayer(std::shared_ptr<IStreamPlayer> player) : streamPlayer_(player) {}
    NapiStreamPlayer() = default;
    ~NapiStreamPlayer() = default;
    std::shared_ptr<IStreamPlayer> GetStreamPlayer()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return streamPlayer_;
    }
    void Reset()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        streamPlayer_.reset();
        listener_.reset();
    }

private:
    static napi_value NapiStreamPlayerConstructor(napi_env env, napi_callback_info info);
    static napi_value SetSurface(napi_env env, napi_callback_info info);
    static napi_value Load(napi_env env, napi_callback_info info);
    static napi_value Start(napi_env env, napi_callback_info info);
    static napi_value Play(napi_env env, napi_callback_info info);
    static napi_value Pause(napi_env env, napi_callback_info info);
    static napi_value Stop(napi_env env, napi_callback_info info);
    static napi_value Next(napi_env env, napi_callback_info info);
    static napi_value Previous(napi_env env, napi_callback_info info);
    static napi_value Seek(napi_env env, napi_callback_info info);
    static napi_value FastForward(napi_env env, napi_callback_info info);
    static napi_value FastRewind(napi_env env, napi_callback_info info);
    static napi_value SetVolume(napi_env env, napi_callback_info info);
    static napi_value SetMute(napi_env env, napi_callback_info info);
    static napi_value SetLoopMode(napi_env env, napi_callback_info info);
    static napi_value SetSpeed(napi_env env, napi_callback_info info);
    static napi_value GetPlayerStatus(napi_env env, napi_callback_info info);
    static napi_value GetPosition(napi_env env, napi_callback_info info);
    static napi_value GetDownloadRate(napi_env env, napi_callback_info info);
    static napi_value GetVolume(napi_env env, napi_callback_info info);
    static napi_value GetMute(napi_env env, napi_callback_info info);
    static napi_value GetLoopMode(napi_env env, napi_callback_info info);
    static napi_value GetPlaySpeed(napi_env env, napi_callback_info info);
    static napi_value GetMediaInfoHolder(napi_env env, napi_callback_info info);
    static napi_value Release(napi_env env, napi_callback_info info);

    static napi_value OnEvent(napi_env env, napi_callback_info info);
    static napi_value OffEvent(napi_env env, napi_callback_info info);

    static napi_status OnStateChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer);
    static napi_status OnPositionChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer);
    static napi_status OnMediaItemChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer);
    static napi_status OnVolumeChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer);
    static napi_status OnVideoSizeChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer);
    static napi_status OnLoopModeChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer);
    static napi_status OnPlaySpeedChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer);
    static napi_status OnPlayerError(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer);
    static napi_status OnNextRequest(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer);
    static napi_status OnPreviousRequest(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer);
    static napi_status OnSeekDone(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer);
    static napi_status OnEndOfStream(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer);
    static napi_status OnImageChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer);

    static napi_status OffStateChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer);
    static napi_status OffPositionChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer);
    static napi_status OffMediaItemChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer);
    static napi_status OffVolumeChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer);
    static napi_status OffVideoSizeChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer);
    static napi_status OffLoopModeChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer);
    static napi_status OffPlaySpeedChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer);
    static napi_status OffPlayerError(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer);
    static napi_status OffNextRequest(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer);
    static napi_status OffPreviousRequest(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer);
    static napi_status OffSeekDone(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer);
    static napi_status OffEndOfStream(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer);
    static napi_status OffImageChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer);

    static NapiStreamPlayer *GetNapiStreamPlayer(napi_env env, napi_callback_info info);
    static napi_status RegisterNativeStreamPlayerListener(NapiStreamPlayer *napiStreamPlayer);

    static std::map<std::string, std::pair<OnEventHandlerType, OffEventHandlerType>> eventHandlers_;
    std::mutex mutex_;
    std::shared_ptr<IStreamPlayer> streamPlayer_;
    std::shared_ptr<NapiStreamPlayerListener> listener_;
    static thread_local napi_ref consRef_;
};
} // namespace CastEngineClient
} // namespace CastEngine
} // namespace OHOS
#endif