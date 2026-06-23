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
 * Description: Cast session implement class.
 * Author: gaoshuai
 * Create: 2023-11
 */

#ifndef CAST_SESSION_IMPL_CLASS_H
#define CAST_SESSION_IMPL_CLASS_H

#include <array>
#include <condition_variable>
#include <mutex>
#include <memory>
#include <optional>
#include <set>
#include <utility>

#include "cast_engine_common.h"
#include "cast_session_common.h"
#include "cast_session_enums.h"
#include "cast_session_impl_stub.h"
#include "connection_manager_listener.h"
#include "channel.h"
#include "channel_info.h"
#include "channel_manager.h"
#include "channel_manager_listener.h"
#include "channel_request.h"
#include "message.h"
#include "i_rtsp_controller.h"
#include "oh_remote_control_event.h"
#include "rtsp_listener.h"
#include "rtsp_param_info.h"
#include "state_machine.h"
#include "i_cast_stream_listener.h"
#include "i_cast_stream_manager.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineService {
using CastSessionRtsp::IRtspController;
using CastSessionRtsp::IRtspListener;
using CastSessionRtsp::ParamInfo;
using CastSessionEnums::MessageId;

class CastSessionImpl : public StateMachine,
    public CastSessionImplStub,
    public std::enable_shared_from_this<CastSessionImpl> {
public:
    CastSessionImpl(const CastSessionProperty &property, const CastLocalDevice &localDevice);
    ~CastSessionImpl() override;
    void SetServiceCallbackForRelease(const std::function<void(int)> &callback);
    int32_t RegisterListener(sptr<ICastSessionListenerImpl> listener) override;
    int32_t UnregisterListener() override;
    int32_t AddDevice(const CastRemoteDevice &remoteDevice) override;
    bool AddDevice(const CastInnerRemoteDevice &remoteDevice) override;
    int32_t RemoveDevice(const std::string &deviceId) override;
    int32_t StartAuth(const AuthInfo &authInfo) override;
    int32_t GetSessionId(std::string &sessionId) override;
    int32_t GetDeviceState(const std::string &deviceId, DeviceState &deviceState) override;
    int32_t SetSessionProperty(const CastSessionProperty &property) override;
    bool ReleaseSessionResources(pid_t pid, uid_t uid) override;
    int32_t CreateMirrorPlayer(sptr<IMirrorPlayerImpl> &mirrorPlayer) override;
    int32_t CreateStreamPlayer(sptr<IStreamPlayerIpc> &streamPlayer) override;
    std::shared_ptr<ICastStreamManager> CreateStreamPlayerManager();
    bool DestroyMirrorPlayer();
    bool DestroyStreamPlayer();
    void Stop() override;
    int32_t Release() override;
    bool Init();
    int32_t NotifyEvent(EventId eventId, std::string &jsonParam) override;
    int32_t SetCastMode(CastMode mode, std::string &jsonParam) override;

    int32_t GetSessionProtocolType(ProtocolType &protocolType) override;
    void SetSessionProtocolType(ProtocolType protocolType) override;
    uint8_t GetSessionState() override;
    int32_t GetRemoteDeviceInfo(std::string deviceId, CastRemoteDevice &remoteDevice) override;

    int32_t Play(const std::string &deviceId);
    int32_t Pause(const std::string &deviceId);
    int32_t SetSurface(sptr<IBufferProducer> producer);
    int32_t DeliverInputEvent(const OHRemoteControlEvent &event);
    int32_t InjectEvent(const OHRemoteControlEvent &event);
    int32_t GetDisplayId(std::string &displayId);
    int32_t ResizeVirtualScreen(uint32_t width, uint32_t height);

private:

    static const std::array<std::string, static_cast<size_t>(MessageId::MSG_ID_MAX)> MESSAGE_ID_STRING;

    enum class ModuleState : uint8_t {
        IDLE,
        STARTING,
        START_SUCCESS,
        START_FAILED
    };

    enum class CastSessionRemoteEventId : uint8_t {
        MIRROR_TO_UI = 1,
        UI_TO_MIRROR,
        READY_TO_PLAYING,
        MIRROR_TO_STREAM,
        STREAM_TO_MIRROR,
        RESPONSE_MODE_SWITCH,
    };
    class ChannelManagerListenerImpl;
    class RtspListenerImpl;
    class ConnectManagerListenerImpl;
    class CastStreamListenerImpl;
    class BaseState;
    class DefaultState;
    class DisconnectedState;
    class AuthingState;
    class ConnectingState;
    class ConnectedState;
    class PausedState;
    class PlayingState;
    class DisconnectingState;
    class MirrorToStreamState;
    class StreamToMirrorState;
    class StreamState;

    static constexpr int MODULE_ID_RTSP = 1000;
    static constexpr int MODULE_ID_MEDIA = 1001;
    static constexpr int MODULE_ID_RC = 1002;
    static constexpr int MODULE_ID_DLNA = 1004;
    static constexpr int MODULE_ID_CHANNEL = 1005;
    static constexpr int MODULE_ID_CAST_PROJECTION = 1006;
    static constexpr int MODULE_ID_CAST_STREAM = 1009;
    static constexpr int MODULE_ID_CAST_SESSION = 2000;
    static constexpr int MODULE_ID_UI = 2001;
    static constexpr int MODULE_ID_MEDIA_NEW = 2002;
    // COOPERATION/HICAR use it to colse media channel, don't modify it.
    static constexpr double CAST_VERSION = 1.0;
    static constexpr int TIMEOUT_CONNECT = 30 * 1000;
    static constexpr int TIMEOUT_WAIT_CONFIRM = 35 * 1000;
    static constexpr int TIMEOUT_WAIT_INPUT_PIN_CODE = 45 * 1000;
    static constexpr int INVALID_PORT = -1;
    static constexpr int UNUSED_VALUE = -1;
    static constexpr int NO_DELAY = 0;
    static constexpr int UNNEEDED_PORT = -2;
    static constexpr unsigned int SOCKET_PORT_BITS = 16;
    static constexpr unsigned int SOCKET_PORT_MASK = 0xFFFF;
    static constexpr int ERR_CODE = -1;
    static constexpr int ID_INC_STEP = 1;
    static constexpr int RENDER_READY = 1;
    static constexpr int NOT_RENDER_READY = 0;
    static constexpr int MAX_SESSION_ID = 0xFFFFFFF;
    static constexpr int MAX_PORT = 65535;
    static constexpr int MIN_PORT = 1024;

    void SetLocalDevice(const CastLocalDevice &localDevice);
    std::shared_ptr<CastRemoteDeviceInfo> FindRemoteDevice(const std::string &deviceId);
    std::shared_ptr<CastRemoteDeviceInfo> FindRemoteDeviceLocked(const std::string &deviceId);
    void UpdateRemoteDeviceStateLocked(const std::string &deviceId, DeviceState state);
    void UpdateRemoteDeviceSessionId(const std::string &deviceId, int sessionId);
    void UpdateRemoteDeviceInfoFromCastDeviceDataManager(const std::string &deviceId);
    void ChangeDeviceState(DeviceState state, const std::string &deviceId,
        int32_t reasonCode = REASON_DEFAULT);
    void ChangeDeviceStateInner(DeviceState state, const std::string &deviceId,
        int32_t reasonCode = REASON_DEFAULT);
    void OnEvent(EventId eventId, const std::string &data);
    void OnRemoteCtrlEvent(int eventType, const uint8_t *data, uint32_t len);
    bool TransferTo(std::shared_ptr<BaseState> state);
    bool IsAllowTransferState(SessionState desiredState) const;
    std::string GetCurrentRemoteDeviceId();
    bool ProcessAuth(const std::string &deviceId);
    int ProcessConnect(const Message &msg);
    bool ProcessSetUp(const Message &msg);
    bool ProcessSetUpSuccess(const Message &msg);
    bool ProcessPlay(const Message &msg);
    bool ProcessPlayReq(const Message &msg);
    bool ProcessPause(const Message &msg);
    bool ProcessPauseReq(const Message &msg);
    bool ProcessDisconnect(const Message &msg);
    bool ProcessError(const Message &msg);
    bool ProcessTriggerReq(const Message &msg);
    bool ProcessUpdateVideoSize(const Message &msg);
    bool ProcessStateEvent(MessageId msgId, const Message &msg);
    bool IsSupportFeature(const std::set<int> &featureSet, int supportFeature);
    bool IsConnected() const;
    bool SendEventChange(int moduleId, int event, const std::string &param);
    void DisconnectPhysicalLink(const std::string &deviceId);
    void RemoveRemoteDevice(const std::string &deviceId);
    bool AddRemoteDevice(const CastRemoteDeviceInfo &remoteDeviceInfo);
    std::shared_ptr<ChannelRequest> BuildChannelRequest(const std::string &remoteDeviceId, bool isSupportVtp,
        ModuleType moduleType);
    int SetupRemoteControl(const CastInnerRemoteDevice &remote);
    bool IsVtpUsed(ChannelType type);
    bool IsChannelClient(ChannelType type);
    bool IsChannelNeeded(ChannelType type);
    std::pair<int, int> GetLeagacyMediaPort(ChannelType type, int port, bool isLegacy);
    std::pair<int, int> GetMediaPort(ChannelType type, int port, bool isLegacy);
    std::optional<int> SetupMedia(const CastInnerRemoteDevice &remote, ChannelType type, int ports);
    void InitRtspParamInfo(std::shared_ptr<CastRemoteDeviceInfo> remoteDeviceInfo);
    std::shared_ptr<ICastStreamManager> StreamManagerGetter();
    sptr<IMirrorPlayerImpl> MirrorPlayerGetter();
    void ProcessRtspEvent(int moduleId, int event, const std::string &param);
    void ProcessRtspEventInner(int moduleId, int event, const std::string &param);
    void OtherAddDevice(const CastInnerRemoteDevice &remoteDevice);
    void SendConsultData(const std::string &deviceId, int port);
    int32_t ConnectDlnaDevice(CastInnerRemoteDevice remoteDeviceInfo);
    bool IsDlnaDevice(CastInnerRemoteDevice remoteDeviceInfo);

    void MirrorRcvVideoFrame();
    bool ProcessSetCastMode(const Message &msg);
    void UpdateScreenInfo(uint64_t screenId, uint16_t width, uint16_t height);
    void UpdateDefaultDisplayRotationInfo(int rotation, uint16_t width, uint16_t height);

    bool ProcessMirrorToStream();
    bool PauseMirrorForStream();
    bool PlayAfterSwitchToStream();

    bool ProcessStreamToMirror();
    bool PlayMirrorForChange();

    bool IsStreamMode();
    std::string GetPlayerControllerCapability();
    bool IsSink();
    int CreateStreamChannel();
    void SendCastRenderReadyOption(int isReady);
    void OnEventInner(sptr<CastSessionImpl> session, EventId eventId, const std::string &jsonParam);
    void WaitSinkSetProperty();
    void SetWifiScene(unsigned int scene);
    void SetMirrorToStreamState(bool state);
    int32_t RemoveDeviceInner(const std::string &deviceId);
    std::shared_ptr<DefaultState> defaultState_;
    std::shared_ptr<DisconnectedState> disconnectedState_;
    std::shared_ptr<AuthingState> authingState_;
    std::shared_ptr<ConnectingState> connectingState_;
    std::shared_ptr<ConnectedState> connectedState_;
    std::shared_ptr<PausedState> pausedState_;
    std::shared_ptr<PlayingState> playingState_;
    std::shared_ptr<DisconnectingState> disconnectingState_;

    bool isMirrorPlaying_ { false };
    std::shared_ptr<StreamState> streamState_;
    std::shared_ptr<MirrorToStreamState> mirrorToStreamState_;
    std::shared_ptr<StreamToMirrorState> streamToMirrorState_;

    static std::atomic<int> idCount_;

    std::function<void(int)> serviceCallback_;
    std::map<pid_t, sptr<ICastSessionListenerImpl>> listeners_;
    sptr<IMirrorPlayerImpl> mirrorPlayer_;
    std::shared_ptr<ICastStreamManager> streamManager_;
    int sessionId_{ -1 };
    int rtspPort_{ -1 };
    std::list<CastRemoteDeviceInfo> remoteDeviceList_;
    CastLocalDevice localDevice_;
    uint32_t dlnaDeviceId_{ static_cast<uint32_t>(INVALID_ID) };
    CastSessionProperty property_;
    ParamInfo rtspParamInfo_;
    bool mirrorToStream_ { false };
    SessionState sessionState_{ SessionState::DISCONNECTED };
    std::atomic<bool> isTearDownReceived_ { false };

    std::shared_ptr<ChannelManager> channelManager_;
    std::shared_ptr<ChannelManagerListenerImpl> channelManagerListener_;
    std::shared_ptr<IRtspController> rtspControl_;
    std::shared_ptr<RtspListenerImpl> rtspListener_;
    std::shared_ptr<ConnectManagerListenerImpl> connectManagerListener_;
    sptr<IStreamPlayerIpc> streamPlayer_;

    std::mutex mutex_;
    std::mutex mirrorMutex_;
    std::mutex streamMutex_;
    std::mutex serviceCallbackMutex_;
    std::condition_variable cond_;
    std::condition_variable setProperty_;
    std::mutex dataTransMutex_;
    CastMode castMode_ = CastMode::MIRROR_CAST;
    ModuleState mediaState_{ ModuleState::IDLE };
    ModuleState remoteCtlState_{ ModuleState::IDLE };

    using StateProcessor = bool (CastSessionImpl::*)(const Message &msg);
    std::array<StateProcessor, static_cast<size_t>(MessageId::MSG_ID_MAX)> stateProcessor_{
        nullptr,                                  // MSG_CONNECT
        &CastSessionImpl::ProcessSetUp,           // MSG_SETUP
        &CastSessionImpl::ProcessSetUpSuccess,    // MSG_SETUP_SUCCESS
        nullptr,                                  // MSG_SETUP_FAILED
        nullptr,                                  // MSG_SETUP_DONE
        &CastSessionImpl::ProcessPlay,            // MSG_PLAY
        &CastSessionImpl::ProcessPause,           // MSG_PAUSE
        &CastSessionImpl::ProcessPlayReq,         // MSG_PLAY_REQ
        &CastSessionImpl::ProcessPauseReq,        // MSG_PAUSE_REQ
        &CastSessionImpl::ProcessDisconnect,      // MSG_DISCONNECT
        &CastSessionImpl::ProcessDisconnect,      // MSG_CONNECT_TIMEOUT
        &CastSessionImpl::ProcessTriggerReq,      // MSG_PROCESS_TRIGGER_REQ
        &CastSessionImpl::ProcessUpdateVideoSize, // MSG_UPDATE_VIDEO_SIZE
        nullptr,                                  // MSG_STREAM_RECV_ACTION_EVENT_FROM_PEERS
        nullptr,                                  // MSG_STREAM_SEND_ACTION_EVENT_TO_PEERS
        nullptr,                                  // MSG_PEER_RENDER_READY
        &CastSessionImpl::ProcessError,           // MSG_ERROR
    };
};
} // namespace CastEngineService
} // namespace CastEngine
} // namespace OHOS

#endif
