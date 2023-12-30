/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: Cast session interface.
 * Author: zhangge
 * Create: 2022-06-15
 */

#ifndef I_CAST_SESSION_H
#define I_CAST_SESSION_H

#include "cast_engine_common.h"
#include "oh_remote_control_event.h"
#include "i_stream_player.h"
#include "i_mirror_player.h"

namespace OHOS {
namespace CastEngine {
class EXPORT ICastSessionListener {
public:
    ICastSessionListener() = default;
    ICastSessionListener(const ICastSessionListener &) = delete;
    ICastSessionListener &operator = (const ICastSessionListener &) = delete;
    ICastSessionListener(ICastSessionListener &&) = delete;
    ICastSessionListener &operator = (ICastSessionListener &&) = delete;
    virtual ~ICastSessionListener() = default;

    virtual void OnDeviceState(const DeviceStateInfo &stateInfo) = 0;
    virtual void OnEvent(const EventId &eventId, const std::string &jsonParam) = 0;
    virtual void OnRemoteCtrlEvent(int eventType, const uint8_t *data, uint32_t len) {}
};

class EXPORT ICastSession {
public:
    ICastSession() = default;
    ICastSession(const ICastSession &) = delete;
    ICastSession &operator = (const ICastSession &) = delete;
    ICastSession(ICastSession &&) = delete;
    ICastSession &operator = (ICastSession &&) = delete;
    virtual ~ICastSession() = default;

    virtual int32_t RegisterListener(std::shared_ptr<ICastSessionListener> listener) = 0;
    virtual int32_t UnregisterListener() = 0;
    virtual int32_t AddDevice(const CastRemoteDevice &remoteDevice) = 0;
    virtual int32_t RemoveDevice(const std::string &deviceId) = 0;
    virtual int32_t StartAuth(const AuthInfo &authInfo) = 0;
    virtual int32_t GetSessionId(std::string &sessionId) = 0;
    virtual int32_t SetSessionProperty(const CastSessionProperty &property) = 0;
    virtual int32_t CreateMirrorPlayer(std::shared_ptr<IMirrorPlayer> &mirrorPlayer) = 0;
    virtual int32_t CreateStreamPlayer(std::shared_ptr<IStreamPlayer> &streamPlayer) = 0;
    virtual int32_t NotifyEvent(EventId eventId, std::string &jsonParam) = 0;
    virtual int32_t SetCastMode(CastMode mode, std::string &jsonParam) = 0;
    virtual int32_t Release() = 0;
};
} // namespace CastEngine
} // namespace OHOS

#endif
