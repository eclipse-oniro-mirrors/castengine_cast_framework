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
 * Description: supply napi interface realization for cast session.
 * Author: zhangjingnan
 * Create: 2022-7-11
 */

#include <memory>
#include "cast_engine_log.h"
#include "cast_engine_common.h"
#include "i_cast_session.h"
#include "napi_castengine_utils.h"
#include "napi_cast_session_listener.h"
#include "napi_cast_session.h"
#include "napi_stream_player.h"
#include "napi_mirror_player.h"
#include "napi_async_work.h"

using namespace OHOS::CastEngine::CastEngineClient;
using namespace std;

namespace OHOS {
namespace CastEngine {
namespace CastEngineClient {
DEFINE_CAST_ENGINE_LABEL("Cast-Napi-Session");

thread_local napi_ref NapiCastSession::consRef_ = nullptr;

std::map<std::string, std::pair<NapiCastSession::OnEventHandlerType,
    NapiCastSession::OffEventHandlerType>>
    NapiCastSession::eventHandlers_ = {
    { "event", { OnInnerEvent, OffInnerEvent } },
    { "deviceState", { OnDeviceState, OffDeviceState } }
};

void NapiCastSession::DefineCastSessionJSClass(napi_env env)
{
    napi_property_descriptor NapiCastSessionDesc[] = {
        DECLARE_NAPI_FUNCTION("on", OnEvent),
        DECLARE_NAPI_FUNCTION("off", OffEvent),
        DECLARE_NAPI_FUNCTION("addDevice", AddDevice),
        DECLARE_NAPI_FUNCTION("removeDevice", RemoveDevice),
        DECLARE_NAPI_FUNCTION("getSessionId", GetSessionId),
        DECLARE_NAPI_FUNCTION("setSessionProperty", SetSessionProperty),
        DECLARE_NAPI_FUNCTION("createMirrorPlayer", CreateMirrorPlayer),
        DECLARE_NAPI_FUNCTION("createStreamPlayer", CreateStreamPlayer),
        DECLARE_NAPI_FUNCTION("setCastMode", SetCastMode),
        DECLARE_NAPI_FUNCTION("release", Release),
        DECLARE_NAPI_FUNCTION("getRemoteDeviceInfo", GetRemoteDeviceInfo),
    };

    napi_value castSession = nullptr;
    constexpr int initialRefCount = 1;
    napi_status status = napi_define_class(env, "castSession", NAPI_AUTO_LENGTH, NapiCastSessionConstructor, nullptr,
        sizeof(NapiCastSessionDesc) / sizeof(NapiCastSessionDesc[0]), NapiCastSessionDesc, &castSession);
    if (status != napi_ok) {
        CLOGE("napi_define_class failed");
        return;
    }
    status = napi_create_reference(env, castSession, initialRefCount, &consRef_);
    if (status != napi_ok) {
        CLOGE("DefineCastSessionJSClass napi_create_reference failed");
    }
}

napi_value NapiCastSession::NapiCastSessionConstructor(napi_env env, napi_callback_info info)
{
    CLOGD("NapiCastSession start to construct in");
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr));
    CLOGD("NapiCastSession construct successfully");
    return thisVar;
}

napi_status NapiCastSession::CreateNapiCastSession(napi_env env, shared_ptr<ICastSession> session, napi_value &out)
{
    CLOGD("Start to create napiCastSession in");
    napi_value result = nullptr;
    napi_value constructor = nullptr;
    if (consRef_ == nullptr || session == nullptr) {
        CLOGE("CreateNapiCastSession input is null");
        return napi_generic_failure;
    }
    napi_status status = napi_get_reference_value(env, consRef_, &constructor);
    if (status != napi_ok || constructor == nullptr) {
        CLOGE("CreateNapiCastSession napi_get_reference_value failed");
        return napi_generic_failure;
    }

    constexpr size_t argc = 0;
    status = napi_new_instance(env, constructor, argc, nullptr, &result);
    if (status != napi_ok) {
        CLOGE("CreateNapiCastSession napi_new_instance failed");
        return napi_generic_failure;
    }

    NapiCastSession *napiCastSession = new (std::nothrow) NapiCastSession(session);
    if (napiCastSession == nullptr) {
        CLOGE("NapiCastSession is nullptr");
        return napi_generic_failure;
    }
    auto finalize = [](napi_env env, void *data, void *hint) {
        NapiCastSession *session = reinterpret_cast<NapiCastSession *>(data);
        if (session != nullptr) {
            CLOGI("Session deconstructed");
            delete session;
            session = nullptr;
        }
    };
    if (napi_wrap(env, result, napiCastSession, finalize, nullptr, nullptr) != napi_ok) {
        CLOGE("CreateNapiCastSession napi_wrap failed");
        delete napiCastSession;
        napiCastSession = nullptr;
        return napi_generic_failure;
    }
    out = result;
    CLOGD("Create napiCastSession successfully");
    return napi_ok;
}

NapiCastSession *NapiCastSession::GetNapiCastSession(napi_env env, napi_callback_info info)
{
    napi_value thisVar = nullptr;
    NAPI_CALL_BASE(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr), nullptr);

    NapiCastSession *napiCastSession = nullptr;
    NAPI_CALL_BASE(env, napi_unwrap(env, thisVar, reinterpret_cast<void **>(&napiCastSession)), nullptr);
    if (napiCastSession == nullptr) {
        CLOGE("napi_unwrap napiStreamPlayer is null");
        return nullptr;
    }

    return napiCastSession;
}

napi_value NapiCastSession::OnEvent(napi_env env, napi_callback_info info)
{
    constexpr size_t expectedArgc = 2;
    napi_value argv[expectedArgc] = { 0 };
    napi_valuetype expectedTypes[expectedArgc] = { napi_string, napi_function };
    if (!GetJSFuncParams(env, info, argv, expectedArgc, expectedTypes)) {
        return GetUndefinedValue(env);
    }
    std::string eventName = ParseString(env, argv[0]);
    NapiCastSession *napiSession = GetNapiCastSession(env, info);
    if (napiSession == nullptr) {
        CLOGE("napiSession is null");
        return GetUndefinedValue(env);
    }
    auto it = eventHandlers_.find(eventName);
    if (it == eventHandlers_.end()) {
        CLOGE("event name invalid");
        return GetUndefinedValue(env);
    }

    if (RegisterNativeSessionListener(napiSession) == napi_generic_failure) {
        return GetUndefinedValue(env);
    }
    if (it->second.first(env, argv[1], napiSession) != napi_ok) {
        CLOGE("event name invalid");
    }

    return GetUndefinedValue(env);
}

napi_value NapiCastSession::OffEvent(napi_env env, napi_callback_info info)
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
    NapiCastSession *napiSession = GetNapiCastSession(env, info);
    if (napiSession == nullptr) {
        CLOGE("napiSession is null");
        return GetUndefinedValue(env);
    }
    if (it->second.second(env, argv[1], napiSession) != napi_ok) {
        CLOGE("event name invalid");
    }

    return GetUndefinedValue(env);
}

napi_status NapiCastSession::RegisterNativeSessionListener(NapiCastSession *napiSession)
{
    if (napiSession == nullptr) {
        CLOGE("napiSession is null");
        return napi_generic_failure;
    }
    if (napiSession->NapiListenerGetter()) {
        return napi_ok;
    }
    auto session = napiSession->GetCastSession();
    if (!session) {
        CLOGE("Session is null");
        return napi_generic_failure;
    }

    auto listener = std::make_shared<NapiCastSessionListener>();
    if (!listener) {
        CLOGE("Failed to malloc session listener");
        return napi_generic_failure;
    }
    int32_t ret = session->RegisterListener(listener);
    if (ret != CAST_ENGINE_SUCCESS) {
        CLOGE("native register session listener failed");
        return napi_generic_failure;
    }
    napiSession->NapiListenerSetter(listener);

    return napi_ok;
}

napi_status NapiCastSession::OnInnerEvent(napi_env env, napi_value callback, NapiCastSession *napiSession)
{
    if (napiSession == nullptr) {
        CLOGE("napiSession is null");
        return napi_generic_failure;
    }
    auto napiListener = napiSession->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("cast session manager callback is null");
        return napi_generic_failure;
    }
    if (napiListener->AddCallback(env, NapiCastSessionListener::EVENT_ON_EVENT,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiCastSession::OnDeviceState(napi_env env, napi_value callback, NapiCastSession *napiSession)
{
    if (napiSession == nullptr) {
        CLOGE("napiSession is null");
        return napi_generic_failure;
    }
    auto napiListener = napiSession->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("cast session manager callback is null");
        return napi_generic_failure;
    }
    if (napiListener->AddCallback(env, NapiCastSessionListener::EVENT_DEVICE_STATE,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiCastSession::OffInnerEvent(napi_env env, napi_value callback, NapiCastSession *napiSession)
{
    if (napiSession == nullptr) {
        CLOGE("napiSession is null");
        return napi_generic_failure;
    }
    auto napiListener = napiSession->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("cast session manager callback is null");
        return napi_generic_failure;
    }
    if (napiListener->RemoveCallback(env, NapiCastSessionListener::EVENT_ON_EVENT,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_status NapiCastSession::OffDeviceState(napi_env env, napi_value callback, NapiCastSession *napiSession)
{
    if (napiSession == nullptr) {
        CLOGE("napiSession is null");
        return napi_generic_failure;
    }
    auto napiListener = napiSession->NapiListenerGetter();
    if (!napiListener) {
        CLOGE("cast session manager callback is null");
        return napi_generic_failure;
    }
    if (napiListener->RemoveCallback(env, NapiCastSessionListener::EVENT_DEVICE_STATE,
        callback) != napi_ok) {
        return napi_generic_failure;
    }
    return napi_ok;
}

napi_value NapiCastSession::AddDevice(napi_env env, napi_callback_info info)
{
    CLOGD("Start to add device in");
    struct ConcreteTask : public NapiAsyncTask {
        CastRemoteDevice castRemoteDevice_;
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
        napiAsyntask->castRemoteDevice_ = GetCastRemoteDeviceFromJS(env, argv[0]);
    };
    napiAsyntask->GetJSInfo(env, info, inputParser);
    auto executor = [napiAsyntask]() {
        auto *napiSession = reinterpret_cast<NapiCastSession *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiSession != nullptr, "napiSession is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        shared_ptr<ICastSession> castSession = napiSession->GetCastSession();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, castSession, "ICastSession is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = castSession->AddDevice(napiAsyntask->castRemoteDevice_);
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "AddDevice failed : no permission";
            } else if (ret == ERR_INVALID_PARAM) {
                napiAsyntask->errMessage = "AddDevice failed : invalid parameters";
            } else {
                napiAsyntask->errMessage = "AddDevice failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [env](napi_value &output) { output = GetUndefinedValue(env); };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "AddDevice", executor, complete);
}

napi_value NapiCastSession::RemoveDevice(napi_env env, napi_callback_info info)
{
    CLOGD("Start to remove device in");
    struct ConcreteTask : public NapiAsyncTask {
        string deviceId_;
        DeviceRemoveAction actionType_;
    };
    auto napiAsyntask = std::make_shared<ConcreteTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }

    auto inputParser = [env, napiAsyntask](size_t argc, napi_value *argv) {
        constexpr size_t expectedArgcDeviceId = 1;
        constexpr size_t expectedArgcType = 2;
        CHECK_ARGS_RETURN_VOID(napiAsyntask, (argc == expectedArgcDeviceId || argc == expectedArgcType),
            "invalid arguments", NapiErrors::errcode_[ERR_INVALID_PARAM]);
        napi_valuetype expectedTypes[expectedArgcType] = {napi_string, napi_number};
        bool isParamsTypeValid = CheckJSParamsType(env, argv, argc, expectedTypes);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, isParamsTypeValid, "invalid arguments",
            NapiErrors::errcode_[ERR_INVALID_PARAM]);
        napiAsyntask->deviceId_ = ParseString(env, argv[0]);
        napiAsyntask->actionType_ = (argc == expectedArgcType) ?
            static_cast<DeviceRemoveAction>(ParseInt32(env, argv[1])) : DeviceRemoveAction::ACTION_DISCONNECT;
    };
    napiAsyntask->GetJSInfo(env, info, inputParser);
    auto executor = [napiAsyntask]() {
        auto *napiSession = reinterpret_cast<NapiCastSession *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiSession != nullptr, "napiSession is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        shared_ptr<ICastSession> castSession = napiSession->GetCastSession();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, castSession, "ICastSession is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = castSession->RemoveDevice(napiAsyntask->deviceId_);
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "RemoveDevice failed : no permission";
            } else if (ret == ERR_INVALID_PARAM) {
                napiAsyntask->errMessage = "RemoveDevice failed : invalid parameters";
            } else {
                napiAsyntask->errMessage = "RemoveDevice failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [env](napi_value &output) { output = GetUndefinedValue(env); };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "RemoveDevice", executor, complete);
}

napi_value NapiCastSession::GetSessionId(napi_env env, napi_callback_info info)
{
    CLOGD("Start to get sessionId in");
    struct ConcreteTask : public NapiAsyncTask {
        string sessionId_;
    };
    auto napiAsyntask = std::make_shared<ConcreteTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }

    napiAsyntask->GetJSInfo(env, info);
    auto executor = [napiAsyntask]() {
        auto *napiSession = reinterpret_cast<NapiCastSession *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiSession != nullptr, "napiSession is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        shared_ptr<ICastSession> castSession = napiSession->GetCastSession();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, castSession, "ICastSession is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = castSession->GetSessionId(napiAsyntask->sessionId_);
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "GetSessionId failed : no permission";
            } else {
                napiAsyntask->errMessage = "GetSessionId failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [env, napiAsyntask](napi_value &output) {
        napiAsyntask->status =
            napi_create_string_utf8(env, napiAsyntask->sessionId_.c_str(), NAPI_AUTO_LENGTH, &output);
        CHECK_STATUS_RETURN_VOID(napiAsyntask, "napi_create_string_utf8 failed",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
    };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "GetSessionId", executor, complete);
}

napi_value NapiCastSession::SetSessionProperty(napi_env env, napi_callback_info info)
{
    CLOGD("Start to set sessionProperty in");
    struct ConcreteTask : public NapiAsyncTask {
        CastSessionProperty castSessionProperty_;
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
        napiAsyntask->castSessionProperty_ = GetCastSessionPropertyFromJS(env, argv[0]);
    };
    napiAsyntask->GetJSInfo(env, info, inputParser);
    auto executor = [napiAsyntask]() {
        auto *napiSession = reinterpret_cast<NapiCastSession *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiSession != nullptr, "napiSession is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        shared_ptr<ICastSession> castSession = napiSession->GetCastSession();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, castSession, "ICastSession is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = castSession->SetSessionProperty(napiAsyntask->castSessionProperty_);
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "SetSessionProperty failed : no permission";
            } else if (ret == ERR_INVALID_PARAM) {
                napiAsyntask->errMessage = "SetSessionProperty failed : invalid parameters";
            } else {
                napiAsyntask->errMessage = "SetSessionProperty failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [env](napi_value &output) { output = GetUndefinedValue(env); };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "SetSessionProperty", executor, complete);
}

napi_value NapiCastSession::CreateMirrorPlayer(napi_env env, napi_callback_info info)
{
    CLOGD("Start to create mirror Player");
    struct ConcreteTask : public NapiAsyncTask {
        shared_ptr<IMirrorPlayer> player_;
    };
    auto napiAsyntask = std::make_shared<ConcreteTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }

    napiAsyntask->GetJSInfo(env, info);
    auto executor = [napiAsyntask]() {
        auto *napiSession = reinterpret_cast<NapiCastSession *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiSession != nullptr, "napiSession is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        shared_ptr<ICastSession> castSession = napiSession->GetCastSession();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, castSession, "ICastSession is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = castSession->CreateMirrorPlayer(napiAsyntask->player_);
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "CreateMirrorPlayer failed : no permission";
            } else {
                napiAsyntask->errMessage = "CreateMirrorPlayer failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [napiAsyntask](napi_value &output) {
        napiAsyntask->status =
            NapiMirrorPlayer::CreateNapiMirrorPlayer(napiAsyntask->env, napiAsyntask->player_, output);
        CHECK_STATUS_RETURN_VOID(napiAsyntask, "convert native object to javascript object failed",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
    };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "CreateMirrorPlayer", executor, complete);
}

napi_value NapiCastSession::CreateStreamPlayer(napi_env env, napi_callback_info info)
{
    CLOGD("Start to create Stream Player");
    struct ConcreteTask : public NapiAsyncTask {
        shared_ptr<IStreamPlayer> player_;
    };
    auto napiAsyntask = std::make_shared<ConcreteTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }

    napiAsyntask->GetJSInfo(env, info);
    auto executor = [napiAsyntask]() {
        auto *napiSession = reinterpret_cast<NapiCastSession *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiSession != nullptr, "napiSession is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        shared_ptr<ICastSession> castSession = napiSession->GetCastSession();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, castSession, "ICastSession is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = castSession->CreateStreamPlayer(napiAsyntask->player_);
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "CreateStreamPlayer failed : no permission";
            } else {
                napiAsyntask->errMessage = "CreateStreamPlayer failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [napiAsyntask](napi_value &output) {
        napiAsyntask->status =
            NapiStreamPlayer::CreateNapiStreamPlayer(napiAsyntask->env, napiAsyntask->player_, output);
        CHECK_STATUS_RETURN_VOID(napiAsyntask, "convert native object to javascript object failed",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
    };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "CreateStreamPlayer", executor, complete);
}

napi_value NapiCastSession::SetCastMode(napi_env env, napi_callback_info info)
{
    CLOGD("Start to set cast mode in");
    struct ConcreteTask : public NapiAsyncTask {
        CastMode castMode_;
        string jsonParam_;
    };
    auto napiAsyntask = std::make_shared<ConcreteTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }

    auto inputParser = [env, napiAsyntask](size_t argc, napi_value *argv) {
        constexpr size_t expectedArgc = 2;
        CHECK_ARGS_RETURN_VOID(napiAsyntask, argc == expectedArgc, "invalid arguments",
            NapiErrors::errcode_[ERR_INVALID_PARAM]);
        napi_valuetype expectedTypes[expectedArgc] = { napi_number, napi_string };
        bool isParamsTypeValid = CheckJSParamsType(env, argv, expectedArgc, expectedTypes);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, isParamsTypeValid, "invalid arguments",
            NapiErrors::errcode_[ERR_INVALID_PARAM]);
        napiAsyntask->castMode_ = static_cast<CastMode>(ParseInt32(env, argv[0]));
        napiAsyntask->jsonParam_ = ParseString(env, argv[1]);
    };
    napiAsyntask->GetJSInfo(env, info, inputParser);
    auto executor = [napiAsyntask]() {
        auto *napiSession = reinterpret_cast<NapiCastSession *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiSession != nullptr, "napiSession is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        shared_ptr<ICastSession> castSession = napiSession->GetCastSession();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, castSession, "ICastSession is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = castSession->SetCastMode(napiAsyntask->castMode_, napiAsyntask->jsonParam_);
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "SetCastMode failed : no permission";
            } else if (ret == ERR_INVALID_PARAM) {
                napiAsyntask->errMessage = "SetCastMode failed : invalid parameters";
            } else {
                napiAsyntask->errMessage = "SetCastMode failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };

    auto complete = [env](napi_value &output) { output = GetUndefinedValue(env); };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "SetCastMode", executor, complete);
}

napi_value NapiCastSession::Release(napi_env env, napi_callback_info info)
{
    CLOGD("Start to release in");
    auto napiAsyntask = std::make_shared<NapiAsyncTask>();
    if (napiAsyntask == nullptr) {
        CLOGE("Create NapiAsyncTask failed");
        return GetUndefinedValue(env);
    }
    napiAsyntask->GetJSInfo(env, info);

    auto executor = [napiAsyntask]() {
        auto *napiSession = reinterpret_cast<NapiCastSession *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiSession != nullptr, "napiSession is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        shared_ptr<ICastSession> castSession = napiSession->GetCastSession();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, castSession, "ICastSession is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = castSession->Release();
        if (ret == CAST_ENGINE_SUCCESS) {
            napiSession->Reset();
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

napi_value NapiCastSession::GetRemoteDeviceInfo(napi_env env, napi_callback_info info)
{
    CLOGD("Start to get remote deviceInfo in");
    struct ConcreteTask : public NapiAsyncTask {
        string sessionId_;
        CastRemoteDevice castRemoteDevice_;
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
        napiAsyntask->sessionId_ = ParseString(env, argv[0]);
    };

    napiAsyntask->GetJSInfo(env, info, inputParser);
    auto executor = [napiAsyntask]() {
        auto *napiSession = reinterpret_cast<NapiCastSession *>(napiAsyntask->native);
        CHECK_ARGS_RETURN_VOID(napiAsyntask, napiSession != nullptr, "napiSession is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        shared_ptr<ICastSession> castSession = napiSession->GetCastSession();
        CHECK_ARGS_RETURN_VOID(napiAsyntask, castSession, "ICastSession is null",
            NapiErrors::errcode_[CAST_ENGINE_ERROR]);
        int32_t ret = castSession->GetRemoteDeviceInfo(napiAsyntask->sessionId_, napiAsyntask->castRemoteDevice_);
        if (ret != CAST_ENGINE_SUCCESS) {
            if (ret == ERR_NO_PERMISSION) {
                napiAsyntask->errMessage = "GetRemoteDeviceInfo failed : no permission";
            } else if (ret == ERR_INVALID_PARAM) {
                napiAsyntask->errMessage = "GetRemoteDeviceInfo failed : invalid parameters";
            } else {
                napiAsyntask->errMessage = "GetRemoteDeviceInfo failed : native server exception";
            }
            napiAsyntask->status = napi_generic_failure;
            napiAsyntask->errCode = NapiErrors::errcode_[ret];
        }
    };
    auto complete = [env, napiAsyntask](napi_value &output) {
        output = ConvertCastRemoteDeviceToJS(env, napiAsyntask->castRemoteDevice_);
    };
    return NapiAsyncWork::Enqueue(env, napiAsyntask, "GetRemoteDeviceInfo", executor, complete);
}

std::shared_ptr<NapiCastSessionListener> NapiCastSession::NapiListenerGetter()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return listener_;
}

void NapiCastSession::NapiListenerSetter(std::shared_ptr<NapiCastSessionListener> listener)
{
    std::lock_guard<std::mutex> lock(mutex_);
    listener_ = listener;
}
} // namespace CastEngineClient
} // namespace CastEngine
} // namespace OHOS