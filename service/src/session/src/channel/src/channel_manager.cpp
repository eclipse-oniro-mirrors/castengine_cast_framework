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
 * Description: channel manager
 * Author: sunhong
 * Create: 2022-01-19
 */

#include "channel_manager.h"
#include "cast_engine_log.h"
#include "softbus/softbus_connection.h"
#include "tcp/tcp_connection.h"
#include "utils.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineService {
DEFINE_CAST_ENGINE_LABEL("Cast-ChannelManager");

ChannelManager::ChannelManager(const int sessionIndex, std::shared_ptr<IChannelManagerListener> channelManagerListener)
    : sessionIndex_(sessionIndex), channelManagerListener_(channelManagerListener)
{
    CLOGD("In, sessionId_ = %{public}d, sessionIndex_ = %{public}d.", sessionId_, sessionIndex_);
    connectionListener_ = std::make_shared<ConnectionListenerInner>(channelManagerListener_);
}

ChannelManager::~ChannelManager()
{
    DestroyAllChannels();
};

std::shared_ptr<Connection> ChannelManager::GetConnection(ChannelLinkType linkType)
{
    std::shared_ptr<Connection> connection;
    switch (linkType) {
        case ChannelLinkType::SOFT_BUS:
            connection = std::make_shared<SoftBusConnection>();
            CLOGD("GetConnection, Create SoftBus Connection, linkType = %{public}d.", linkType);
            break;
        case ChannelLinkType::VTP:
        case ChannelLinkType::TCP:
            connection = std::make_shared<TcpConnection>();
            CLOGD("GetConnection, Create Tcp Connection, linkType = %{public}d.", linkType);
            break;
        default:
            CLOGE("Invalid linkType, linkType = %{public}d.", linkType);
            break;
    }
    return connection;
}

/*
 * server (start listen) or client (start connection)
 *
 * ----------------------------------
 * |  VTP  |  TCP  | SoftBus
 * ----------------------------------
 * sink | server| client|  client
 * ----------------------------------
 * source| client| server|  server
 * ----------------------------------
 */
int ChannelManager::CreateChannel(ChannelRequest &request, std::shared_ptr<IChannelListener> channelListener)
{
    CLOGD("CreateChannel Enter.");
    if (channelListener == nullptr || !IsRequestValid(request)) {
        CLOGE("CreateChannel, channelListener is null or request is invalid.");
        return RET_ERR;
    }

    request.connectionId = ++connectionNum_;
    std::shared_ptr<Connection> connection = GetConnection(request.linkType);
    if (!connection) {
        CLOGE("Failed to get connection.");
        return RET_ERR;
    }
    connection->SetConnectionListener(connectionListener_);

    bool isVtp = (request.linkType == ChannelLinkType::VTP);
    bool isSink = (request.sessionProperty.endType == EndType::CAST_SINK);
    int remoteCtrl = -1;
    CLOGI("CreateChannel In, linkType = %{public}d, isVtp = %{public}d, isSink = %{public}d.", request.linkType,
        isVtp, isSink);
    if ((isVtp && isSink) || (!isVtp && !isSink)) {
        CLOGV("CreateChannel In, StartListen.");
        remoteCtrl = connection->StartListen(request, channelListener);
    } else {
        CLOGV("CreateChannel In, StartConnection.");
        remoteCtrl = connection->StartConnection(request, channelListener);
    }
    if (!remoteCtrl) {
        std::lock_guard<std::mutex> lg(connectionMapMtx_);
        connectionMap_.insert(std::pair<ChannelRequest, std::shared_ptr<Connection>>(request, connection));
    }
    return remoteCtrl;
}

int ChannelManager::CreateChannel(ChannelRequest &request, std::shared_ptr<IChannelListener> channelListener,
    ChannelFileSchema &fileSchema)
{
    CLOGD("CreateChannel Enter.");
    if (channelListener == nullptr || !IsRequestValid(request)) {
        CLOGE("CreateChannel, channelListener is null or request is invalid.");
        return RET_ERR;
    }

    request.connectionId = ++connectionNum_;
    std::shared_ptr<Connection> connection = GetConnection(request.linkType);
    if (!connection) {
        CLOGE("Failed to get connection.");
        return RET_ERR;
    }
    connection->SetConnectionListener(connectionListener_);

    bool isVtp = (request.linkType == ChannelLinkType::VTP);
    bool isSink = (request.sessionProperty.endType == EndType::CAST_SINK);
    int remoteCtrl = -1;
    CLOGI("CreateChannel In, linkType = %{public}d, isVtp = %{public}d, isSink = %{public}d.", request.linkType,
        isVtp, isSink);
    if ((isVtp && isSink) || (!isVtp && !isSink)) {
        CLOGV("CreateChannel In, StartConnection.");
        remoteCtrl = connection->StartConnection(request, channelListener);
    } else {
        CLOGV("CreateChannel In, StartListen.");
        remoteCtrl = connection->StartListen(request, channelListener);
    }
    if (!remoteCtrl) {
        std::lock_guard<std::mutex> lg(connectionMapMtx_);
        connectionMap_.insert(std::pair<ChannelRequest, std::shared_ptr<Connection>>(request, connection));
    }
    return remoteCtrl;
}

bool ChannelManager::IsRequestValid(const ChannelRequest &request) const
{
    if (request.linkType != ChannelLinkType::SOFT_BUS && request.remoteDeviceInfo.ipAddress.empty()) {
        CLOGE("linkType is not SoftBus and remoteIp is empty, linkType = %{public}d.", request.linkType);
        return false;
    }

    if (request.linkType == ChannelLinkType::SOFT_BUS && request.remoteDeviceInfo.deviceId.empty()) {
        CLOGE("linkType is SoftBus and remoteDeviceId is empty.");
        return false;
    }
    CLOGD("IsRequestValid In, remoteDeviceId = %{public}s.", Utils::Mask(request.remoteDeviceInfo.deviceId).c_str());
    return true;
}

bool ChannelManager::DestroyChannel(const Channel &channel)
{
    CLOGD("DestroyChannel Enter, Specify specific channel.");

    std::lock_guard<std::mutex> lg(connectionMapMtx_);
    auto it = connectionMap_.find(channel.GetRequest());
    if (it != connectionMap_.end()) {
        std::shared_ptr<Connection> connection = it->second;
        connection->CloseConnection();
        connectionMap_.erase(it);
        return true;
    } else {
        CLOGE("DestroyChannel, Can't find Channel.");
        return false;
    }
}

bool ChannelManager::DestroyChannel(ModuleType moduleType)
{
    CLOGD("DestroyChannel Enter, Specify specific channel type.");

    std::lock_guard<std::mutex> lg(connectionMapMtx_);

    for (auto it = connectionMap_.begin(); it != connectionMap_.end();) {
        if (it->first.moduleType == moduleType) {
            it->second->CloseConnection();
            connectionMap_.erase(it);
            return true;
        } else {
            it++;
        }
    }
    return false;
}

void ChannelManager::DestroyAllChannels()
{
    CLOGD("DestroyAllChannels Enter.");

    std::lock_guard<std::mutex> lg(connectionMapMtx_);

    for (auto it = connectionMap_.begin(); it != connectionMap_.end();) {
        it->second->CloseConnection();
        it = connectionMap_.erase(it);
    }
}

bool ChannelManager::IsAllChannelOpened() const
{
    return true;
}
} // namespace CastEngineService
} // namespace CastEngine
} // namespace OHOS