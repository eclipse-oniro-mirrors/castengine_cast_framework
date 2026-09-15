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
 * Description: supply stream player listener realization for napi interface.
 * Author: huangchanggui
 * Create: 2023-1-11
 */

#include "napi_stream_player_listener.h"
#include <uv.h>
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "cast_engine_log.h"
#include "cast_engine_common.h"
#include "napi_castengine_utils.h"
#include "pixel_map_napi.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineClient {
DEFINE_CAST_ENGINE_LABEL("Cast-Napi-StreamPlayerListener");

NapiStreamPlayerListener::~NapiStreamPlayerListener()
{
    CLOGD("destrcutor in");
}

void NapiStreamPlayerListener::HandleEvent(int32_t event, NapiArgsGetter &getter)
{
    std::lock_guard<std::mutex> lockGuard(lock_);
    if (!callback_) {
        CLOGE("callback_ is nullptr, event:%{public}d", event);
        return;
    }
    callback_->HandleEvent(event, getter);
}

void NapiStreamPlayerListener::OnStateChanged(const PlayerStates playbackState, bool isPlayWhenReady)
{
    CLOGD("OnStateChanged start");
    NapiArgsGetter napiArgsGetter = [playbackState, isPlayWhenReady](napi_env env, int &argc, napi_value *argv) {
        argc = CALLBACK_ARGC_TWO;
        auto status = napi_create_int32(env, static_cast<int>(playbackState), &argv[0]);
        CHECK_RETURN_VOID(status == napi_ok, "napi_create_int32 failed");
        status = napi_get_boolean(env, isPlayWhenReady, &argv[1]);
        CHECK_RETURN_VOID(status == napi_ok, "napi_get_boolean failed");
    };
    HandleEvent(EVENT_PLAYER_STATUS_CHANGED, napiArgsGetter);
    CLOGD("OnStateChanged finish");
}

void NapiStreamPlayerListener::OnPositionChanged(int position, int bufferPosition, int duration)
{
    CLOGD("OnPositionChanged start");
    std::chrono::steady_clock::duration timestampOrigin = std::chrono::steady_clock::now().time_since_epoch();
    uint64_t timestamp =
        static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(timestampOrigin).count());
    NapiArgsGetter napiArgsGetter = [position, bufferPosition, duration, timestamp](napi_env env, int &argc,
        napi_value *argv) {
        argc = CALLBACK_ARGC_FOUR;
        auto status = napi_create_int32(env, position, &argv[0]);
        CHECK_RETURN_VOID(status == napi_ok, "napi_create_int32 failed");
        status = napi_create_int32(env, bufferPosition, &argv[1]);
        CHECK_RETURN_VOID(status == napi_ok, "napi_create_int32 failed");
        status = napi_create_int32(env, duration, &argv[2]);
        CHECK_RETURN_VOID(status == napi_ok, "napi_create_int32 failed");
        status = napi_create_bigint_uint64(env, timestamp, &argv[3]);
        CHECK_RETURN_VOID(status == napi_ok, "napi_create_bigint_uint64 failed");
    };
    HandleEvent(EVENT_POSITION_CHANGED, napiArgsGetter);
    CLOGD("OnPositionChanged finish");
}

void NapiStreamPlayerListener::OnMediaItemChanged(const MediaInfo &mediaInfo)
{
    CLOGD("OnMediaItemChanged start");
    NapiArgsGetter napiArgsGetter = [mediaInfo](napi_env env, int &argc, napi_value *argv) {
        argc = CALLBACK_ARGC_ONE;
        argv[0] = ConvertMediaInfoToJS(env, mediaInfo);
    };
    HandleEvent(EVENT_MEDIA_ITEM_CHANGED, napiArgsGetter);
    CLOGD("OnMediaItemChanged finish");
}

void NapiStreamPlayerListener::OnMediaInfoChanged(const MediaInfo &mediaInfo)
{
    CLOGD("OnMediaInfoChanged start");
    NapiArgsGetter napiArgsGetter = [mediaInfo](napi_env env, int &argc, napi_value *argv) {
        argc = CALLBACK_ARGC_ONE;
        argv[0] = ConvertMediaInfoToJS(env, mediaInfo);
    };
    HandleEvent(EVENT_MEDIA_INFO_CHANGED, napiArgsGetter);
    CLOGD("OnMediaInfoChanged finish");
}

void NapiStreamPlayerListener::OnVolumeChanged(int volume, int maxVolume)
{
    CLOGD("OnVolumeChanged start");
    NapiArgsGetter napiArgsGetter = [volume](napi_env env, int &argc, napi_value *argv) {
        argc = CALLBACK_ARGC_ONE;
        auto status = napi_create_int32(env, volume, &argv[0]);
        CHECK_RETURN_VOID(status == napi_ok, "napi_create_int32 failed");
    };
    HandleEvent(EVENT_VOLUME_CHANGED, napiArgsGetter);
    CLOGD("OnVolumeChanged finish");
}

void NapiStreamPlayerListener::OnVideoSizeChanged(int width, int height)
{
    CLOGD("OnVideoSizeChanged start");
    NapiArgsGetter napiArgsGetter = [width, height](napi_env env, int &argc, napi_value *argv) {
        argc = CALLBACK_ARGC_TWO;
        auto status = napi_create_int32(env, width, &argv[0]);
        CHECK_RETURN_VOID(status == napi_ok, "napi_create_int32 failed");
        status = napi_create_int32(env, height, &argv[1]);
        CHECK_RETURN_VOID(status == napi_ok, "napi_create_int32 failed");
    };
    HandleEvent(EVENT_VIDEO_SIZE_CHANGED, napiArgsGetter);
    CLOGD("OnVideoSizeChanged finish");
}

void NapiStreamPlayerListener::OnLoopModeChanged(const LoopMode loopMode)
{
    CLOGD("OnLoopModeChanged start");
    NapiArgsGetter napiArgsGetter = [loopMode](napi_env env, int &argc, napi_value *argv) {
        argc = CALLBACK_ARGC_ONE;
        auto status = napi_create_int32(env, static_cast<int>(loopMode), &argv[0]);
        CHECK_RETURN_VOID(status == napi_ok, "napi_create_int32 failed");
    };
    HandleEvent(EVENT_LOOP_MODE_CHANGED, napiArgsGetter);
    CLOGD("OnLoopModeChanged finish");
}

void NapiStreamPlayerListener::OnAvailableCapabilityChanged(const StreamCapability &streamCapability)
{
    CLOGD("OnAvailableCapabilityChanged finish");
}

void NapiStreamPlayerListener::OnPlaySpeedChanged(const PlaybackSpeed speed)
{
    CLOGD("OnPlaySpeedChanged start");
    NapiArgsGetter napiArgsGetter = [speed](napi_env env, int &argc, napi_value *argv) {
        argc = CALLBACK_ARGC_ONE;
        auto status = napi_create_int32(env, static_cast<int>(speed), &argv[0]);
        CHECK_RETURN_VOID(status == napi_ok, "napi_create_double failed");
    };
    HandleEvent(EVENT_PLAY_SPEED_CHANGED, napiArgsGetter);
    CLOGD("OnPlaySpeedChanged finish");
}

void NapiStreamPlayerListener::OnPlayerError(int errorCode, const std::string &errorMsg)
{
    CLOGD("OnPlayerError start");
    NapiArgsGetter napiArgsGetter = [errorCode, errorMsg](napi_env env, int &argc, napi_value *argv) {
        argc = CALLBACK_ARGC_TWO;
        auto status = napi_create_int32(env, errorCode, &argv[0]);
        CHECK_RETURN_VOID(status == napi_ok, "napi_create_int32 failed");
        status = napi_create_string_utf8(env, errorMsg.c_str(), NAPI_AUTO_LENGTH, &argv[1]);
        CHECK_RETURN_VOID(status == napi_ok, "napi_create_string_utf8 failed");
    };
    HandleEvent(EVENT_PLAYER_ERROR, napiArgsGetter);
    CLOGD("OnPlayerError finish");
}

void NapiStreamPlayerListener::OnNextRequest()
{
    CLOGD("OnNextRequest start");
    NapiArgsGetter napiArgsGetter = [](napi_env env, int &argc, napi_value *argv) {};
    HandleEvent(EVENT_NEXT_REQUEST, napiArgsGetter);
    CLOGD("OnNextRequest finish");
}

void NapiStreamPlayerListener::OnPreviousRequest()
{
    CLOGD("OnPreviousRequest start");
    NapiArgsGetter napiArgsGetter = [](napi_env env, int &argc, napi_value *argv) {};
    HandleEvent(EVENT_PREVIOUS_REQUEST, napiArgsGetter);
    CLOGD("OnPreviousRequest finish");
}

void NapiStreamPlayerListener::OnSeekDone(int position)
{
    CLOGD("OnSeekDone start");
    NapiArgsGetter napiArgsGetter = [position](napi_env env, int &argc, napi_value *argv) {
        argc = CALLBACK_ARGC_ONE;
        auto status = napi_create_int32(env, position, &argv[0]);
        CHECK_RETURN_VOID(status == napi_ok, "napi_create_int32 failed");
    };
    HandleEvent(EVENT_SEEK_DONE, napiArgsGetter);
    CLOGD("OnSeekDone finish");
}

void NapiStreamPlayerListener::OnEndOfStream(int isLooping)
{
    CLOGD("OnEndOfStream start");
    NapiArgsGetter napiArgsGetter = [isLooping](napi_env env, int &argc, napi_value *argv) {
        argc = CALLBACK_ARGC_ONE;
        auto status = napi_create_int32(env, isLooping, &argv[0]);
        CHECK_RETURN_VOID(status == napi_ok, "napi_create_int32 failed");
    };
    HandleEvent(EVENT_END_OF_STREAM, napiArgsGetter);
    CLOGD("OnEndOfStream finish");
}

void NapiStreamPlayerListener::OnPlayRequest(const MediaInfo &mediaInfo)
{
    CLOGD("OnPlayRequest start");
    NapiArgsGetter napiArgsGetter = [mediaInfo](napi_env env, int &argc, napi_value *argv) {
        argc = CALLBACK_ARGC_ONE;
        argv[0] = ConvertMediaInfoToJS(env, mediaInfo);
    };
    HandleEvent(EVENT_PLAY_REQUEST, napiArgsGetter);
    CLOGD("OnPlayRequest finish");
}

void NapiStreamPlayerListener::OnImageChanged(std::shared_ptr<Media::PixelMap> pixelMap)
{
    CLOGD("OnImageChanged start");
    NapiArgsGetter napiArgsGetter = [pixelMap](napi_env env, int &argc, napi_value *argv) {
        argc = CALLBACK_ARGC_ONE;
        std::shared_ptr<Media::PixelMap> imagePixel(pixelMap);
        argv[0] = Media::PixelMapNapi::CreatePixelMap(env, imagePixel);
    };
    HandleEvent(EVENT_IMAGE_CHANGED, napiArgsGetter);
    CLOGD("OnImageChanged finish");
}

void NapiStreamPlayerListener::OnAlbumCoverChanged(std::shared_ptr<Media::PixelMap> pixelMap)
{
    CLOGD("OnAlbumCoverChanged start");
}

void NapiStreamPlayerListener::OnKeyRequest(const std::string &mediaId, const std::vector<uint8_t> &keyRequestData)
{
    CLOGD("OnKeyRequest start");
}

void NapiStreamPlayerListener::OnData(const DataType dataType, const std::string &dataStr)
{
    CLOGD("OnData start");
}

napi_status NapiStreamPlayerListener::AddCallback(napi_env env, int32_t event, napi_value callback)
{
    CLOGI("Add callback %{public}d", event);
    std::lock_guard<std::mutex> lockGuard(lock_);
    if (callback_ == nullptr) {
        callback_ = std::make_shared<NapiCallback>(env);
        if (callback_ == nullptr) {
            CLOGE("no memory");
            return napi_generic_failure;
        }
    }
    return callback_->AddCallback(env, event, callback);
}

napi_status NapiStreamPlayerListener::RemoveCallback(napi_env env, int32_t event, napi_value callback)
{
    CLOGI("Remove callback %{public}d", event);
    std::lock_guard<std::mutex> lockGuard(lock_);
    if (!callback_) {
        CLOGE("callback_ is nullptr");
        return napi_ok;
    }
    return callback_->RemoveCallback(env, event, callback);
}
} // namespace CastEngineClient
} // namespace CastEngine
} // namespace OHOS
