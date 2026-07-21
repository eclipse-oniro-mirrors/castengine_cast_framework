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
 * Description: rtsp controller
 * Author: dingkang
 * Create: 2022-01-24
 */

#include "rtsp_controller.h"

#include <iterator>

#include "cast_device_data_manager.h"
#include "cast_engine_log.h"
#include "encrypt_decrypt.h"
#include "rtsp_package.h"
#include "utils.h"
#include "parameters.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineService {
namespace CastSessionRtsp {
DEFINE_CAST_ENGINE_LABEL("Cast-Rtsp-Controller");

std::shared_ptr<IRtspController> IRtspController::GetInstance(std::shared_ptr<IRtspListener> listener,
    ProtocolType protocolType, EndType endType)
{
    auto rtspController = std::make_shared<RtspController>(listener, protocolType, endType);
    if (!rtspController) {
        CLOGE("rtspController is nullptr");
        return nullptr;
    }
    rtspController->Init();

    return std::static_pointer_cast<IRtspController>(rtspController);
}

RtspController::RtspController(std::shared_ptr<IRtspListener> listener, ProtocolType protocolType, EndType endType)
    : protocolType_(protocolType), listener_(listener), endType_(endType)
{
    CLOGI("Out, endType %{public}d", endType);
}

RtspController::~RtspController()
{
    CLOGI("~RtspController in.");
}

void RtspController::Init()
{
    rtspNetManager_ = std::make_shared<RtspChannelManager>(shared_from_this(), protocolType_);
    ResponseFuncMapInit();
    RequestFuncMapInit();
}

std::shared_ptr<IChannelListener> RtspController::GetChannelListener()
{
    CLOGD("In, get channel listener.");
    return rtspNetManager_->GetChannelListener();
}

void RtspController::AddChannel(std::shared_ptr<Channel> channel, const CastInnerRemoteDevice &device)
{
    rtspNetManager_->AddChannel(channel, device);
    deviceId_ = device.deviceId;

    CLOGD("Out, deviceId %{public}s", Utils::Mask(deviceId_).c_str());
}

void RtspController::RemoveChannel(std::shared_ptr<Channel> channel)
{
    rtspNetManager_->RemoveChannel(channel);
    CLOGD("Out");
}

bool RtspController::Start(const ParamInfo &sourceParam, const uint8_t *sessionKey, uint32_t sessionKeyLength)
{
    this->paramInfo_ = sourceParam;
    negotiatedParamInfo_ = this->paramInfo_;
    if (!rtspNetManager_->StartSession(sessionKey, sessionKeyLength)) {
        CLOGE("StartSession failed");
        return false;
    }
    state_ = RtspEngineState::STATE_STARTED;
    CLOGD("Out");

    return true;
}

bool RtspController::Action(ActionType actionType)
{
    int action = static_cast<int>(actionType);
    CLOGD("Action in %{public}s endType %{public}d", ACTION_TYPE_STR[action].c_str(), endType_);

    std::string requestStr;
    // Source端同Sink端携带字段有差异，兼容处理;
    if (endType_ == EndType::CAST_SOURCE) {
        requestStr = RtspEncap::EncapActionRequest(actionType, paramInfo_.GetVersion(), ++currentSeq_);
        waitRsp_ = WaitResponse::WAITING_RSP_SET_PARAM_M5;
    } else {
        if (actionType >= ActionType::SETUP && actionType <= ActionType::SEND_EVENT_CHANGE) {
            CLOGD("ActionType::%{public}d", actionType);
        }
        switch (actionType) {
            case ActionType::PLAY:
                requestStr = RtspEncap::EncapPlayRequest(++currentSeq_, "", INVALID_VALUE);
                waitRsp_ = WaitResponse::WAITING_RSP_PLAY_M7;
                break;
            case ActionType::PAUSE:
                requestStr = RtspEncap::EncapPauseRequest(++currentSeq_, "");
                waitRsp_ = WaitResponse::WAITING_RSP_PAUSE_M9;
                break;
            case ActionType::TEARDOWN:
                requestStr = RtspEncap::EncapTearDownRequest(++currentSeq_, "");
                waitRsp_ = WaitResponse::WAITING_RSP_TEARDOWN_M8;
                break;

            default:
                CLOGE("No this action.");
                return false;
        }
    }
    rtspNetManager_->SendRtspData(requestStr);
    if (actionType == ActionType::TEARDOWN) {
        CLOGD("ActionType::TEARDOWN, stop engine.");
        StopEngine();
    }
    return true;
}

bool RtspController::SendAction(ActionType type)
{
    CLOGD("Send action method is %{public}d", type);
    std::string request = RtspEncap::EncapActionRequest(type, paramInfo_.GetVersion(), ++currentSeq_);
    if (request.empty()) {
        CLOGE("SendAction request is null.");
        return false;
    }
    return rtspNetManager_->SendRtspData(request);
}

bool RtspController::SendEventChange(int moduleId, int event, const std::string &param)
{
    CLOGD("Module %{public}d send event %{public}d param %{public}s", moduleId, event, param.c_str());
    std::string request =
        RtspEncap::EncapEventChangeRequest(moduleId, event, param, paramInfo_.GetVersion(), ++currentSeq_);
    if (request.empty()) {
        CLOGE("Send event change message is null.");
        return false;
    }

    return rtspNetManager_->SendRtspData(request);
}

void RtspController::OnPeerReady(bool isSoftbus)
{
    CLOGD("OnPeerReady isSoftbus %{public}d endType %{public}d.", isSoftbus, endType_);
    if (endType_ == EndType::CAST_SOURCE) {
        CLOGD("Is isSoftbus %{public}d, Source %{public}d waitting for sink msg.", isSoftbus, endType_);
        return;
    }
    bool isSendSuccess = true;

    if (isSoftbus) {
        isSendSuccess = SendOptionM1M2();
        waitRsp_ = WaitResponse::WAITING_RSP_OPT_M2;
    } else {
        EncryptDecrypt &instance = EncryptDecrypt::GetInstance();
        auto algStr = instance.GetEncryptInfo();
        if (algStr.size() >= ONE_ENCAP_ITEM_LEN) {
            algStr = algStr.substr(0, ONE_ENCAP_ITEM_LEN - 1);
        }
        int version = instance.GetVersion();
        CLOGD("AuthNeg: Get algStr is %{public}s version %{public}d", algStr.c_str(), version);

        std::string req = RtspEncap::EncapAnnounce(algStr, ++currentSeq_, version);
        isSendSuccess = rtspNetManager_->SendRtspData(req);
        waitRsp_ = WaitResponse::WAITING_RSP_ANNOUNCE;
    }

    if (!isSendSuccess) {
        CLOGE("Send rtsp data failed");
        listener_->OnError(ERROR_CODE_DEFAULT);
    }
}

bool RtspController::OnRequest(RtspParse &request)
{
    bool isSuccess = true;
    bool isMethodSupport = false;
    for (auto &func : requestFuncMap_) {
        if (Utils::StartWith(request.GetFirstLine(), func.first)) {
            isSuccess = (this->*requestFuncMap_[func.first])(request);
            isMethodSupport = true;
            break;
        }
    }

    if (!isMethodSupport) {
        isSuccess = SendErrorResponse(request, "405 Method Not Allowed");
    }

    if (!isSuccess && (listener_ != nullptr)) {
        CLOGE("OnRequest error");
        listener_->OnError(ERROR_CODE_DEFAULT);
    }

    return isSuccess;
}

bool RtspController::OnResponse(RtspParse &response)
{
    bool isSuccess = true;
    bool isMethodSupport = false;
    CLOGD("OnResponse in State:%{public}d", waitRsp_);

    if (RtspParse::ParseIntSafe(response.GetHeader()["cseq"]) == currentKeepAliveCseq_) {
        CLOGD("ProcessKaResponse");
        return ProcessCommonResponse(response);
    }

    for (auto &func : responseFuncMap_) {
        if (waitRsp_ == func.first) {
            isSuccess = (this->*responseFuncMap_[waitRsp_])(response);
            isMethodSupport = true;
            break;
        }
    }

    if (!isMethodSupport) {
        CLOGE("Response state, waitRsp is %{public}d", waitRsp_);
    }

    if (!isSuccess && (listener_ != nullptr)) {
        CLOGE("OnResponse error in State %{public}d", waitRsp_);
        listener_->OnError(ERROR_CODE_DEFAULT);
    }
    return isSuccess;
}

void RtspController::DetectKeepAliveFeature() const
{
    CLOGD("Detect keep alive feature, set cast keep alive interval.");
}

void RtspController::SetupPort(int serverPort, int remotectlPort, int cpPort)
{
    CLOGD("SetupPort");
    std::string rsp = RtspEncap::EncapSetupResponse(paramInfo_, currentSetUpSeq_, serverPort, remotectlPort, cpPort);
    bool isSuccess = rtspNetManager_->SendRtspData(rsp);
    state_ = RtspEngineState::STATE_ESTABLISHED;
    if (!isSuccess && (listener_ != nullptr)) {
        CLOGE("Send setup response error.");
        listener_->OnError(ERROR_CODE_DEFAULT);
    }

    return;
}

void RtspController::SendCastRenderReadyOption(int isReady)
{
    CLOGD("RenderReady: isReady %{public}d", isReady);
    std::string rsp = RtspEncap::EncapCastRenderReadyRequest(++currentSeq_, "", isReady);
    bool isSuccess = rtspNetManager_->SendRtspData(rsp);
    if (!isSuccess && (listener_ != nullptr)) {
        CLOGE("Send Cast Render Ready request error.");
        listener_->OnError(ERROR_CODE_DEFAULT);
        return;
    }
    waitRsp_ = WaitResponse::WAITING_RSP_NONE;
}

bool RtspController::DealAnnounceRequest(RtspParse &response)
{
    if (endType_ != EndType::CAST_SOURCE) {
        return true;
    }
    rtspNetManager_->SetNegAlgorithmId(negotiatedParamInfo_.GetEncryptionParamInfo().controlChannelAlgId);

    SendOptionM1M2();
    waitRsp_ = WaitResponse::WAITING_RSP_OPT_M1;

    CLOGI("Out, SendOptionM1M2.");
    return true;
}

void RtspController::OnPeerGone()
{
    state_ = RtspEngineState::STATE_STOPPING;
    CLOGE("Event onPeerGone flag");

    if (listener_ == nullptr) {
        CLOGE("rtspListener is null");
        return;
    }
    listener_->OnError(ERROR_CODE_DEFAULT);
}

void RtspController::OnTimeKeepAlive()
{
    SendKeepAliveRequest();
}

bool RtspController::StopEngine()
{
    CLOGD("StopEngine");
    state_ = RtspEngineState::STATE_STOPPED;
    rtspNetManager_->StopSession();
    return true;
}

std::set<std::string> RtspController::ParseCipherItem(const std::string &item) const
{
    if (item.empty()) {
        return {};
    }
    std::set<std::string> supportedCipherLists;
    std::vector<std::string> cipherLists;
    Utils::SplitString(item, cipherLists, ", ");
    for (size_t index = 0; index < cipherLists.size(); index++) {
        if (Utils::ToLower(cipherLists[index]) == EncryptDecrypt::CIPHER_AES_CTR_128) {
            supportedCipherLists.insert(EncryptDecrypt::CIPHER_AES_CTR_128);
        } else if (Utils::ToLower(cipherLists[index]) == EncryptDecrypt::CIPHER_AES_GCM_128) {
            supportedCipherLists.insert(EncryptDecrypt::CIPHER_AES_GCM_128);
        }
    }
    return supportedCipherLists;
}

bool RtspController::ProcessAnnounceRequest(RtspParse &request)
{
    if (endType_ == EndType::CAST_SOURCE) {
        std::string rsp = RtspEncap::EncapCommonResponse(request, STATUS_OK_STR);
        rtspNetManager_->SendRtspData(rsp);
    }
    std::string content = request.GetHeader()["encrypt_description"];
    if (content.empty() && (listener_ != nullptr)) {
        CLOGE("ProcessAnnounceRequest No encrypt_description.");

        return false;
    }

    std::string encryptStr = RtspParse::GetTargetStr(content, "encrypt_list=", COMMON_SEPARATOR);
    if (encryptStr.empty() && (listener_ != nullptr)) {
        CLOGE("Get encrypt str fail.");

        return false;
    }

    // only support ctr
    EncryptDecrypt &instance = EncryptDecrypt::GetInstance();
    int version = instance.GetVersion();
    CLOGI("AuthNeg: Get algStr is %{public}s version %{public}d", encryptStr.c_str(), version);

    std::set<std::string> cipherList = ParseCipherItem(encryptStr);
    if (cipherList.empty() && (listener_ != nullptr)) {
        CLOGE("cipherList is empty");
        return false;
    }

    EncryptionParamInfo encryptionParamInfo{};
    encryptionParamInfo.controlChannelAlgId = static_cast<uint32_t>(instance.GetControlEncryptCipher(cipherList));
    encryptionParamInfo.dataChannelAlgId = static_cast<uint32_t>(instance.GetMediaEncryptCipher(cipherList));
    negotiatedParamInfo_.SetEncryptionParamInfo(encryptionParamInfo);

    if (endType_ == EndType::CAST_SOURCE) {
        std::string sendStr;
        for (auto &cipher : cipherList) {
            if (sendStr.empty()) {
                sendStr = cipher;
                continue;
            }
            sendStr += ", " + cipher;
        }
        std::string req = RtspEncap::EncapAnnounce(sendStr, ++currentSeq_, version);
        rtspNetManager_->SendRtspData(req);
        waitRsp_ = WaitResponse::WAITING_RSP_ANNOUNCE;
    } else {
        std::string rsp = RtspEncap::EncapCommonResponse(request, STATUS_OK_STR);
        rtspNetManager_->SendRtspData(rsp);
        rtspNetManager_->SetNegAlgorithmId(negotiatedParamInfo_.GetEncryptionParamInfo().controlChannelAlgId);
    }
    CLOGD("Out.");
    return true;
}

bool RtspController::ProcessOptionRequest(RtspParse &request)
{
    CLOGD("Receive get option request M1.");
    double version = paramInfo_.GetVersion();
    int seqid = RtspParse::ParseIntSafe(request.GetHeader()["cseq"]);
    std::string response = RtspEncap::EncapResponseOption(version, seqid);
    bool isSuccess = rtspNetManager_->SendRtspData(response);
    if (!isSuccess) {
        CLOGE("Send M1 response error");
        return false;
    }

    if (endType_ == EndType::CAST_SOURCE) {
        SendGetParamM3();
        waitRsp_ = WaitResponse::WAITING_RSP_GET_PARAM_M3;
    } else {
        SendOptionM1M2();
        waitRsp_ = WaitResponse::WAITING_RSP_OPT_M2;
    }

    return true;
}

bool RtspController::ProcessSetupRequest(RtspParse &request)
{
    int port = INVALID_VALUE;
    currentSetUpSeq_ = RtspParse::ParseIntSafe(request.GetHeader()["cseq"]);
    if (negotiatedParamInfo_.GetDeviceTypeParamInfo().remoteDeviceType == DeviceType::DEVICE_CAST_PLUS ||
        negotiatedParamInfo_.GetSupportVtpOpt() != VtpType::VTP_NOT_SUPPORT_VIDEO ||
        protocolType_ == ProtocolType::HICAR ||
        protocolType_ == ProtocolType::SUPER_LAUNCHER) {
        port = ProcessGetPort(request);
    }
    if (listener_ != nullptr) {
        listener_->OnSetup(negotiatedParamInfo_, port, INVALID_VALUE, deviceId_);
    }
    return true;
}

bool RtspController::ProcessGetParameterRequestM3(RtspParse &request)
{
    CLOGD("Receive get param request M3.");
    int seqid = RtspParse::ParseIntSafe(request.GetHeader()["cseq"]);
    std::string response = RtspEncap::EncapResponseGetParamM3(paramInfo_, request, seqid);

    bool isSuccess = rtspNetManager_->SendRtspData(response);
    if (!isSuccess) {
        CLOGE("send rtsp response M3 fail.");
    }
    return isSuccess;
}

void RtspController::ProcessGetTrigger(RtspParse &request, const std::string &notifyTrigger) const
{
    CLOGD("Receive get trigger request %{public}s", notifyTrigger.c_str());
    std::string response = RtspEncap::EncapCommonResponse(request, STATUS_OK_STR);
    bool isSuccess = rtspNetManager_->SendRtspData(response);
    if (!isSuccess) {
        CLOGE("Send rtsp process set_param response fail.");
    }
    if (!notifyTrigger.empty()) {
        int trigger = RtspParse::ParseIntSafe(notifyTrigger);
        CLOGD("Trigger is %{public}d", trigger);
        if ((trigger != INVALID_VALUE) && (listener_ != nullptr)) {
            listener_->NotifyTrigger(trigger);
        }
    }
}

void RtspController::ProcessEventChangeRequest(RtspParse &request) const
{
    CLOGD("Receive event change data request.");
    // The response has been returned at the function call, there is no need to respond to the source.
    int moduleId = INVALID_VALUE;
    if (UnOrderedMapContains(request.GetHeader(), MODULE_ID)) {
        moduleId = RtspParse::ParseIntSafe(request.GetHeader()[MODULE_ID]);
    }
    int event = INVALID_VALUE;
    if (UnOrderedMapContains(request.GetHeader(), EVENT)) {
        event = RtspParse::ParseIntSafe(request.GetHeader()[EVENT]);
    }
    std::string param = "";
    if (UnOrderedMapContains(request.GetHeader(), PARAM)) {
        param = request.GetHeader()[PARAM];
    }
    CLOGD("Receive event change request module %{public}d event %{public}d", moduleId, event);
    if ((moduleId != INVALID_VALUE) && (event != INVALID_VALUE) && (listener_ != nullptr)) {
        listener_->NotifyEventChange(moduleId, event, param);
    }
}

bool RtspController::ProcessPlayRequest(RtspParse &request)
{
    CLOGD("Process play request in.");
    int port = INVALID_VALUE;
    bool isSuccess = Utils::StartWith(request.GetFirstLine(), "PLAY");
    if (!isSuccess) {
        CLOGE("Process play request error");
        port = ((negotiatedParamInfo_.GetSupportVtpOpt() != VtpType::VTP_NOT_SUPPORT_VIDEO)) ? ProcessGetPort(request) :
            INVALID_VALUE;
        return (listener_ != nullptr) && listener_->OnPlay(negotiatedParamInfo_, port, deviceId_) && isSuccess;
    }
    std::string rsp = RtspEncap::EncapCommonResponse(request, STATUS_OK_STR);
    rsp.append("Session: timeout=30").append("\n");
    rsp.append("Range: npt=now-").append("\n");

    isSuccess = rtspNetManager_->SendRtspData(rsp);
    if (!isSuccess) {
        CLOGE("SendRtspData fail.");
    }

    port = ((negotiatedParamInfo_.GetSupportVtpOpt() != VtpType::VTP_NOT_SUPPORT_VIDEO)) ? ProcessGetPort(request) :
        INVALID_VALUE;
    return (listener_ != nullptr) && listener_->OnPlay(negotiatedParamInfo_, port, deviceId_) && isSuccess;
}

bool RtspController::ProcessPauseRequest(RtspParse &request)
{
    CLOGD("Process pause request in.");
    bool isSuccess = Utils::StartWith(request.GetFirstLine(), "PAUSE");
    if (!isSuccess) {
        CLOGE("Process pause request error");
        return (listener_ != nullptr) && listener_->OnPause() && isSuccess;
    }
    std::string rsp = RtspEncap::EncapCommonResponse(request, STATUS_OK_STR);
    isSuccess = rtspNetManager_->SendRtspData(rsp);
    if (!isSuccess) {
        CLOGE("SendRtspData fail.");
    }

    return (listener_ != nullptr) && listener_->OnPause() && isSuccess;
}

bool RtspController::ProcessTearDownRequest(RtspParse &request)
{
    CLOGI("Receive sink teardown request.");

    if (!Utils::StartWith(request.GetFirstLine(), "Teardown")) {
        CLOGE("Process teardown request error");
        if (listener_ != nullptr) {
            listener_->OnTearDown();
        }
        return false;
    }
    std::string response = RtspEncap::EncapCommonResponse(request, STATUS_OK_STR);
    bool isSuccess = rtspNetManager_->SendRtspData(response);
    if (!isSuccess) {
        CLOGE("SendRtspData fail.");
    }
    StopEngine();

    if (listener_ != nullptr) {
        listener_->OnTearDown();
    }

    return isSuccess;
}

bool RtspController::ProcessRenderReadyRequest(RtspParse &request)
{
    CLOGD("Process render ready request in.");
    int readyFlag = INVALID_VALUE;
    std::string response = RtspEncap::EncapCommonResponse(request, STATUS_OK_STR);
    bool isSuccess = rtspNetManager_->SendRtspData(response);
    if (!isSuccess) {
        CLOGE("Process render ready request error");
        return listener_->OnPlayerReady(negotiatedParamInfo_, deviceId_, readyFlag) && isSuccess;
    }
    std::string notifyReadyFlag = request.GetHeader()["readyflag"];
    if (notifyReadyFlag.empty()) {
        CLOGE("Process render ready request error");
        return listener_->OnPlayerReady(negotiatedParamInfo_, deviceId_, readyFlag);
    }

    return listener_->OnPlayerReady(negotiatedParamInfo_, deviceId_, RtspParse::ParseIntSafe(notifyReadyFlag));
}

void RtspController::ProcessTriggerMethod(RtspParse &request, const std::string &triggerMethod)
{
    CLOGD("Receive trigger method is %{public}s", triggerMethod.c_str());
    std::string response = RtspEncap::EncapCommonResponse(request, STATUS_OK_STR);
    bool isSuccess = rtspNetManager_->SendRtspData(response);
    if (!isSuccess) {
        CLOGE("send rtsp common response fail.");
    }

    if (triggerMethod == ACTION_TYPE_STR[static_cast<int>(ActionType::SETUP)]) {
        if (negotiatedParamInfo_.GetProjectionMode() == ProjectionMode::STREAM) {
            CLOGI("ProcessTriggerMethod, stream mode.");
            listener_->ProcessStreamMode(negotiatedParamInfo_, deviceId_);
            return;
        }
        int port = 0;
        if (paramInfo_.GetSupportVtpOpt() != VtpType::VTP_NOT_SUPPORT_VIDEO) {
            port = listener_->StartMediaVtp(negotiatedParamInfo_);
            CLOGD("Encap Setup, StartMediaVtp result is %{public}d", port);
        }
        std::string requestStr = RtspEncap::EncapSetupRequest(++currentSeq_, "", port);
        rtspNetManager_->SendRtspData(requestStr);
        waitRsp_ = WaitResponse::WAITING_RSP_SETUP_M6;
    } else if (triggerMethod == ACTION_TYPE_STR[static_cast<int>(ActionType::PLAY)]) {
        CLOGD("Trigger method is %{public}s", triggerMethod.c_str());
        listener_->OnPlay(negotiatedParamInfo_, 0, deviceId_);
    } else if (triggerMethod == ACTION_TYPE_STR[static_cast<int>(ActionType::PAUSE)]) {
        listener_->OnPause();
    } else if (triggerMethod == ACTION_TYPE_STR[static_cast<int>(ActionType::TEARDOWN)]) {
        CLOGI("Receive teardown trigger method");
        listener_->OnTearDown();
    } else if (triggerMethod == ACTION_TYPE_STR[static_cast<int>(ActionType::SEND_EVENT_CHANGE)]) {
        ProcessEventChangeRequest(request);
    } else {
        CLOGE("No this triggerMethod %{public}s.", triggerMethod.c_str());
    }
}

bool RtspController::ProcessSetParamRequestM4(RtspParse &request)
{
    CLOGD("Process SetParameter M4 endType_ %{public}d.", endType_);
    std::string requestStr;
    if (UnOrderedMapContains(request.GetHeader(), "his_version")) {
        double version = RtspParse::ParseDoubleSafe(request.GetHeader()["his_version"]);
        negotiatedParamInfo_.SetVersion(version);
        CLOGD("Source HiSight version is %.2f", negotiatedParamInfo_.GetVersion());
    }
    if (UnOrderedMapContains(request.GetHeader(), "his_device_type")) {
        requestStr = request.GetHeader()["his_device_type"];
        ProcessSourceDeviceType(requestStr);
    }
    if (UnOrderedMapContains(request.GetHeader(), "his_video_formats")) {
        ProcessVideoInfo(request.GetHeader()["his_video_formats"]);
    }

    ProcessAudioInfo(request);

    if (UnOrderedMapContains(request.GetHeader(), "his_feature")) {
        ProcessFeatureSet(request.GetHeader()["his_feature"]);
    }
    if (UnOrderedMapContains(request.GetHeader(), "his_feature")) {
        ProcessSinkVtp(request.GetHeader()["his_vtp"]);
    }
    if (UnOrderedMapContains(request.GetHeader(), "his_extended_field")) {
        ProcessProjectionMode(request.GetHeader()["his_extended_field"]);
    }
    if (UnOrderedMapContains(request.GetHeader(), "his_uibc_capability")) {
        ProcessUibc(request.GetHeader()["his_uibc_capability"]);
        CLOGD("ProcessUibc finish.");
    } else {
        CLOGE("Don't support UIBC.");
        RemoteControlParamInfo remoteControlParamInfo{};
        remoteControlParamInfo.isSupportUibc = false;
        negotiatedParamInfo_.SetRemoteControlParamInfo(remoteControlParamInfo);
    }

    if (UnOrderedMapContains(request.GetHeader(), "his_media_capability")) {
        std::string mediaCapability = request.GetHeader()["his_media_capability"];
        std::string controllerCapability = request.GetHeader()["his_player_controller_capability"];
        CLOGD("OnData mediaCapability %{public}s controllerCapability %{public}s", mediaCapability.c_str(),
            controllerCapability.c_str());
        ProcessModuleCustomParams(mediaCapability, controllerCapability);
    } else {
        CLOGD("Not carry his_media_capability.");
        negotiatedParamInfo_.SetMediaCapability(nullptr);
    }

    if (endType_ == EndType::CAST_SINK) {
        std::string response = RtspEncap::EncapCommonResponse(request, STATUS_OK_STR);
        return rtspNetManager_->SendRtspData(response);
    }
    return true;
}

bool RtspController::ProcessSetParamRequest(RtspParse &request)
{
    CLOGD("Receive set param request.");

    std::string notifyTrigger = request.GetHeader()["trigger"];
    if (!notifyTrigger.empty()) {
        ProcessGetTrigger(request, notifyTrigger);
        return true;
    }
    std::string triggerMethod = request.GetHeader()["his_trigger_method"];
    if (!triggerMethod.empty()) {
        ProcessTriggerMethod(request, triggerMethod);
        return true;
    }

    return ProcessSetParamRequestM4(request);
}

bool RtspController::ProcessCommonResponse(RtspParse &response)
{
    if (response.GetStatusCode() != STATUS_OK) {
        CLOGE("Send common response status is not 200 ok, status code is %{public}d", response.GetStatusCode());
        return false;
    }
    return true;
}

void RtspController::ProcessAudioInfo(RtspParse &parseInfo)
{
    AudioProperty audioProperty = negotiatedParamInfo_.GetAudioProperty();

    std::string content = parseInfo.GetHeader()["his_audio_codecs"];
    if (!content.empty()) {
        audioProperty.codec = RtspParse::ParseUint32Safe(content);
    }

    content = parseInfo.GetHeader()["his_audio_formats"];
    if (!content.empty()) {
        ProcessAudioExpandInfo(content, audioProperty);
    }

    negotiatedParamInfo_.SetAudioProperty(audioProperty);
}

bool RtspController::ProcessGetParamM3Response(RtspParse &response)
{
    CLOGD("Process GetParam M3 response in.");
    if ((response.GetStatusCode() != STATUS_OK) || (!UnOrderedMapContains(response.GetHeader(), "his_version"))) {
        CLOGE("Process M3 Rsp Error, status code is %{public}d or not have his_version.", response.GetStatusCode());
        return false;
    }
    negotiatedParamInfo_.SetVersion(RtspParse::ParseDoubleSafe(response.GetHeader()["his_version"]));
    CLOGD("Sink HiSight version is %.2f", negotiatedParamInfo_.GetVersion());
    negotiatedParamInfo_.SetSupportUWB(RtspParse::ParseDoubleSafe(response.GetHeader()["his_support_uwb"]) == 1);
    
    // 考虑向前兼容性，需要先解析device type
    if (response.GetHeader()["his_device_type"].empty()) {
        ProcessSinkDeviceType("");
    } else {
        ProcessSinkDeviceType((*(response.GetHeader().find("his_device_type"))).second);
    }

    std::string content = response.GetHeader()["his_video_formats"];
    if (content.empty()) {
        CLOGE("Process M3 Rsp Error, sink not have his_video_formats.");
        return false;
    }
    ProcessVideoInfo(content);

    ProcessAudioInfo(response);

    ProcessFeatureSet(response.GetHeader()["his_feature"]);

    content = response.GetHeader()["his_uibc_capability"];
    if (content.empty()) {
        CLOGE("Sink doesn't appear to support UIBC.");
        RemoteControlParamInfo remoteControlParamInfo{};
        remoteControlParamInfo.isSupportUibc = false;
        negotiatedParamInfo_.SetRemoteControlParamInfo(remoteControlParamInfo);
    } else {
        ProcessUibc(content);
        CLOGD("Negotiated UIBC result is %{public}d", negotiatedParamInfo_.GetRemoteControlParamInfo().isSupportUibc);
    }
    ProcessSinkVtp(response.GetHeader()["his_vtp"]);
    // his_media_capability 处理
    ProcessModuleCustomParams(response.GetHeader()["his_media_capability"],
        response.GetHeader()["his_player_controller_capability"]);

    return true;
}

bool RtspController::ProcessSetParamM4Response(RtspParse &response)
{
    bool ret = true;
    CLOGD("Process SetParam M4 response in.");
    if (response.GetStatusCode() != STATUS_OK) {
        CLOGE("Send common response status is not 200 ok, status code is %{public}d", response.GetStatusCode());
        return false;
    }
    ret = SendAction(ActionType::SETUP);
    waitRsp_ = WaitResponse::WAITING_RSP_SET_PARAM_M5;
    return ret;
}

bool RtspController::ProcessSetParamM5Response(RtspParse &response)
{
    CLOGD("Process SetParam M5 response in.");
    waitRsp_ = WaitResponse::WAITING_RSP_NONE;
    if (response.GetStatusCode() != STATUS_OK) {
        CLOGE("Send common response status is not 200 ok, status code is %{public}d", response.GetStatusCode());
        return false;
    }

    return true;
}

bool RtspController::ProcessSetupM6Response(RtspParse &response)
{
    CLOGD("Process Setup M6 response.");
    if (response.GetStatusCode() != STATUS_OK) {
        return false;
    }

    std::string tmpStr = response.GetHeader()["transport"];
    if (tmpStr.empty()) {
        CLOGD("processSetupRequest, not have transport.");
        return false;
    }

    std::string portStr = RtspParse::GetTargetStr(tmpStr, "server_port=", COMMON_SEPARATOR);
    if (portStr.empty()) {
        CLOGD("Get server port str is empty.");
        return false;
    }
    int serverPort = RtspParse::ParseIntSafe(portStr);
    if (serverPort == INVALID_VALUE) {
        CLOGE("Get server port is -1.");
    }

    portStr = RtspParse::GetTargetStr(tmpStr, "remotectl_port=", COMMON_SEPARATOR);
    if (portStr.empty()) {
        CLOGE("Get remotectl port str is empty.");
        return false;
    }
    int remoteCtlPort = RtspParse::ParseIntSafe(portStr);
    if (remoteCtlPort == INVALID_VALUE) {
        CLOGE("Get remotectl port is -1.");
    }
    CLOGD("Media server port: %{public}d, remotectl server port: %{public}d", serverPort, remoteCtlPort);
    listener_->OnSetup(negotiatedParamInfo_, serverPort, remoteCtlPort, deviceId_);

    return true;
}

bool RtspController::ProcessPlayM7Response(RtspParse &response)
{
    CLOGD("WaitRsp state %{public}d receive play response, status %{public}d.", waitRsp_, response.GetStatusCode());
    waitRsp_ = WaitResponse::WAITING_RSP_NONE;
    return true;
}

bool RtspController::ProcessTearDownM8Response(RtspParse &response)
{
    CLOGI("WaitRsp state %{public}d receive teardown response, status %{public}d.", waitRsp_, response.GetStatusCode());
    listener_->OnTearDown();

    return true;
}

bool RtspController::ProcessPauseM9Response(RtspParse &response)
{
    CLOGD("WaitRsp state %{public}d receive pause response, status %{public}d.", waitRsp_, response.GetStatusCode());
    listener_->OnPause();
    return true;
}

bool RtspController::ProcessKaResponse(RtspParse &response)
{
    CLOGD("WaitRsp state %{public}d receive ka response, status %{public}d.", waitRsp_, response.GetStatusCode());
    waitRsp_ = WaitResponse::WAITING_RSP_NONE;
    return true;
}

bool RtspController::PreProcessUibc(const std::string &content, std::string &categoryList)
{
    CLOGD("In, %{public}s.", content.c_str());

    if (!paramInfo_.GetRemoteControlParamInfo().isSupportUibc) {
        CLOGE("ProcessUibc, local not support uibc.");
        return false;
    }

    categoryList = RtspParse::GetTargetStr(content, "input_category_list=", COMMON_SEPARATOR);
    if (categoryList.empty()) {
        CLOGE("ProcessUibc, not include input_category_list.");
        return false;
    }
    if (categoryList.find("HIDC") == std::string::npos && categoryList.find("GENERIC") == std::string::npos) {
        CLOGE("ProcessUibc, input_category_list not HIDC or GENERIC.");
        return false;
    }

    return true;
}

void RtspController::ProcessUibc(const std::string &content)
{
    CLOGD("In, %{public}s.", content.c_str());
    std::string categoryList;
    RemoteControlParamInfo remoteControlParamInfo{};

    if (!PreProcessUibc(content, categoryList)) {
        remoteControlParamInfo.isSupportUibc = false;
        negotiatedParamInfo_.SetRemoteControlParamInfo(remoteControlParamInfo);
        return;
    }

    remoteControlParamInfo.isSupportUibc = true;

    if (categoryList.find("GENERIC") != std::string::npos && paramInfo_.GetRemoteControlParamInfo().isSupportGeneric) {
        std::string genericStr = RtspParse::GetTargetStr(content, "generic_cap_list=", COMMON_SEPARATOR);
        if (genericStr.empty()) {
            CLOGE("No generic_cap_list.");
            return;
        }

        if (paramInfo_.GetRemoteControlParamInfo().genericList.size() <= 0) {
            CLOGE("Local genericList is empty.");
            return;
        }

        ProcessUibcDetermine(genericStr, remoteControlParamInfo.genericList,
            paramInfo_.GetRemoteControlParamInfo().genericList);
        remoteControlParamInfo.isSupportGeneric = true;
    }

    if (categoryList.find("HIDC") != std::string::npos && paramInfo_.GetRemoteControlParamInfo().isSupportHidc) {
        std::string hidcStr = RtspParse::GetTargetStr(content, "hidc_cap_list=", COMMON_SEPARATOR);
        if (hidcStr.empty()) {
            CLOGE("No hidc_cap_list.");
            return;
        }

        if (paramInfo_.GetRemoteControlParamInfo().hidcList.size() <= 0) {
            CLOGE("Local hidcList is empty.");
            return;
        }

        ProcessUibcDetermine(hidcStr, remoteControlParamInfo.hidcList,
            paramInfo_.GetRemoteControlParamInfo().hidcList);
        remoteControlParamInfo.isSupportHidc = true;
    }

    ProcessUibcVendor(content, remoteControlParamInfo);
    negotiatedParamInfo_.SetRemoteControlParamInfo(remoteControlParamInfo);

    return;
}

void RtspController::ProcessUibcVendor(const std::string &content, RemoteControlParamInfo &remoteControlParamInfo)
{
    CLOGD("In, %{public}s.", content.c_str());
    if (paramInfo_.GetRemoteControlParamInfo().isSupportVendor) {
        std::string vendorStr = RtspParse::GetTargetStr(content, "vendor_cap_list=", COMMON_SEPARATOR);
        if (vendorStr.empty()) {
            CLOGE("No vendor_cap_list.");
            return;
        }

        if (paramInfo_.GetRemoteControlParamInfo().vendorList.size() <= 0) {
            CLOGE("Local vendor_list is empty.");
            return;
        }

        ProcessUibcDetermine(vendorStr, remoteControlParamInfo.vendorList,
            paramInfo_.GetRemoteControlParamInfo().vendorList);
        remoteControlParamInfo.isSupportVendor = true;
    }
}

void RtspController::ProcessUibcDetermine(const std::string &givenStr, std::vector<std::string> &intersection,
    const std::vector<std::string> &localList)
{
    CLOGD("In, %{public}s.", givenStr.c_str());

    std::vector<std::string> splitStrings;
    Utils::SplitString(givenStr, splitStrings, ", ");
    intersection.clear();
    for (auto &iter : splitStrings) {
        auto it = std::find(localList.begin(), localList.end(), iter);
        if (it == localList.end()) {
            CLOGI("local not support event:%{public}s", iter.c_str());
            continue;
        }
        intersection.push_back(iter);
    }
}

void RtspController::ProcessSinkBitrate(const std::string &content, VideoProperty &videoProperty)
{
    CLOGD("In, %{public}s.", content.c_str());
    // 未设置的情况下，由media处理
    std::string strBitrate = RtspParse::GetTargetStr(content, "bitrate", COMMON_SEPARATOR);
    uint32_t bitrate = (!strBitrate.empty()) ? RtspParse::ParseUint32Safe(strBitrate) : 0;
    std::string strMinBitrate = RtspParse::GetTargetStr(content, "vbr-min", COMMON_SEPARATOR);
    std::string strMaxBitrate = RtspParse::GetTargetStr(content, "vbr-max", COMMON_SEPARATOR);
    uint32_t minBitrate = (!strMinBitrate.empty()) ? RtspParse::ParseUint32Safe(strMinBitrate) : 0;
    uint32_t maxBitrate = (!strMaxBitrate.empty()) ? RtspParse::ParseUint32Safe(strMaxBitrate) : 0;
    constexpr int twoValue = 2;

    if ((bitrate >= VIDEO_BITRATE_MIN) && (bitrate <= VIDEO_BITRATE_MAX)) {
        if (minBitrate >= VIDEO_BITRATE_MIN && maxBitrate <= VIDEO_BITRATE_MAX && bitrate >= minBitrate &&
            bitrate <= maxBitrate) {
            videoProperty.minBitrate = minBitrate;
            videoProperty.maxBitrate = maxBitrate;
        } else if (minBitrate == 0 && maxBitrate == 0) {
            CLOGD("Sink vbr range not setting, vbr-min and vbr-max using default value.");
        } else {
            CLOGD("Sink bitrate illegal %{public}d  min %{public}d max %{public}d", bitrate, minBitrate, maxBitrate);
            return;
        }
        videoProperty.bitrate = bitrate;
        return;
    }

    if (minBitrate >= VIDEO_BITRATE_MIN && maxBitrate <= VIDEO_BITRATE_MAX && minBitrate <= maxBitrate) {
        bitrate = static_cast<uint32_t>(round(static_cast<float>(minBitrate + maxBitrate) / twoValue));
        videoProperty.bitrate = bitrate;
        videoProperty.minBitrate = minBitrate;
        videoProperty.maxBitrate = maxBitrate;
    }
}

void RtspController::ProcessSinkVideoForResolution(const std::string &content, VideoProperty &videoProperty)
{
    CLOGD("In, %{public}s.", content.c_str());
    std::string strHeight = RtspParse::GetTargetStr(content, "height", COMMON_SEPARATOR);
    std::string strWidth = RtspParse::GetTargetStr(content, "width", COMMON_SEPARATOR);
    uint32_t height = (!strHeight.empty()) ? RtspParse::ParseUint32Safe(strHeight) : 0;
    uint32_t width = (!strWidth.empty()) ? RtspParse::ParseUint32Safe(strWidth) : 0;
    screenParam_[KEY_BEFORE_VIDEO_HEIGHT] = height;
    screenParam_[KEY_BEFORE_VIDEO_WIDTH] = width;
    if ((height > 0) && (width > 0)) {
        videoProperty.videoHeight = height;
        videoProperty.videoWidth = width;
    }
}

void RtspController::ProcessVideoInfo(const std::string &content)
{
    CLOGD("before: videoWidth %{public}d videoHeight %{public}d fps %{public}d codecType %{public}d gop %{public}d",
        negotiatedParamInfo_.GetVideoProperty().videoWidth, negotiatedParamInfo_.GetVideoProperty().videoHeight,
        negotiatedParamInfo_.GetVideoProperty().fps, negotiatedParamInfo_.GetVideoProperty().codecType,
        negotiatedParamInfo_.GetVideoProperty().gop);
    CLOGD("His_video_formats: %{public}s", content.c_str());

    VideoProperty videoProperty = negotiatedParamInfo_.GetVideoProperty();

    ProcessSinkVideoForResolution(content, videoProperty);

    std::string strCodecs = RtspParse::GetTargetStr(content, "codecs", COMMON_SEPARATOR);
    int codecs = (!strCodecs.empty()) ? RtspParse::ParseIntSafe(strCodecs) : 0;
    if (codecs > 0) {
        int localCodecs = static_cast<int>(paramInfo_.GetVideoProperty().codecType);
        videoProperty.codecType = static_cast<VideoCodecType>(std::min(localCodecs, codecs));
    }

    std::string strFps = RtspParse::GetTargetStr(content, "fps", COMMON_SEPARATOR);
    if (!strFps.empty()) {
        uint32_t fps = RtspParse::ParseUint32Safe(strFps);
        if ((fps >= VIDEO_FPS_MIN) && (fps <= VIDEO_FPS_60)) {
            videoProperty.fps = fps;
        }
    }
    std::string strGop = RtspParse::GetTargetStr(content, "gop", COMMON_SEPARATOR);
    if (!strGop.empty()) {
        int gop = RtspParse::ParseIntSafe(strGop);
        if ((gop == VIDEO_GOP_IPPP) || ((gop >= VIDEO_GOP_MIN) && (gop <= VIDEO_GOP_MAX))) {
            videoProperty.gop = static_cast<uint32_t>(gop);
        }
    }
    std::string strDpi = RtspParse::GetTargetStr(content, "dpi", COMMON_SEPARATOR);
    if (!strDpi.empty()) {
        videoProperty.dpi = RtspParse::ParseUint32Safe(strDpi);
    }
    std::string strScreenWidth = RtspParse::GetTargetStr(content, "scr-w", COMMON_SEPARATOR);
    std::string strScreenHeight = RtspParse::GetTargetStr(content, "scr-h", COMMON_SEPARATOR);
    uint32_t screenHeight = (!strScreenHeight.empty()) ? RtspParse::ParseUint32Safe(strScreenHeight) : 0;
    uint32_t screenWidth = (!strScreenWidth.empty()) ? RtspParse::ParseUint32Safe(strScreenWidth) : 0;
    if ((screenHeight > 0) && (screenWidth > 0)) {
        videoProperty.screenHeight = screenHeight;
        videoProperty.screenWidth = screenWidth;
    }
    ProcessSinkBitrate(content, videoProperty);
    negotiatedParamInfo_.SetVideoProperty(videoProperty);
    CLOGD("after: videoWidth %{public}d videoHeight %{public}d fps %{public}d codecType %{public}d gop %{public}d",
        negotiatedParamInfo_.GetVideoProperty().videoWidth, negotiatedParamInfo_.GetVideoProperty().videoHeight,
        negotiatedParamInfo_.GetVideoProperty().fps, negotiatedParamInfo_.GetVideoProperty().codecType,
        negotiatedParamInfo_.GetVideoProperty().gop);
    AddScreenParam();
}

void RtspController::AddScreenParam()
{
    CLOGI("AddScreenParam");

    screenParam_[KEY_SCREEN_HEIGHT] = negotiatedParamInfo_.GetVideoProperty().screenHeight;
    screenParam_[KEY_SCREEN_WIDTH] = negotiatedParamInfo_.GetVideoProperty().screenWidth;
    screenParam_[KEY_DPI] = negotiatedParamInfo_.GetVideoProperty().dpi;
    screenParam_[KEY_AFTER_VIDEO_HEIGHT] = negotiatedParamInfo_.GetVideoProperty().videoHeight;
    screenParam_[KEY_AFTER_VIDEO_WIDTH] = negotiatedParamInfo_.GetVideoProperty().videoWidth;
    std::string data = screenParam_.dump();
    listener_->NotifyScreenParam(data);
}

void RtspController::ProcessAudioExpandInfo(const std::string &content, AudioProperty &audioProperty)
{
    CLOGD("before: sampleRate %{public}d sampleBitWidth %{public}d channelConfig %{public}d bitrate %{public}d",
        negotiatedParamInfo_.GetAudioProperty().sampleRate, negotiatedParamInfo_.GetAudioProperty().sampleBitWidth,
        negotiatedParamInfo_.GetAudioProperty().channelConfig, negotiatedParamInfo_.GetAudioProperty().bitrate);
    CLOGD("his_audio_formats: %{public}s", content.c_str());

    std::string strSampleRate = RtspParse::GetTargetStr(content, "sample-rate", COMMON_SEPARATOR);
    if (!strSampleRate.empty()) {
        audioProperty.sampleRate = RtspParse::ParseUint32Safe(strSampleRate);
    }

    std::string strSampleBitWidth = RtspParse::GetTargetStr(content, "sample-bit-width", COMMON_SEPARATOR);
    if (!strSampleBitWidth.empty()) {
        audioProperty.sampleBitWidth = static_cast<uint8_t>(RtspParse::ParseIntSafe(strSampleBitWidth));
    }

    std::string strChannelConfig = RtspParse::GetTargetStr(content, "channel-config", COMMON_SEPARATOR);
    if (!strChannelConfig.empty()) {
        audioProperty.channelConfig = RtspParse::ParseUint32Safe(strChannelConfig);
    }

    std::string strBitrate = RtspParse::GetTargetStr(content, "bitrate", COMMON_SEPARATOR);
    if (!strBitrate.empty()) {
        audioProperty.bitrate = RtspParse::ParseUint32Safe(strBitrate);
    }

    CLOGD("after: sampleRate %{public}d sampleBitWidth %{public}d channelConfig %{public}d bitrate %{public}d",
        negotiatedParamInfo_.GetAudioProperty().sampleRate, negotiatedParamInfo_.GetAudioProperty().sampleBitWidth,
        negotiatedParamInfo_.GetAudioProperty().channelConfig, negotiatedParamInfo_.GetAudioProperty().bitrate);
}
void RtspController::ProcessSinkDeviceType(const std::string &content)
{
    CLOGD("In, %{public}s.", content.c_str());
    DeviceTypeParamInfo deviceTypeParamInfo{};
    deviceTypeParamInfo.localDeviceType = paramInfo_.GetDeviceTypeParamInfo().localDeviceType;
    deviceTypeParamInfo.localDeviceSubtype = paramInfo_.GetDeviceTypeParamInfo().localDeviceSubtype;
    if (content.empty()) {
        deviceTypeParamInfo.remoteDeviceType = paramInfo_.GetDeviceTypeParamInfo().remoteDeviceType;
        deviceTypeParamInfo.remoteDeviceSubtype = SubDeviceType::SUB_DEVICE_DEFAULT;
        CLOGD("The sink device type is not carried, device type %{public}d subtype %{public}d",
            deviceTypeParamInfo.remoteDeviceType, deviceTypeParamInfo.remoteDeviceSubtype);
    } else {
        std::string strDeviceType = RtspParse::GetTargetStr(content, "device_type", COMMON_SEPARATOR);
        int deviceType = (!strDeviceType.empty()) ? RtspParse::ParseIntSafe(strDeviceType) : 0;
        if (deviceType > 0) {
            deviceTypeParamInfo.remoteDeviceType = static_cast<DeviceType>(deviceType);
        }
        std::string strDeviceSubtype = RtspParse::GetTargetStr(content, "subtype", COMMON_SEPARATOR);
        int deviceSubtype = (!strDeviceSubtype.empty()) ? RtspParse::ParseIntSafe(strDeviceSubtype) : 0;
        if (deviceSubtype > 0) {
            deviceTypeParamInfo.remoteDeviceSubtype = static_cast<SubDeviceType>(deviceSubtype);
        }
        CLOGD("Sink device type %{public}d  subtype %{public}d", deviceType, deviceSubtype);
    }
    negotiatedParamInfo_.SetDeviceTypeParamInfo(deviceTypeParamInfo);
}

// sink端和source端remote device与local device相反
void RtspController::ProcessSourceDeviceType(const std::string &content)
{
    CLOGD("In, %{public}s.", content.c_str());
    DeviceTypeParamInfo deviceTypeParamInfo{};
    deviceTypeParamInfo.localDeviceType = paramInfo_.GetDeviceTypeParamInfo().localDeviceType;
    deviceTypeParamInfo.localDeviceSubtype = paramInfo_.GetDeviceTypeParamInfo().localDeviceSubtype;
    if (content.empty()) {
        deviceTypeParamInfo.remoteDeviceType = paramInfo_.GetDeviceTypeParamInfo().remoteDeviceType;
        deviceTypeParamInfo.remoteDeviceSubtype = SubDeviceType::SUB_DEVICE_DEFAULT;
        CLOGD("The device type is not carried, device type %{public}d subtype %{public}d",
            deviceTypeParamInfo.remoteDeviceType, deviceTypeParamInfo.remoteDeviceSubtype);
    } else {
        std::string strSourceDeviceType = RtspParse::GetTargetStr(content, "source_device_type", COMMON_SEPARATOR);
        int sourceDeviceType = (!strSourceDeviceType.empty()) ? RtspParse::ParseIntSafe(strSourceDeviceType) : 0;
        if (sourceDeviceType > 0) {
            deviceTypeParamInfo.remoteDeviceType = static_cast<DeviceType>(sourceDeviceType);
        }

        std::string strSrcDeviceSubtype = RtspParse::GetTargetStr(content, "source_subtype", COMMON_SEPARATOR);
        int srcDeviceSubtype = (!strSrcDeviceSubtype.empty()) ? RtspParse::ParseIntSafe(strSrcDeviceSubtype) : 0;
        if (srcDeviceSubtype > 0) {
            deviceTypeParamInfo.remoteDeviceSubtype = static_cast<SubDeviceType>(srcDeviceSubtype);
        }

        std::string strSinkDeviceType = RtspParse::GetTargetStr(content, "device_type", COMMON_SEPARATOR);
        int sinkDeviceType = (!strSinkDeviceType.empty()) ? RtspParse::ParseIntSafe(strSinkDeviceType) : 0;
        if (sinkDeviceType > 0) {
            deviceTypeParamInfo.localDeviceType = static_cast<DeviceType>(sinkDeviceType);
        }
        std::string strSinkDeviceSubtype = RtspParse::GetTargetStr(content, "subtype", COMMON_SEPARATOR);
        int sinkDeviceSubtype = (!strSinkDeviceSubtype.empty()) ? RtspParse::ParseIntSafe(strSinkDeviceSubtype) : 0;
        if (sinkDeviceSubtype > 0) {
            deviceTypeParamInfo.localDeviceSubtype = static_cast<SubDeviceType>(sinkDeviceSubtype);
        }
        CLOGD("Device source type %{public}d subtype %{public}d, sink type %{public}d, subtype %{public}d",
            sourceDeviceType, srcDeviceSubtype, sinkDeviceType, sinkDeviceSubtype);
    }
    negotiatedParamInfo_.SetDeviceTypeParamInfo(deviceTypeParamInfo);
}

void RtspController::ProcessFeatureSet(const std::string &content)
{
    CLOGD("In, %{public}s.", content.c_str());
    std::set<int> featureSet;
    if (content.empty() || paramInfo_.GetFeatureSet().empty()) {
        CLOGD("Source or sink feature is empty.");
        negotiatedParamInfo_.SetFeatureSet(featureSet);
        return;
    }
    std::string categoryList = RtspParse::GetTargetStr(content, "input_feature_set=", COMMON_SEPARATOR);
    if (categoryList.empty()) {
        CLOGE("Not include input_feature_set.");
        negotiatedParamInfo_.SetFeatureSet(featureSet);
        return;
    }

    std::vector<std::string> array;
    Utils::SplitString(Utils::Trim(categoryList), array, ", ");
    for (const std::string &parameter : array) {
        if (!parameter.empty()) {
            CLOGD("Sink feature: %{public}s", parameter.c_str());
            featureSet.insert(static_cast<int>(RtspParse::ParseIntSafe(parameter.c_str())));
        }
    }
    std::set<int> negotiateFeatureSet;
    set_intersection(paramInfo_.GetFeatureSet().begin(), paramInfo_.GetFeatureSet().end(), featureSet.begin(),
        featureSet.end(), inserter(negotiateFeatureSet, negotiateFeatureSet.begin()));
    negotiatedParamInfo_.SetFeatureSet(negotiateFeatureSet);
}

void RtspController::ProcessSinkVtp(const std::string &content)
{
    CLOGD("In, %{public}s.", content.c_str());
    if (content.empty()) {
        CLOGI("Peer not support vtp.");
        negotiatedParamInfo_.SetSupportVtpOpt(VtpType::VTP_NOT_SUPPORT_VIDEO);
        return;
    }
    auto tmp = content;
    tmp = Utils::ToLower(tmp);
    if (tmp == "supportav") {
        negotiatedParamInfo_.SetSupportVtpOpt(paramInfo_.GetSupportVtpOpt());
        CLOGI("Peer support vtp, Negotiated Vtp result is %{public}d", negotiatedParamInfo_.GetSupportVtpOpt());
    } else if (tmp == "supported" || tmp == "support_power_saving") {
        negotiatedParamInfo_.SetSupportVtpOpt(system::GetBoolParameter(PARAM_VTP_SUPPORT, true) ?
            VtpType::VTP_SUPPORT_VIDEO :
            VtpType::VTP_NOT_SUPPORT_VIDEO);
        CLOGI("Peer support vtp for video, Negotiated Vtp result is %{public}d",
            negotiatedParamInfo_.GetSupportVtpOpt());
    } else {
        CLOGI("Peer carries invalid vtp flag %{public}s", content.c_str());
        negotiatedParamInfo_.SetSupportVtpOpt(VtpType::VTP_NOT_SUPPORT_VIDEO);
    }
}

void RtspController::ProcessProjectionMode(const std::string &content)
{
    CLOGD("In, %{public}s.", content.c_str());
    if (content.empty()) {
        CLOGE("projection mode is empty, use default mode");
        return;
    }

    auto tmp = RtspParse::GetTargetStr(content, "projection_mode", COMMON_SEPARATOR);
    tmp = Utils::ToLower(tmp);
    if (tmp == "mirror") {
        negotiatedParamInfo_.SetProjectionMode(ProjectionMode::MIRROR);
    } else if (tmp == "media_resource") {
        negotiatedParamInfo_.SetProjectionMode(ProjectionMode::STREAM);
    }
    CLOGI("projection mode %{public}s", tmp.c_str());
}

void RtspController::ProcessModuleCustomParams(const std::string &mediaParams, const std::string &controllerParams)
{
    auto controllerParamsProcessed = controllerParams;
    auto strPos = controllerParams.find(COMMON_SEPARATOR);
    if (strPos != std::string::npos) {
        controllerParamsProcessed = controllerParams.substr(0, strPos);
    }

    auto mediaParamsProcessed = mediaParams;
    auto mediaStrPos = mediaParams.find(COMMON_SEPARATOR);
    if (strPos != std::string::npos) {
        mediaParamsProcessed = mediaParams.substr(0, mediaStrPos);
    }
    CLOGD("In, mediaParams:%{public}s controllerParams:%{public}s.", mediaParamsProcessed.c_str(),
        controllerParamsProcessed.c_str());

    if (listener_ == nullptr) {
        CLOGE("Listener is null.");
        return;
    }
    listener_->NotifyModuleCustomParamsNegotiation(mediaParamsProcessed, controllerParamsProcessed);
}

bool RtspController::SendOptionM1M2()
{
    CLOGD("Send Option M1M2");
    std::string request = RtspEncap::EncapRequestOption(++currentSeq_);
    if (request.empty()) {
        CLOGE("SendM1 request std::string is empty");
        return false;
    }
    return rtspNetManager_->SendRtspData(request);
}

bool RtspController::SendGetParamM3()
{
    CLOGD("Send GetParam M3");
    std::string request = RtspEncap::EncapRequestGetParameter(paramInfo_, ++currentSeq_);
    if (request.empty()) {
        CLOGE("SendM3 request std::string is empty");
        return false;
    }
    return rtspNetManager_->SendRtspData(request);
}

bool RtspController::SendSetParamM4()
{
    CLOGD("Send SetParam M4");
    std::string req =
        RtspEncap::EncapSetParameterM4Request(negotiatedParamInfo_, paramInfo_.GetVersion(), "", ++currentSeq_);
    if (req.empty()) {
        CLOGE("SendM4 request std::string is empty");
        return false;
    }
    return rtspNetManager_->SendRtspData(req);
}

bool RtspController::SendKeepAliveRequest()
{
    CLOGD("Send KeepAlive request");
    std::string request = RtspEncap::EncapKeepAliveRequest(++currentSeq_, paramInfo_.GetVersion());
    if (request.empty()) {
        CLOGE("SendM10 keep alive request std::string is empty");
        return false;
    }
    currentKeepAliveCseq_ = currentSeq_;
    CLOGD("SendM10, currentKeepAliveCseq :%{public}d", currentKeepAliveCseq_);
    bool isSuccess = rtspNetManager_->SendRtspData(request);
    waitRsp_ = WaitResponse::WAITING_RSP_KA;

    return isSuccess;
}

bool RtspController::SendErrorResponse(RtspParse &request, const std::string &errorDetail) const
{
    CLOGD("Send error response");
    std::string response;
    response.append("RTSP/1.0");
    response.append(errorDetail);
    response.append("\n");
    response.append("CSeq: ");
    response.append(std::to_string(request.GetSeq()));
    response.append("\n");
    return rtspNetManager_->SendRtspData(response);
}

void RtspController::ModuleCustomParamsNegotiationDone()
{
    CLOGD("Module custom params negotiation done, send M4 request.");
    SendSetParamM4();
    waitRsp_ = WaitResponse::WAITING_RSP_SET_PARAM_M4;
}

bool RtspController::UnOrderedMapContains(std::unordered_map<std::string, std::string> map,
    const std::string &key) const
{
    std::unordered_map<std::string, std::string>::iterator it = map.find(key);
    if (it == map.end()) {
        return false;
    } else {
        return true;
    }
}

const std::set<int> &RtspController::GetNegotiatedFeatureSet()
{
    return negotiatedParamInfo_.GetFeatureSet();
}

void RtspController::SetNegotiatedMediaCapability(const std::string &negotiationMediaParams)
{
    negotiatedParamInfo_.SetMediaCapability(negotiationMediaParams);
}

void RtspController::SetNegotiatedStreamCapability(const std::string &controllerParams)
{
    negotiatedParamInfo_.SetStreamCapability(controllerParams);
}

void RtspController::SetNegotiatedPlayerControllerCapability(const std::string &negotiationParams)
{
    negotiatedParamInfo_.SetPlayerControllerCapability(negotiationParams);
}

int RtspController::ProcessGetPort(RtspParse &request) const
{
    CLOGD("Process GetPort.");
    /* "Transport: RTP/AVP/UDP;unicast;client_port=xxx" */
    std::string tmpStr = request.GetHeader()["transport"];
    if (tmpStr.empty()) {
        CLOGD("processSetupRequest, not have transport.");
        return INVALID_VALUE;
    }

    std::string portStr = RtspParse::GetTargetStr(tmpStr, "client_port=", "");
    if (portStr.empty()) {
        CLOGD("Get vtpPort Str is empty.");
        return INVALID_VALUE;
    }
    int port = RtspParse::ParseIntSafe(portStr);
    if (port == INVALID_VALUE) {
        CLOGD("Get vtpPort is -1.");
    }
    return port;
}

void RtspController::ResponseFuncMapInit()
{
    responseFuncMap_[WaitResponse::WAITING_RSP_OPT_M1] = &RtspController::ProcessCommonResponse;
    responseFuncMap_[WaitResponse::WAITING_RSP_OPT_M2] = &RtspController::ProcessCommonResponse;
    responseFuncMap_[WaitResponse::WAITING_RSP_GET_PARAM_M3] = &RtspController::ProcessGetParamM3Response;
    responseFuncMap_[WaitResponse::WAITING_RSP_SET_PARAM_M4] = &RtspController::ProcessSetParamM4Response;
    responseFuncMap_[WaitResponse::WAITING_RSP_SET_PARAM_M5] = &RtspController::ProcessSetParamM5Response;
    responseFuncMap_[WaitResponse::WAITING_RSP_SETUP_M6] = &RtspController::ProcessSetupM6Response;
    responseFuncMap_[WaitResponse::WAITING_RSP_PLAY_M7] = &RtspController::ProcessPlayM7Response;
    responseFuncMap_[WaitResponse::WAITING_RSP_TEARDOWN_M8] = &RtspController::ProcessTearDownM8Response;
    responseFuncMap_[WaitResponse::WAITING_RSP_PAUSE_M9] = &RtspController::ProcessPauseM9Response;
    responseFuncMap_[WaitResponse::WAITING_RSP_KA] = &RtspController::ProcessKaResponse;
    responseFuncMap_[WaitResponse::WAITING_RSP_ANNOUNCE] = &RtspController::DealAnnounceRequest;
}

void RtspController::RequestFuncMapInit()
{
    requestFuncMap_["ANNOUNCE"] = &RtspController::ProcessAnnounceRequest;
    requestFuncMap_["OPTIONS"] = &RtspController::ProcessOptionRequest;
    requestFuncMap_["SETUP"] = &RtspController::ProcessSetupRequest;
    requestFuncMap_["PLAY"] = &RtspController::ProcessPlayRequest;
    requestFuncMap_["PAUSE"] = &RtspController::ProcessPauseRequest;
    requestFuncMap_["TEARDOWN"] = &RtspController::ProcessTearDownRequest;
    requestFuncMap_["RENDER_READY"] = &RtspController::ProcessRenderReadyRequest;
    requestFuncMap_["SET_PARAMETER"] = &RtspController::ProcessSetParamRequest;
    requestFuncMap_["GET_PARAMETER"] = &RtspController::ProcessGetParameterRequestM3;
}
} // namespace CastSessionRtsp
} // namespace CastEngineService
} // namespace CastEngine
} // namespace OHOS