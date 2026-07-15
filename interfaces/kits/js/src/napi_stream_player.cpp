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
 * Description: supply napi interface realization for stream player.
 * Author: huangchanggui
 * Create: 2023-1-11
 */

#include "napi_stream_player.h"
#include <memory>
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "cast_engine_log.h"
#include "cast_engine_common.h"
#include "napi_castengine_utils.h"
#include "napi_async_work.h"
#include "napi_errors.h"
#include "cast_engine_errors.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineClient {
DEFINE_CAST_ENGINE_LABEL("Cast-Napi-StreamPlayer");

thread_local napi_ref NapiStreamPlayer::consRef_ = nullptr;

std::map<std::string, std::pair<NapiStreamPlayer::OnEventHandlerType,
    NapiStreamPlayer::OffEventHandlerType>>
    NapiStreamPlayer::eventHandlers_ = {
    { "stateChanged", { OnStateChanged, OffStateChanged } },
    { "positionChanged", { OnPositionChanged, OffPositionChanged } },
    { "mediaItemChanged", { OnMediaItemChanged, OffMediaItemChanged } },
    { "volumeChanged", { OnVolumeChanged, OffVolumeChanged } },
    { "videoSizeChanged", { OnVideoSizeChanged, OffVideoSizeChanged } },
    { "loopModeChanged", { OnLoopModeChanged, OffLoopModeChanged } },
    { "playSpeedChanged", { OnPlaySpeedChanged, OffPlaySpeedChanged } },
    { "playerError", { OnPlayerError, OffPlayerError } },
    { "nextRequest", { OnNextRequest, OffNextRequest } },
    { "previousRequest", { OnPreviousRequest, OffPreviousRequest } },
    { "seekDone", { OnSeekDone, OffSeekDone } },
    { "endOfStream", { OnEndOfStream, OffEndOfStream } },
    { "imageChanged", { OnImageChanged, OffImageChanged } }
};

void NapiStreamPlayer::DefineStreamPlayerJSClass(napi_env env)
{
    napi_property_descriptor playerDesc[] = {
        DECLARE_NAPI_FUNCTION("on", OnEvent),
        DECLARE_NAPI_FUNCTION("off", OffEvent),
        DECLARE_NAPI_FUNCTION("setSurface", SetSurface),
        DECLARE_NAPI_FUNCTION("load", Load),
        DECLARE_NAPI_FUNCTION("start", Start),
        DECLARE_NAPI_FUNCTION("play", Play),
        DECLARE_NAPI_FUNCTION("pause", Pause),
        DECLARE_NAPI_FUNCTION("stop", Stop),
        DECLARE_NAPI_FUNCTION("next", Next),
        DECLARE_NAPI_FUNCTION("previous", Previous),
        DECLARE_NAPI_FUNCTION("seek", Seek),
        DECLARE_NAPI_FUNCTION("fastForward", FastForward),
        DECLARE_NAPI_FUNCTION("fastRewind", FastRewind),
        DECLARE_NAPI_FUNCTION("setVolume", SetVolume),
        DECLARE_NAPI_FUNCTION("setLoopMode", SetLoopMode),
        DECLARE_NAPI_FUNCTION("setSpeed", SetSpeed),
        DECLARE_NAPI_FUNCTION("setMute", SetMute),
        DECLARE_NAPI_FUNCTION("getPlayerStatus", GetPlayerStatus),
        DECLARE_NAPI_FUNCTION("getPosition", GetPosition),
        DECLARE_NAPI_FUNCTION("getVolume", GetVolume),
        DECLARE_NAPI_FUNCTION("getMute", GetMute),
        DECLARE_NAPI_FUNCTION("getLoopMode", GetLoopMode),
        DECLARE_NAPI_FUNCTION("getPlaySpeed", GetPlaySpeed),
        DECLARE_NAPI_FUNCTION("getMediaInfoHolder", GetMediaInfoHolder),
        DECLARE_NAPI_FUNCTION("release", Release)
    };

    napi_value constructor = nullptr;
    constexpr int initialRefCount = 1;
    napi_status status = napi_define_class(env, "streamPlayer", NAPI_AUTO_LENGTH, NapiStreamPlayerConstructor, nullptr,
        sizeof(playerDesc) / sizeof(playerDesc[0]), playerDesc, &constructor);
    if (status != napi_ok) {
        CLOGE("napi_define_class failed");
        return;
    }
    status = napi_create_reference(env, constructor, initialRefCount, &consRef_);
    if (status != napi_ok) {
        CLOGE("DefineStreamPlayerJSClass napi_create_reference failed");
    }
}

napi_value NapiStreamPlayer::NapiStreamPlayerConstructor(napi_env env, napi_callback_info info)
{
    CLOGD("NapiStreamPlayer construct in");
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr));
    CLOGD("NapiStreamPlayer construct successfully");
    return thisVar;
}

napi_status NapiStreamPlayer::CreateNapiStreamPlayer(napi_env env, std::shared_ptr<IStreamPlayer> player,
    napi_value &out)
{
    CLOGD("create napiStreamPlayer in");
    napi_value result = nullptr;
    napi_value constructor = nullptr;
    if (consRef_ == nullptr || player == nullptr) {
        CLOGE("napiStreamPlayer input is null");
        return napi_generic_failure;
    }
    napi_status status = napi_get_reference_value(env, consRef_, &constructor);
    if (status != napi_ok || constructor == nullptr) {
        CLOGE("CreateNapiStreamPlayer napi_get_reference_value failed");
        return napi_generic_failure;
    }

    constexpr size_t argc = 0;
    status = napi_new_instance(env, constructor, argc, nullptr, &result);
    if (status != napi_ok) {
        CLOGE("CreateNapiStreamPlayer napi_new_instance failed");
        return napi_generic_failure;
    }

    NapiStreamPlayer *napiStreamPlayer = new (std::nothrow) NapiStreamPlayer(player);
    if (napiStreamPlayer == nullptr) {
        CLOGE("napiStreamPlayer is nullptr");
        return napi_generic_failure;
    }
    auto finalize = [](napi_env env, void *data, void *hint) {
        NapiStreamPlayer *napiStreamPlayer = reinterpret_cast<NapiStreamPlayer *>(data);
        if (napiStreamPlayer != nullptr) {
            CLOGI("napiStreamPlayer deconstructed");
            delete napiStreamPlayer;
            napiStreamPlayer = nullptr;
        }
    };
    if (napi_wrap(env, result, napiStreamPlayer, finalize, nullptr, nullptr) != napi_ok) {
        CLOGE("CreateNapiStreamPlayer napi_wrap failed");
        delete napiStreamPlayer;
        napiStreamPlayer = nullptr;
        return napi_generic_failure;
    }
    out = result;
    CLOGD("Create napiStreamPlayer successfully");
    return napi_ok;
}

NapiStreamPlayer *NapiStreamPlayer::GetNapiStreamPlayer(napi_env env, napi_callback_info info)
{
    napi_value thisVar = nullptr;
    NAPI_CALL_BASE(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr), nullptr);

    NapiStreamPlayer *napiStreamPlayer = nullptr;
    NAPI_CALL_BASE(env, napi_unwrap(env, thisVar, reinterpret_cast<void **>(&napiStreamPlayer)), nullptr);
    if (napiStreamPlayer == nullptr) {
        CLOGE("napi_unwrap napiStreamPlayer is null");
        return nullptr;
    }

    return napiStreamPlayer;
}

napi_value NapiStreamPlayer::OnEvent(napi_env env, napi_callback_info info)
{
    constexpr size_t expectedArgc = 2;
    napi_value argv[expectedArgc] = { 0 };
    napi_valuetype expectedTypes[expectedArgc] = { napi_string, napi_function };
    if (!GetJSFuncParams(env, info, argv, expectedArgc, expectedTypes)) {
        return GetUndefinedValue(env);
    }

    std::string eventName = ParseString(env, argv[0]);
    NapiStreamPlayer *napiStreamPlayer = GetNapiStreamPlayer(env, info);
    if (napiStreamPlayer == nullptr) {
        CLOGE("IStreamPlayer is null");
        return GetUndefinedValue(env);
    }
    auto it = eventHandlers_.find(eventName);
    if (it == eventHandlers_.end()) {
        CLOGE("event name invalid");
        return GetUndefinedValue(env);
    }

    if (RegisterNativeStreamPlayerListener(napiStreamPlayer) == napi_generic_failure) {
        return GetUndefinedValue(env);
    }
    if (it->second.first(env, argv[1], napiStreamPlayer) != napi_ok) {
        CLOGE("event name invalid");
    }

    return GetUndefinedValue(env);
}

napi_value NapiStreamPlayer::OffEvent(napi_env env, napi_callback_info info)
{
    constexpr size_t expectedArgc = 2;
    napi_value argv[expectedArgc] = { 0 };
    napi_valuetype expectedTypes[expectedArgc] = { napi_string, napi_function};
    if (!GetJSFuncParams(env, info, argv, expectedArgc, expectedTypes)) {
        return GetUndefinedValue(env);
    }

    std::string eventName = ParseString(env, argv[0]);
    auto it = eventHandlers_.find(eventName);
    if (it == eventHandlers_.end()) {
        CLOGE("event name invalid");
        return GetUndefinedValue(env);
    }
    NapiStreamPlayer *napiStreamPlayer = GetNapiStreamPlayer(env, info);
    if (napiStreamPlayer == nullptr) {
        CLOGE("IStreamPlayer is null");
        return GetUndefinedValue(env);
    }
    if (it->second.second(env, argv[1], napiStreamPlayer) != napi_ok) {
        CLOGE("event name invalid");
    }

    return GetUndefinedValue(env);
}

napi_status NapiStreamPlayer::OnStateChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer)
{
    if (napiStreamPlayer == nullptr) {
        CLOGE("napiStreamPlayer is null");
        return napi_generic_failure;
    }
    auto napiListener = napiStreamPlayer->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("napi stream player callback is null");
        return napi_generic_failure;
    }
    if (napiListener->AddCallback(env, NapiStreamPlayerListener::EVENT_PLAYER_STATUS_CHANGED,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiStreamPlayer::OnPositionChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer)
{
    if (napiStreamPlayer == nullptr) {
        CLOGE("napiStreamPlayer is null");
        return napi_generic_failure;
    }
    auto napiListener = napiStreamPlayer->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("napi stream player callback is null");
        return napi_generic_failure;
    }
    if (napiListener->AddCallback(env, NapiStreamPlayerListener::EVENT_POSITION_CHANGED,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiStreamPlayer::OnMediaItemChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer)
{
    if (napiStreamPlayer == nullptr) {
        CLOGE("napiStreamPlayer is null");
        return napi_generic_failure;
    }
    auto napiListener = napiStreamPlayer->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("napi stream player callback is null");
        return napi_generic_failure;
    }
    if (napiListener->AddCallback(env, NapiStreamPlayerListener::EVENT_MEDIA_ITEM_CHANGED,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiStreamPlayer::OnVolumeChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer)
{
    if (napiStreamPlayer == nullptr) {
        CLOGE("napiStreamPlayer is null");
        return napi_generic_failure;
    }
    auto napiListener = napiStreamPlayer->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("napi stream player callback is null");
        return napi_generic_failure;
    }
    if (napiListener->AddCallback(env, NapiStreamPlayerListener::EVENT_VOLUME_CHANGED,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiStreamPlayer::OnVideoSizeChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer)
{
    if (napiStreamPlayer == nullptr) {
        CLOGE("napiStreamPlayer is null");
        return napi_generic_failure;
    }
    auto napiListener = napiStreamPlayer->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("napi stream player callback is null");
        return napi_generic_failure;
    }
    if (napiListener->AddCallback(env, NapiStreamPlayerListener::EVENT_VIDEO_SIZE_CHANGED,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiStreamPlayer::OnLoopModeChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer)
{
    if (napiStreamPlayer == nullptr) {
        CLOGE("napiStreamPlayer is null");
        return napi_generic_failure;
    }
    auto napiListener = napiStreamPlayer->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("napi stream player callback is null");
        return napi_generic_failure;
    }
    if (napiListener->AddCallback(env, NapiStreamPlayerListener::EVENT_LOOP_MODE_CHANGED,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiStreamPlayer::OnPlaySpeedChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer)
{
    if (napiStreamPlayer == nullptr) {
        CLOGE("napiStreamPlayer is null");
        return napi_generic_failure;
    }
    auto napiListener = napiStreamPlayer->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("napi stream player callback is null");
        return napi_generic_failure;
    }
    if (napiListener->AddCallback(env, NapiStreamPlayerListener::EVENT_PLAY_SPEED_CHANGED,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiStreamPlayer::OnPlayerError(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer)
{
    if (napiStreamPlayer == nullptr) {
        CLOGE("napiStreamPlayer is null");
        return napi_generic_failure;
    }
    auto napiListener = napiStreamPlayer->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("napi stream player callback is null");
        return napi_generic_failure;
    }
    if (napiListener->AddCallback(env, NapiStreamPlayerListener::EVENT_PLAYER_ERROR,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiStreamPlayer::OnNextRequest(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer)
{
    if (napiStreamPlayer == nullptr) {
        CLOGE("napiStreamPlayer is null");
        return napi_generic_failure;
    }
    auto napiListener = napiStreamPlayer->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("napi stream player callback is null");
        return napi_generic_failure;
    }
    if (napiListener->AddCallback(env, NapiStreamPlayerListener::EVENT_NEXT_REQUEST,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiStreamPlayer::OnPreviousRequest(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer)
{
    if (napiStreamPlayer == nullptr) {
        CLOGE("napiStreamPlayer is null");
        return napi_generic_failure;
    }
    auto napiListener = napiStreamPlayer->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("napi stream player callback is null");
        return napi_generic_failure;
    }
    if (napiListener->AddCallback(env, NapiStreamPlayerListener::EVENT_PREVIOUS_REQUEST,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiStreamPlayer::OnSeekDone(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer)
{
    if (napiStreamPlayer == nullptr) {
        CLOGE("napiStreamPlayer is null");
        return napi_generic_failure;
    }
    auto napiListener = napiStreamPlayer->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("napi stream player callback is null");
        return napi_generic_failure;
    }
    if (napiListener->AddCallback(env, NapiStreamPlayerListener::EVENT_SEEK_DONE,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiStreamPlayer::OnEndOfStream(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer)
{
    if (napiStreamPlayer == nullptr) {
        CLOGE("napiStreamPlayer is null");
        return napi_generic_failure;
    }
    auto napiListener = napiStreamPlayer->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("napi stream player callback is null");
        return napi_generic_failure;
    }
    if (napiListener->AddCallback(env, NapiStreamPlayerListener::EVENT_END_OF_STREAM,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiStreamPlayer::OnImageChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer)
{
    if (napiStreamPlayer == nullptr) {
        CLOGE("napiStreamPlayer is null");
        return napi_generic_failure;
    }
    auto napiListener = napiStreamPlayer->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("napi stream player callback is null");
        return napi_generic_failure;
    }
    if (napiListener->AddCallback(env, NapiStreamPlayerListener::EVENT_IMAGE_CHANGED,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiStreamPlayer::OffStateChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer)
{
    if (napiStreamPlayer == nullptr) {
        CLOGE("napiStreamPlayer is null");
        return napi_generic_failure;
    }
    auto napiListener = napiStreamPlayer->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("napi stream player callback is null");
        return napi_generic_failure;
    }
    if (napiListener->RemoveCallback(env,
        NapiStreamPlayerListener::EVENT_PLAYER_STATUS_CHANGED, callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiStreamPlayer::OffPositionChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer)
{
    if (napiStreamPlayer == nullptr) {
        CLOGE("napiStreamPlayer is null");
        return napi_generic_failure;
    }
    auto napiListener = napiStreamPlayer->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("napi stream player callback is null");
        return napi_generic_failure;
    }
    if (napiListener->RemoveCallback(env, NapiStreamPlayerListener::EVENT_POSITION_CHANGED,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiStreamPlayer::OffMediaItemChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer)
{
    if (napiStreamPlayer == nullptr) {
        CLOGE("napiStreamPlayer is null");
        return napi_generic_failure;
    }
    auto napiListener = napiStreamPlayer->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("napi stream player callback is null");
        return napi_generic_failure;
    }
    if (napiListener->RemoveCallback(env, NapiStreamPlayerListener::EVENT_MEDIA_ITEM_CHANGED,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiStreamPlayer::OffVolumeChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer)
{
    if (napiStreamPlayer == nullptr) {
        CLOGE("napiStreamPlayer is null");
        return napi_generic_failure;
    }
    auto napiListener = napiStreamPlayer->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("napi stream player callback is null");
        return napi_generic_failure;
    }
    if (napiListener->RemoveCallback(env, NapiStreamPlayerListener::EVENT_VOLUME_CHANGED,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiStreamPlayer::OffVideoSizeChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer)
{
    if (napiStreamPlayer == nullptr) {
        CLOGE("napiStreamPlayer is null");
        return napi_generic_failure;
    }
    auto napiListener = napiStreamPlayer->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("napi stream player callback is null");
        return napi_generic_failure;
    }
    if (napiListener->RemoveCallback(env, NapiStreamPlayerListener::EVENT_VIDEO_SIZE_CHANGED,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiStreamPlayer::OffLoopModeChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer)
{
    if (napiStreamPlayer == nullptr) {
        CLOGE("napiStreamPlayer is null");
        return napi_generic_failure;
    }
    auto napiListener = napiStreamPlayer->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("napi stream player callback is null");
        return napi_generic_failure;
    }
    if (napiListener->RemoveCallback(env, NapiStreamPlayerListener::EVENT_LOOP_MODE_CHANGED,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiStreamPlayer::OffPlaySpeedChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer)
{
    if (napiStreamPlayer == nullptr) {
        CLOGE("napiStreamPlayer is null");
        return napi_generic_failure;
    }
    auto napiListener = napiStreamPlayer->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("napi stream player callback is null");
        return napi_generic_failure;
    }
    if (napiListener->RemoveCallback(env, NapiStreamPlayerListener::EVENT_PLAY_SPEED_CHANGED,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiStreamPlayer::OffPlayerError(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer)
{
    if (napiStreamPlayer == nullptr) {
        CLOGE("napiStreamPlayer is null");
        return napi_generic_failure;
    }
    auto napiListener = napiStreamPlayer->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("napi stream player callback is null");
        return napi_generic_failure;
    }
    if (napiListener->RemoveCallback(env, NapiStreamPlayerListener::EVENT_PLAYER_ERROR,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiStreamPlayer::OffNextRequest(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer)
{
    if (napiStreamPlayer == nullptr) {
        CLOGE("napiStreamPlayer is null");
        return napi_generic_failure;
    }
    auto napiListener = napiStreamPlayer->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("napi stream player callback is null");
        return napi_generic_failure;
    }
    if (napiListener->RemoveCallback(env, NapiStreamPlayerListener::EVENT_NEXT_REQUEST,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiStreamPlayer::OffPreviousRequest(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer)
{
    if (napiStreamPlayer == nullptr) {
        CLOGE("napiStreamPlayer is null");
        return napi_generic_failure;
    }
    auto napiListener = napiStreamPlayer->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("napi stream player callback is null");
        return napi_generic_failure;
    }
    if (napiListener->RemoveCallback(env, NapiStreamPlayerListener::EVENT_PREVIOUS_REQUEST,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiStreamPlayer::OffSeekDone(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer)
{
    if (napiStreamPlayer == nullptr) {
        CLOGE("napiStreamPlayer is null");
        return napi_generic_failure;
    }
    auto napiListener = napiStreamPlayer->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("napi stream player callback is null");
        return napi_generic_failure;
    }
    if (napiListener->RemoveCallback(env, NapiStreamPlayerListener::EVENT_SEEK_DONE,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiStreamPlayer::OffEndOfStream(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer)
{
    if (napiStreamPlayer == nullptr) {
        CLOGE("napiStreamPlayer is null");
        return napi_generic_failure;
    }
    auto napiListener = napiStreamPlayer->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("napi stream player callback is null");
        return napi_generic_failure;
    }
    if (napiListener->RemoveCallback(env, NapiStreamPlayerListener::EVENT_END_OF_STREAM,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiStreamPlayer::OffImageChanged(napi_env env, napi_value callback, NapiStreamPlayer *napiStreamPlayer)
{
    if (napiStreamPlayer == nullptr) {
        CLOGE("napiStreamPlayer is null");
        return napi_generic_failure;
    }
    auto napiListener = napiStreamPlayer->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("napi stream player callback is null");
        return napi_generic_failure;
    }
    if (napiListener->RemoveCallback(env, NapiStreamPlayerListener::EVENT_IMAGE_CHANGED,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiStreamPlayer::RegisterNativeStreamPlayerListener(NapiStreamPlayer *napiStreamPlayer)
{
    if (napiStreamPlayer == nullptr) {
        CLOGE("napiStreamPlayer is null");
        return napi_generic_failure;
    }
    if (napiStreamPlayer->NapiListenerGetter()) {
        return napi_ok;
    }
    auto streamPlayer = napiStreamPlayer->GetStreamPlayer();
    if (!streamPlayer) {
        CLOGE("StreamPlayer is null");
        return napi_generic_failure;
    }

    auto listener = std::make_shared<NapiStreamPlayerListener>();
    if (!listener) {
        CLOGE("Failed to malloc stream player listener");
        return napi_generic_failure;
    }
    int32_t ret = streamPlayer->RegisterListener(listener);
    if (ret != CAST_ENGINE_SUCCESS) {
        CLOGE("native register stream player listener failed");
        return napi_generic_failure;
    }
    napiStreamPlayer->NapiListenerSetter(listener);

    return napi_ok;
}

napi_value NapiStreamPlayer::SetSurface(napi_env env, napi_callback_info info)
{
    CLOGD("Start to set surface in");
    struct ConcreteTask : public NapiAsyncTask {
        std::string surfaceId_;
    };
    auto napiAsyntask = std::make_shared<ConcreteTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }

    auto inputParser = [env, napiAsyntask](size_t argc, napi_value *argv) {
        constexpr size_t expectedArgc = 1;
        CHECK_ARGS_RETURN_VOID(napiAsyntask, argc == expectedArgc, "invalid arguments",
            NapiErrors::errcode_[ERR_INVALID_PARAM]);
        napi_valuetype expectedTypes[expectedArgc] = { napi_string };
        bool isParamsTypeValid = CheckJSParamsType(env, argv, expectedArgc, expectedTypes);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, isParamsTypeValid, "invalid arguments",
            NapiErrors::errcode_[ERR_INVALID_PARAM]);
        napiAsyntask->surfaceId_ = ParseString(env, argv[0]);
    };
    napiAsyntask->GetJSInfo(env, info, inputParser);
    auto executor = [napiAsyntask]() {
        auto *napiStreamPlayer = reinterpret_cast<NapiStreamPlayer *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiStreamPlayer != nullptr, "napiStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        std::shared_ptr<IStreamPlayer> streamPlayer = napiStreamPlayer->GetStreamPlayer();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, streamPlayer, "IStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = streamPlayer->SetSurface(napiAsyntask->surfaceId_);
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "SetStreamSurface failed : no permission";
            } else if (ret == ERR_INVALID_PARAM) {
                napiAsyntask->errMessage = "SetStreamSurface failed : invalid parameters";
            } else {
                napiAsyntask->errMessage = "SetStreamSurface failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [env](napi_value &output) { output = GetUndefinedValue(env); };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "SetStreamSurface", executor, complete);
}

napi_value NapiStreamPlayer::Load(napi_env env, napi_callback_info info)
{
    CLOGD("Start to load in");
    struct ConcreteTask : public NapiAsyncTask {
        MediaInfo mediaInfo_;
    };
    auto napiAsyntask = std::make_shared<ConcreteTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }

    auto inputParser = [env, napiAsyntask](size_t argc, napi_value *argv) {
        constexpr size_t expectedArgc = 1;
        CHECK_ARGS_RETURN_VOID(napiAsyntask, argc == expectedArgc, "invalid arguments",
            NapiErrors::errcode_[ERR_INVALID_PARAM]);
        napi_valuetype expectedTypes[expectedArgc] = { napi_object };
        bool isParamsTypeValid = CheckJSParamsType(env, argv, expectedArgc, expectedTypes);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, isParamsTypeValid, "invalid arguments",
            NapiErrors::errcode_[ERR_INVALID_PARAM]);
        bool isMediaInfoValid = GetMediaInfoFromJS(env, argv[0], napiAsyntask->mediaInfo_);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, isMediaInfoValid, "invalid arguments",
            NapiErrors::errcode_[ERR_INVALID_PARAM]);
    };
    napiAsyntask->GetJSInfo(env, info, inputParser);
    auto executor = [napiAsyntask]() {
        auto *napiStreamPlayer = reinterpret_cast<NapiStreamPlayer *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiStreamPlayer != nullptr, "napiStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        std::shared_ptr<IStreamPlayer> streamPlayer = napiStreamPlayer->GetStreamPlayer();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, streamPlayer, "IStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = streamPlayer->Load(napiAsyntask->mediaInfo_);
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "Load failed : no permission";
            } else {
                napiAsyntask->errMessage = "Load failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [env](napi_value &output) { output = GetUndefinedValue(env); };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "Load", executor, complete);
}

napi_value NapiStreamPlayer::Start(napi_env env, napi_callback_info info)
{
    CLOGD("Start to start in");
    struct ConcreteTask : public NapiAsyncTask {
        MediaInfo mediaInfo_;
    };
    auto napiAsyntask = std::make_shared<ConcreteTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }

    auto inputParser = [env, napiAsyntask](size_t argc, napi_value *argv) {
        constexpr size_t expectedArgc = 1;
        CHECK_ARGS_RETURN_VOID(napiAsyntask, argc == expectedArgc, "invalid arguments",
            NapiErrors::errcode_[ERR_INVALID_PARAM]);
        napi_valuetype expectedTypes[expectedArgc] = { napi_object };
        bool isParamsTypeValid = CheckJSParamsType(env, argv, expectedArgc, expectedTypes);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, isParamsTypeValid, "invalid arguments",
            NapiErrors::errcode_[ERR_INVALID_PARAM]);
        bool isMediaInfoValid = GetMediaInfoFromJS(env, argv[0], napiAsyntask->mediaInfo_);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, isMediaInfoValid, "invalid arguments",
            NapiErrors::errcode_[ERR_INVALID_PARAM]);
    };
    napiAsyntask->GetJSInfo(env, info, inputParser);
    auto executor = [napiAsyntask]() {
        auto *napiStreamPlayer = reinterpret_cast<NapiStreamPlayer *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiStreamPlayer != nullptr, "napiStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        std::shared_ptr<IStreamPlayer> streamPlayer = napiStreamPlayer->GetStreamPlayer();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, streamPlayer, "IStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = streamPlayer->Play(napiAsyntask->mediaInfo_);
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "Start failed : no permission";
            } else {
                napiAsyntask->errMessage = "Start failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [env](napi_value &output) { output = GetUndefinedValue(env); };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "Start", executor, complete);
}

napi_value NapiStreamPlayer::Play(napi_env env, napi_callback_info info)
{
    CLOGD("Start to play in");
    struct ConcreteTask : public NapiAsyncTask {
        bool isRemotePlay_ = true;
        int index_;
    };
    auto napiAsyntask = std::make_shared<ConcreteTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }
    auto inputParser = [env, napiAsyntask](size_t argc, napi_value *argv) {
        constexpr size_t expectedRemotePlayArgc = 0;
        constexpr size_t expectedLocalPlayArgc = 1;
        CHECK_ARGS_RETURN_VOID(napiAsyntask, argc == expectedRemotePlayArgc || argc == expectedLocalPlayArgc,
            "invalid arguments", NapiErrors::errcode_[ERR_INVALID_PARAM]);
        if (argc == expectedLocalPlayArgc) {
            napi_valuetype expectedTypes[expectedLocalPlayArgc] = { napi_number };
            bool isParamsTypeValid = CheckJSParamsType(env, argv, expectedLocalPlayArgc, expectedTypes);
            CHECK_ARGS_RETURN_VOID(napiAsyntask, isParamsTypeValid, "invalid arguments",
                NapiErrors::errcode_[ERR_INVALID_PARAM]);
            napiAsyntask->index_ = ParseInt32(env, argv[0]);
            napiAsyntask->isRemotePlay_ = false;
        }
    };
    napiAsyntask->GetJSInfo(env, info, inputParser);
    auto executor = [napiAsyntask]() {
        auto *napiStreamPlayer = reinterpret_cast<NapiStreamPlayer *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiStreamPlayer != nullptr, "napiStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        std::shared_ptr<IStreamPlayer> streamPlayer = napiStreamPlayer->GetStreamPlayer();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, streamPlayer, "IStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = CAST_ENGINE_ERROR;
        if (napiAsyntask->isRemotePlay_) {
            ret = streamPlayer->Play();
        } else {
            ret = streamPlayer->Play(napiAsyntask->index_);
        }
        if (ret != CAST_ENGINE_SUCCESS) {
            napiAsyntask->errMessage = "Play failed";
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };
    auto complete = [env](napi_value &output) { output = GetUndefinedValue(env); };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "Play", executor, complete);
}

napi_value NapiStreamPlayer::Pause(napi_env env, napi_callback_info info)
{
    CLOGD("Start to pause in");
    auto napiAsyntask = std::make_shared<NapiAsyncTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }

    napiAsyntask->GetJSInfo(env, info);
    auto executor = [napiAsyntask]() {
        auto *napiStreamPlayer = reinterpret_cast<NapiStreamPlayer *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiStreamPlayer != nullptr, "napiStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        std::shared_ptr<IStreamPlayer> streamPlayer = napiStreamPlayer->GetStreamPlayer();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, streamPlayer, "IStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = streamPlayer->Pause();
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "Pause failed : no permission";
            } else {
                napiAsyntask->errMessage = "Pause failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [env](napi_value &output) { output = GetUndefinedValue(env); };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "Pause", executor, complete);
}

napi_value NapiStreamPlayer::Stop(napi_env env, napi_callback_info info)
{
    CLOGD("Start to stop in");
    auto napiAsyntask = std::make_shared<NapiAsyncTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }

    napiAsyntask->GetJSInfo(env, info);
    auto executor = [napiAsyntask]() {
        auto *napiStreamPlayer = reinterpret_cast<NapiStreamPlayer *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiStreamPlayer != nullptr, "napiStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        std::shared_ptr<IStreamPlayer> streamPlayer = napiStreamPlayer->GetStreamPlayer();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, streamPlayer, "IStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = streamPlayer->Stop();
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "Stop failed : no permission";
            } else {
                napiAsyntask->errMessage = "Stop failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [env](napi_value &output) { output = GetUndefinedValue(env); };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "Stop", executor, complete);
}

napi_value NapiStreamPlayer::Next(napi_env env, napi_callback_info info)
{
    CLOGD("Start to next in");
    auto napiAsyntask = std::make_shared<NapiAsyncTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }

    napiAsyntask->GetJSInfo(env, info);
    auto executor = [napiAsyntask]() {
        auto *napiStreamPlayer = reinterpret_cast<NapiStreamPlayer *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiStreamPlayer != nullptr, "napiStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        std::shared_ptr<IStreamPlayer> streamPlayer = napiStreamPlayer->GetStreamPlayer();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, streamPlayer, "IStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = streamPlayer->Next();
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "Next failed : no permission";
            } else {
                napiAsyntask->errMessage = "Next failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [env](napi_value &output) { output = GetUndefinedValue(env); };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "Next", executor, complete);
}

napi_value NapiStreamPlayer::Previous(napi_env env, napi_callback_info info)
{
    CLOGD("Start to previous in");
    auto napiAsyntask = std::make_shared<NapiAsyncTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }

    napiAsyntask->GetJSInfo(env, info);
    auto executor = [napiAsyntask]() {
        auto *napiStreamPlayer = reinterpret_cast<NapiStreamPlayer *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiStreamPlayer != nullptr, "napiStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        std::shared_ptr<IStreamPlayer> streamPlayer = napiStreamPlayer->GetStreamPlayer();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, streamPlayer, "IStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = streamPlayer->Previous();
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "Previous failed : no permission";
            } else {
                napiAsyntask->errMessage = "Previous failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [env](napi_value &output) { output = GetUndefinedValue(env); };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "Previous", executor, complete);
}

napi_value NapiStreamPlayer::Seek(napi_env env, napi_callback_info info)
{
    CLOGD("Start to seek in");
    struct ConcreteTask : public NapiAsyncTask {
        int position_;
    };
    auto napiAsyntask = std::make_shared<ConcreteTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }

    auto inputParser = [env, napiAsyntask](size_t argc, napi_value *argv) {
        constexpr size_t expectedArgc = 1;
        CHECK_ARGS_RETURN_VOID(napiAsyntask, argc == expectedArgc, "invalid arguments",
            NapiErrors::errcode_[ERR_INVALID_PARAM]);
        napi_valuetype expectedTypes[expectedArgc] = { napi_number };
        bool isParamsTypeValid = CheckJSParamsType(env, argv, expectedArgc, expectedTypes);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, isParamsTypeValid, "invalid arguments",
            NapiErrors::errcode_[ERR_INVALID_PARAM]);
        napiAsyntask->position_ = ParseInt32(env, argv[0]);
    };
    napiAsyntask->GetJSInfo(env, info, inputParser);
    auto executor = [napiAsyntask]() {
        auto *napiStreamPlayer = reinterpret_cast<NapiStreamPlayer *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiStreamPlayer != nullptr, "napiStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        std::shared_ptr<IStreamPlayer> streamPlayer = napiStreamPlayer->GetStreamPlayer();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, streamPlayer, "IStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = streamPlayer->Seek(napiAsyntask->position_);
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "Seek failed : no permission";
            } else if (ret == ERR_INVALID_PARAM) {
                napiAsyntask->errMessage = "Seek failed : invalid parameters";
            } else {
                napiAsyntask->errMessage = "Seek failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [env](napi_value &output) { output = GetUndefinedValue(env); };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "Seek", executor, complete);
}

napi_value NapiStreamPlayer::FastForward(napi_env env, napi_callback_info info)
{
    CLOGD("Start to FastForward in");
    struct ConcreteTask : public NapiAsyncTask {
        int delta_;
    };
    auto napiAsyntask = std::make_shared<ConcreteTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }

    auto inputParser = [env, napiAsyntask](size_t argc, napi_value *argv) {
        constexpr size_t expectedArgc = 1;
        CHECK_ARGS_RETURN_VOID(napiAsyntask, argc == expectedArgc, "invalid arguments",
            NapiErrors::errcode_[ERR_INVALID_PARAM]);
        napi_valuetype expectedTypes[expectedArgc] = { napi_number };
        bool isParamsTypeValid = CheckJSParamsType(env, argv, expectedArgc, expectedTypes);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, isParamsTypeValid, "invalid arguments",
            NapiErrors::errcode_[ERR_INVALID_PARAM]);
        napiAsyntask->delta_ = ParseInt32(env, argv[0]);
    };
    napiAsyntask->GetJSInfo(env, info, inputParser);
    auto executor = [napiAsyntask]() {
        auto *napiStreamPlayer = reinterpret_cast<NapiStreamPlayer *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiStreamPlayer != nullptr, "napiStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        std::shared_ptr<IStreamPlayer> streamPlayer = napiStreamPlayer->GetStreamPlayer();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, streamPlayer, "IStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = streamPlayer->FastForward(napiAsyntask->delta_);
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "FastForward failed : no permission";
            } else if (ret == ERR_INVALID_PARAM) {
                napiAsyntask->errMessage = "FastForward failed : invalid parameters";
            } else {
                napiAsyntask->errMessage = "FastForward failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [env](napi_value &output) { output = GetUndefinedValue(env); };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "FastForward", executor, complete);
}

napi_value NapiStreamPlayer::FastRewind(napi_env env, napi_callback_info info)
{
    CLOGD("Start to FastRewind in");
    struct ConcreteTask : public NapiAsyncTask {
        int delta_;
    };
    auto napiAsyntask = std::make_shared<ConcreteTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }

    auto inputParser = [env, napiAsyntask](size_t argc, napi_value *argv) {
        constexpr size_t expectedArgc = 1;
        CHECK_ARGS_RETURN_VOID(napiAsyntask, argc == expectedArgc, "invalid arguments",
            NapiErrors::errcode_[ERR_INVALID_PARAM]);
        napi_valuetype expectedTypes[expectedArgc] = { napi_number };
        bool isParamsTypeValid = CheckJSParamsType(env, argv, expectedArgc, expectedTypes);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, isParamsTypeValid, "invalid arguments",
            NapiErrors::errcode_[ERR_INVALID_PARAM]);
        napiAsyntask->delta_ = ParseInt32(env, argv[0]);
    };
    napiAsyntask->GetJSInfo(env, info, inputParser);
    auto executor = [napiAsyntask]() {
        auto *napiStreamPlayer = reinterpret_cast<NapiStreamPlayer *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiStreamPlayer != nullptr, "napiStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        std::shared_ptr<IStreamPlayer> streamPlayer = napiStreamPlayer->GetStreamPlayer();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, streamPlayer, "IStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = streamPlayer->FastRewind(napiAsyntask->delta_);
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "FastRewind failed : no permission";
            } else if (ret == ERR_INVALID_PARAM) {
                napiAsyntask->errMessage = "FastRewind failed : invalid parameters";
            } else {
                napiAsyntask->errMessage = "FastRewind failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [env](napi_value &output) { output = GetUndefinedValue(env); };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "FastRewind", executor, complete);
}

napi_value NapiStreamPlayer::SetVolume(napi_env env, napi_callback_info info)
{
    CLOGD("Start to set volume in");
    struct ConcreteTask : public NapiAsyncTask {
        int volume_;
    };
    auto napiAsyntask = std::make_shared<ConcreteTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }

    auto inputParser = [env, napiAsyntask](size_t argc, napi_value *argv) {
        constexpr size_t expectedArgc = 1;
        CHECK_ARGS_RETURN_VOID(napiAsyntask, argc == expectedArgc, "invalid arguments",
            NapiErrors::errcode_[ERR_INVALID_PARAM]);
        napi_valuetype expectedTypes[expectedArgc] = { napi_number };
        bool isParamsTypeValid = CheckJSParamsType(env, argv, expectedArgc, expectedTypes);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, isParamsTypeValid, "invalid arguments",
            NapiErrors::errcode_[ERR_INVALID_PARAM]);
        napiAsyntask->volume_ = ParseInt32(env, argv[0]);
    };
    napiAsyntask->GetJSInfo(env, info, inputParser);
    auto executor = [napiAsyntask]() {
        auto *napiStreamPlayer = reinterpret_cast<NapiStreamPlayer *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiStreamPlayer != nullptr, "napiStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        std::shared_ptr<IStreamPlayer> streamPlayer = napiStreamPlayer->GetStreamPlayer();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, streamPlayer, "IStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = streamPlayer->SetVolume(napiAsyntask->volume_);
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "SetVolume failed : no permission";
            } else if (ret == ERR_INVALID_PARAM) {
                napiAsyntask->errMessage = "SetVolume failed : invalid parameters";
            } else {
                napiAsyntask->errMessage = "SetVolume failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [env](napi_value &output) { output = GetUndefinedValue(env); };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "SetVolume", executor, complete);
}

napi_value NapiStreamPlayer::SetLoopMode(napi_env env, napi_callback_info info)
{
    CLOGD("Start to set loop mode in");
    struct ConcreteTask : public NapiAsyncTask {
        LoopMode loopmode_;
    };
    auto napiAsyntask = std::make_shared<ConcreteTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }

    auto inputParser = [env, napiAsyntask](size_t argc, napi_value *argv) {
        constexpr size_t expectedArgc = 1;
        CHECK_ARGS_RETURN_VOID(napiAsyntask, argc == expectedArgc, "invalid arguments",
            NapiErrors::errcode_[ERR_INVALID_PARAM]);
        napi_valuetype expectedTypes[expectedArgc] = { napi_number };
        bool isParamsTypeValid = CheckJSParamsType(env, argv, expectedArgc, expectedTypes);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, isParamsTypeValid, "invalid arguments",
            NapiErrors::errcode_[ERR_INVALID_PARAM]);
        napiAsyntask->loopmode_ = static_cast<LoopMode>(ParseInt32(env, argv[0]));
    };
    napiAsyntask->GetJSInfo(env, info, inputParser);
    auto executor = [napiAsyntask]() {
        auto *napiStreamPlayer = reinterpret_cast<NapiStreamPlayer *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiStreamPlayer != nullptr, "napiStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        std::shared_ptr<IStreamPlayer> streamPlayer = napiStreamPlayer->GetStreamPlayer();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, streamPlayer, "IStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = streamPlayer->SetLoopMode(napiAsyntask->loopmode_);
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "SetLoopMode failed : no permission";
            } else if (ret == ERR_INVALID_PARAM) {
                napiAsyntask->errMessage = "SetLoopMode failed : invalid parameters";
            } else {
                napiAsyntask->errMessage = "SetLoopMode failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [env](napi_value &output) { output = GetUndefinedValue(env); };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "SetLoopMode", executor, complete);
}

napi_value NapiStreamPlayer::SetSpeed(napi_env env, napi_callback_info info)
{
    CLOGD("Start to set speed in");
    struct ConcreteTask : public NapiAsyncTask {
        PlaybackSpeed speed_;
    };
    auto napiAsyntask = std::make_shared<ConcreteTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }

    auto inputParser = [env, napiAsyntask](size_t argc, napi_value *argv) {
        constexpr size_t expectedArgc = 1;
        CHECK_ARGS_RETURN_VOID(napiAsyntask, argc == expectedArgc, "invalid arguments",
            NapiErrors::errcode_[ERR_INVALID_PARAM]);
        napi_valuetype expectedTypes[expectedArgc] = { napi_number };
        bool isParamsTypeValid = CheckJSParamsType(env, argv, expectedArgc, expectedTypes);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, isParamsTypeValid, "invalid arguments",
            NapiErrors::errcode_[ERR_INVALID_PARAM]);
        napiAsyntask->speed_ = static_cast<PlaybackSpeed>(ParseInt32(env, argv[0]));
    };
    napiAsyntask->GetJSInfo(env, info, inputParser);
    auto executor = [napiAsyntask]() {
        auto *napiStreamPlayer = reinterpret_cast<NapiStreamPlayer *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiStreamPlayer != nullptr, "napiStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        std::shared_ptr<IStreamPlayer> streamPlayer = napiStreamPlayer->GetStreamPlayer();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, streamPlayer, "IStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = streamPlayer->SetSpeed(napiAsyntask->speed_);
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "SetSpeed failed : no permission";
            } else if (ret == ERR_INVALID_PARAM) {
                napiAsyntask->errMessage = "SetSpeed failed : invalid parameters";
            } else {
                napiAsyntask->errMessage = "SetSpeed failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [env](napi_value &output) { output = GetUndefinedValue(env); };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "SetSpeed", executor, complete);
}

napi_value NapiStreamPlayer::SetMute(napi_env env, napi_callback_info info)
{
    CLOGD("Start to set mute in");
    struct ConcreteTask : public NapiAsyncTask {
        bool setMute_;
    };
    auto napiAsyntask = std::make_shared<ConcreteTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }

    auto inputParser = [env, napiAsyntask](size_t argc, napi_value *argv) {
        constexpr size_t expectedArgc = 1;
        CHECK_ARGS_RETURN_VOID(napiAsyntask, argc == expectedArgc, "invalid arguments",
            NapiErrors::errcode_[ERR_INVALID_PARAM]);
        napi_valuetype expectedTypes[expectedArgc] = { napi_boolean };
        bool isParamsTypeValid = CheckJSParamsType(env, argv, expectedArgc, expectedTypes);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, isParamsTypeValid, "invalid arguments",
            NapiErrors::errcode_[ERR_INVALID_PARAM]);
        napiAsyntask->setMute_ = ParseBool(env, argv[0]);
    };
    napiAsyntask->GetJSInfo(env, info, inputParser);
    auto executor = [napiAsyntask]() {
        auto *napiStreamPlayer = reinterpret_cast<NapiStreamPlayer *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiStreamPlayer != nullptr, "napiStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        std::shared_ptr<IStreamPlayer> streamPlayer = napiStreamPlayer->GetStreamPlayer();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, streamPlayer, "IStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = streamPlayer->SetMute(napiAsyntask->setMute_);
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "SetMute failed : no permission";
            } else {
                napiAsyntask->errMessage = "SetMute failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [env](napi_value &output) { output = GetUndefinedValue(env); };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "SetSpeed", executor, complete);
}

napi_value NapiStreamPlayer::GetPlayerStatus(napi_env env, napi_callback_info info)
{
    CLOGD("Start to get player status in");
    struct ConcreteTask : public NapiAsyncTask {
        PlayerStates playerStatus_;
    };
    auto napiAsyntask = std::make_shared<ConcreteTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }

    napiAsyntask->GetJSInfo(env, info);
    auto executor = [napiAsyntask]() {
        auto *napiStreamPlayer = reinterpret_cast<NapiStreamPlayer *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiStreamPlayer != nullptr, "napiStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        std::shared_ptr<IStreamPlayer> streamPlayer = napiStreamPlayer->GetStreamPlayer();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, streamPlayer, "IStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = streamPlayer->GetPlayerStatus(napiAsyntask->playerStatus_);
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "GetPlayerStatus failed : no permission";
            } else {
                napiAsyntask->errMessage = "GetPlayerStatus failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [env, napiAsyntask](napi_value &output) {
        napiAsyntask->status = napi_create_int32(env, static_cast<int32_t>(napiAsyntask->playerStatus_), &output);
        CHECK_STATUS_RETURN_VOID(napiAsyntask, "napi_create_int32 failed", NapiErrors::errcode_[CAST_ENGINE_ERROR]);
    };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "GetPlayerStatus", executor, complete);
}

napi_value NapiStreamPlayer::GetPosition(napi_env env, napi_callback_info info)
{
    CLOGD("Start to get position in");
    struct ConcreteTask : public NapiAsyncTask {
        int position_;
    };
    auto napiAsyntask = std::make_shared<ConcreteTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }

    napiAsyntask->GetJSInfo(env, info);
    auto executor = [napiAsyntask]() {
        auto *napiStreamPlayer = reinterpret_cast<NapiStreamPlayer *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiStreamPlayer != nullptr, "napiStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        std::shared_ptr<IStreamPlayer> streamPlayer = napiStreamPlayer->GetStreamPlayer();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, streamPlayer, "IStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = streamPlayer->GetPosition(napiAsyntask->position_);
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "GetPosition failed : no permission";
            } else {
                napiAsyntask->errMessage = "GetPosition failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [env, napiAsyntask](napi_value &output) {
        napiAsyntask->status = napi_create_int32(env, napiAsyntask->position_, &output);
        CHECK_STATUS_RETURN_VOID(napiAsyntask, "napi_create_int32 failed", NapiErrors::errcode_[CAST_ENGINE_ERROR]);
    };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "GetPosition", executor, complete);
}

napi_value NapiStreamPlayer::GetVolume(napi_env env, napi_callback_info info)
{
    CLOGD("Start to get volume in");
    struct ConcreteTask : public NapiAsyncTask {
        int volume_;
        int maxVolume_;
    };
    auto napiAsyntask = std::make_shared<ConcreteTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }

    napiAsyntask->GetJSInfo(env, info);
    auto executor = [napiAsyntask]() {
        auto *napiStreamPlayer = reinterpret_cast<NapiStreamPlayer *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiStreamPlayer != nullptr, "napiStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        std::shared_ptr<IStreamPlayer> streamPlayer = napiStreamPlayer->GetStreamPlayer();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, streamPlayer, "IStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = streamPlayer->GetVolume(napiAsyntask->volume_, napiAsyntask->maxVolume_);
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "GetVolume failed : no permission";
            } else {
                napiAsyntask->errMessage = "GetVolume failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [env, napiAsyntask](napi_value &output) {
        napiAsyntask->status = napi_create_int32(env, napiAsyntask->volume_, &output);
        CHECK_STATUS_RETURN_VOID(napiAsyntask, "napi_create_int32 failed", NapiErrors::errcode_[CAST_ENGINE_ERROR]);
    };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "GetVolume", executor, complete);
}

napi_value NapiStreamPlayer::GetMute(napi_env env, napi_callback_info info)
{
    CLOGD("Start to get mute in");
    struct ConcreteTask : public NapiAsyncTask {
        bool isMute_;
    };
    auto napiAsyntask = std::make_shared<ConcreteTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }

    napiAsyntask->GetJSInfo(env, info);
    auto executor = [napiAsyntask]() {
        auto *napiStreamPlayer = reinterpret_cast<NapiStreamPlayer *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiStreamPlayer != nullptr, "napiStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        std::shared_ptr<IStreamPlayer> streamPlayer = napiStreamPlayer->GetStreamPlayer();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, streamPlayer, "IStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = streamPlayer->GetMute(napiAsyntask->isMute_);
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "GetMute failed : no permission";
            } else {
                napiAsyntask->errMessage = "GetMute failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [env, napiAsyntask](napi_value &output) {
        napiAsyntask->status = napi_create_int32(env, napiAsyntask->isMute_, &output);
        CHECK_STATUS_RETURN_VOID(napiAsyntask, "napi_create_int32 failed", NapiErrors::errcode_[CAST_ENGINE_ERROR]);
    };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "GetMute", executor, complete);
}

napi_value NapiStreamPlayer::GetLoopMode(napi_env env, napi_callback_info info)
{
    CLOGD("Start to get loop mode in");
    struct ConcreteTask : public NapiAsyncTask {
        LoopMode loopMode_;
    };
    auto napiAsyntask = std::make_shared<ConcreteTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }

    napiAsyntask->GetJSInfo(env, info);
    auto executor = [napiAsyntask]() {
        auto *napiStreamPlayer = reinterpret_cast<NapiStreamPlayer *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiStreamPlayer != nullptr, "napiStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        std::shared_ptr<IStreamPlayer> streamPlayer = napiStreamPlayer->GetStreamPlayer();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, streamPlayer, "IStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = streamPlayer->GetLoopMode(napiAsyntask->loopMode_);
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "GetLoopMode failed : no permission";
            } else {
                napiAsyntask->errMessage = "GetLoopMode failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [env, napiAsyntask](napi_value &output) {
        napiAsyntask->status = napi_create_int32(env, static_cast<int32_t>(napiAsyntask->loopMode_), &output);
        CHECK_STATUS_RETURN_VOID(napiAsyntask, "napi_create_int32 failed", NapiErrors::errcode_[CAST_ENGINE_ERROR]);
    };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "GetLoopMode", executor, complete);
}

napi_value NapiStreamPlayer::GetPlaySpeed(napi_env env, napi_callback_info info)
{
    CLOGD("Start to get play speed in");
    struct ConcreteTask : public NapiAsyncTask {
        PlaybackSpeed speed_;
    };
    auto napiAsyntask = std::make_shared<ConcreteTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }

    napiAsyntask->GetJSInfo(env, info);
    auto executor = [napiAsyntask]() {
        auto *napiStreamPlayer = reinterpret_cast<NapiStreamPlayer *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiStreamPlayer != nullptr, "napiStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        std::shared_ptr<IStreamPlayer> streamPlayer = napiStreamPlayer->GetStreamPlayer();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, streamPlayer, "IStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = streamPlayer->GetPlaySpeed(napiAsyntask->speed_);
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "GetPlaySpeed failed : no permission";
            } else {
                napiAsyntask->errMessage = "GetPlaySpeed failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [env, napiAsyntask](napi_value &output) {
        napiAsyntask->status = napi_create_int32(env, static_cast<int32_t>(napiAsyntask->speed_), &output);
        CHECK_STATUS_RETURN_VOID(napiAsyntask, "napi_create_int32 failed", NapiErrors::errcode_[CAST_ENGINE_ERROR]);
    };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "GetPlaySpeed", executor, complete);
}

napi_value NapiStreamPlayer::GetMediaInfoHolder(napi_env env, napi_callback_info info)
{
    CLOGD("Start to get play info holder in");
    struct ConcreteTask : public NapiAsyncTask {
        MediaInfoHolder mediaInfoHolder_;
    };
    auto napiAsyntask = std::make_shared<ConcreteTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }

    napiAsyntask->GetJSInfo(env, info);
    auto executor = [napiAsyntask]() {
        auto *napiStreamPlayer = reinterpret_cast<NapiStreamPlayer *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiStreamPlayer != nullptr, "napiStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        std::shared_ptr<IStreamPlayer> streamPlayer = napiStreamPlayer->GetStreamPlayer();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, streamPlayer, "IStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = streamPlayer->GetMediaInfoHolder(napiAsyntask->mediaInfoHolder_);
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "GetMediaInfoHolder failed : no permission";
            } else {
                napiAsyntask->errMessage = "GetMediaInfoHolder failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [napiAsyntask](napi_value &output) {
        output = ConvertMediaInfoHolderToJS(napiAsyntask->env, napiAsyntask->mediaInfoHolder_);
    };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "GetMediaInfoHolder", executor, complete);
}

napi_value NapiStreamPlayer::Release(napi_env env, napi_callback_info info)
{
    CLOGD("Start to release in");
    auto napiAsyntask = std::make_shared<NapiAsyncTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }
    napiAsyntask->GetJSInfo(env, info);

    auto executor = [napiAsyntask]() {
        auto *napiStreamPlayer = reinterpret_cast<NapiStreamPlayer *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiStreamPlayer != nullptr, "napiStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        std::shared_ptr<IStreamPlayer> streamPlayer = napiStreamPlayer->GetStreamPlayer();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, streamPlayer, "IStreamPlayer is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = streamPlayer->Release();
        if (ret == CAST_ENGINE_SUCCESS) {
            napiStreamPlayer->Reset();
        } else if (ret == ERR_NO_PERMISSION) {
            napiAsyntask->errMessage = "Release failed : no permission";
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        } else {
            napiAsyntask->errMessage = "Release failed : native server exception";
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [env](napi_value &output) { output = GetUndefinedValue(env); };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "Release", executor, complete);
}

std::shared_ptr<NapiStreamPlayerListener> NapiStreamPlayer::NapiListenerGetter()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return listener_;
}

void NapiStreamPlayer::NapiListenerSetter(std::shared_ptr<NapiStreamPlayerListener> listener)
{
    std::lock_guard<std::mutex> lock(mutex_);
    listener_ = listener;
}
} // namespace CastEngineClient
} // namespace CastEngine
} // namespace OHOS