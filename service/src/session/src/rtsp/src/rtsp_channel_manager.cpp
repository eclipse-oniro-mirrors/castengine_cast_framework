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
 * Description: rtsp parameter class
 * Author: dingkang
 * Create: 2022-01-28
 */

#include "rtsp_channel_manager.h"

#include "cast_engine_log.h"
#include "encrypt_decrypt.h"
#include "rtsp_basetype.h"
#include "securec.h"
#include "utils.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineService {
namespace CastSessionRtsp {
DEFINE_CAST_ENGINE_LABEL("Cast-Rtsp-Net-Manager");

void RtspChannelManager::ChannelListener::OnDataReceived(const uint8_t *buffer, unsigned int length, long timeCost)
{
    CLOGD("==============Received data length %{public}u timeCost %{public}ld================", length, timeCost);

    auto channelManager = channelManager_.lock();
    if (channelManager == nullptr) {
        CLOGE("channelManager == nullptr");
        return;
    }
    if (!((channelManager->algorithmId_ > 0) &&
        !Utils::IsArrayAllZero(channelManager->sessionKeys_, SESSION_KEY_LENGTH))) {
        CLOGD("==============Not Authed Recv Msg ================");
        CLOGD("Algorithm id %{public}d, length %{public}u.", channelManager->algorithmId_, length);
        channelManager->OnData(buffer, length);
    } else {
        int decryptDataLen = 0;
        auto decryContent =
            EncryptDecrypt::GetInstance().DecryptData(channelManager->algorithmId_, { channelManager->sessionKeys_,
            channelManager->sessionKeyLength_ }, { buffer, static_cast<int>(length) }, decryptDataLen);
        if (!decryContent) {
            CLOGE("ERROR: decode fail, length[%{public}u]", length);
            return;
        }
        CLOGD("==============Authed Recv Msg ================, decryContent length %{public}u", length);
        channelManager->OnData(decryContent.get(), decryptDataLen);
    }
}

RtspChannelManager::RtspChannelManager(std::shared_ptr<RtspListenerInner> listener, ProtocolType protocolType)
    : listener_(listener), protocolType_(protocolType)
{
    CLOGI("Out, ProtocolType:%{public}d", protocolType_);
}

RtspChannelManager::~RtspChannelManager()
{
    CLOGI("In.");
    memset_s(sessionKeys_, SESSION_KEY_LENGTH, 0, SESSION_KEY_LENGTH);
    channelListener_ = nullptr;
}

std::shared_ptr<IChannelListener> RtspChannelManager::GetChannelListener()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (channelListener_ == nullptr) {
        channelListener_ = std::make_shared<ChannelListener>(shared_from_this());
    }
    return channelListener_;
}

void RtspChannelManager::AddChannel(std::shared_ptr<Channel> channel, const CastInnerRemoteDevice &device)
{
    if (channel_ != nullptr) {
        CLOGE("channel exists!");
    }
    channel_ = channel;

    bool isSoftbus = channel->GetRequest().linkType == ChannelLinkType::SOFT_BUS;
    auto listener = listener_.lock();
    if (!listener) {
        CLOGE("listener is nullptr");
        return;
    }
    CLOGD("LinkType %{public}d listener_ is %{public}d", isSoftbus, listener == nullptr);
    listener->OnPeerReady(isSoftbus);
}

void RtspChannelManager::RemoveChannel(std::shared_ptr<Channel> channel)
{
    channel_ = nullptr;
}

bool RtspChannelManager::StartSession(const uint8_t *sessionKey, uint32_t sessionKeyLength)
{
    if (sessionKey != nullptr && sessionKeyLength != 0) {                                                            
        if (memcpy_s(sessionKeys_, sessionKeyLength, sessionKey, sessionKeyLength) != 0) {
            CLOGE("SessionKey Copy Error!");
            return falsel
        }                                                                                                                                       
    }
    isSessionActive_ = true;
    sessionKeyLength_ = sessionKeyLength;
    return true;
}

void RtspChannelManager::StopSession()
{
    CLOGD("Stop session.");
    if (isSessionActive_) {
        memset_s(sessionKeys_, SESSION_KEY_LENGTH, 0, SESSION_KEY_LENGTH);
        isSessionActive_ = false;
        auto listener = listener_.lock();
        if (listener) {
            listener->OnPeerGone();
        }
    }
}

void RtspChannelManager::OnConnected(ChannelLinkType channelLinkType)
{
    auto listener = listener_.lock();
    if (!listener) {
        CLOGE("listener is nullptr");
        return;
    }
    bool isSoftbus = channelLinkType == ChannelLinkType::SOFT_BUS;
    CLOGI("IsSoftbus %{public}d.", isSoftbus);
    listener->OnPeerReady(isSoftbus);
}

void RtspChannelManager::OnData(const uint8_t *data, unsigned int length)
{
    std::string str(reinterpret_cast<const char *>(data), length);
    CLOGD("In, %{public}s %{public}s", (str.find("RTSP/") == 0) ? "Response...\r\n" : "Request...\r\n", str.c_str());
    auto listener = listener_.lock();
    if (!listener) {
        CLOGE("listener is nullptr");
        return;
    }
    RtspParse msg;
    RtspParse::ParseMsg(str, msg);
    if (Utils::StartWith(str, "RTSP/")) {
        listener->OnResponse(msg);
    } else {
        listener->OnRequest(msg);
    }
}

void RtspChannelManager::OnError(const std::string &errorCode)
{
    CLOGI("In, %{public}s.", errorCode.c_str());
    auto listener = listener_.lock();
    if (!listener) {
        CLOGE("listener is nullptr");
        return;
    }
    listener->OnPeerGone();
}

void RtspChannelManager::OnClosed(const std::string &errorCode)
{
    CLOGI("OnClosed %{public}s.", errorCode.c_str());
    auto listener = listener_.lock();
    if (!listener) {
        CLOGE("listener is nullptr");
        return;
    }
    listener->OnPeerGone();
}

bool RtspChannelManager::SendData(const std::string &dataFrame)
{
    auto channel = channel_;
    if (channel == nullptr) {
        CLOGE("SendData, channel is nullptr.");
        return false;
    }
    size_t pktlen = dataFrame.size();
    if (channel->GetRequest().linkType == ChannelLinkType::SOFT_BUS ||
        Utils::IsArrayAllZero(sessionKeys_, SESSION_KEY_LENGTH) || algorithmId_ <= 0) {
        CLOGD("SendData, get data finish.");
        return channel->Send(reinterpret_cast<const uint8_t *>(dataFrame.c_str()), pktlen);
    }
    int encryptedDataLen = 0;
    auto encryptedData = EncryptDecrypt::GetInstance().EncryptData(algorithmId_, { sessionKeys_, sessionKeyLength_ },
        { reinterpret_cast<const uint8_t *>(dataFrame.c_str()), pktlen }, encryptedDataLen);
    if (!encryptedData) {
        CLOGE("Encrypt data failed, pktlen: %{public}zu", pktlen);
        return false;
    }
    CLOGD("SendData, encrypt data finish.");

    CLOGD("SendData, encryptedDataLen %{public}d pktlen %{public}zu.", encryptedDataLen, pktlen);
    return channel->Send(encryptedData.get(), encryptedDataLen);
}

bool RtspChannelManager::SendRtspData(const std::string &request)
{
    CLOGD("Send rtsp data.");
    if (request.empty()) {
        CLOGE("request string is null");
        return false;
    }

    if (!isSessionActive_) {
        CLOGE("IsSessionActive_ %{public}d SendRtspData... %{public}zu", isSessionActive_, request.length());
        return false;
    }

    return SendData(request);
}

void RtspChannelManager::SetNegAlgorithmId(int algorithmId)
{
    algorithmId_ = algorithmId;
    CLOGI("SetNegAlgorithmId algorithmId %{public}d.", algorithmId);
}
} // namespace CastSessionRtsp
} // namespace CastEngineService
} // namespace CastEngine
} // namespace OHOS