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
 * Description: softbus connection.
 * Author: sunhong
 * Create: 2022-01-22
 */

#include "softbus_connection.h"

#include <algorithm>
#include <chrono>
#include <numeric>
#include <random>
#include <thread>
#include <cinttypes>

#include "cast_device_data_manager.h"
#include "cast_engine_log.h"
#include "securec.h"
#include "transport.h"
#include "utils.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineService {
DEFINE_CAST_ENGINE_LABEL("Cast-SoftBusConnection");

const std::string SoftBusConnection::PACKAGE_NAME = "CastEngineService";
const std::string SoftBusConnection::AUTH_SESSION_NAME_FACTOR = "AUTH";
const std::string SoftBusConnection::RTSP_SESSION_NAME_FACTOR = "RTSP";
const std::string SoftBusConnection::RTCP_SESSION_NAME_FACTOR = "RTCP";
const std::string SoftBusConnection::VIDEO_SESSION_NAME_FACTOR = "VIDEO";
const std::string SoftBusConnection::AUDIO_SESSION_NAME_FACTOR = "AUDIO";
const std::string SoftBusConnection::CONTROL_SESSION_NAME_FACTOR = "CONTROL";
const std::string SoftBusConnection::FILES_SESSION_NAME_FACTOR = "FILES";
const std::string SoftBusConnection::BYTES_SESSION_NAME_FACTOR = "BYTES";
const std::string SoftBusConnection::STREAM_SESSION_NAME_FACTOR = "CAST_STREAM";
const std::string SoftBusConnection::SESSION_NAME_PREFIX = "CastPlusNetSession";

IFileSendListener SoftBusConnection::fileSendListener_ {
    .OnSendFileProcess = SoftBusConnection::OnSendFileProcess,
    .OnSendFileFinished = SoftBusConnection::OnSendFileFinished,
    .OnFileTransError = SoftBusConnection::OnFileTransError
};

IFileReceiveListener SoftBusConnection::fileReceiveListener_ = {
    .OnReceiveFileStarted = SoftBusConnection::OnReceiveFileStarted,
    .OnReceiveFileProcess = SoftBusConnection::OnReceiveFileProcess,
    .OnReceiveFileFinished = SoftBusConnection::OnReceiveFileFinished,
    .OnFileTransError = SoftBusConnection::OnFileTransError
};


std::unordered_map<std::string, std::shared_ptr<SoftBusConnection>> SoftBusConnection::connectionMap_;

std::mutex SoftBusConnection::connectionMapMtx_;

SoftBusConnection::SoftBusConnection() : isActivelyOpen_(false), isPassiveClose_(false)
{
    CLOGV("SoftBusConnection Construct Enter.");
}

SoftBusConnection::~SoftBusConnection()
{
    CLOGV("Enter.");
    CloseConnection();
}

std::pair<bool, std::shared_ptr<SoftBusConnection>> SoftBusConnection::GetConnection(int sessionId)
{
    std::shared_ptr<SoftBusConnection> conn;
    auto ret = std::make_pair(false, conn);

    std::string mySessionName = SoftBusWrapper::GetSoftBusMySessionName(sessionId);
    if (mySessionName.empty()) {
        CLOGE("Find mySessionName Failed in GetConnection, sessionId = %{public}d.", sessionId);
        return ret;
    }
    return GetConnection(mySessionName);
}

std::pair<bool, std::shared_ptr<SoftBusConnection>> SoftBusConnection::GetConnection(std::string sessionName)
{
    std::lock_guard<std::mutex> lg(connectionMapMtx_);
    auto iter = connectionMap_.find(sessionName);
    if (iter == connectionMap_.end()) {
        CLOGE("Find Conn Failed in GetConnection, mySessionName = %{public}s.", sessionName.c_str());
        return std::make_pair(false, nullptr);
    }
    return std::make_pair(true, connectionMap_[sessionName]);
}

int SoftBusConnection::StartConnection(const ChannelRequest &request, std::shared_ptr<IChannelListener> channelListener)
{
    CLOGD("SoftBus Start Connection Enter.");
    StashRequest(request);
    StashConnectionInfo(request);
    SetRequest(request);
    SetListener(channelListener);
    SetActivelyOpenFlag(true);

    std::thread(&SoftBusConnection::SetupSession, this, channelListener, shared_from_this()).detach();
    return request.remoteDeviceInfo.sessionId;
}

int SoftBusConnection::StartListen(const ChannelRequest &request, std::shared_ptr<IChannelListener> channelListener)
{
    CLOGD("SoftBus Start Listen Enter.");
    StashRequest(request);
    StashConnectionInfo(request);
    SetRequest(request);
    SetListener(channelListener);
    SetActivelyOpenFlag(false);

    startConnectTime_ = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now())
                            .time_since_epoch()
                            .count();
    std::string mySessionName = softbus_.GetSpecMySessionName();
    int ret = SoftBusWrapper::StartSoftBusService(PACKAGE_NAME, mySessionName, &sessionListener_);
    if (ret < RET_OK) {
        CLOGE("StartSoftBusService Failed When Listening. mySessionName = %{public}s, ret = %{public}d.",
            mySessionName.c_str(), ret);
    }

    std::lock_guard<std::mutex> lg(connectionMapMtx_);
    connectionMap_[mySessionName] = shared_from_this();

    return request.remoteDeviceInfo.sessionId;
}

int SoftBusConnection::OnConnectionSessionOpened(int sessionId, int result)
{
    CLOGI("In, sessionId = %{public}d, result = %{public}d.", sessionId, result);
    auto ret = GetConnection(sessionId);
    if (!ret.first) {
        CLOGE("Get Connection Failed, sessionId = %{public}d, result = %{public}d.", sessionId, result);
        return RET_ERR;
    }

    std::shared_ptr<SoftBusConnection> softBusConn = ret.second;

    softBusConn->SetPassiveCloseFlag(false);
    bool isActivelyOpen = softBusConn->GetActivelyOpenFlag();
    std::string sessionName = softBusConn->GetSoftBus().GetSpecMySessionName();
    CLOGD("SessionName = %{public}s, isActivelyOpen = %{public}d.", sessionName.c_str(), isActivelyOpen);

    if (!isActivelyOpen) {
        softBusConn->GetSoftBus().SetSessionId(sessionId);
    }

    time_t currentTimestamp = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now())
                                  .time_since_epoch()
                                  .count();
    CLOGI("Open Session Succ, sessionName = %{public}s, sessionId = %{public}d", sessionName.c_str(), sessionId);
    CLOGI("costTime = %{public}" PRIu64 "ms", (currentTimestamp - softBusConn->startConnectTime_));
    softBusConn->listener_->OnConnectionOpened(softBusConn);
    return RET_OK;
}

void SoftBusConnection::OnConnectionSessionClosed(int sessionId)
{
    CLOGI("SoftBusConnection OnConnectionSessionClosed Enter, sessionId = %{public}d.", sessionId);
    auto ret = GetConnection(sessionId);
    if (!ret.first) {
        CLOGE("OnConnectionSessionClosed, Get Connection Failed, sessionId = %{public}d.", sessionId);
        return;
    }

    auto softBusConn = ret.second;
    softBusConn->SetPassiveCloseFlag(true);
    softBusConn->CloseConnection();

    softBusConn->listener_->OnConnectionClosed(softBusConn);
}

void SoftBusConnection::OnConnectionMessageReceived(int sessionId, const void *data, unsigned int dataLen)
{
    CLOGD("SoftBusConnection OnConnectionMessageReceived Enter, sessionId = %{public}d.", sessionId);
}

void SoftBusConnection::OnConnectionBytesReceived(int sessionId, const void *data, unsigned int dataLen)
{
    CLOGD("In, sessionId = %{public}d, dataLen = %{public}u.", sessionId, dataLen);
    if (data == nullptr || dataLen == 0 || sessionId == INVALID_ID) {
        CLOGE("Wrong params, sessionId = %{public}d, datatLen = %{public}u.", sessionId, dataLen);
        return;
    }

    auto connection = GetConnection(sessionId);
    if (!connection.first) {
        CLOGE("Get Connection Failed, sessionId = %{public}d", sessionId);
        return;
    }

    std::shared_ptr<SoftBusConnection> softBusConn = connection.second;
    std::shared_ptr<IChannelListener> channelListener = softBusConn->GetListener();
    if (!channelListener) {
        CLOGE("channelListener is null, sessionId = %{public}d", sessionId);
        return;
    }

    channelListener->OnDataReceived(reinterpret_cast<const uint8_t *>(data), dataLen, 0);
    CLOGD("Out, sessionId = %{public}d, channelListener refCnt = %{public}ld.", sessionId, channelListener.use_count());
    return;
}

void SoftBusConnection::OnConnectionStreamReceived(int sessionId, const StreamData *data, const StreamData *ext,
    const StreamFrameInfo *param)
{
    CLOGD("In, sessionId = %{public}d.", sessionId);
    if (data == nullptr || data->buf == nullptr || data->bufLen <= 0) {
        CLOGE("Wrong params, sessionId = %{public}d", sessionId);
        return;
    }

    auto connection = GetConnection(sessionId);
    if (!connection.first) {
        CLOGE("Get Connection Failed, sessionId = %{public}d.", sessionId);
        return;
    }

    std::shared_ptr<SoftBusConnection> softBusConn = connection.second;
    std::shared_ptr<IChannelListener> channelListener = softBusConn->GetListener();

    if (!channelListener) {
        CLOGE("ChannelListener is null, sessionId = %{public}d.", sessionId);
        return;
    }

    channelListener->OnDataReceived(reinterpret_cast<uint8_t *>(data->buf), static_cast<unsigned int>(data->bufLen), 0);
    CLOGD("Out, channelListener refCnt = %{public}ld, length = %{public}d.", channelListener.use_count(), data->bufLen);
}

void SoftBusConnection::OnConnectionFileReceived(int32_t socket, FileEvent *event)
{
    if (event == nullptr) {
        CLOGE("event is null, sessionId = %{public}d", socket);
        return;
    }
    FileEventType type = event->type;
    switch (type) {
        case FILE_EVENT_RECV_UPDATE_PATH: {
            // fix me event->UpdateRecvPath UpdateRecvFilePath
            break;
        }
        case FILE_EVENT_SEND_PROCESS: {
            OnSendFileProcess(socket, event->bytesProcessed, event->bytesTotal);
            break;
        }
        case FILE_EVENT_SEND_FINISH: {
            OnSendFileFinished(socket, (event->files)[0]);
            break;
        }
        case FILE_EVENT_SEND_ERROR: {
            OnFileTransError(socket);
            break;
        }
        case FILE_EVENT_RECV_START: {
            OnReceiveFileStarted(socket, (event->files)[0], event->fileCnt);
            break;
        }
        case FILE_EVENT_RECV_PROCESS: {
            OnReceiveFileProcess(socket, (event->files)[0], event->bytesProcessed, event->bytesTotal);
            break;
        }
        case FILE_EVENT_RECV_FINISH: {
            OnReceiveFileFinished(socket, (event->files)[0], event->fileCnt);
            break;
        }
        case FILE_EVENT_RECV_ERROR: {
            OnFileTransError(socket);
            break;
        }
        default: {
            break;
        }
    }
}

void SoftBusConnection::OnConnectionSessionEvent(int sessionId, int eventId, int tvCount, const QosTv *tvList)
{
    CLOGD("SoftBusConnection OnConnectionSessionEvent Enter, sessionId = %{public}d eventId = %{public}d.", sessionId,
        eventId);
}

/*
 * Softbus file trans callback function
 */
int SoftBusConnection::OnSendFileProcess(int sessionId, uint64_t bytesUpload, uint64_t bytesTotal)
{
    auto ret = GetConnection(sessionId);
    if (!ret.first) {
        CLOGE("OnSendFileProcess, Get Connection Failed, sessionId = %d.", sessionId);
        return RET_ERR;
    }

    std::shared_ptr<SoftBusConnection> softBusConn = ret.second;
    float percent = (static_cast<float>(bytesUpload) / bytesTotal) * 100;
    std::shared_ptr<IChannelListener> channelListener = softBusConn->GetListener();
    if (!channelListener) {
        CLOGE("channelListener is null, sessionId = %d.", sessionId);
        return RET_ERR;
    }
    channelListener->OnSendFileProcess(static_cast<int>(floor(percent)));

    return RET_OK;
}

int SoftBusConnection::OnSendFileFinished(int sessionId, const char *firstFile)
{
    CLOGD("OnSendFileFinished invoked, firstFile is: %{public}s", Utils::Mask(std::string(firstFile)).c_str());
    auto ret = GetConnection(sessionId);
    if (!ret.first) {
        CLOGE("OnSendFileFinished, Get Connection Failed, sessionId = %d.", sessionId);
        return RET_ERR;
    }

    std::shared_ptr<SoftBusConnection> softBusConn = ret.second;
    int percent = 100;
    std::shared_ptr<IChannelListener> channelListener = softBusConn->GetListener();
    if (!channelListener) {
        CLOGE("channelListener is null, sessionId = %d.", sessionId);
        return RET_ERR;
    }
    channelListener->OnFilesSent(std::string(firstFile), percent);
    return RET_OK;
}

void SoftBusConnection::OnFileTransError(int sessionId)
{
    CLOGE("OnFileTransError invoked, session id is: %d", sessionId);
    auto ret = GetConnection(sessionId);
    if (!ret.first) {
        CLOGE("OnFileTransError, Get Connection Failed, sessionId = %d.", sessionId);
        return;
    }

    std::shared_ptr<SoftBusConnection> softBusConn = ret.second;
    std::shared_ptr<IChannelListener> channelListener = softBusConn->GetListener();
    if (!channelListener) {
        CLOGE("channelListener is null, sessionId = %d.", sessionId);
        return;
    }
    channelListener->OnFileTransError();
}

/*
 * Softbus file recv callback function
 */
int SoftBusConnection::OnReceiveFileStarted(int sessionId, const char *files, int fileCnt)
{
    CLOGD("OnReceiveFileStarted invoked, files is: %{public}s, fileCnt: %d",
        Utils::Mask(std::string(files)).c_str(), fileCnt);
    return RET_OK;
}

int SoftBusConnection::OnReceiveFileProcess(int sessionId, const char *firstFile, uint64_t bytesUpload,
    uint64_t bytesTotal)
{
    return RET_OK;
}

void SoftBusConnection::OnReceiveFileFinished(int sessionId, const char *files, int fileCnt)
{
    CLOGD("OnReceiveFileFinished invoked, files is: %{public}s fileCnt: %d",
        Utils::Mask(std::string(files)).c_str(), fileCnt);
    auto ret = GetConnection(sessionId);
    if (!ret.first) {
        CLOGE("OnReceiveFileFinished, Get Connection Failed, sessionId = %d.", sessionId);
        return;
    }
    std::shared_ptr<SoftBusConnection> softBusConn = ret.second;
    int percent = 100;
    std::shared_ptr<IChannelListener> channelListener = softBusConn->GetListener();
    if (!channelListener) {
        CLOGE("channelListener is null, sessionId = %d.", sessionId);
        return;
    }
    channelListener->OnFilesReceived(std::string(files), percent);
    return;
}

int SoftBusConnection::SetupSession(std::shared_ptr<IChannelListener> channelListener,
    std::shared_ptr<SoftBusConnection> hold)
{
    std::string mySessionName = softbus_.GetSpecMySessionName();
    CLOGD("SetupSession In, mySessionName = %{public}s.%{public}s", mySessionName.c_str(),
        Utils::Mask(channelRequest_.remoteDeviceInfo.deviceId).c_str());
    int ret = SoftBusWrapper::StartSoftBusService(PACKAGE_NAME, mySessionName, &sessionListener_);
    if (ret != RET_OK) {
        CLOGE("StartSoftBusService Failed In SetupSession, mySessionName = %{public}s, ret = %{public}d.",
            mySessionName.c_str(), ret);
        listener_->OnConnectionConnectFailed(channelRequest_, RET_ERR);
        return RET_ERR;
    }

    // server/source
    int sessionId = -1;

    auto networkId = CastDeviceDataManager::GetInstance().GetDeviceNetworkId(channelRequest_.remoteDeviceInfo.deviceId);
    if (networkId == std::nullopt) {
        CLOGE("networkId is null");
        listener_->OnConnectionConnectFailed(channelRequest_, RET_ERR);
        return RET_ERR;
    }
    sessionId = softbus_.OpenSoftBusSession(networkId.value(), softbus_.GetSpecGroupId());
    CLOGD("SetupSession In, mySessionName = %{public}s, sessionId = %{public}d, remoteNetworkId = %{public}s.",
        mySessionName.c_str(), sessionId, Utils::Mask(networkId.value()).c_str());
    if (sessionId <= 0) {
        CLOGE("Open SoftBus Session Failed, mySessionName = %{public}s, sessionId = %{public}d.", mySessionName.c_str(),
            sessionId);
        listener_->OnConnectionConnectFailed(channelRequest_, RET_ERR);
        return RET_ERR;
    }

    softbus_.SetSessionId(sessionId);

    std::lock_guard<std::mutex> lg(connectionMapMtx_);
    connectionMap_[mySessionName] = shared_from_this();

    return RET_OK;
}

void SoftBusConnection::StashConnectionInfo(const ChannelRequest &request)
{
    // For SoftBus, remotePort in request is remotedevice's sessionId.
    std::string sessionName = CreateSessionName(request.moduleType, request.remoteDeviceInfo.sessionId);
    softbus_.SetMySessionName(sessionName);
    softbus_.SetPeerSessionName(sessionName);
    int sessionType = GetSessionType(request.moduleType);
    // sessionType = TYPE_BYTES; // 规避软总线stream session通道bug，视频流使用bytes类型session进行传输
    softbus_.SetSessionType(sessionType);
    softbus_.SetAttrbute(sessionType);
}

int SoftBusConnection::GetSessionType(ModuleType moduleType) const
{
    CLOGD("In, moduleType = %{public}d.", moduleType);
    switch (moduleType) {
        case ModuleType::AUTH:
        case ModuleType::RTSP:
        case ModuleType::RTCP:
        case ModuleType::REMOTE_CONTROL:
        case ModuleType::STREAM:
            CLOGD("GetSessionType TYPE_BYTES, ModuleType = %{public}d.", moduleType);
            return TYPE_BYTES;
        case ModuleType::AUDIO:
        case ModuleType::VIDEO:
            CLOGD("GetSessionType TYPE_STREAM, ModuleType = %{public}d.", moduleType);
            return TYPE_STREAM;
        case ModuleType::UI_FILES:
            CLOGD("GetSessionType TYPE_FILE, ModuleType = %{public}d.", moduleType);
            return TYPE_FILE;
        case ModuleType::UI_BYTES:
            CLOGD("GetSessionType TYPE_BYTES, ModuleType = %{public}d.", moduleType);
            return TYPE_BYTES;
        default:
            CLOGE("GetSessionType failed, ModuleType = %{public}d.", moduleType);
            return TYPE_BYTES;
    }
}

std::string SoftBusConnection::CreateSessionName(ModuleType moduleType, int sessionId)
{
    CLOGD("In, moduleType = %{public}d, sessionId = %{public}d.", moduleType, sessionId);
    std::string sessionName = SESSION_NAME_PREFIX;
    std::string factor;
    switch (moduleType) {
        case ModuleType::AUTH:
            factor = AUTH_SESSION_NAME_FACTOR;
            break;
        case ModuleType::RTSP:
            factor = RTSP_SESSION_NAME_FACTOR;
            break;
        case ModuleType::RTCP:
            factor = RTCP_SESSION_NAME_FACTOR;
            break;
        case ModuleType::AUDIO:
            factor = AUDIO_SESSION_NAME_FACTOR;
            break;
        case ModuleType::VIDEO:
            factor = VIDEO_SESSION_NAME_FACTOR;
            break;
        case ModuleType::REMOTE_CONTROL:
            factor = CONTROL_SESSION_NAME_FACTOR;
            break;
        case ModuleType::STREAM:
            factor = STREAM_SESSION_NAME_FACTOR;
            break;
        case ModuleType::UI_FILES:
            factor = FILES_SESSION_NAME_FACTOR;
            break;
        case ModuleType::UI_BYTES:
            factor = BYTES_SESSION_NAME_FACTOR;
            break;
        default:
            CLOGE("CreateSessionName failed, ModuleTyp = %{public}d.", moduleType);
            return sessionName;
    }
    sessionName.append(factor);
    sessionName.append(std::to_string(sessionId));
    CLOGD("Out, sessionName = %{public}s.", sessionName.c_str());
    return sessionName;
}

void SoftBusConnection::CloseConnection()
{
    std::string mySessionName = softbus_.GetSpecMySessionName();

    {
        std::lock_guard<std::mutex> lg(connectionMapMtx_);
        if (connectionMap_.count(mySessionName) != 0) {
            CLOGD("Dele Element In connectionMap_, mySessionName = %{public}s.", mySessionName.c_str());
            connectionMap_.erase(mySessionName);
        }
    }

    bool isPassiveClose = GetPassiveCloseFlag();
    if (!isPassiveClose) {
        softbus_.CloseSoftBusSession();
    } else {
        SetPassiveCloseFlag(false);
    }

    SoftBusWrapper::StopService(PACKAGE_NAME, softbus_.GetSpecMySessionName());
    if (GetActivelyOpenFlag()) {
        CLOGD("SoftBus CloseConnection, Sink End.");
        return;
    }
    CLOGD("SoftBus CloseConnection, Source End.");
}

bool SoftBusConnection::Send(const uint8_t *buf, int bufLen)
{
    if (buf == nullptr || bufLen <= 0) {
        CLOGE("Data or length is illegal.");
        return false;
    }

    int ret;
    if (softbus_.GetSessionType() == TYPE_BYTES) {
        CLOGD("SoftBus Send bytes.");
        ret = softbus_.SendSoftBusBytes(buf, bufLen);
    } else {
        CLOGD("SoftBus Send stream.");
        ret = softbus_.SendSoftBusStream(buf, bufLen);
    }
    return (ret == 0) ? true : false;
}

SoftBusWrapper &SoftBusConnection::GetSoftBus()
{
    SoftBusWrapper &softbus = softbus_;
    return softbus;
}

bool SoftBusConnection::GetActivelyOpenFlag() const
{
    return isActivelyOpen_;
}

void SoftBusConnection::SetActivelyOpenFlag(bool isActivelyOpen)
{
    isActivelyOpen_ = isActivelyOpen;
}

bool SoftBusConnection::GetPassiveCloseFlag() const
{
    return isPassiveClose_;
}

void SoftBusConnection::SetPassiveCloseFlag(bool isPassiveClose)
{
    isPassiveClose_ = isPassiveClose;
}
} // namespace CastEngineService
} // namespace CastEngine
} // namespace OHOS