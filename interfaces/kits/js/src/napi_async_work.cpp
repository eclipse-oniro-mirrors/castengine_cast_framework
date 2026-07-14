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
 * Description: supply napi async work realization for interfaces.
 * Author: zhangjingnan
 * Create: 2023-4-11
 */

#include "napi_async_work.h"
#include "napi_castengine_utils.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineClient {
DEFINE_CAST_ENGINE_LABEL("Cast-Napi-AsyncWork");

NapiAsyncTask::~NapiAsyncTask()
{
    CLOGD("no memory leak after callback or promise[resolved/rejected]");
    if (env != nullptr) {
        if (work != nullptr) {
            NAPI_CALL_RETURN_VOID(env, napi_delete_async_work(env, work));
        }
        if (callbackRef != nullptr) {
            NAPI_CALL_RETURN_VOID(env, napi_delete_reference(env, callbackRef));
        }
        env = nullptr;
    }
}

void NapiAsyncTask::GetJSInfo(napi_env envi, napi_callback_info info, NapiCbInfoParser parser, bool sync)
{
    env = envi;
    size_t argc = ARGC_MAX;
    napi_value argv[ARGC_MAX] = {nullptr};
    napi_value thisVar = nullptr;
    status = napi_get_cb_info(envi, info, &argc, argv, &thisVar, nullptr);
    if (status != napi_ok || argc > ARGC_MAX) {
        CLOGE("napi_get_cb_info failed");
        return;
    }
    status = napi_unwrap(envi, thisVar, &native);
    if (status != napi_ok) {
        CLOGE("napi_unwrap failed");
        return;
    }
    if (native == nullptr) {
        CLOGD("thisVar is null");
    }

    if (!sync && argc > 0) {
        // get the last arguments :: <callback>
        size_t index = argc - 1;
        napi_valuetype type = napi_undefined;
        status = napi_typeof(envi, argv[index], &type);
        if ((status == napi_ok) && (type == napi_function)) {
            status = napi_create_reference(envi, argv[index], 1, &callbackRef);
            if (status != napi_ok) {
                CLOGE("napi_get_cb_info failed");
                return;
            }
            argc = index;
            CLOGD("async callback, no promise");
        } else {
            CLOGD("no callback, async promise");
        }
    }

    if (parser) {
        CLOGD("input parser exists");
        parser(argc, argv);
    }
}

napi_value NapiAsyncWork::Enqueue(napi_env env, std::shared_ptr<NapiAsyncTask> napiAsyncTask, const std::string &name,
    NapiAsyncExecute execute, NapiAsyncComplete complete)
{
    CLOGI("name=%{public}s", name.c_str());
    napiAsyncTask->execute = std::move(execute);
    napiAsyncTask->complete = std::move(complete);
    napiAsyncTask->taskName = name;
    napi_value promise = nullptr;
    if (napiAsyncTask->callbackRef == nullptr) {
        NAPI_CALL(napiAsyncTask->env, napi_create_promise(napiAsyncTask->env, &napiAsyncTask->deferred, &promise));
        CLOGD("create deferred promise");
    } else {
        NAPI_CALL(napiAsyncTask->env, napi_get_undefined(napiAsyncTask->env, &promise));
    }

    napi_value resourceName = nullptr;
    NAPI_CALL(napiAsyncTask->env,
        napi_create_string_utf8(napiAsyncTask->env, name.c_str(), NAPI_AUTO_LENGTH, &resourceName));
    napi_create_async_work(
        napiAsyncTask->env, nullptr, resourceName,
        [](napi_env env, void *data) {
            if (data == nullptr) {
                CLOGE("napi_async_execute_callback nullptr");
                return;
            }
            auto task = reinterpret_cast<NapiAsyncTask *>(data);
            CLOGD("napi_async_execute_callback status=%{public}d", task->status);
            if (task->execute && task->status == napi_ok) {
                task->execute();
            }
            task->execute = nullptr;
        },
        [](napi_env env, napi_status status, void *data) {
            if (data == nullptr) {
                CLOGE("napi_async_complete_callback nullptr");
                return;
            }
            auto task = reinterpret_cast<NapiAsyncTask *>(data);
            CLOGD("napi_async_complete_callback status=%{public}d, status=%{public}d", status, task->status);
            if ((status != napi_ok) && (task->status == napi_ok)) {
                task->status = status;
            }
            if ((task->complete) && (status == napi_ok) && (task->status == napi_ok)) {
                task->complete(task->output);
            }
            task->complete = nullptr;
            GenerateOutput(task);
            task->hold.reset(); // release napiAsyncTask.
        }, reinterpret_cast<void *>(napiAsyncTask.get()), &napiAsyncTask->work);
    NAPI_CALL(napiAsyncTask->env, napi_queue_async_work(napiAsyncTask->env, napiAsyncTask->work));
    napiAsyncTask->hold = napiAsyncTask; // save crossing-thread ctxt.
    return promise;
}

void NapiAsyncWork::GenerateOutput(NapiAsyncTask *napiAsyncTask)
{
    napi_value result[RESULT_ALL] = {nullptr};
    if (napiAsyncTask->status == napi_ok) {
        NAPI_CALL_RETURN_VOID(napiAsyncTask->env, napi_get_undefined(napiAsyncTask->env, &result[RESULT_ERROR]));
        if (napiAsyncTask->output == nullptr) {
            NAPI_CALL_RETURN_VOID(napiAsyncTask->env, napi_get_undefined(napiAsyncTask->env, &napiAsyncTask->output));
        }
        result[RESULT_DATA] = napiAsyncTask->output;
    } else {
        napi_value message = nullptr;
        napi_value code = nullptr;
        NAPI_CALL_RETURN_VOID(napiAsyncTask->env,
            napi_create_string_utf8(napiAsyncTask->env, napiAsyncTask->errMessage.c_str(), NAPI_AUTO_LENGTH, &message));
        NAPI_CALL_RETURN_VOID(napiAsyncTask->env,
            napi_create_error(napiAsyncTask->env, nullptr, message, &result[RESULT_ERROR]));
        NAPI_CALL_RETURN_VOID(napiAsyncTask->env, napi_create_int32(napiAsyncTask->env, napiAsyncTask->errCode, &code));
        NAPI_CALL_RETURN_VOID(napiAsyncTask->env,
            napi_set_named_property(napiAsyncTask->env, result[RESULT_ERROR], "code", code));
        NAPI_CALL_RETURN_VOID(napiAsyncTask->env, napi_get_undefined(napiAsyncTask->env, &result[RESULT_DATA]));
    }

    if (napiAsyncTask->deferred != nullptr) {
        if (napiAsyncTask->status == napi_ok) {
            CLOGD("deferred promise resolved");
            NAPI_CALL_RETURN_VOID(napiAsyncTask->env,
                napi_resolve_deferred(napiAsyncTask->env, napiAsyncTask->deferred, result[RESULT_DATA]));
        } else {
            CLOGD("deferred promise rejected");
            NAPI_CALL_RETURN_VOID(napiAsyncTask->env,
                napi_reject_deferred(napiAsyncTask->env, napiAsyncTask->deferred, result[RESULT_ERROR]));
        }
    } else {
        CallJSFunc(napiAsyncTask->env, napiAsyncTask->callbackRef, RESULT_ALL, result);
    }
}
} // namespace CastEngineClient
} // namespace CastEngine
} // namespace OHOS