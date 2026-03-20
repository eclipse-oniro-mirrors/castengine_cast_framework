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
 * Description: supply a helper to write/read common cast engine structures through ipc
 * Author: zhangge
 * Create: 2022-06-15
 */

#include "cast_engine_common_helper.h"

#include <optional>
#include "cast_engine_log.h"
#include "securec.h"

namespace OHOS {
namespace CastEngine {
DEFINE_CAST_ENGINE_LABEL("Cast-Engine-helper");
constexpr uint32_t MAX_DRM_CAPABILITY_SIZE = 50;

namespace {
constexpr int SESSION_KEY_LENGTH = 16;
bool WriteVideoSize(Parcel &parcel, const VideoSize &videoSize)
{
    return parcel.WriteInt32(videoSize.width) && parcel.WriteInt32(videoSize.height);
}

const VideoSize ReadVideoSize(Parcel &parcel)
{
    return { static_cast<uint32_t>(parcel.ReadInt32()), static_cast<uint32_t>(parcel.ReadInt32()) };
}

bool WriteWindowProperty(Parcel &parcel, const WindowProperty &property)
{
    return parcel.WriteInt32(property.startX) && parcel.WriteInt32(property.startY) &&
        parcel.WriteInt32(property.width) && parcel.WriteInt32(property.height);
}

const WindowProperty ReadWindowProperty(Parcel &parcel)
{
    WindowProperty property;
    property.startX = static_cast<uint32_t>(parcel.ReadInt32());
    property.startY = static_cast<uint32_t>(parcel.ReadInt32());
    property.width = static_cast<uint32_t>(parcel.ReadInt32());
    property.height = static_cast<uint32_t>(parcel.ReadInt32());

    return property;
}

bool WriteVideoProperty(Parcel &parcel, const VideoProperty &property)
{
    return parcel.WriteInt32(static_cast<int32_t>(property.videoWidth)) &&
        parcel.WriteInt32(static_cast<int32_t>(property.videoHeight)) &&
        parcel.WriteInt32(static_cast<int32_t>(property.fps)) &&
        parcel.WriteInt32(static_cast<int32_t>(property.codecType)) &&
        parcel.WriteInt32(static_cast<int32_t>(property.gop)) &&
        parcel.WriteInt32(static_cast<int32_t>(property.bitrate)) &&
        parcel.WriteInt32(static_cast<int32_t>(property.minBitrate)) &&
        parcel.WriteInt32(static_cast<int32_t>(property.maxBitrate)) &&
        parcel.WriteInt32(static_cast<int32_t>(property.dpi)) &&
        parcel.WriteInt32(static_cast<int32_t>(property.colorStandard)) &&
        parcel.WriteInt32(static_cast<int32_t>(property.screenWidth)) &&
        parcel.WriteInt32(static_cast<int32_t>(property.screenHeight)) &&
        parcel.WriteInt32(static_cast<int32_t>(property.profile)) &&
        parcel.WriteInt32(static_cast<int32_t>(property.level));
}

std::optional<VideoProperty> ReadVideoProperty(Parcel &parcel)
{
    VideoProperty property;
    property.videoWidth = static_cast<uint32_t>(parcel.ReadInt32());
    property.videoHeight = static_cast<uint32_t>(parcel.ReadInt32());
    property.fps = static_cast<uint32_t>(parcel.ReadInt32());
    auto codecType = parcel.ReadInt32();
    property.gop = parcel.ReadInt32();
    property.bitrate = static_cast<uint32_t>(parcel.ReadInt32());
    property.minBitrate = static_cast<uint32_t>(parcel.ReadInt32());
    property.maxBitrate = static_cast<uint32_t>(parcel.ReadInt32());
    property.dpi = static_cast<uint32_t>(parcel.ReadInt32());
    auto colorStandard = parcel.ReadInt32();
    property.screenWidth = static_cast<uint32_t>(parcel.ReadInt32());
    property.screenHeight = static_cast<uint32_t>(parcel.ReadInt32());
    property.profile = static_cast<uint32_t>(parcel.ReadInt32());
    property.level = static_cast<uint32_t>(parcel.ReadInt32());

    if (!IsColorStandard(colorStandard) || !IsVideoCodecType(codecType)) {
        return std::nullopt;
    }

    property.codecType = static_cast<VideoCodecType>(codecType);
    property.colorStandard = static_cast<ColorStandard>(colorStandard);
    return property;
}


bool WriteAudioProperty(Parcel &parcel, const AudioProperty &property)
{
    return parcel.WriteInt32(static_cast<int32_t>(property.sampleRate)) &&
        parcel.WriteInt32(static_cast<int32_t>(property.sampleBitWidth)) &&
        parcel.WriteInt32(static_cast<int32_t>(property.channelConfig)) &&
        parcel.WriteInt32(static_cast<int32_t>(property.bitrate)) &&
        parcel.WriteInt32(static_cast<int32_t>(property.codec));
}

const AudioProperty ReadAudioProperty(Parcel &parcel)
{
    AudioProperty property;
    property.sampleRate = static_cast<uint32_t>(parcel.ReadInt32());
    property.sampleBitWidth = static_cast<uint8_t>(parcel.ReadInt32());
    property.channelConfig = static_cast<uint32_t>(parcel.ReadInt32());
    property.bitrate = static_cast<uint32_t>(parcel.ReadInt32());
    property.codec = static_cast<uint32_t>(parcel.ReadInt32());
    return property;
}

int GetLocalFd(const std::string &url)
{
    if (url.empty()) {
        CLOGE("url is empty.");
        return INVALID_VALUE;
    }
    char *nextPtr = nullptr;
    int fd = static_cast<int>(std::strtol(url.c_str(), &nextPtr, DECIMALISM));
    if (errno == ERANGE || *nextPtr != '\0') {
        return INVALID_VALUE;
    }
    return fd;
}
} // namespace

bool WriteCastRemoteDevice(Parcel &parcel, const CastRemoteDevice &device)
{
    bool res = parcel.WriteInt32(static_cast<int32_t>(device.deviceType)) &&
        parcel.WriteInt32(static_cast<int32_t>(device.capability)) &&
        parcel.WriteInt32(static_cast<int32_t>(device.subDeviceType)) &&
        parcel.WriteInt32(static_cast<int32_t>(device.channelType)) && parcel.WriteString(device.deviceId) &&
        parcel.WriteString(device.deviceName) && parcel.WriteString(device.ipAddress) &&
        parcel.WriteString(device.networkId) && parcel.WriteString(device.localIpAddress) &&
        parcel.WriteBool(device.isLeagacy) && parcel.WriteInt32(device.sessionId) &&
        parcel.WriteUint32(device.mediumTypes) && parcel.WriteUint32(device.protocolCapabilities) &&
        parcel.WriteString(device.modelName) && parcel.WriteString(device.manufacturerName) &&
        parcel.WriteBool(device.isTrushed);
    if (device.sessionKeyLength == SESSION_KEY_LENGTH && device.sessionKey) {
        res = res && parcel.WriteUint32(device.sessionKeyLength);
        res = res && parcel.WriteBuffer(device.sessionKey, device.sessionKeyLength);
    } else {
        parcel.WriteUint32(0);
    }
    uint32_t drmCapabilitySize = device.drmCapabilities.size();
    if (drmCapabilitySize > MAX_DRM_CAPABILITY_SIZE) {
        CLOGE("drmCapabilitySize(%{public}u) is invalid", drmCapabilitySize);
        return false;
    }
    res = res && parcel.WriteUint32(drmCapabilitySize);
    for (auto iter = device.drmCapabilities.begin(); iter != device.drmCapabilities.end(); iter++) {
        res = res && parcel.WriteString(*iter);
    }
    return res;
}

bool ReadCastRemoteDevice(Parcel &parcel, CastRemoteDevice &device)
{
    auto remote = ReadCastRemoteDevice(parcel);
    if (remote == nullptr) {
        CLOGE("ReadCastRemoteDevice failed");
        return false;
    }

    device = *remote;
    return true;
}

std::unique_ptr<CastRemoteDevice> ReadCastRemoteDevice(Parcel &parcel)
{
    auto device = std::make_unique<CastRemoteDevice>();
    auto deviceType = parcel.ReadInt32();
    auto capability = parcel.ReadInt32();
    auto subDeviceType = parcel.ReadInt32();
    auto channelType = parcel.ReadInt32();
    if (!IsDeviceType(deviceType) || !IsSubDeviceType(subDeviceType) ||
        !IsChannelType(channelType) || !IsCapabilityType(capability)) {
        CLOGE("ReadCastRemoteDevice error");
        return nullptr;
    }
    device->deviceType = static_cast<DeviceType>(deviceType);
    device->capability = static_cast<CapabilityType>(capability);
    device->subDeviceType = static_cast<SubDeviceType>(subDeviceType);
    device->channelType = static_cast<ChannelType>(channelType);
    device->deviceId = parcel.ReadString();
    device->deviceName = parcel.ReadString();
    device->ipAddress = parcel.ReadString();
    device->networkId = parcel.ReadString();
    device->localIpAddress = parcel.ReadString();
    device->isLeagacy = parcel.ReadBool();
    device->sessionId = parcel.ReadInt32();
    device->mediumTypes = parcel.ReadUint32();
    device->protocolCapabilities = parcel.ReadUint32();
    device->modelName = parcel.ReadString();
    device->manufacturerName = parcel.ReadString();
    device->isTrushed = parcel.ReadBool();
    device->sessionKeyLength = parcel.ReadUint32();
    if (device->sessionKeyLength == SESSION_KEY_LENGTH) {
        device->sessionKey = parcel.ReadBuffer(static_cast<size_t>(device->sessionKeyLength));
    } else {
        device->sessionKeyLength = 0;
        device->sessionKey = nullptr;
    }
    uint32_t drmCapabilitySize = parcel.ReadUint32();
    if (drmCapabilitySize > MAX_DRM_CAPABILITY_SIZE) {
        CLOGE("drmCapabilitySize(%{public}u) is invalid", drmCapabilitySize);
        return nullptr;
    }
    for (uint32_t i = 0; i < drmCapabilitySize; i++) {
        device->drmCapabilities.push_back(parcel.ReadString());
    }

    return device;
}

bool WriteStreamCapability(MessageParcel &parcel, const StreamCapability &streamCapability)
{
    return parcel.WriteBool(streamCapability.isPlaySupported) &&
           parcel.WriteBool(streamCapability.isPauseSupported) &&
           parcel.WriteBool(streamCapability.isStopSupported) &&
           parcel.WriteBool(streamCapability.isNextSupported) &&
           parcel.WriteBool(streamCapability.isPreviousSupported) &&
           parcel.WriteBool(streamCapability.isSeekSupported) &&
           parcel.WriteBool(streamCapability.isFastForwardSupported) &&
           parcel.WriteBool(streamCapability.isFastRewindSupported) &&
           parcel.WriteBool(streamCapability.isLoopModeSupported) &&
           parcel.WriteBool(streamCapability.isToggleFavoriteSupported) &&
           parcel.WriteBool(streamCapability.isSetVolumeSupported);
}

StreamCapability ReadStreamCapability(MessageParcel &parcel)
{
    StreamCapability streamCapability;
    streamCapability.isPlaySupported = parcel.ReadBool();
    streamCapability.isPauseSupported = parcel.ReadBool();
    streamCapability.isStopSupported = parcel.ReadBool();
    streamCapability.isNextSupported = parcel.ReadBool();
    streamCapability.isPreviousSupported = parcel.ReadBool();
    streamCapability.isSeekSupported = parcel.ReadBool();
    streamCapability.isFastForwardSupported = parcel.ReadBool();
    streamCapability.isFastRewindSupported = parcel.ReadBool();
    streamCapability.isLoopModeSupported = parcel.ReadBool();
    streamCapability.isToggleFavoriteSupported = parcel.ReadBool();
    streamCapability.isSetVolumeSupported = parcel.ReadBool();
    return streamCapability;
}

bool WriteMediaInfo(MessageParcel &parcel, const MediaInfo &mediaInfo)
{
    if (mediaInfo.mediaUrl.empty()) {
        CLOGE("mediaUrl is empty");
        return false;
    }
    int fd = GetLocalFd(mediaInfo.mediaUrl);
    if (fd != INVALID_VALUE) {
        if (!parcel.WriteString("localFd") || !parcel.WriteFileDescriptor(fd)) {
            CLOGE("Write local fd failed, fd = %{public}d", fd);
            return false;
        }
    } else if (!parcel.WriteString("path") || !parcel.WriteString(mediaInfo.mediaUrl)) {
        CLOGE("Write path failed");
        return false;
    }
    return parcel.WriteString(mediaInfo.mediaId) && parcel.WriteString(mediaInfo.mediaName) &&
        parcel.WriteString(mediaInfo.mediaType) && parcel.WriteUint32(mediaInfo.mediaSize) &&
        parcel.WriteString(mediaInfo.albumCoverUrl) && parcel.WriteString(mediaInfo.albumTitle) &&
        parcel.WriteString(mediaInfo.mediaArtist) && parcel.WriteString(mediaInfo.lrcUrl) &&
        parcel.WriteString(mediaInfo.lrcContent) && parcel.WriteString(mediaInfo.appIconUrl) &&
        parcel.WriteString(mediaInfo.appName) && parcel.WriteUint32(mediaInfo.startPosition) &&
        parcel.WriteUint32(mediaInfo.duration) && parcel.WriteUint32(mediaInfo.closingCreditsPosition) &&
        parcel.WriteString(mediaInfo.drmType);
}

std::unique_ptr<MediaInfo> ReadMediaInfo(MessageParcel &parcel)
{
    auto mediaInfo = std::make_unique<MediaInfo>();
    if (mediaInfo == nullptr) {
        CLOGE("Failed to malloc mediaInfo");
        return nullptr;
    }
    std::string urlType = parcel.ReadString();
    if (urlType == "localFd") {
        CLOGD("localFd");
        mediaInfo->mediaUrl = std::to_string(parcel.ReadFileDescriptor());
    } else {
        CLOGD("online or localPath");
        mediaInfo->mediaUrl = parcel.ReadString();
    }
    mediaInfo->mediaId = parcel.ReadString();
    mediaInfo->mediaName = parcel.ReadString();
    mediaInfo->mediaType = parcel.ReadString();
    mediaInfo->mediaSize = parcel.ReadUint32();
    mediaInfo->albumCoverUrl = parcel.ReadString();
    mediaInfo->albumTitle = parcel.ReadString();
    mediaInfo->mediaArtist = parcel.ReadString();
    mediaInfo->lrcUrl = parcel.ReadString();
    mediaInfo->lrcContent = parcel.ReadString();
    mediaInfo->appIconUrl = parcel.ReadString();
    mediaInfo->appName = parcel.ReadString();
    mediaInfo->startPosition = parcel.ReadUint32();
    mediaInfo->duration = parcel.ReadUint32();
    mediaInfo->closingCreditsPosition = parcel.ReadUint32();
    mediaInfo->drmType = parcel.ReadString();

    return mediaInfo;
}

bool WriteMediaInfoHolder(MessageParcel &parcel, const MediaInfoHolder &mediaInfoHolder)
{
    bool ret = parcel.WriteUint32(mediaInfoHolder.currentIndex);
    ret = ret && parcel.WriteUint32(mediaInfoHolder.progressRefreshInterval);
    ret = ret && parcel.WriteUint32(static_cast<uint32_t>(mediaInfoHolder.mediaInfoList.size()));
    for (auto iter = mediaInfoHolder.mediaInfoList.begin(); iter != mediaInfoHolder.mediaInfoList.end(); iter++) {
        ret = ret && WriteMediaInfo(parcel, *iter);
    }
    return ret;
}

std::unique_ptr<MediaInfoHolder> ReadMediaInfoHolder(MessageParcel &parcel)
{
    auto mediaInfoHolder = std::make_unique<MediaInfoHolder>();
    if (mediaInfoHolder == nullptr) {
        CLOGE("Failed to malloc mediaInfoHolder");
        return nullptr;
    }
    mediaInfoHolder->currentIndex = parcel.ReadUint32();
    mediaInfoHolder->progressRefreshInterval = parcel.ReadUint32();
    uint32_t infoListSize = parcel.ReadUint32();
    if (infoListSize > MAX_FILE_NUM) {
        CLOGE("The number of list exceeds the upper limit. infoListSize: %{public}u", infoListSize);
        return nullptr;
    }
    for (uint32_t i = 0; i < infoListSize; i++) {
        auto mediaInfo = ReadMediaInfo(parcel);
        if (mediaInfo == nullptr) {
            return nullptr;
        }
        mediaInfoHolder->mediaInfoList.push_back(*mediaInfo);
    }
    return mediaInfoHolder;
}

bool WriteCastLocalDevice(Parcel &parcel, const CastLocalDevice &device)
{
    return parcel.WriteString(device.deviceId) && parcel.WriteString(device.deviceName) &&
        parcel.WriteInt32(static_cast<int32_t>(device.deviceType)) &&
        parcel.WriteInt32(static_cast<int32_t>(device.subDeviceType)) && parcel.WriteString(device.ipAddress) &&
        parcel.WriteInt32(static_cast<int32_t>(device.triggerType)) && parcel.WriteString(device.authData);
}

std::unique_ptr<CastLocalDevice> ReadCastLocalDevice(Parcel &parcel)
{
    auto device = std::make_unique<CastLocalDevice>();
    device->deviceId = parcel.ReadString();
    device->deviceName = parcel.ReadString();
    auto deviceType = parcel.ReadInt32();
    auto subDeviceType = parcel.ReadInt32();
    device->ipAddress = parcel.ReadString();
    auto triggerType = parcel.ReadInt32();
    device->authData = parcel.ReadString();

    if (!IsDeviceType(deviceType) || !IsSubDeviceType(subDeviceType) || !IsTriggerType(triggerType)) {
        CLOGE("ReadCastLocalDevice error");
        return nullptr;
    }
    device->deviceType = static_cast<DeviceType>(deviceType);
    device->subDeviceType = static_cast<SubDeviceType>(subDeviceType);
    device->triggerType = static_cast<TriggerType>(triggerType);

    return device;
}

bool WriteCastSessionProperty(Parcel &parcel, const CastSessionProperty &property)
{
    return parcel.WriteInt32(static_cast<int32_t>(property.protocolType)) &&
        parcel.WriteInt32(static_cast<int32_t>(property.endType)) &&
        WriteAudioProperty(parcel, property.audioProperty) && WriteVideoProperty(parcel, property.videoProperty) &&
        WriteWindowProperty(parcel, property.windowProperty);
}

std::unique_ptr<CastSessionProperty> ReadCastSessionProperty(Parcel &parcel)
{
    auto property = std::make_unique<CastSessionProperty>();
    if (property == nullptr) {
        CLOGE("Failed to malloc cast session property");
        return nullptr;
    }
    auto protocolType = parcel.ReadInt32();
    if (!IsProtocolType(protocolType)) {
        return nullptr;
    }
    auto endType = parcel.ReadInt32();
    if (!IsEndType(endType)) {
        return nullptr;
    }

    property->protocolType = static_cast<ProtocolType>(protocolType);
    property->endType = static_cast<EndType>(endType);
    property->audioProperty = ReadAudioProperty(parcel);
    auto videoProperty = ReadVideoProperty(parcel);
    if (videoProperty == std::nullopt) {
        return nullptr;
    }
    property->videoProperty = videoProperty.value();
    property->windowProperty = ReadWindowProperty(parcel);
    return property;
}

bool WritePropertyContainer(Parcel &parcel, const PropertyContainer &container)
{
    return parcel.WriteInt32(static_cast<int32_t>(container.type)) &&
        (((container.type == PropertyType::VIDEO_SIZE) && WriteVideoSize(parcel, container.videoSize)) ||
        ((container.type == PropertyType::VIDEO_FPS) && parcel.WriteInt32(container.videoFps)) ||
        ((container.type == PropertyType::WINDOW_SIZE) && WriteWindowProperty(parcel, container.windowProperty)));
}

std::unique_ptr<PropertyContainer> ReadPropertyContainer(Parcel &parcel)
{
    auto container = std::make_unique<PropertyContainer>();
    if (container == nullptr) {
        CLOGE("Failed to malloc property container");
        return nullptr;
    }
    int32_t type = parcel.ReadInt32();
    if (!IsPropertyType(type)) {
        return nullptr;
    }
    container->type = static_cast<PropertyType>(type);
    if (container->type == PropertyType::VIDEO_SIZE) {
        container->videoSize = ReadVideoSize(parcel);
    } else if (container->type == PropertyType::VIDEO_FPS) {
        container->videoFps = static_cast<unsigned int>(parcel.ReadInt32());
    } else {
        container->windowProperty = ReadWindowProperty(parcel);
    }

    return container;
}

bool WriteAuthInfo(Parcel &parcel, const AuthInfo &authInfo)
{
    return parcel.WriteInt32(static_cast<int32_t>(authInfo.authMode)) &&
        parcel.WriteInt32(static_cast<int32_t>(authInfo.authCode)) && parcel.WriteString(authInfo.deviceId);
}

std::unique_ptr<AuthInfo> ReadAuthInfo(Parcel &parcel)
{
    auto authInfo = std::make_unique<AuthInfo>();
    if (authInfo == nullptr) {
        CLOGE("Failed to malloc auth info");
        return nullptr;
    }

    authInfo->authMode = static_cast<int>(parcel.ReadInt32());
    authInfo->authCode = static_cast<uint32_t>(parcel.ReadInt32());
    authInfo->deviceId = parcel.ReadString();
    return authInfo;
}

bool WriteTouchPoint(Parcel &parcel, const OHNativeXcomponentTouchPoint &touchPoint)
{
    return parcel.WriteInt32(touchPoint.id) && parcel.WriteFloat(touchPoint.screenX) &&
        parcel.WriteFloat(touchPoint.screenY) && parcel.WriteFloat(touchPoint.x) && parcel.WriteFloat(touchPoint.y) &&
        parcel.WriteUint32(static_cast<uint32_t>(touchPoint.type)) && parcel.WriteDouble(touchPoint.size) &&
        parcel.WriteFloat(touchPoint.force) && parcel.WriteInt64(touchPoint.timeStamp) &&
        parcel.WriteBool(touchPoint.isPressed);
}

bool WriteTouchPoints(Parcel &parcel, const OHNativeXcomponentTouchEvent touchEvent)
{
    if (touchEvent.numPoints > OH_MAX_TOUCH_POINTS_NUMBER) {
        CLOGE("numPoints exceeds max number");
        return false;
    }
    for (uint32_t i = 0; i < touchEvent.numPoints; i++) {
        if (!WriteTouchPoint(parcel, touchEvent.touchPoints[i])) {
            return false;
        }
    }
    return true;
}

bool WriteTouchEvent(Parcel &parcel, const OHNativeXcomponentTouchEvent &touchEvent)
{
    return parcel.WriteInt32(touchEvent.id) && parcel.WriteFloat(touchEvent.screenX) &&
        parcel.WriteFloat(touchEvent.screenY) && parcel.WriteFloat(touchEvent.x) && parcel.WriteFloat(touchEvent.y) &&
        parcel.WriteUint32(static_cast<uint32_t>(touchEvent.type)) && parcel.WriteDouble(touchEvent.size) &&
        parcel.WriteFloat(touchEvent.force) && parcel.WriteInt64(touchEvent.deviceId) &&
        parcel.WriteInt64(touchEvent.timeStamp) && parcel.WriteUint32(touchEvent.numPoints) &&
        WriteTouchPoints(parcel, touchEvent);
}


bool WriteMouseEvent(Parcel &parcel, const OHNativeXcomponentMouseEvent &mouseEvent)
{
    return parcel.WriteFloat(mouseEvent.x) && parcel.WriteFloat(mouseEvent.y) &&
        parcel.WriteFloat(mouseEvent.screenX) && parcel.WriteFloat(mouseEvent.screenY) &&
        parcel.WriteInt64(mouseEvent.timestamp) && parcel.WriteUint32(static_cast<uint32_t>(mouseEvent.action)) &&
        parcel.WriteUint32(static_cast<uint32_t>(mouseEvent.button));
}

bool WriteWhellEvent(Parcel &parcel, const OHNativeXcomponentWheelEvent &wheelEvent)
{
    return parcel.WriteUint32(static_cast<uint32_t>(wheelEvent.direction)) &&
        parcel.WriteUint8(wheelEvent.indication) && parcel.WriteUint8(wheelEvent.scrollUnit) &&
        parcel.WriteUint16(wheelEvent.wheelDis) && parcel.WriteFloat(wheelEvent.x) && parcel.WriteFloat(wheelEvent.y);
}

bool WriteKeyEvent(Parcel &parcel, const OHNativeXcomponentKeyEvent &keyEvent)
{
    return parcel.WriteUint8(keyEvent.reserved) && parcel.WriteUint16(keyEvent.keyCode1) &&
        parcel.WriteUint16(keyEvent.keyCode2) && parcel.WriteUint32(keyEvent.metaState) &&
        parcel.WriteUint32(static_cast<uint32_t>(keyEvent.type));
}

bool WriteContentEvent(Parcel &parcel, const OHNativeXcomponentContentEvent &contentEvent)
{
    return parcel.WriteUint16(contentEvent.msgLen) && parcel.WriteBuffer(contentEvent.inputText, contentEvent.msgLen);
}

bool WriteFocusEvent(Parcel &parcel, const OHNativeXcomponentFocusEvent &focusEvent)
{
    return parcel.WriteUint8(focusEvent.focusStat) && parcel.WriteDouble(focusEvent.cursorX1) &&
        parcel.WriteDouble(focusEvent.cursorY1) && parcel.WriteDouble(focusEvent.cursorX2) &&
        parcel.WriteDouble(focusEvent.cursorY2);
}

bool WriteInputMethodEvent(Parcel &parcel, const OHNativeXcomponentInputMethodEvent &inputMethodEvent)
{
    return parcel.WriteUint32(static_cast<uint32_t>(inputMethodEvent.type)) &&
        (((inputMethodEvent.type == OHNativeXcomponentInputMethodEventType::OH_NATIVEXCOMPONENT_INPUT_CONTENT) &&
        WriteContentEvent(parcel, inputMethodEvent.contentEvent)) ||
        ((inputMethodEvent.type == OHNativeXcomponentInputMethodEventType::OH_NATIVEXCOMPONENT_INPUT_FOCUS) &&
        WriteFocusEvent(parcel, inputMethodEvent.focusEvent)));
}

bool WriteVirtualKeyEvent(Parcel &parcel, const OHNativeXcomponentVirtualKeyEvent &virtualKeyEvent)
{
    return parcel.WriteInt32(static_cast<int32_t>(virtualKeyEvent.type)) && parcel.WriteFloat(virtualKeyEvent.x) &&
        parcel.WriteFloat(virtualKeyEvent.y);
}

bool WriteRemoteControlEvent(Parcel &parcel, const OHRemoteControlEvent &event)
{
    return parcel.WriteInt32(static_cast<int32_t>(event.eventType)) &&
        (((event.eventType == XcomponentEventType::REMOTECONTROL_TOUCH) && WriteTouchEvent(parcel, event.touchEvent)) ||
        ((event.eventType == XcomponentEventType::REMOTECONTROL_MOUSE) && WriteMouseEvent(parcel, event.mouseEvent)) ||
        ((event.eventType == XcomponentEventType::REMOTECONTROL_WHEEL) && WriteWhellEvent(parcel, event.wheelEvent)) ||
        ((event.eventType == XcomponentEventType::REMOTECONTROL_KEY) && WriteKeyEvent(parcel, event.keyEvent)) ||
        ((event.eventType == XcomponentEventType::REMOTECONTROL_INPUT_METHOD) &&
        WriteInputMethodEvent(parcel, event.inputMethodEvent)) ||
        ((event.eventType == XcomponentEventType::REMOTECONTROL_VIRTUAL_KEY) &&
        WriteVirtualKeyEvent(parcel, event.virtualKeyEvent)));
}

const OHNativeXcomponentTouchPoint ReadTouchPoint(Parcel &parcel)
{
    return { parcel.ReadInt32(),  parcel.ReadFloat(),
        parcel.ReadFloat(),  parcel.ReadFloat(),
        parcel.ReadFloat(),  static_cast<OHNativeXcomponentTouchEventType>(parcel.ReadUint32()),
        parcel.ReadDouble(), parcel.ReadFloat(),
        parcel.ReadInt64(),  parcel.ReadBool() };
}

void ReadTouchPoints(Parcel &parcel, uint32_t numPoints, OHNativeXcomponentTouchPoint points[])
{
    if (numPoints > OH_MAX_TOUCH_POINTS_NUMBER) {
        CLOGE("numPoints is invalid, numPoints:%{public}d", numPoints);
        return;
    }
    for (uint32_t i = 0; i < numPoints; i++) {
        points[i] = ReadTouchPoint(parcel);
    }
}

void ReadTouchEvent(Parcel &parcel, OHNativeXcomponentTouchEvent &touchEvent)
{
    touchEvent.id = parcel.ReadInt32();
    touchEvent.screenX = parcel.ReadFloat();
    touchEvent.screenY = parcel.ReadFloat();
    touchEvent.x = parcel.ReadFloat();
    touchEvent.y = parcel.ReadFloat();
    touchEvent.type = static_cast<OHNativeXcomponentTouchEventType>(parcel.ReadUint32());
    touchEvent.size = parcel.ReadDouble();
    touchEvent.force = parcel.ReadFloat();
    touchEvent.deviceId = parcel.ReadInt64();
    touchEvent.timeStamp = parcel.ReadInt64();
    touchEvent.numPoints = parcel.ReadUint32();
    ReadTouchPoints(parcel, touchEvent.numPoints, touchEvent.touchPoints);
}

const OHNativeXcomponentMouseEvent ReadMouseEvent(Parcel &parcel)
{
    return { parcel.ReadFloat(),
        parcel.ReadFloat(),
        parcel.ReadFloat(),
        parcel.ReadFloat(),
        parcel.ReadInt64(),
        static_cast<OHNativeXcomponentMouseEventAction>(parcel.ReadUint32()),
        static_cast<OHNativeXcomponentMouseEventButton>(parcel.ReadUint32()) };
}

const OHNativeXcomponentWheelEvent ReadWhellEvent(Parcel &parcel)
{
    return { static_cast<OHNativeXcomponentWheelEventDirection>(parcel.ReadUint32()),
        parcel.ReadUint8(),
        parcel.ReadUint8(),
        parcel.ReadUint16(),
        parcel.ReadFloat(),
        parcel.ReadFloat() };
}

const OHNativeXcomponentKeyEvent ReadKeyEvent(Parcel &parcel)
{
    return { parcel.ReadUint8(), parcel.ReadUint16(), parcel.ReadUint16(), parcel.ReadUint32(),
        static_cast<OHNativeXcomponentKeyEventType>(parcel.ReadUint32()) };
}

void ReadContentEvent(Parcel &parcel, OHNativeXcomponentContentEvent &contentEvent)
{
    contentEvent.msgLen = parcel.ReadUint16();
    int32_t err = memcpy_s(contentEvent.inputText,
        OH_MAX_CONTENT_LEN, parcel.ReadBuffer(contentEvent.msgLen), contentEvent.msgLen);
    if (err != 0) {
        CLOGE("memcpy_s inputText failed, err = %{public}d.", err);
    }
}

const OHNativeXcomponentFocusEvent ReadFocusEvent(Parcel &parcel)
{
    return { parcel.ReadUint8(), parcel.ReadDouble(), parcel.ReadDouble(), parcel.ReadDouble(), parcel.ReadDouble() };
}


void ReadInputMethodEvent(Parcel &parcel, OHNativeXcomponentInputMethodEvent &inputMethodEvent)
{
    inputMethodEvent.type = static_cast<OHNativeXcomponentInputMethodEventType>(parcel.ReadUint16());
    if (inputMethodEvent.type == OHNativeXcomponentInputMethodEventType::OH_NATIVEXCOMPONENT_INPUT_CONTENT) {
        return ReadContentEvent(parcel, inputMethodEvent.contentEvent);
    }
    inputMethodEvent.focusEvent = ReadFocusEvent(parcel);
}

const OHNativeXcomponentVirtualKeyEvent ReadVirtualKeyEvent(Parcel &parcel)
{
    return { static_cast<OHNativeXcomponentVirtualKeyEventType>(parcel.ReadInt32()), parcel.ReadFloat(),
        parcel.ReadFloat() };
}

std::unique_ptr<OHRemoteControlEvent> ReadRemoteControlEvent(Parcel &parcel)
{
    auto remoteControlEvent = std::make_unique<OHRemoteControlEvent>();
    if (remoteControlEvent == nullptr) {
        CLOGE("Failed to malloc remote control event");
        return nullptr;
    }

    remoteControlEvent->eventType = static_cast<XcomponentEventType>(parcel.ReadUint32());
    switch (remoteControlEvent->eventType) {
        case XcomponentEventType::REMOTECONTROL_TOUCH:
            ReadTouchEvent(parcel, remoteControlEvent->touchEvent);
            break;
        case XcomponentEventType::REMOTECONTROL_MOUSE:
            remoteControlEvent->mouseEvent = ReadMouseEvent(parcel);
            break;
        case XcomponentEventType::REMOTECONTROL_WHEEL:
            remoteControlEvent->wheelEvent = ReadWhellEvent(parcel);
            break;
        case XcomponentEventType::REMOTECONTROL_KEY:
            remoteControlEvent->keyEvent = ReadKeyEvent(parcel);
            break;
        case XcomponentEventType::REMOTECONTROL_INPUT_METHOD:
            ReadInputMethodEvent(parcel, remoteControlEvent->inputMethodEvent);
            break;
        case XcomponentEventType::REMOTECONTROL_VIRTUAL_KEY:
            remoteControlEvent->virtualKeyEvent = ReadVirtualKeyEvent(parcel);
            break;
        default:
            CLOGE("Parameter error, event:(%d) not support", static_cast<uint32_t>(remoteControlEvent->eventType));
            remoteControlEvent = nullptr;
            break;
    }
    return remoteControlEvent;
}

bool WriteDeviceStateInfo(Parcel &parcel, const DeviceStateInfo &stateInfo)
{
    return parcel.WriteInt32(static_cast<int32_t>(stateInfo.deviceState)) &&
           parcel.WriteString(stateInfo.deviceId) &&
           parcel.WriteInt32(static_cast<int32_t>(stateInfo.reasonCode));
}

std::unique_ptr<DeviceStateInfo> ReadDeviceStateInfo(Parcel &parcel)
{
    auto stateInfo = std::make_unique<DeviceStateInfo>();

    // Parse device state
    int32_t state = parcel.ReadInt32();
    if (!IsDeviceState(state)) {
        CLOGE("incorrect device state");
        return nullptr;
    }
    stateInfo->deviceState = static_cast<DeviceState>(state);

    // Parse deviceId
    std::string deviceId = parcel.ReadString();
    if (deviceId.empty()) {
        CLOGE("device id is empty");
        return nullptr;
    }
    stateInfo->deviceId = deviceId;

    // Parse event code
    int32_t reasonCode = parcel.ReadInt32();
    stateInfo->reasonCode = static_cast<ReasonCode>(reasonCode);

    return stateInfo;
}

bool WriteEvent(Parcel &parcel, const EventId &eventId, const std::string &jsonParam)
{
    return parcel.WriteInt32(static_cast<int32_t>(eventId)) && parcel.WriteString(jsonParam);
}

bool ReadEvent(Parcel &parcel, int32_t &eventId, std::string &jsonParam)
{
    eventId = parcel.ReadInt32();
    if (!IsEventId(eventId)) {
        CLOGE("incorrect event id");
        return false;
    }
    jsonParam = parcel.ReadString();
    return true;
}

void SetDataCapacity(MessageParcel &parcel, const FileFdMap &fileList, uint32_t tokenSize)
{
    uint32_t totalSize = tokenSize;
    totalSize += sizeof(uint32_t);
    for (const auto &[srcPath, fdPair] : fileList) {
        totalSize += srcPath.length();
        totalSize += sizeof(fdPair.first);
        totalSize += fdPair.second.length();
    }
    if (totalSize > parcel.GetDataCapacity()) {
        CLOGD("SetDataCapacity totalSize: %d", totalSize);
        parcel.SetMaxCapacity(totalSize + totalSize);
        parcel.SetDataCapacity(totalSize);
    }
}

bool WriteFileList(MessageParcel &parcel, const FileFdMap &fileList)
{
    bool ret = parcel.WriteUint32(fileList.size());
    for (const auto &[srcPath, fdPair] : fileList) {
        ret = ret && parcel.WriteString(srcPath);
        ret = ret && parcel.WriteFileDescriptor(fdPair.first);
        ret = ret && parcel.WriteString(fdPair.second);
    }
    return ret;
}

bool ReadFileList(MessageParcel &parcel, FileFdMap &fileList)
{
    uint32_t fileListSize = parcel.ReadUint32();
    if (fileListSize > MAX_FILE_NUM) {
        CLOGE("The number of files exceeds the upper limit. fileListSize: %{public}u", fileListSize);
        return false;
    }
    for (uint32_t i = 0; i < fileListSize; i++) {
        std::string src = parcel.ReadString();
        int32_t fd = parcel.ReadFileDescriptor();
        std::string dst = parcel.ReadString();
        fileList[src] = std::make_pair(fd, dst);
    }
    return true;
}

bool WriteRcvFdFileMap(MessageParcel &parcel, const RcvFdFileMap &rcvFdFileMap)
{
    bool ret = parcel.WriteUint32(rcvFdFileMap.size());
    for (const auto &[fd, path] : rcvFdFileMap) {
        ret = ret && parcel.WriteFileDescriptor(fd);
        ret = ret && parcel.WriteString(path);
    }
    return ret;
}

bool ReadRcvFdFileMap(MessageParcel &parcel, RcvFdFileMap &rcvFdFileMap)
{
    uint32_t fileListSize = parcel.ReadUint32();
    if (fileListSize > MAX_FILE_NUM) {
        CLOGE("The number of files exceeds the upper limit. fileListSize: %{public}u", fileListSize);
        return false;
    }
    for (uint32_t i = 0; i < fileListSize; i++) {
        int32_t fd = parcel.ReadFileDescriptor();
        std::string path = parcel.ReadString();
        rcvFdFileMap[fd] = path;
    }
    return true;
}
} // namespace CastEngine
} // namespace OHOS
