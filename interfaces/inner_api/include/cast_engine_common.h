/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Description: Cast engine related common data stucture definitions.
 * Author: zhangge
 * Create: 2022-06-15
 */

#ifndef CAST_ENGINE_COMMON_H
#define CAST_ENGINE_COMMON_H

#include <array>
#include <map>
#include <string>

#include <message_parcel.h>
#define EXPORT __attribute__((visibility("default")))

namespace OHOS {
namespace CastEngine {
enum class EXPORT DeviceType {
    DEVICE_OTHERS = 0,
    DEVICE_SCREEN_PLAYER = 1,
    DEVICE_HW_TV = 2,
    DEVICE_SOUND_BOX = 3,
    DEVICE_HICAR = 4,
    DEVICE_MATEBOOK = 5,
    DEVICE_PAD = 6,
    DEVICE_CAST_PLUS = 7,
    DEVICE_TYPE_2IN1 = 8
};

inline bool EXPORT IsDeviceType(int32_t type)
{
    return (type >= static_cast<int32_t>(DeviceType::DEVICE_OTHERS)) &&
        (type <= static_cast<int32_t>(DeviceType::DEVICE_TYPE_2IN1));
}

enum class EXPORT SubDeviceType {
    SUB_DEVICE_DEFAULT = 0,
    SUB_DEVICE_MATEBOOK_PAD = 51,
    SUB_DEVICE_CAST_PLUS_WHITEBOARD = 0x00B2
};

inline bool EXPORT IsSubDeviceType(int32_t type)
{
    return (type == static_cast<int32_t>(SubDeviceType::SUB_DEVICE_DEFAULT)) ||
        (type == static_cast<int32_t>(SubDeviceType::SUB_DEVICE_MATEBOOK_PAD)) ||
        (type == static_cast<int32_t>(SubDeviceType::SUB_DEVICE_CAST_PLUS_WHITEBOARD));
}

enum class EXPORT TriggerType {
    UNSPEC_TAG = 0,
    PASSIVE_MATCH_TAG = 1,
    ACTIVE_MATCH_TAG = 2,
    PASSIVE_BIND_TAG = 3,
};

inline bool EXPORT IsTriggerType(int32_t type)
{
    return (type >= static_cast<int32_t>(TriggerType::UNSPEC_TAG)) &&
        (type <= static_cast<int32_t>(TriggerType::PASSIVE_BIND_TAG));
}

enum class EXPORT DeviceState {
    CONNECTING,
    CONNECTED,
    PAUSED,
    PLAYING,
    DISCONNECTING,
    DISCONNECTED,
    STREAM,
    DEVICE_STATE_MAX,
};

const EXPORT std::array<std::string, static_cast<size_t>(DeviceState::DEVICE_STATE_MAX)> DEVICE_STATE_STRING = {
    "CONNECTING", "CONNECTED", "PAUSED", "PLAYING", "DISCONNECTING", "DISCONNECTED", "STREAM",
};

inline bool EXPORT IsDeviceState(int32_t state)
{
    return (state >= static_cast<int32_t>(DeviceState::CONNECTING)) &&
        (state < static_cast<int32_t>(DeviceState::DEVICE_STATE_MAX));
}

enum class EXPORT ServiceStatus {
    DISCONNECTED,
    DISCONNECTING,
    CONNECTING,
    CONNECTED,
};

inline bool EXPORT IsServiceStatus(int32_t status)
{
    return (status >= static_cast<int32_t>(ServiceStatus::DISCONNECTED)) &&
        (status <= static_cast<int32_t>(ServiceStatus::CONNECTED));
}

enum class EXPORT DeviceStatusState {
    DEVICE_AVAILABLE,
    DEVICE_CONNECTED,
    DEVICE_DISCONNECTED,
    DEVICE_CONNECT_REQ,
};

inline bool EXPORT IsDeviceStatus(int32_t status)
{
    return (status >= static_cast<int32_t>(DeviceStatusState::DEVICE_AVAILABLE)) &&
        (status <= static_cast<int32_t>(DeviceStatusState::DEVICE_CONNECT_REQ));
}

enum class EXPORT PropertyType {
    VIDEO_SIZE,
    VIDEO_FPS,
    WINDOW_SIZE,
};

inline bool EXPORT IsPropertyType(int32_t type)
{
    return (type >= static_cast<int32_t>(PropertyType::VIDEO_SIZE)) &&
        (type <= static_cast<int32_t>(PropertyType::WINDOW_SIZE));
}

enum class EXPORT ChannelType {
    SOFT_BUS,
    LEGACY_CHANNEL,
};

inline bool EXPORT IsChannelType(int32_t type)
{
    return (type == static_cast<int32_t>(ChannelType::SOFT_BUS)) ||
        (type == static_cast<int32_t>(ChannelType::LEGACY_CHANNEL));
}

enum class EXPORT ProtocolType {
    CAST_PLUS_MIRROR = 1<<0,
    CAST_PLUS_STREAM = 1<<1,
    MIRACAST = 1<<2,
    DLNA = 1<<3,
    COOPERATION = 1<<4
};

inline bool EXPORT IsProtocolType(int32_t type)
{
    return (static_cast<uint32_t>(type) & (static_cast<uint32_t>(ProtocolType::CAST_PLUS_MIRROR) |
        static_cast<uint32_t>(ProtocolType::CAST_PLUS_STREAM) |
        static_cast<uint32_t>(ProtocolType::MIRACAST) |
        static_cast<uint32_t>(ProtocolType::DLNA) |
        static_cast<uint32_t>(ProtocolType::COOPERATION))) != 0;
}

enum class EXPORT EndType {
    CAST_SINK = 0x10,
    CAST_SOURCE = 0x11,
};

inline bool EXPORT IsEndType(int32_t type)
{
    return (type == static_cast<int32_t>(EndType::CAST_SINK)) || (type == static_cast<int32_t>(EndType::CAST_SOURCE));
}

struct EXPORT DeviceStateInfo {
    DeviceState deviceState{ DeviceState::DISCONNECTED };
    std::string deviceId{};
};

struct EXPORT VideoSize {
    uint32_t width;
    uint32_t height;
};

struct EXPORT WindowProperty {
    uint32_t startX;
    uint32_t startY;
    uint32_t width;
    uint32_t height;
};

struct EXPORT PropertyContainer {
    PropertyType type;
    union {
        VideoSize videoSize;
        uint32_t videoFps;
        WindowProperty windowProperty;
    };
};

enum class EXPORT ColorStandard {
    BT709 = 1,
    BT601_PAL = 2,
    BT601_NTSC = 3,
    BT2020 = 6,
};

inline bool EXPORT IsColorStandard(int32_t color)
{
    return (color == static_cast<int32_t>(ColorStandard::BT709)) ||
        (color == static_cast<int32_t>(ColorStandard::BT601_PAL)) ||
        (color == static_cast<int32_t>(ColorStandard::BT601_NTSC)) ||
        (color == static_cast<int32_t>(ColorStandard::BT2020));
}

enum class EXPORT VideoCodecType {
    H264 = 1,
    H265 = 2,
};

inline bool EXPORT IsVideoCodecType(int32_t type)
{
    return (type == static_cast<int32_t>(VideoCodecType::H264)) || type == static_cast<int32_t>(VideoCodecType::H265);
}

struct EXPORT AudioProperty {
    uint32_t sampleRate;
    uint8_t sampleBitWidth;
    uint32_t channelConfig;
    uint32_t bitrate;
    uint32_t codec;
};

struct EXPORT VideoProperty {
    uint32_t videoWidth{ 0 };
    uint32_t videoHeight{ 0 };
    uint32_t fps{ 0 };
    VideoCodecType codecType{ VideoCodecType::H264 };
    uint32_t gop{ 0 };
    uint32_t bitrate{ 0 };
    uint32_t minBitrate{ 0 };
    uint32_t maxBitrate{ 0 };
    uint32_t dpi{ 0 };
    ColorStandard colorStandard{ ColorStandard::BT709 };
    uint32_t screenWidth{ 0 };
    uint32_t screenHeight{ 0 };
    uint32_t profile{ 0 };
    uint32_t level{ 0 };
};

struct EXPORT CastSessionProperty {
    ProtocolType protocolType{ ProtocolType::CAST_PLUS_MIRROR };
    EndType endType{ EndType::CAST_SINK };
    AudioProperty audioProperty;
    VideoProperty videoProperty;
    WindowProperty windowProperty;
};

struct EXPORT CastLocalDevice {
    std::string deviceId;
    std::string deviceName;
    DeviceType deviceType;
    SubDeviceType subDeviceType;
    std::string ipAddress;
    TriggerType triggerType;
    std::string authData;
};

struct EXPORT AuthInfo {
    int authMode;
    uint32_t authCode;
    std::string deviceId;
};

struct EXPORT CastRemoteDevice {
    std::string deviceId;
    std::string deviceName;
    DeviceType deviceType;
    SubDeviceType subDeviceType;
    std::string ipAddress;
    ChannelType channelType;
};

enum class EXPORT CastMode {
    MIRROR_CAST = 1,
    APP_CAST = 2,
};

// Parameters for cast mode
const std::string EXPORT KEY_BUNDLE_NAME = "bundleName";
const std::string EXPORT KEY_PID = "pid";
const std::string EXPORT KEY_UID = "uid";
const std::string EXPORT KEY_APP_MIN_COMPATIBLE_VERSION = "minCompatibleVersionCode";
const std::string EXPORT KEY_APP_TARGET_VERSION = "targetVersion";
const int EXPORT MAX_DEVICE_NUM = 100;
const int32_t EXPORT MAX_FILE_NUM = 16 * 1024;

enum class EXPORT EventId {
    EVENT_BEGIN = 1,
    EVENT_END = 5000,
};

inline bool EXPORT IsEventId(int32_t state)
{
    return (state > static_cast<int32_t>(EventId::EVENT_BEGIN)) && (state < static_cast<int32_t>(EventId::EVENT_END));
}

struct EXPORT MediaInfo {
    std::string mediaId;
    std::string mediaName;
    std::string mediaUrl;
    std::string mediaType;
    size_t mediaSize;
    uint32_t startPosition;
    uint32_t duration;
    uint32_t closingCreditsPosition;
    std::string albumCoverUrl;
    std::string albumTitle;
    std::string mediaArtist;
    std::string lrcUrl;
    std::string lrcContent;
    std::string appIconUrl;
    std::string appName;
};

struct EXPORT MediaInfoHolder {
    uint32_t currentIndex;
    std::vector<MediaInfo> mediaInfoList;
    uint32_t progressRefreshInterval;
};

// <source file, <fd, target file path>>
using FileFdMap = std::map<std::string, std::pair<int, std::string>>;

// <fd, file path>
using RcvFdFileMap = std::map<int, std::string>;

enum class EXPORT PlayerStates {
    PLAYER_STATE_ERROR = 0,
    PLAYER_IDLE = 1,
    PLAYER_INITIALIZED = 2,
    PLAYER_PREPARING = 3,
    PLAYER_PREPARED = 4,
    PLAYER_STARTED = 5,
    PLAYER_PAUSED = 6,
    PLAYER_STOPPED = 7,
    PLAYER_PLAYBACK_COMPLETE = 8,
    PLAYER_RELEASED = 9
};

enum class EXPORT LoopMode {
    LOOP_MODE_SEQUENCE = 1,
    LOOP_MODE_SINGLE = 2,
    LOOP_MODE_LIST = 3,
    LOOP_MODE_SHUFFLE = 4
};

enum class EXPORT PlaybackSpeed {
    SPEED_FORWARD_0_75_X = 0,
    SPEED_FORWARD_1_00_X = 1,
    SPEED_FORWARD_1_25_X = 2,
    SPEED_FORWARD_1_75_X = 3,
    SPEED_FORWARD_2_00_X = 4
};

inline constexpr int EXPORT INVALID_ID = -1;
inline constexpr int EXPORT INVALID_VALUE = -1;
inline constexpr int EXPORT DECIMALISM = 10;
inline constexpr int EXPORT SUBSYS_CASTPLUS_SYS_ABILITY_ID_BEGIN = 0x00010000;
inline constexpr int EXPORT CAST_ENGINE_SA_ID = 5526; // SUBSYS_CASTPLUS_SYS_ABILITY_ID_BEGIN + 10; // 65546
inline constexpr int EXPORT SUBSYS_CASTPLUS_SYS_ABILITY_ID_END = 0x0001001f;
} // namespace CastEngine
} // namespace OHOS

#endif
