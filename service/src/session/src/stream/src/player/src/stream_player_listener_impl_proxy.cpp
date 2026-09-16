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
 * Description: supply cast session listener implement proxy class.
 * Author: huangchanggui
 * Create: 2023-01-13
 */

#include "stream_player_listener_impl_proxy.h"
#include "cast_engine_common_helper.h"
#include "cast_engine_log.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineService {
DEFINE_CAST_ENGINE_LABEL("Cast-StreamPlayer-ListenerImplProxy");

void StreamPlayerListenerImplProxy::OnStateChanged(const PlayerStates playbackState, bool isPlayWhenReady)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return;
    }
    if (!data.WriteInt32(static_cast<int>(playbackState))) {
        CLOGE("Failed to write the playbackState");
        return;
    }
    if (!data.WriteBool(isPlayWhenReady)) {
        CLOGE("Failed to write the isPlayWhenReady");
        return;
    }
    if (Remote()->SendRequest(ON_PLAYER_STATUS_CHANGED, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when reporting player status changed");
    }
}

void StreamPlayerListenerImplProxy::OnPositionChanged(int position, int bufferPosition, int duration)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return;
    }
    if (!data.WriteInt32(position)) {
        CLOGE("Failed to write the position");
        return;
    }
    if (!data.WriteInt32(bufferPosition)) {
        CLOGE("Failed to write the bufferPosition");
        return;
    }
    if (!data.WriteInt32(duration)) {
        CLOGE("Failed to write the duration");
        return;
    }
    if (Remote()->SendRequest(ON_POSITION_CHANGED, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when reporting position changed");
    }
}

void StreamPlayerListenerImplProxy::OnMediaItemChanged(const MediaInfo &mediaInfo)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return;
    }
    if (!WriteMediaInfo(data, mediaInfo)) {
        CLOGE("Failed to write the mediaInfo");
        return;
    }
    if (Remote()->SendRequest(ON_MEDIA_ITEM_CHANGED, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when reporting media item changed");
    }
}

void StreamPlayerListenerImplProxy::OnVolumeChanged(int volume, int maxVolume)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return;
    }
    if (!data.WriteInt32(volume)) {
        CLOGE("Failed to write the volume");
        return;
    }
    if (!data.WriteInt32(maxVolume)) {
        CLOGE("Failed to write the maxVolume");
        return;
    }
    if (Remote()->SendRequest(ON_VOLUME_CHANGED, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when reporting volume changed");
    }
}

void StreamPlayerListenerImplProxy::OnLoopModeChanged(const LoopMode loopMode)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return;
    }
    if (!data.WriteInt32(static_cast<int>(loopMode))) {
        CLOGE("Failed to write the loopMode");
        return;
    }
    if (Remote()->SendRequest(ON_REPEAT_MODE_CHANGED, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when reporting repeat mode changed");
    }
}

void StreamPlayerListenerImplProxy::OnPlaySpeedChanged(const PlaybackSpeed speed)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return;
    }
    if (!data.WriteInt32(static_cast<int>(speed))) {
        CLOGE("Failed to write the speed");
        return;
    }
    if (Remote()->SendRequest(ON_PLAY_SPEED_CHANGED, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when reporting play speed changed");
    }
}

void StreamPlayerListenerImplProxy::OnPlayerError(int errorCode, const std::string &errorMsg)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return;
    }
    if (!data.WriteInt32(errorCode)) {
        CLOGE("Failed to write the errorCode");
        return;
    }
    if (!data.WriteString(errorMsg)) {
        CLOGE("Failed to write the errorMsg");
        return;
    }
    if (Remote()->SendRequest(ON_PLAYER_ERROR, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when reporting player error");
    }
}

void StreamPlayerListenerImplProxy::OnVideoSizeChanged(int width, int height)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return;
    }
    if (!data.WriteInt32(width)) {
        CLOGE("Failed to write the width");
        return;
    }
    if (!data.WriteInt32(height)) {
        CLOGE("Failed to write the height");
        return;
    }
    if (Remote()->SendRequest(ON_VIDEO_SIZE_CHANGED, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when reporting video size changed");
    }
}

void StreamPlayerListenerImplProxy::OnNextRequest()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return;
    }
    if (Remote()->SendRequest(ON_NEXT_REQUEST, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when reporting next request");
    }
}

void StreamPlayerListenerImplProxy::OnPreviousRequest()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return;
    }
    if (Remote()->SendRequest(ON_PREVIOUS_REQUEST, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when reporting previous request");
    }
}

void StreamPlayerListenerImplProxy::OnSeekDone(int position)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return;
    }
    if (!data.WriteInt32(position)) {
        CLOGE("Failed to write the position");
        return;
    }
    if (Remote()->SendRequest(ON_SEEK_DONE, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when reporting seek done");
    }
}

void StreamPlayerListenerImplProxy::OnEndOfStream(int isLooping)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return;
    }
    if (!data.WriteInt32(isLooping)) {
        CLOGE("Failed to write the position");
        return;
    }
    if (Remote()->SendRequest(ON_END_OF_STREAM, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when reporting end of stream");
    }
}

void StreamPlayerListenerImplProxy::OnPlayRequest(const MediaInfo &mediaInfo)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return;
    }
    if (!WriteMediaInfo(data, mediaInfo)) {
        CLOGE("Failed to write the mediaInfo");
        return;
    }
    if (Remote()->SendRequest(ON_PLAY_REQUEST, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when reporting play request");
    }
}

void StreamPlayerListenerImplProxy::OnImageChanged(std::shared_ptr<Media::PixelMap> pixelMap)
{
    if (pixelMap == nullptr) {
        CLOGE("PixelMap is null");
        return;
    }
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
 
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return;
    }
    if (!pixelMap->Marshalling(data)) {
        CLOGE("Failed to write the pixelMap");
        return;
    }
    if (Remote()->SendRequest(ON_IMAGE_CHANGED, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when reporting image changed");
    }
}

void StreamPlayerListenerImplProxy::OnAlbumCoverChanged(std::shared_ptr<Media::PixelMap> pixelMap)
{
    if (pixelMap == nullptr) {
        CLOGE("PixelMap is null");
        return;
    }
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
 
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return;
    }
    if (!pixelMap->Marshalling(data)) {
        CLOGE("Failed to write the pixelMap");
        return;
    }
    if (Remote()->SendRequest(ON_ALBUM_COVER_CHANGED, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when reporting album cover changed");
    }
}

void StreamPlayerListenerImplProxy::OnKeyRequest(const std::string &mediaId, const std::vector<uint8_t> &keyRequestData)
{
    CHECK_AND_RETURN_LOG(keyRequestData.size() > 0, "The size of keyRequestData < 0");
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    auto len = keyRequestData.size();
    if (len > data.GetDataCapacity()) {
        CLOGD("SetDataCapacity totalSize: %zu", len);

        data.SetMaxCapacity(len + len);
        data.SetDataCapacity(len);
    }

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return;
    }

    if (!data.WriteString(mediaId)) {
        CLOGE("Failed to write mediaId");
        return;
    }

    if (!data.WriteInt32(keyRequestData.size())) {
        CLOGE("Write keyRequestData size failed");
        return;
    }

    if (len != 0) {
        if (!data.WriteBuffer(keyRequestData.data(), len)) {
            CLOGE("write keyRequestData failed");
            return;
        }
    }

    if (Remote()->SendRequest(ON_KEY_REQUEST, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when reporting key request data");
        return;
    }
}

void StreamPlayerListenerImplProxy::OnAvailableCapabilityChanged(const StreamCapability &streamCapability)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return;
    }
    if (!WriteStreamCapability(data, streamCapability)) {
        CLOGE("Failed to write the streamCapability");
        return;
    }
    if (Remote()->SendRequest(ON_AVAILABLE_CAPABILITY_CHANGED, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when reporting streamCapability");
        return;
    }
}

void StreamPlayerListenerImplProxy::OnMediaInfoChanged(const MediaInfo &mediaInfo)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
 
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        CLOGE("Failed to write the interface token");
        return;
    }
    if (!WriteMediaInfo(data, mediaInfo)) {
        CLOGE("Failed to write the mediaInfo");
        return;
    }
    if (Remote() == nullptr) {
        CLOGE("Remote() is null");
        return;
    }
    if (Remote()->SendRequest(ON_MEDIA_INFO_CHANGED, data, reply, option) != ERR_NONE) {
        CLOGE("Failed to send ipc request when reporting media info change");
    }
}
} // namespace CastEngineService
} // namespace CastEngine
} // namespace OHOS
