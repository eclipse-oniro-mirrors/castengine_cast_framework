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

#ifndef CAST_ENGINE_COMMON_HELPER_H
#define CAST_ENGINE_COMMON_HELPER_H

#include "cast_engine_common.h"
#include "oh_remote_control_event.h"

namespace OHOS {
namespace CastEngine {
bool WriteCastRemoteDevice(Parcel &parcel, const CastRemoteDevice &device);
std::unique_ptr<CastRemoteDevice> ReadCastRemoteDevice(Parcel &parcel);
bool ReadCastRemoteDevice(Parcel &parcel, CastRemoteDevice &device);

bool WriteStreamCapability(MessageParcel &parcel, const StreamCapability &streamCapability);
StreamCapability ReadStreamCapability(MessageParcel &parcel);

bool WriteMediaInfo(MessageParcel &parcel, const MediaInfo &mediaInfo);
std::unique_ptr<MediaInfo> ReadMediaInfo(MessageParcel &parcel);

bool WriteMediaInfoHolder(MessageParcel &parcel, const MediaInfoHolder &mediaInfoHolder);
std::unique_ptr<MediaInfoHolder> ReadMediaInfoHolder(MessageParcel &parcel);

bool WriteCastLocalDevice(Parcel &parcel, const CastLocalDevice &device);
std::unique_ptr<CastLocalDevice> ReadCastLocalDevice(Parcel &parcel);

bool WriteCastSessionProperty(Parcel &parcel, const CastSessionProperty &property);
std::unique_ptr<CastSessionProperty> ReadCastSessionProperty(Parcel &parcel);

bool WritePropertyContainer(Parcel &parcel, const PropertyContainer &device);
std::unique_ptr<PropertyContainer> ReadPropertyContainer(Parcel &parcel);

bool WriteAuthInfo(Parcel &parcel, const AuthInfo &authInfo);
std::unique_ptr<AuthInfo> ReadAuthInfo(Parcel &parcel);

bool WriteRemoteControlEvent(Parcel &parcel, const OHRemoteControlEvent &event);
std::unique_ptr<OHRemoteControlEvent> ReadRemoteControlEvent(Parcel &parcel);

bool WriteDeviceStateInfo(Parcel &parcel, const DeviceStateInfo &stateInfo);
std::unique_ptr<DeviceStateInfo> ReadDeviceStateInfo(Parcel &parcel);

bool WriteEvent(Parcel &parcel, const EventId &eventId, const std::string &jsonParam);
bool ReadEvent(Parcel &parcel, int32_t &eventId, std::string &jsonParam);

void SetDataCapacity(MessageParcel &parcel, const FileFdMap &fileList, uint32_t tokenSize);

bool WriteRcvFdFileMap(MessageParcel &parcel, const RcvFdFileMap &rcvFdFileMap);

inline bool EXPORT IsTouchEventType(uint32_t type)
{
    return (type == static_cast<uint32_t>(OHNativeXcomponentTouchEventType::OH_NATIVEXCOMPONENT_TOUCH_DOWN)) ||
        (type == static_cast<uint32_t>(OHNativeXcomponentTouchEventType::OH_NATIVEXCOMPONENT_TOUCH_UP)) ||
        (type == static_cast<uint32_t>(OHNativeXcomponentTouchEventType::OH_NATIVEXCOMPONENT_TOUCH_MOVE)) ||
        (type == static_cast<uint32_t>(OHNativeXcomponentTouchEventType::OH_NATIVEXCOMPONENT_TOUCH_CANCEL)) ||
        (type == static_cast<uint32_t>(OHNativeXcomponentTouchEventType::OH_NATIVEXCOMPONENT_TOUCH_PULL_IN_WINDOW));
}

inline bool EXPORT IsMouseEventAction(uint32_t type)
{
    return (type == static_cast<uint32_t>(OHNativeXcomponentMouseEventAction::OH_NATIVEXCOMPONENT_MOUSE_PRESS)) ||
        (type == static_cast<uint32_t>(OHNativeXcomponentMouseEventAction::OH_NATIVEXCOMPONENT_MOUSE_RELEASE)) ||
        (type == static_cast<uint32_t>(OHNativeXcomponentMouseEventAction::OH_NATIVEXCOMPONENT_MOUSE_MOVE)) ||
        (type == static_cast<uint32_t>(OHNativeXcomponentMouseEventAction::OH_NATIVEXCOMPONENT_MOUSE_NONE)) ||
        (type == static_cast<uint32_t>(OHNativeXcomponentMouseEventAction::OH_NATIVEXCOMPONENT_MOUSE_PULL_IN_WINDOW));
}

inline bool EXPORT IsMouseEventButton(uint32_t type)
{
    return (type == static_cast<uint32_t>(OHNativeXcomponentMouseEventButton::OH_NATIVEXCOMPONENT_LEFT_BUTTON)) ||
        (type == static_cast<uint32_t>(OHNativeXcomponentMouseEventButton::OH_NATIVEXCOMPONENT_MIDDLE_BUTTON)) ||
        (type == static_cast<uint32_t>(OHNativeXcomponentMouseEventButton::OH_NATIVEXCOMPONENT_RIGHT_BUTTON));
}

using WheelEventDirection = OHNativeXcomponentWheelEventDirection;
inline bool EXPORT IsWheelEventDirection(int32_t type)
{
    return (type == static_cast<int32_t>(WheelEventDirection::OH_NATIVEXCOMPONENT_WHEEL_VERTICAL)) ||
        (type == static_cast<int32_t>(WheelEventDirection::OH_NATIVEXCOMPONENT_WHEEL_HORIZONTAL));
}

inline bool EXPORT IsKeyEventType(uint32_t type)
{
    return (type == static_cast<uint32_t>(OHNativeXcomponentKeyEventType::OH_NATIVEXCOMPONENT_KEY_DOWN)) ||
        (type == static_cast<uint32_t>(OHNativeXcomponentKeyEventType::OH_NATIVEXCOMPONENT_KEY_UP));
}

using InputMethodEventType = OHNativeXcomponentInputMethodEventType;
inline bool EXPORT IsInputMethodEventType(uint16_t type)
{
    return (type == static_cast<uint16_t>(InputMethodEventType::OH_NATIVEXCOMPONENT_INPUT_CONTENT)) ||
        (type == static_cast<uint16_t>(InputMethodEventType::OH_NATIVEXCOMPONENT_INPUT_FOCUS));
}

using VirtualKeyEventType = OHNativeXcomponentVirtualKeyEventType;
inline bool EXPORT IsVirtualKeyEventType(int32_t type)
{
    return (type == static_cast<int32_t>(VirtualKeyEventType::OH_NATIVEXCOMPONENT_VIRTUALKEY_BACK)) ||
        (type == static_cast<int32_t>(VirtualKeyEventType::OH_NATIVEXCOMPONENT_VIRTUALKEY_HOME)) ||
        (type == static_cast<int32_t>(VirtualKeyEventType::OH_NATIVEXCOMPONENT_VIRTUALKEY_RECENT_APP)) ||
        (type == static_cast<int32_t>(VirtualKeyEventType::OH_NATIVEXCOMPONENT_VIRTUALKEY_QUICK_SETTING));
}
} // namespace CastEngine
} // namespace OHOS

#endif
