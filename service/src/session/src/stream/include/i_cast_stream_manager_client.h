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
 * Description: cast stream manager client interface.
 * Author: zhangjingnan
 * Create: 2023-09-01
 */

#ifndef I_CAST_STREAM_MANAGER_CLIENT_H
#define I_CAST_STREAM_MANAGER_CLIENT_H

#include "i_stream_player_impl.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineService {
class ICastStreamManagerClient {
public:
    virtual ~ICastStreamManagerClient() = default;

    virtual bool RegisterListener(sptr<IStreamPlayerListenerImpl> listener) = 0;
    virtual bool UnregisterListener() = 0;
    virtual bool NotifyPeerLoad(const MediaInfo &mediaInfo) = 0;
    virtual bool NotifyPeerPlay(const MediaInfo &mediaInfo) = 0;
    virtual bool NotifyPeerPause() = 0;
    virtual bool NotifyPeerResume() = 0;
    virtual bool NotifyPeerStop() = 0;
    virtual bool NotifyPeerNext() = 0;
    virtual bool NotifyPeerPrevious() = 0;
    virtual bool NotifyPeerSeek(int position) = 0;
    virtual bool NotifyPeerFastForward(int delta) = 0;
    virtual bool NotifyPeerFastRewind(int delta) = 0;
    virtual bool NotifyPeerSetVolume(int volume) = 0;
    virtual bool NotifyPeerSetMute(bool mute) = 0;
    virtual bool NotifyPeerSetRepeatMode(int mode) = 0;
    virtual bool NotifyPeerSetAvailableCapability(const StreamCapability &streamCapability) = 0;
    virtual bool NotifyPeerSetSpeed(int speed) = 0;
    virtual bool NotifyPeerKeyResponse(const std::string &mediaId, const std::vector<uint8_t> &response) = 0;
    virtual bool NotifyPeerMediaInfoChanged(const MediaInfo &mediaInfo) = 0;
    virtual void OnEvent(EventId eventId, const std::string &data) = 0;
    virtual PlayerStates GetPlayerStatus() = 0;
    virtual int GetPosition() = 0;
    virtual int GetDuration() = 0;
    virtual int GetVolume() = 0;
    virtual bool GetMute() = 0;
    virtual int GetMaxVolume() = 0;
    virtual LoopMode GetLoopMode() = 0;
    virtual StreamCapability GetAvailableCapability() = 0;
    virtual PlaybackSpeed GetPlaySpeed() = 0;
    virtual bool IsDoubleFrame() = 0;
    virtual bool TransferToStreamMode(const MediaInfo &mediaInfo) = 0;
    virtual bool DisconnectSession(std::string deviceId) = 0;
};
} // namespace CastEngineService
} // namespace CastEngine
} // namespace OHOS
#endif // I_CAST_STREAM_MANAGER_CLIENT_H