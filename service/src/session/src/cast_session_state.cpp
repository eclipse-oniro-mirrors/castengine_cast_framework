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
 * Description: Cast Session states realization.
 * Author: zhangge
 * Create: 2022-07-19
 */

#include "cast_session_impl.h"
#include "cast_engine_log.h"
#include "cast_engine_dfx.h"
#include "utils.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineService {
DEFINE_CAST_ENGINE_LABEL("Cast-Session-State");

void CastSessionImpl::BaseState::Enter(SessionState state)
{
    auto session = session_.promote();
    if (!session) {
        CLOGE("Session is invalid");
        return;
    }

    // BaseState and its inherited classes work under the strict control of the session,
    // so the session_ used in the BaseState member function must not be null.
    session->sessionState_ = state;
    CLOGI("%{public}s enter", SESSION_STATE_STRING[static_cast<int>(state)].c_str());
}

void CastSessionImpl::BaseState::Exit()
{
    auto session = session_.promote();
    if (!session) {
        CLOGE("Session is invalid");
        return;
    }

    CLOGI("%{public}s exit", SESSION_STATE_STRING[static_cast<int>(session->sessionState_)].c_str());
}

bool CastSessionImpl::BaseState::HandleMessage(const Message &msg)
{
    auto session = session_.promote();
    if (!session) {
        CLOGE("Session is invalid");
        return false;
    }

    CLOGI("msg: %{public}s, session state: %{public}s", session->MESSAGE_ID_STRING[msg.what_].c_str(),
        SESSION_STATE_STRING[static_cast<int>(session->sessionState_)].c_str());
    return true;
}

SessionState CastSessionImpl::BaseState::GetStateId() const
{
    return stateId_;
}

void CastSessionImpl::DefaultState::Enter()
{
    BaseState::Enter(SessionState::DEFAULT);
}

void CastSessionImpl::DefaultState::Exit()
{
    BaseState::Exit();
}

bool CastSessionImpl::DefaultState::HandleMessage(const Message &msg)
{
    auto session = session_.promote();
    if (!session) {
        CLOGE("Session is invalid");
        return false;
    }

    BaseState::HandleMessage(msg);

    MessageId msgId = static_cast<MessageId>(msg.what_);
    std::string deviceId = msg.strArg_;
    switch (msgId) {
        case MessageId::MSG_CONNECT_TIMEOUT:
            session->ProcessStateEvent(MessageId::MSG_CONNECT_TIMEOUT, msg);
            break;
        default:
            CLOGW("unsupported msg: %{public}s, in default state", MESSAGE_ID_STRING[msgId].c_str());
            return false;
    }
    return true;
}

void CastSessionImpl::DisconnectedState::Enter()
{
    BaseState::Enter(SessionState::DISCONNECTED);
    auto session = session_.promote();
    if (!session) {
        CLOGE("Session is invalid");
        return;
    }
    if (session->property_.protocolType == ProtocolType::CAST_PLUS_MIRROR ||
        session->property_.protocolType == ProtocolType::HICAR) {
        Utils::LightAndLockScreen();
    }
}

void CastSessionImpl::DisconnectedState::Exit()
{
    BaseState::Exit();
}

bool CastSessionImpl::DisconnectedState::HandleMessage(const Message &msg)
{
    auto session = session_.promote();
    if (!session) {
        CLOGE("Session is invalid");
        return false;
    }

    BaseState::HandleMessage(msg);

    MessageId msgId = static_cast<MessageId>(msg.what_);
    std::string deviceId = msg.strArg_;
    switch (msgId) {
        case MessageId::MSG_START_AUTH:
                session->ChangeDeviceState(DeviceState::AUTHING, deviceId);
                session->RemoveMessage(Message(static_cast<int>(MessageId::MSG_CONNECT_TIMEOUT)));
                session->SendCastMessageDelayed(MessageId::MSG_CONNECT_TIMEOUT, TIMEOUT_CONNECT, deviceId);
                session->TransferTo(session->authingState_);
                session->ProcessAuth(deviceId);
            break;
        case MessageId::MSG_CONNECT:
            if (session->ProcessConnect(msg) >= 0) {
                session->ChangeDeviceState(DeviceState::CONNECTING, deviceId);
                session->SendCastMessageDelayed(MessageId::MSG_CONNECT_TIMEOUT, TIMEOUT_CONNECT, deviceId);
                session->TransferTo(session->connectingState_);
            } else {
                // Connection failed
                session->ChangeDeviceState(DeviceState::DISCONNECTED, deviceId);
            }
            break;
        case MessageId::MSG_DISCONNECT:
            session->DisconnectPhysicalLink(deviceId);
            session->RemoveRemoteDevice(deviceId);
            break;
        default:
            CLOGW("unsupported msg: %{public}s, in disconnected state", MESSAGE_ID_STRING[msgId].c_str());
            return false;
    }
    return true;
}

void CastSessionImpl::AuthingState::Enter()
{
    BaseState::Enter(SessionState::AUTHING);
}

void CastSessionImpl::AuthingState::Exit()
{
    BaseState::Exit();
}

bool CastSessionImpl::AuthingState::HandleMessage(const Message &msg)
{
    auto session = session_.promote();
    if (!session) {
        CLOGE("Session is invalid");
        return false;
    }

    BaseState::HandleMessage(msg);

    MessageId msgId = static_cast<MessageId>(msg.what_);
    std::string deviceId = msg.strArg_;
    switch (msgId) {
        case MessageId::MSG_CONNECT: {
            int port = session->ProcessConnect(msg);
            if (port >= 0) {
                session->ChangeDeviceState(DeviceState::CONNECTING, deviceId);
                session->RemoveMessage(Message(static_cast<int>(MessageId::MSG_CONNECT_TIMEOUT)));
                session->SendCastMessageDelayed(MessageId::MSG_CONNECT_TIMEOUT, TIMEOUT_CONNECT, deviceId);
                session->TransferTo(session->connectingState_);
                session->SendConsultData(deviceId, port);
            } else {
                session->ChangeDeviceState(DeviceState::DISCONNECTED, deviceId);
            }
            break;
        }
        case MessageId::MSG_DISCONNECT:
            session->ProcessDisconnect(msg);
            session->TransferTo(session->disconnectingState_);
            session->ChangeDeviceState(DeviceState::DISCONNECTED, deviceId, msg.arg1_);
            session->RemoveRemoteDevice(deviceId);
            break;
        case MessageId::MSG_ERROR:
            session->ProcessError(msg);
            session->TransferTo(session->disconnectingState_);
            break;
        case MessageId::MSG_AUTHING:
            session->ChangeDeviceState(DeviceState::AUTHING, deviceId, msg.arg1_);
            if (msg.arg1_ == REASON_SHOW_TRUST_SELECT_UI || msg.arg1_ == REASON_TRUST_BY_SINK) {
                session->RemoveMessage(Message(static_cast<int>(MessageId::MSG_CONNECT_TIMEOUT)));
                int waitTimeout =
                    (msg.arg1_ == REASON_SHOW_TRUST_SELECT_UI ? TIMEOUT_WAIT_CONFIRM : TIMEOUT_WAIT_INPUT_PIN_CODE);
                session->SendCastMessageDelayed(MessageId::MSG_CONNECT_TIMEOUT, msg.arg1_, waitTimeout, deviceId);
            }
            break;
        default:
            CLOGW("unsupported msg: %{public}s, in authing state", MESSAGE_ID_STRING[msgId].c_str());
            return false;
    }
    return true;
}


void CastSessionImpl::ConnectingState::Enter()
{
    BaseState::Enter(SessionState::CONNECTING);
}

void CastSessionImpl::ConnectingState::Exit()
{
    BaseState::Exit();
}

bool CastSessionImpl::ConnectingState::HandleMessage(const Message &msg)
{
    auto session = session_.promote();
    if (!session) {
        CLOGE("Session is invalid");
        return false;
    }
    BaseState::HandleMessage(msg);
    MessageId msgId = static_cast<MessageId>(msg.what_);
    switch (msgId) {
        case MessageId::MSG_SETUP:
            HandleSetupMessage(msg, session);
            break;
        case MessageId::MSG_SETUP_SUCCESS:
            HandleSetupSuccessMessage(msg, msgId, session);
            break;
        case MessageId::MSG_DISCONNECT:
            HandleDisconnectMessage(msg, session);
            break;
        case MessageId::MSG_ERROR:
            HandleErrorMessage(msg, session);
            break;
        case MessageId::MSG_PEER_RENDER_READY:
            HandleRenderReadyMessage(msg, session);
            break;
        case MessageId::MSG_CONNECT:
        case MessageId::MSG_PLAY:
        case MessageId::MSG_PLAY_REQ:
            HandleConnectMessage(msg, msgId, session);
            break;
        default:
            CLOGW("unsupported msg: %{public}s, in connecting state", MESSAGE_ID_STRING[msgId].c_str());
            return false;
    }
    return true;
}

void CastSessionImpl::ConnectingState::HandleSetupMessage(const Message &msg, sptr<CastSessionImpl> session)
{
    if (!session) {
        CLOGE("Session is invalid");
        return;
    }
    std::string deviceId = msg.strArg_;
    if (!session->ProcessSetUp(msg)) {
        session->DisconnectPhysicalLink(deviceId);
        session->TransferTo(session->disconnectingState_);
        session->ChangeDeviceState(DeviceState::DISCONNECTED, deviceId);
        session->RemoveRemoteDevice(deviceId);
    }
}

void CastSessionImpl::ConnectingState::HandleSetupSuccessMessage(const Message &msg, const MessageId &msgId,
    sptr<CastSessionImpl> session)
{
    if (!session) {
        CLOGE("Session is invalid");
        return;
    }
    if (session->ProcessSetUpSuccess(msg)) {
        session->RemoveMessage(Message(static_cast<int>(MessageId::MSG_CONNECT_TIMEOUT)));
        CLOGI("in connecting state, defer message: %{public}d", msgId);
        session->DeferMessage(msg);
        session->TransferTo(session->connectedState_);
    };
}

void CastSessionImpl::ConnectingState::HandleDisconnectMessage(const Message &msg, sptr<CastSessionImpl> session)
{
    if (!session) {
        CLOGE("Session is invalid");
        return;
    }
    std::string deviceId = msg.strArg_;
    session->ProcessDisconnect(msg);
    
    session->RemoveMessage(Message(static_cast<int>(MessageId::MSG_CONNECT_TIMEOUT)));
    session->TransferTo(session->disconnectingState_);
    session->ChangeDeviceState(DeviceState::DISCONNECTED, deviceId, msg.arg1_);
    session->RemoveRemoteDevice(deviceId);
}

void CastSessionImpl::ConnectingState::HandleErrorMessage(const Message &msg, sptr<CastSessionImpl> session)
{
    if (!session) {
        CLOGE("Session is invalid");
        return;
    }
    session->ProcessError(msg);
    session->TransferTo(session->disconnectingState_);
}

void CastSessionImpl::ConnectingState::HandleRenderReadyMessage(const Message &msg, sptr<CastSessionImpl> session)
{
    if (!session) {
        CLOGE("Session is invalid");
        return;
    }
    std::string deviceId = msg.strArg_;
    if (session->IsStreamMode()) {
        session->RemoveMessage(Message(static_cast<int>(MessageId::MSG_CONNECT_TIMEOUT)));
        session->ChangeDeviceState(DeviceState::STREAM, deviceId);
        session->TransferTo(session->streamState_);
    }
}

void CastSessionImpl::ConnectingState::HandleConnectMessage(const Message &msg, const MessageId &msgId,
    sptr<CastSessionImpl> session)
{
    if (!session) {
        CLOGE("Session is invalid");
        return;
    }
    CLOGI("in connecting state, defer message: %{public}d", msgId);
    session->DeferMessage(msg);
}

void CastSessionImpl::ConnectedState::Enter()
{
    BaseState::Enter(SessionState::CONNECTED);

    auto session = session_.promote();
    if (!session) {
        CLOGE("Session is invalid");
        return;
    }

    if (session->property_.protocolType == ProtocolType::COOPERATION) {
        HiSysEventWriteWrap(__func__, {
                {"BIZ_SCENE", static_cast<int32_t>(BIZSceneType::COOPERATION)},
                {"BIZ_STATE", static_cast<int32_t>(BIZStateType::BIZ_STATE_END)},
                {"BIZ_STAGE", static_cast<int32_t>(BIZSceneStage::COOPERATION_CAST_SUCCESS)},
                {"STAGE_RES", static_cast<int32_t>(StageResType::STAGE_RES_SUCCESS)},
                {"ERROR_CODE", CAST_RADAR_SUCCESS}}, {
                {"TO_CALL_PKG", DSOFTBUS_NAME},
                {"LOCAL_SESS_NAME", ""},
                {"PEER_SESS_NAME", ""},
                {"PEER_UDID", ""}});
    } else {
        auto sceneType = GetBIZSceneType(static_cast<int>(session->property_.protocolType));
        HiSysEventWriteWrap(__func__, {
                {"BIZ_SCENE", sceneType},
                {"BIZ_STATE", static_cast<int32_t>(BIZStateType::BIZ_STATE_END)},
                {"BIZ_STAGE", static_cast<int32_t>(BIZSceneStage::CAST_SUCCESS)},
                {"STAGE_RES", static_cast<int32_t>(StageResType::STAGE_RES_SUCCESS)},
                {"ERROR_CODE", CAST_RADAR_SUCCESS}}, {
                {"TO_CALL_PKG", DSOFTBUS_NAME},
                {"LOCAL_SESS_NAME", ""},
                {"PEER_SESS_NAME", ""},
                {"PEER_UDID", ""}});
    }
}

void CastSessionImpl::ConnectedState::Exit()
{
    BaseState::Exit();
}

bool CastSessionImpl::ConnectedState::HandleMessage(const Message &msg)
{
    auto session = session_.promote();
    if (!session) {
        return false;
    }
    BaseState::HandleMessage(msg);
    MessageId msgId = static_cast<MessageId>(msg.what_);
    const auto &deviceId = msg.strArg_;
    Message msgError = msg;
    switch (msgId) {
        case MessageId::MSG_CONNECT:
            // Designed for 1->N scenarios
            session->ChangeDeviceState(DeviceState::CONNECTING, deviceId);
            if (session->ProcessConnect(msg) >= 0) {
                TransferToPaused(deviceId);
            } else {
                session->ChangeDeviceState(DeviceState::DISCONNECTED, deviceId);
            }
            return true;
        case MessageId::MSG_SETUP_SUCCESS:
            if (!session->IsStreamMode()) {
                TransferToPaused(deviceId);
            } else if (session->IsSink()) {
                session->ChangeDeviceState(DeviceState::STREAM, deviceId);
                session->TransferTo(session->streamState_);
            }
            return true;
        case MessageId::MSG_DISCONNECT:
            session->ProcessDisconnect(msg);
            session->TransferTo(session->disconnectingState_);
            session->ChangeDeviceState(DeviceState::DISCONNECTED, deviceId);
            session->RemoveRemoteDevice(deviceId);
            session->SetMirrorToStreamState(false);
            return true;
        case MessageId::MSG_ERROR:
            msgError.arg1_ = REASON_DEFAULT;
            session->ProcessError(msgError);
            session->TransferTo(session->disconnectingState_);
            return true;
        case MessageId::MSG_PLAY_REQ:
            CLOGI("in connected state, defer message: %{public}d", msgId);
            session->DeferMessage(msg);
            return true;
        case MessageId::MSG_UPDATE_VIDEO_SIZE:
            session->ProcessStateEvent(MessageId::MSG_UPDATE_VIDEO_SIZE, msg);
            return true;
        default:
            CLOGW("unsupported msg: %{public}s, in connected state", MESSAGE_ID_STRING[msgId].c_str());
            return false;
    }
}

void CastSessionImpl::ConnectedState::TransferToPaused(std::string deviceId)
{
    auto session = session_.promote();
    if (!session) {
        CLOGE("Session is invalid");
        return;
    }
    if (session->property_.protocolType != ProtocolType::HICAR
        && session->property_.protocolType != ProtocolType::SUPER_LAUNCHER) {
        session->ChangeDeviceState(DeviceState::CONNECTED, deviceId);
    }
    session->ChangeDeviceState(DeviceState::PAUSED, deviceId);
    session->TransferTo(session->pausedState_);
}

void CastSessionImpl::StreamState::Enter()
{
    BaseState::Enter(SessionState::STREAM);
}

void CastSessionImpl::StreamState::Exit()
{
    BaseState::Exit();
}

bool CastSessionImpl::StreamState::HandleMessage(const Message &msg)
{
    auto session = session_.promote();
    if (!session) {
        CLOGE("Session is invalid");
        return false;
    }
    BaseState::HandleMessage(msg);
    MessageId msgId = static_cast<MessageId>(msg.what_);
    const auto &param = msg.strArg_;
    auto streamManager = session->StreamManagerGetter();
    switch (msgId) {
        case MessageId::MSG_STREAM_RECV_ACTION_EVENT_FROM_PEERS:
            if (msg.arg1_ == ICastStreamManager::MODULE_EVENT_ID_CHANNEL_CREATE ||
                msg.arg1_ == ICastStreamManager::MODULE_EVENT_ID_STREAM_CHANNEL) {
                if (session->CreateStreamChannel() == INVALID_PORT) {
                    session->SendCastMessage(MessageId::MSG_ERROR);
                }
                break;
            }
            if (streamManager != nullptr) {
                streamManager->ProcessActionsEvent(msg.arg1_, param);
            } else {
                CLOGE("streamManager is nullptr");
            }
            break;
        case MessageId::MSG_STREAM_SEND_ACTION_EVENT_TO_PEERS:
            session->SendEventChange(MODULE_ID_CAST_STREAM, msg.arg1_, param);
            break;
        case MessageId::MSG_PEER_RENDER_READY:
            session->SendCastRenderReadyOption(msg.arg1_);
            break;
        case MessageId::MSG_SWITCH_TO_MIRROR:
            if (!session->IsSink()) {
                session->ProcessStreamToMirror();
                session->ChangeDeviceState(DeviceState::STREAM_TO_MIRROR, session->GetCurrentRemoteDeviceId());
                session->TransferTo(session->streamToMirrorState_);
                session->SetMirrorToStreamState(false);
            }
            break;
        default:
            CLOGW("unsupported msg: %{public}s, in stream state", MESSAGE_ID_STRING[msgId].c_str());
            return false;
    }
    return true;
}

void CastSessionImpl::PausedState::Enter()
{
    BaseState::Enter(SessionState::PAUSED);
}

void CastSessionImpl::PausedState::Exit()
{
    BaseState::Exit();
}

bool CastSessionImpl::PausedState::HandleMessage(const Message &msg)
{
    auto session = session_.promote();
    if (!session) {
        CLOGE("Session is invalid");
        return false;
    }

    BaseState::HandleMessage(msg);
    MessageId msgId = static_cast<MessageId>(msg.what_);

    switch (msgId) {
        case MessageId::MSG_PLAY:
        case MessageId::MSG_PLAY_REQ:
        // vtp disconnect after paused for 30s. setup success after play request
        case MessageId::MSG_SETUP_SUCCESS:
            if (session->ProcessStateEvent(msgId, msg)) {
                session->ChangeDeviceState(DeviceState::PLAYING, msg.strArg_);
                session->TransferTo(session->playingState_);
            }
            break;
        case MessageId::MSG_PROCESS_TRIGGER_REQ:
            session->ProcessTriggerReq(msg);
            break;
        default:
            CLOGW("unsupported msg: %{public}s, in paused state", MESSAGE_ID_STRING[msgId].c_str());
            return false;
    }
    return true;
}

void CastSessionImpl::PlayingState::Enter()
{
    BaseState::Enter(SessionState::PLAYING);
    auto session = session_.promote();
    if (!session) {
        CLOGE("Session is invalid");
        return;
    }
    if (session->property_.protocolType == ProtocolType::CAST_PLUS_MIRROR ||
        session->property_.protocolType == ProtocolType::HICAR) {
        Utils::EnablePowerForceTimingOut();
    }
}

void CastSessionImpl::PlayingState::Exit()
{
    auto session = session_.promote();
    if (!session) {
        CLOGE("Session is invalid");
        BaseState::Exit();
        return;
    }
    if (session->property_.protocolType == ProtocolType::CAST_PLUS_MIRROR ||
        session->property_.protocolType == ProtocolType::HICAR) {
        Utils::ResetPowerForceTimingOut();
    }
    BaseState::Exit();
}

bool CastSessionImpl::PlayingState::HandleMessage(const Message &msg)
{
    auto session = session_.promote();
    if (!session) {
        CLOGE("Session is invalid");
        return false;
    }
    const auto &param = msg.strArg_;
    BaseState::HandleMessage(msg);
    MessageId msgId = static_cast<MessageId>(msg.what_);
    switch (msgId) {
        case MessageId::MSG_PAUSE:
        case MessageId::MSG_PAUSE_REQ:
            if (session->ProcessStateEvent(msgId, msg)) {
                session->ChangeDeviceState(DeviceState::PAUSED, msg.strArg_);
                session->TransferTo(session->pausedState_);
            }
            break;
        // Reserved MSG_PLAY and MSG_PLAY_REQ for 1->N scenarios
        case MessageId::MSG_PLAY:
        case MessageId::MSG_PLAY_REQ:
            break;
        case MessageId::MSG_UPDATE_VIDEO_SIZE:
            session->ProcessStateEvent(MessageId::MSG_UPDATE_VIDEO_SIZE, msg);
            break;
        case MessageId::MSG_SET_CAST_MODE:
            session->ProcessSetCastMode(msg);
            break;
        case MessageId::MSG_MIRROR_SEND_ACTION_EVENT_TO_PEERS:
            session->SendEventChange(msg.arg1_, msg.arg2_, param);
            break;
        case MessageId::MSG_SWITCH_TO_STREAM:
            if (!session->IsSink()) {
                session->ProcessMirrorToStream();
                session->ChangeDeviceState(DeviceState::MIRROR_TO_STREAM, session->GetCurrentRemoteDeviceId());
                session->TransferTo(session->mirrorToStreamState_);
            }
            break;
        case MessageId::MSG_PROCESS_TRIGGER_REQ:
            session->ProcessTriggerReq(msg);
            break;
        default:
            CLOGW("unsupported msg: %{public}s, in playing state", MESSAGE_ID_STRING[msgId].c_str());
            return false;
    }
    return true;
}

void CastSessionImpl::StreamToMirrorState::Enter()
{
    BaseState::Enter(SessionState::STREAM_TO_MIRROR);
}
 
void CastSessionImpl::StreamToMirrorState::Exit()
{
    BaseState::Exit();
}
 
bool CastSessionImpl::StreamToMirrorState::HandleMessage(const Message &msg)
{
    auto session = session_.promote();
    if (!session) {
        CLOGE("Session is invalid");
        return false;
    }
    BaseState::HandleMessage(msg);
    MessageId msgId = static_cast<MessageId>(msg.what_);
    std::string deviceId = msg.strArg_;
    switch (msgId) {
        case MessageId::MSG_PEER_RENDER_READY:
            CLOGI("MSG_PEER_RENDER_READY in");
            session->ProcessStreamToMirror();
            session->ChangeDeviceState(DeviceState::PLAYING, session->GetCurrentRemoteDeviceId());
            session->TransferTo(session->playingState_);
            session->SetMirrorToStreamState(false);
            break;
        case MessageId::MSG_PEER_RENDER_FAIL:
            CLOGI("MSG_PEER_RENDER_FAIL in");
            session->ChangeDeviceState(DeviceState::STREAM, session->GetCurrentRemoteDeviceId());
            session->TransferTo(session->streamState_);
            break;
        default:
            CLOGW("unsupported msg: %{public}s, in StreamToMirror state", MESSAGE_ID_STRING[msgId].c_str());
            return false;
    }
    return true;
}

void CastSessionImpl::DisconnectingState::Enter()
{
    auto session = session_.promote();
    if (!session) {
        CLOGE("Session is invalid");
        return;
    }

    BaseState::Enter(SessionState::DISCONNECTING);
    session->TransferTo(session->disconnectedState_);
}

void CastSessionImpl::DisconnectingState::Exit()
{
    BaseState::Exit();
}

bool CastSessionImpl::DisconnectingState::HandleMessage(const Message &msg)
{
    auto session = session_.promote();
    if (!session) {
        CLOGE("Session is invalid");
        return false;
    }

    BaseState::HandleMessage(msg);
    MessageId msgId = static_cast<MessageId>(msg.what_);

    switch (msgId) {
        case MessageId::MSG_CONNECT:
            CLOGI("in connecting state, defer message: %{public}d", msgId);
            session->DeferMessage(msg);
            break;
        default:
            CLOGW("unsupported msg: %{public}s, in disconnecting state", MESSAGE_ID_STRING[msgId].c_str());
            return false;
    }
    return true;
}
} // namespace CastEngineService
} // namespace CastEngine
} // namespace OHOS