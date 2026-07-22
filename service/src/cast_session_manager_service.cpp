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
 * Description: cast session manager service class
 * Author: zhangge
 * Create: 2022-06-15
 */

#include "cast_session_manager_service.h"

#include <algorithm>
#include <atomic>

#include <ipc_skeleton.h>
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "dm_constants.h"
#include "system_ability_definition.h"

#include "cast_engine_dfx.h"
#include "cast_engine_errors.h"
#include "cast_engine_log.h"
#include "cast_session_impl.h"
#include "cast_session_impl_class.h"
#include "connection_manager.h"
#include "discovery_manager.h"
#include "softbus_error_code.h"
#include "hisysevent.h"
#include "permission.h"
#include "utils.h"
#include "bundle_mgr_client.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineService {
DEFINE_CAST_ENGINE_LABEL("Cast-Service");

REGISTER_SYSTEM_ABILITY_BY_ID(CastSessionManagerService, CAST_ENGINE_SA_ID, false);

namespace SessionServer {

constexpr int ROLE_CLENT = 1;
constexpr int SOFTBUS_OK = 0;
constexpr char SESSION_NAME[] = "CastPlusSessionName";

int OnSessionOpened(int sessionId, int result)
{
    CLOGI("OnSessionOpened, session id = %{public}d, result is %{public}d", sessionId, result);
    if (sessionId <= INVALID_ID || result != SOFTBUS_OK) {
        auto device = CastDeviceDataManager::GetInstance().GetDeviceByTransId(sessionId);
        if (device == std::nullopt) {
            CLOGE("device is empty");
            return result;
        }
        return result;
    }
    int role = GetSessionSide(sessionId);
    if (role == ROLE_CLENT) {
        ConnectionManager::GetInstance().OnConsultSessionOpened(sessionId, true);
    } else {
        ConnectionManager::GetInstance().OnConsultSessionOpened(sessionId, false);
    }
    return SOFTBUS_OK;
}

void OnSessionClosed(int sessionId)
{
    CLOGI("OnSessionClosed, session id = %{public}d", sessionId);
    if (sessionId <= INVALID_ID) {
        return;
    }
}

void OnBytesReceived(int sessionId, const void *data, unsigned int dataLen)
{
    CLOGI("OnBytesReceived,session id = %{public}d, len = %{public}u", sessionId, dataLen);
    if (sessionId <= INVALID_ID || data == nullptr || dataLen == 0) {
        return;
    }
    int role = GetSessionSide(sessionId);
    if (role != ROLE_CLENT) {
        ConnectionManager::GetInstance().OnConsultDataReceived(sessionId, data, dataLen);
    }
}

ISessionListener g_SessionListener = {
    OnSessionOpened, OnSessionClosed, OnBytesReceived, nullptr, nullptr, nullptr
};

// true: softbus service is up, and the session server has been created;
// false: softbus service is up, but the session server failed to create;
// nullopt: softbus service is down.
std::optional<bool> WaitSoftBusInit()
{
    constexpr int sleepTime = 50; // uint: ms
    constexpr int retryTimes = 60 * 20; // total 60s
    int ret = SoftBusErrNo::SOFTBUS_TRANS_SESSION_ADDPKG_FAILED;
    int retryTime = 0;
    while (ret == SoftBusErrNo::SOFTBUS_TRANS_SESSION_ADDPKG_FAILED && retryTime < retryTimes) {
        CLOGI("create session server");
        ret = CreateSessionServer(PKG_NAME, SessionServer::SESSION_NAME, &SessionServer::g_SessionListener);
        if (ret == SOFTBUS_OK) {
            return true;
        }
        retryTime++;
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
    }

    if (retryTime == retryTimes) {
        CLOGE("softbus service is down.");
        return std::nullopt;
    }

    return false;
}

bool SetupSessionServer()
{
    int32_t result = SoftBusErrNo::SOFTBUS_ERR;
    int32_t retryTime = 0;
    constexpr int32_t retryTimes = 20;
    while (result != SessionServer::SOFTBUS_OK && retryTime < retryTimes) {
        CLOGI("retry create session server");
        result = CreateSessionServer(PKG_NAME, SessionServer::SESSION_NAME, &SessionServer::g_SessionListener);
        retryTime++;
    }
    if (result != SessionServer::SOFTBUS_OK) {
        CLOGE("CreateSessionServer failed, ret:%d", result);
        return false;
    }

    return true;
}
}

namespace {
constexpr int DEVICE_MANAGER_SA_ID = 4802;
} // namespace

CastSessionManagerService::CastSessionManagerService(int32_t saId, bool runOnCreate) : SystemAbility(saId, runOnCreate)
{
    CLOGD("construction in");
    myPid_ = getpid();
};

CastSessionManagerService::~CastSessionManagerService()
{
    CLOGD("destruction in");
}

void CastSessionManagerService::OnStart()
{
    bool ret = Publish(this);
    if (!ret) {
        CLOGE("Failed to publish cast session manager service");
        return;
    }

    AddSystemAbilityListener(CAST_ENGINE_SA_ID);
    auto result = SessionServer::WaitSoftBusInit();
    if (result == std::nullopt) {
        CastEngineDfx::WriteErrorEvent(SOURCE_CREATE_SESSION_SERVER_FAIL);
        CLOGE("softbus service is down.");
        return;
    }

    if (result) {
        hasServer_ = true;
        return;
    }

    if (!SessionServer::SetupSessionServer()) {
        CastEngineDfx::WriteErrorEvent(SOURCE_CREATE_SESSION_SERVER_FAIL);
        return;
    }
    hasServer_ = true;
}

void CastSessionManagerService::OnStop()
{
    CLOGI("Stop in");
    RemoveSessionServer(PKG_NAME, SessionServer::SESSION_NAME);
}

void CastSessionManagerService::OnActive(const SystemAbilityOnDemandReason& activeReason)
{
    CLOGI("OnActive in, reasonValue:%{public}s", activeReason.GetValue().c_str());
    isUnloading_.store(false);
}

namespace {
using namespace OHOS::DistributedHardware;
constexpr int AV_SESSION_UID = 6700;

class DiscoveryManagerListener : public IDiscoveryManagerListener {
public:
    DiscoveryManagerListener(sptr<CastSessionManagerService> service) : service_(service) {}

    void OnDeviceFound(const std::vector<CastInnerRemoteDevice> &devices) override
    {
        auto service = service_.promote();
        if (!service) {
            CLOGE("service is null");
            return;
        }

        std::vector<std::pair<CastRemoteDevice, bool>> remoteDevices;
        for (const auto &device : devices) {
            std::pair<CastRemoteDevice, bool> remoteDevice;
            remoteDevice.first = CastRemoteDevice{ device.deviceId, device.deviceName, device.deviceType,
                device.rawDeviceType, device.subDeviceType, device.ipAddress, device.channelType, device.capability,
                device.capabilityInfo, device.networkId, "", 0, nullptr, device.isLegacy, device.sessionId,
                device.drmCapabilities, device.mediumTypes };

            remoteDevice.second = (device.capabilityInfo & static_cast<uint32_t>(ProtocolType::CAST_PLUS_STREAM)) != 0;
            if (device.deviceType == DeviceType::DEVICE_TYPE_2IN1) {
                remoteDevice.second = true;
            }
            remoteDevices.push_back(remoteDevice);
        }
        service->ReportDeviceFound(remoteDevices);
    }

private:
    wptr<CastSessionManagerService> service_;
};
} // namespace

int ConnectionManagerListener::NotifySessionIsReady()
{
    auto service = service_.promote();
    if (!service) {
        CLOGE("service is null");
        return INVALID_ID;
    }

    sptr<ICastSessionImpl> session;
    service->CreateCastSession({}, session);
    if (session == nullptr) {
        return INVALID_ID;
    }

    std::string sessionId{};
    session->GetSessionId(sessionId);
    return Utils::StringToInt((sessionId));
}

void ConnectionManagerListener::ReportSessionCreate(int castSessionId)
{
    auto service = service_.promote();
    if (!service) {
        CLOGE("service is null");
        return;
    }

    sptr<ICastSessionImpl> castSession;
    if (service->GetCastSession(std::to_string(castSessionId), castSession) == CAST_ENGINE_SUCCESS) {
        service->ReportSessionCreate(castSession);
        return;
    }
    CLOGE("Invalid castSessionId: %d.", castSessionId);
}

bool ConnectionManagerListener::NotifyRemoteDeviceIsReady(int castSessionId, const CastInnerRemoteDevice &device)
{
    auto service = service_.promote();
    if (!service) {
        CLOGE("service is null");
        return false;
    }

    sptr<ICastSessionImpl> session;
    service->GetCastSession(std::to_string(castSessionId), session);
    if (session == nullptr) {
        CLOGE("Session is null when consultation data comes!");
        return false;
    }

    if (!session->AddDevice(device)) {
        CLOGE("Session addDevice fail");
        return false;
    }

    ConnectionManager::GetInstance().NotifyConnectStage(device, ConnectStageResult::CONNECT_START);
    return true;
}

void ConnectionManagerListener::NotifyDeviceIsOffline(const std::string &deviceId)
{
    auto service = service_.promote();
    if (!service) {
        CLOGE("service is null");
        return;
    }
    service->ReportDeviceOffline(deviceId);
    CLOGD("OnDeviceOffline out");
}

void ConnectionManagerListener::GrabDevice(int32_t sessionId)
{
    auto service = service_.promote();
    if (!service) {
        CLOGE("service is null");
        return;
    }

    sptr<ICastSessionImpl> session;
    service->GetCastSession(std::to_string(sessionId), session);
    if (!session) {
        CLOGE("The session is null. Failed to obtain the session.");
        return;
    }
    session->Release();
}

int32_t ConnectionManagerListener::GetSessionProtocolType(int sessionId, ProtocolType &protocolType)
{
    CLOGI("GetSessionProtocolType in");

    auto service = service_.promote();
    if (!service) {
        CLOGE("service is null");
        return CAST_ENGINE_ERROR;
    }

    sptr<ICastSessionImpl> session;
    service->GetCastSession(std::to_string(sessionId), session);
    if (!session) {
        CLOGE("The session is null. Failed to obtain the session.");
        return CAST_ENGINE_ERROR;
    }
    return session->GetSessionProtocolType(protocolType);
}

int32_t ConnectionManagerListener::SetSessionProtocolType(int sessionId, ProtocolType protocolType)
{
    CLOGI("SetSessionProtocolType in");

    auto service = service_.promote();
    if (!service) {
        CLOGE("service is null");
        return CAST_ENGINE_ERROR;
    }

    sptr<ICastSessionImpl> session;
    service->GetCastSession(std::to_string(sessionId), session);
    if (!session) {
        CLOGE("The session is null. Failed to obtain the session.");
        return CAST_ENGINE_ERROR;
    }
    session->SetSessionProtocolType(protocolType);
    return CAST_ENGINE_SUCCESS;
}

bool ConnectionManagerListener::LoadSinkSA(const std::string &networkId)
{
    auto service = service_.promote();
    if (!service) {
        CLOGE("service is null");
        return false;
    }
    return service->LoadSinkSA(networkId);
}

int32_t CastSessionManagerService::RegisterListener(sptr<ICastServiceListenerImpl> listener)
{
    CLOGI("RegisterListener in");
    HiSysEventWrite(CAST_ENGINE_DFX_DOMAIN_NAME, "CAST_ENGINE_EVE", HiviewDFX::HiSysEvent::EventType::STATISTIC,
        "SEQUENTIAL_ID", CastEngineDfx::GetSequentialId(), "BIZ_PACKAGE_NAME", CastEngineDfx::GetBizPackageName());

    SharedWLock lock(mutex_);
    if (listener == nullptr) {
        CLOGE("RegisterListener failed, listener is null");
        return CAST_ENGINE_ERROR;
    }

    bool needInitMore = !HasListenerLocked();
    if (!AddListenerLocked(listener)) {
        return CAST_ENGINE_ERROR;
    }

    if (needInitMore) {
        if (!CheckAndWaitDevieManagerServiceInit()) {
            CLOGE("Wait DM SA load timeout");
            ReleaseLocked();
            return CAST_ENGINE_ERROR;
        }

        DiscoveryManager::GetInstance().Init(std::make_shared<DiscoveryManagerListener>(this));
        ConnectionManager::GetInstance().Init(std::make_shared<ConnectionManagerListener>(this));
        sessionMap_.clear();
    }

    serviceStatus_ = ServiceStatus::CONNECTED;
    CLOGI("RegisterListener out");
    return CAST_ENGINE_SUCCESS;
}

int32_t CastSessionManagerService::UnregisterListener()
{
    SharedWLock lock(mutex_);
    CLOGI("UnregisterListener in");

    return RemoveListenerLocked(IPCSkeleton::GetCallingPid());
}

int32_t CastSessionManagerService::Release()
{
    SharedWLock lock(mutex_);
    CLOGI("Release in");
    if (!Permission::CheckPidPermission()) {
        return ERR_NO_PERMISSION;
    }

    return ReleaseLocked();
}

int32_t CastSessionManagerService::ReleaseLocked()
{
    CLOGI("ReleaseLocked in");
    serviceStatus_ = ServiceStatus::DISCONNECTED;
    ReportServiceDieLocked();

    ClearListenersLocked();
    DiscoveryManager::GetInstance().Deinit();
    ConnectionManager::GetInstance().Deinit();
    sessionMap_.clear();
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        CLOGE("get samgr failed");
        return CAST_ENGINE_ERROR;
    }
    int32_t ret = samgr->UnloadSystemAbility(CAST_ENGINE_SA_ID);
    if (ret != ERR_OK) {
        CLOGE("remove system ability failed");
        return CAST_ENGINE_ERROR;
    }
    CLOGI("Release success done");
    return CAST_ENGINE_SUCCESS;
}

int32_t CastSessionManagerService::SetLocalDevice(const CastLocalDevice &localDevice)
{
    CLOGD("SetLocalDevice in");
    SharedWLock lock(mutex_);
    if (!Permission::CheckPidPermission()) {
        return ERR_NO_PERMISSION;
    }
    localDevice_ = localDevice;
    return CAST_ENGINE_SUCCESS;
}

int32_t CastSessionManagerService::GetCastSession(std::string sessionId, sptr<ICastSessionImpl> &castSession)
{
    SharedRLock lock(mutex_);
    if (!Permission::CheckPidPermission()) {
        return ERR_NO_PERMISSION;
    }

    auto session = GetCastSessionInner(sessionId);
    if (session == nullptr) {
        return ERR_SESSION_NOT_EXIST;
    }
    castSession = session;
    return CAST_ENGINE_SUCCESS;
}

sptr<ICastSessionImpl> CastSessionManagerService::GetCastSessionInner(std::string sessionId)
{
    auto innerSessionId = Utils::StringToInt(sessionId);
    auto it = sessionMap_.find(innerSessionId);
    if (it == sessionMap_.end()) {
        CLOGE("No sessionId=%{public}d session.", innerSessionId);
        return nullptr;
    }
    return it->second;
}

int32_t CastSessionManagerService::CreateCastSession(const CastSessionProperty &property,
    sptr<ICastSessionImpl> &castSession)
{
    SharedWLock lock(mutex_);
    CLOGI("CreateCastSession in, protocol:%{public}d, endType:%{public}d.", property.protocolType, property.endType);
    if (serviceStatus_ != ServiceStatus::CONNECTED) {
        CLOGE("not connected");
        return ERR_SERVICE_STATE_NOT_MATCH;
    }

    if (localDevice_.deviceId.empty()) {
        CastLocalDevice local;
        int32_t ret = ConnectionManager::GetInstance().GetLocalDeviceInfo(local);
        if (ret != DM_OK) {
            return ret;
        }
        localDevice_ = local;
    }

    for (auto it = sessionMap_.begin(); it != sessionMap_.end();) {
        ProtocolType protocolType;
        if (it->second->GetSessionProtocolType(protocolType) == CAST_ENGINE_ERROR) {
            CLOGE("GetSessionProtocolType failed");
            return CAST_ENGINE_ERROR;
        }
        auto curSessionState = it->second->GetSessionState();
        if (protocolType == ProtocolType::CAST_PLUS_MIRROR &&
            property.protocolType == ProtocolType::CAST_PLUS_STREAM &&
            curSessionState == static_cast<uint8_t>(SessionState::PLAYING)) {
            CLOGI("MirrorSession exits, return mirrorSession.");
            castSession = it->second;
            return CAST_ENGINE_SUCCESS;
        }
        it++;
    }

    return CAST_ENGINE_SUCCESS;
}

bool CastSessionManagerService::DestroyCastSession(int32_t sessionId)
{
    CLOGD("DestroyCastSession in");
    sptr<ICastSessionImpl> session;
    {
        SharedWLock lock(mutex_);
        auto it = sessionMap_.find(sessionId);
        if (it == sessionMap_.end()) {
            CLOGE("Cast session(%d) has gone.", sessionId);
            return true;
        }
        session = it->second;
        sessionMap_.erase(it);
    }

    ConnectionManager::GetInstance().RemoveSessionListener(sessionId);
    ConnectionManager::GetInstance().UpdateGrabState(false, -1);
    session->Stop();
    CLOGD("Session refcount is %d, session count:%zu", session->GetSptrRefCount(), sessionMap_.size());
    return true;
}

int32_t CastSessionManagerService::SetSinkSessionCapacity(int sessionCapacity)
{
    CLOGD("SetSinkSessionCapacity in, sessionCapacity = %d", sessionCapacity);
    SharedRLock lock(mutex_);
    if (!Permission::CheckPidPermission()) {
        return ERR_NO_PERMISSION;
    }
    sessionCapacity_ = sessionCapacity;
    return CAST_ENGINE_SUCCESS;
}

int32_t CastSessionManagerService::StartDiscovery(int protocols, std::vector<std::string> drmSchemes)
{
    CLOGI("StartDiscovery in, protocolType = %{public}d, drm shcheme size = %{public}zu", protocols, drmSchemes.size());

    DiscoveryManager::GetInstance().StartDiscovery(protocols, drmSchemes);
    return CAST_ENGINE_SUCCESS;
}

int32_t CastSessionManagerService::StopDiscovery()
{
    CLOGI("StopDiscovery in");
    SharedRLock lock(mutex_);
    if (!Permission::CheckPidPermission()) {
        return ERR_NO_PERMISSION;
    }
    DiscoveryManager::GetInstance().StopDiscovery();
    return CAST_ENGINE_SUCCESS;
}

int32_t CastSessionManagerService::StartDeviceLogging(int32_t fd, uint32_t maxSize)
{
    CLOGI("StartDeviceLogging in");
    if (IPCSkeleton::GetCallingUid() != AV_SESSION_UID) {
        if (fd >= 0) {
            close(fd);
        }
        return static_cast<int32_t>(LogCodeId::UID_MISMATCH);
    }
    if (fd >= 0) {
        close(fd);
    }

    // 待实现 DfxCastEngineStartDeviceLogging
    return CAST_ENGINE_ERROR;
}

int32_t CastSessionManagerService::SetDiscoverable(bool enable)
{
    CLOGI("SetDiscoverable in, enable = %{public}d", enable);
    SharedRLock lock(mutex_);
    if (!Permission::CheckPidPermission()) {
        return ERR_NO_PERMISSION;
    }

    if (enable) {
        if (ConnectionManager::GetInstance().EnableDiscoverable() &&
            DiscoveryManager::GetInstance().StartAdvertise()) {
            return CAST_ENGINE_SUCCESS;
        }
    } else {
        if (ConnectionManager::GetInstance().DisableDiscoverable() &&
            DiscoveryManager::GetInstance().StopAdvertise()) {
            return CAST_ENGINE_SUCCESS;
        }
    }
    return CAST_ENGINE_ERROR;
}

void CastSessionManagerService::ReleaseServiceResources(pid_t pid, uid_t uid)
{
    {
        SharedWLock lock(mutex_);
        RemoveListenerLocked(pid);

        CLOGI("Release service resources");
        for (auto it = sessionMap_.begin(); it != sessionMap_.end();) {
            if (it->second->ReleaseSessionResources(pid, uid)) {
                sessionMap_.erase(it++);
                continue;
            }
            it++;
        }
        if (HasListenerLocked()) {
            return;
        }
    }

    if (Release() != CAST_ENGINE_SUCCESS) {
        CLOGE("Release service resources failed");
    }
}

void CastSessionManagerService::AddClientDeathRecipientLocked(pid_t pid, uid_t uid,
    sptr<ICastServiceListenerImpl> listener)
{
    sptr<CastEngineClientDeathRecipient> deathRecipient(
        new (std::nothrow) CastEngineClientDeathRecipient(wptr<CastSessionManagerService>(this), pid, uid));
    if (deathRecipient == nullptr) {
        CLOGE("Alloc death recipient filed");
        return;
    }
    auto obj = listener->AsObject();
    if (!obj) {
        CLOGE("listener->AsObject() is nullptr");
        return;
    }
    if (!obj->AddDeathRecipient(deathRecipient)) {
        CLOGE("Add cast client death recipient failed");
        return;
    }
    CLOGD("add death recipient pid:%d", pid);
    deathRecipientMap_[pid] = deathRecipient;
}

void CastSessionManagerService::RemoveClientDeathRecipientLocked(pid_t pid, sptr<ICastServiceListenerImpl> listener)
{
    auto it = deathRecipientMap_.find(pid);
    if (it != deathRecipientMap_.end()) {
        auto obj = listener->AsObject();
        if (obj) {
            obj->RemoveDeathRecipient(it->second);
        }
        deathRecipientMap_.erase(it);
        CLOGD("remove death recipient pid:%d", pid);
    }
}

bool CastSessionManagerService::AddListenerLocked(sptr<ICastServiceListenerImpl> listener)
{
    pid_t pid = IPCSkeleton::GetCallingPid();
    uid_t uid = static_cast<uid_t>(IPCSkeleton::GetCallingUid());
    Permission::SavePid(pid);
    if (listeners_.count(pid) == 0) {
        listeners_[pid] = std::make_pair(listener, uid);
        AddClientDeathRecipientLocked(pid, uid, listener);
        return true;
    }

    CLOGE("Listener of process(%u) has been registered.", pid);
    return false;
}

int32_t CastSessionManagerService::RemoveListenerLocked(pid_t pid)
{
    Permission::RemovePid(pid);
    int32_t ret = CAST_ENGINE_ERROR;
    if (listeners_.count(pid) == 1) {
        RemoveClientDeathRecipientLocked(pid, listeners_[pid].first);
        listeners_.erase(pid);
        ret = CAST_ENGINE_SUCCESS;
    }

    if (listeners_.empty()) {
        ReleaseLocked();
    }

    return ret;
}

void CastSessionManagerService::ClearListenersLocked()
{
    listeners_.clear();
    Permission::ClearPids();
}

bool CastSessionManagerService::HasListenerLocked()
{
    return listeners_.size() > 0;
}

void CastSessionManagerService::ReportServiceDieLocked()
{
    pid_t pid = IPCSkeleton::GetCallingPid();
    if (pid == myPid_) {
        for (const auto &listener : listeners_) {
            listener.second.first->OnServiceDied();
        }
        return;
    }

    if (listeners_.count(pid) == 1) {
        listeners_[pid].first->OnServiceDied();
        return;
    }
}

void CastSessionManagerService::ReportDeviceFound(const std::vector<std::pair<CastRemoteDevice, bool>> &deviceList)
{
    CLOGI("ReportDeviceFound in");

    SharedRLock lock(mutex_);
    for (const auto &listener : listeners_) {
        std::vector<CastRemoteDevice> deviceInfo;
        for (const auto &device : deviceList) {
            if (listener.second.second == AV_SESSION_UID && !device.second) {
                CLOGI("device don't support stream capability, device name is %s", device.first.deviceName.c_str());
                continue;
            }
            if (listener.second.second != AV_SESSION_UID && device.first.deviceType == DeviceType::DEVICE_TYPE_2IN1) {
                CLOGI("No need to report hwcast , device name is %s", device.first.deviceName.c_str());
                continue;
            }
            deviceInfo.push_back(device.first);
        }
		
        listener.second.first->OnDeviceFound(deviceInfo);
    }
}

void CastSessionManagerService::ReportSessionCreate(const sptr<ICastSessionImpl> &castSession)
{
    SharedRLock lock(mutex_);
    for (const auto &listener : listeners_) {
        listener.second.first->OnSessionCreated(castSession);
    }
}

void CastSessionManagerService::ReportDeviceOffline(const std::string &deviceId)
{
    SharedRLock lock(mutex_);
    for (const auto &listener : listeners_) {
        listener.second.first->OnDeviceOffline(deviceId);
    }
}

bool CastSessionManagerService::LoadSinkSA(const std::string &networkId)
{
    CLOGI("LoadSinkSA in");
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        CLOGE("Failed to get SA manager");
        return false;
    }
    sptr<CastServiceLoadCallback> loadCallback = new (std::nothrow) CastServiceLoadCallback();
    if (loadCallback == nullptr) {
        CLOGE("Failed to new object");
        return false;
    }

    constexpr int32_t sleepTimeMs = 30;
    constexpr int32_t retryTimeMax = 150; // The service startup timeout interval is 4s.
    int32_t result = samgr->LoadSystemAbility(CAST_ENGINE_SA_ID, networkId, loadCallback);
    for (int32_t retryTime = 1; (result != ERR_OK) && (retryTime < retryTimeMax); ++retryTime) {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepTimeMs));
        result = samgr->LoadSystemAbility(CAST_ENGINE_SA_ID, networkId, loadCallback);
        CLOGD("Attemp to reLoad SA for the %d time.", retryTime);
    }

    if (result != ERR_OK) {
        CLOGE("systemAbilityId: %d load failed, result code: %d", CAST_ENGINE_SA_ID, result);
        return false;
    } else {
        CLOGI("Load sink SA success!");
        return true;
    }
}

bool CastSessionManagerService::CheckAndWaitDevieManagerServiceInit()
{
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        CLOGE("get samgr failed");
        return false;
    }
    constexpr int32_t sleepTimeMs = 50;
    constexpr int32_t retryTimes = 200;
    int32_t retryTime = 0;

    while (samgr->CheckSystemAbility(DEVICE_MANAGER_SA_ID) == nullptr && (retryTime < retryTimes)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepTimeMs));
        retryTime++;
    }
    CLOGI("retryTime:%{public}d", retryTime);

    return retryTime < retryTimes;
}

void CastSessionManagerService::CastEngineClientDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &object)
{
    CLOGI("Client died, need release resources, client pid_: %{public}d", pid_);
    sptr<CastSessionManagerService> service = service_.promote();
    std::string bundleName;
    AppExecFwk::BundleMgrClient client;
    if (client.GetNameForUid(uid_, bundleName) != ERR_OK) {
        CLOGE("GetBundleNameByUid failed");
    }

    if (service == nullptr) {
        CLOGE("ServiceStub is nullptr");
        return;
    }
    service->ReleaseServiceResources(pid_, uid_);
}

void CastSessionManagerService::CastServiceLoadCallback::OnLoadSystemAbilitySuccess(
    int32_t systemAbilityId, const sptr<IRemoteObject> &remoteObject)
{
    CLOGI("In systemAbilityId: %d", systemAbilityId);

    if (systemAbilityId != CAST_ENGINE_SA_ID) {
        CLOGE("Start aystemabilityId is not sinkSAId!");
        return;
    }

    if (remoteObject == nullptr) {
        CLOGE("RemoteObject is nullptr.");
        return;
    }
}

void CastSessionManagerService::CastServiceLoadCallback::OnLoadSystemAbilityFail(int32_t systemAbilityId)
{
    CLOGI("In systemAbilityId: %d.", systemAbilityId);

    if (systemAbilityId != CAST_ENGINE_SA_ID) {
        CLOGE("Start aystemabilityId is not sinkSAId!");
        return;
    }
}
} // namespace CastEngineService
} // namespace CastEngine
} // namespace OHOS
