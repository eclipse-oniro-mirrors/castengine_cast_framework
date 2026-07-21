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
 * Description: rtsp network management
 * Author: dingkang
 * Create: 2022-01-24
 */
#ifndef LIBCASTENGINE_RTSP_CHANNEL_MANAGER_H
#define LIBCASTENGINE_RTSP_CHANNEL_MANAGER_H

#include <mutex>

#include "channel.h"
#include "message.h"
#include "rtsp_listener_inner.h"
#include "cast_engine_common.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineService {
namespace CastSessionRtsp {
class RtspChannelManager : public Message, public std::enable_shared_from_this<RtspChannelManager> {
public:
    RtspChannelManager(std::shared_ptr<RtspListenerInner> listener, ProtocolType protocolType);
    ~RtspChannelManager();

    void OnConnected(ChannelLinkType channelLinkType);
    void OnData(const uint8_t *data, unsigned int length);
    void OnError(const std::string &errorCode);
    void OnClosed(const std::string &errorCode);

    bool StartSession(const uint8_t *sessionKey, uint32_t sessionKeyLength);
    void StopSession();

    void AddChannel(std::shared_ptr<Channel> channel, const CastInnerRemoteDevice &device);
    void RemoveChannel(std::shared_ptr<Channel> channel);
    std::shared_ptr<IChannelListener> GetChannelListener();

    bool SendRtspData(const std::string &request);
    void SetNegAlgorithmId(int algorithmId);

private:
    enum class RtspState {
        BASE = 0,
        MSG_RTSP_START,
        MSG_RTSP_DATA,
        MSG_RTSP_MSG,
        MSG_RTSP_CLOSE,
        MSG_SEND_KA,
        MSG_KA_TIMEOUT,
        MSG_NEG_TIMEOUT
    };

    class ChannelListener : public IChannelListener {
    public:
        explicit ChannelListener(std::shared_ptr<RtspChannelManager> channelManager) : channelManager_(channelManager)
        {}
        ~ChannelListener() {}

        void OnDataReceived(const uint8_t *buffer, unsigned int length, long timeCost) final;

    private:
        std::weak_ptr<RtspChannelManager> channelManager_;
    };

    constexpr static int SESSION_KEY_LENGTH = 16;

    bool SendData(const std::string &dataFrame);

    uint8_t sessionKeys_[SESSION_KEY_LENGTH] = {0};
    uint32_t sessionKeyLength_{ 0 };
    std::weak_ptr<RtspListenerInner> listener_;
    bool isSessionActive_{ false };
    std::shared_ptr<Channel> channel_;
    std::shared_ptr<ChannelListener> channelListener_;
    int algorithmId_{ 0 };
    ProtocolType protocolType_;
    std::mutex mutex_;
};
} // namespace CastSessionRtsp
} // namespace CastEngineService
} // namespace CastEngine
} // namespace OHOS
#endif // LIBCASTENGINE_RTSP_CHANNEL_MANAGER_H