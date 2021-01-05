// Copyright (C) 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "emulator/webrtc/Switchboard.h"

#include <api/scoped_refptr.h>            // for scoped_refptr
#include <rtc_base/critical_section.h>    // for CritScope
#include <rtc_base/logging.h>             // for RTC_LOG
#include <rtc_base/ref_counted_object.h>  // for RefCountedObject
#include <map>                            // for map, operator==
#include <memory>                         // for shared_ptr, shared_...
#include <ostream>                        // for operator<<, ostream
#include <string>                         // for string, basic_string
#include <unordered_map>                  // for unordered_map, unor...
#include <utility>                        // for move
#include <vector>                         // for vector

#include "android/base/Log.h"                     // for LogStreamVoidify, LOG
#include "android/base/Optional.h"                // for Optional
#include "android/base/ProcessControl.h"          // for parseEscapedLaunchS...
#include "android/base/containers/BufferQueue.h"  // for BufferQueue, Buffer...
#include "android/base/synchronization/Lock.h"    // for Lock, AutoReadLock
#include "android/base/system/System.h"           // for System, System::Dur...
#include "emulator/webrtc/Participant.h"          // for Parti...
#include "emulator/webrtc/RtcConnection.h"        // for json, RtcConnection
#include "nlohmann/json.hpp"                      // for operator""_json

using namespace android::base;

namespace emulator {
namespace webrtc {

std::string Switchboard::BRIDGE_RECEIVER = "WebrtcVideoBridge";

Switchboard::~Switchboard() {}

Switchboard::Switchboard(EmulatorGrpcClient* client,
                         const std::string& shmPath,
                         const std::string& turnConfig)
    : RtcConnection(client),
      mShmPath(shmPath),
      mTurnConfig(parseEscapedLaunchString(turnConfig)) {}

void Switchboard::rtcConnectionDropped(std::string participant) {
    rtc::CritScope cs(&mCleanupCS);
    mDroppedConnections.push_back(participant);
}

void Switchboard::rtcConnectionClosed(std::string participant) {
    rtc::CritScope cs(&mCleanupClosedCS);
    mClosedConnections.push_back(participant);
}

void Switchboard::finalizeConnections() {
    {
        rtc::CritScope cs(&mCleanupCS);
        for (auto participant : mDroppedConnections) {
            AutoWriteLock lock(mMapLock);
            if (mId.find(participant) != mId.end()) {
                auto queue = mId[participant];
                mLocks.erase(queue.get());
                mId.erase(participant);
            }
            mConnections[participant]->Close();
        }
        mDroppedConnections.clear();
    }

    {
        rtc::CritScope cs(&mCleanupClosedCS);
        for (auto participant : mClosedConnections) {
            mIdentityMap.erase(participant);
            mConnections.erase(participant);
        }
        mClosedConnections.clear();
    }
}

void Switchboard::send(std::string to, json message) {
    RTC_LOG(INFO) << "send to: " << to << " msg: " << message.dump();
    if (mId.find(to) != mId.end()) {
        auto queue = mId[to];
        {
            AutoLock pushLock(*mLocks[queue.get()]);
            if (queue->tryPushLocked(message.dump()) != BufferQueueResult::Ok) {
                LOG(ERROR) << "Unable to push message " << message.dump()
                           << "dropping it";
            }
        }
    } else {
        RTC_LOG(WARNING) << "Sending to an unknown participant!";
    }
}

bool Switchboard::connect(std::string identity) {
    RTC_LOG(INFO) << "Connecting: " << identity;
    mMapLock.lockRead();
    if (mId.find(identity) == mId.end()) {
        mMapLock.unlockRead();
        AutoWriteLock upgrade(mMapLock);
        std::shared_ptr<Lock> bufferLock(new Lock());
        std::shared_ptr<MessageQueue> queue(
                new MessageQueue(kMaxMessageQueueLen, *(bufferLock.get())));
        mId[identity] = queue;
        mLocks[queue.get()] = bufferLock;
    } else {
        mMapLock.unlockRead();
    }

    rtc::scoped_refptr<Participant> stream(
            new rtc::RefCountedObject<Participant>(
                    this, identity, mConnectionFactory.get(), mClient));
    if (!stream->Initialize()) {
        return false;
    }
    mConnections[identity] = stream;

    // Note: Dynamically adding new tracks will likely require stream
    // re-negotiation.
    stream->AddVideoTrack("grpcVideo");
    stream->AddAudioTrack("grpcAudio");
    stream->CreateOffer();

    // Inform the client we are going to start, this is the place
    // where we should send the turn config to the client.
    // Turn is really only needed when your server is
    // locked down pretty well. (Google gce environment for example)
    json turnConfig = "{}"_json;
    if (mTurnConfig.size() > 0) {
        System::ProcessExitCode exitCode;
        Optional<std::string> turn = System::get()->runCommandWithResult(
                mTurnConfig, kMaxTurnConfigTime, &exitCode);
        if (exitCode == 0 && turn && json::jsonaccept(*turn)) {
            json config = json::parse(*turn, nullptr, false);
            if (config.count("iceServers")) {
                turnConfig = config;
            } else {
                RTC_LOG(LS_ERROR) << "Unusable turn config: " << turn;
            }
        }
    }

    // The format is json: {"start" : RTCPeerConnection config}
    // (i.e. result from
    // https://networktraversal.googleapis.com/v1alpha/iceconfig?key=....)
    json start;
    start["start"] = turnConfig;
    send(identity, start);

    return true;
}

void Switchboard::disconnect(std::string identity) {
    {
        AutoReadLock lock(mMapLock);
        if (mId.find(identity) == mId.end()) {
            LOG(ERROR) << "Trying to remove unknown queue, ignoring";
            return;
        }
    }
    LOG(INFO) << "disconnect: " << identity;
    AutoWriteLock upgrade(mMapLock);
    // Unlikely but someone could have yanked identity out of the map
    if (mId.find(identity) == mId.end()) {
        LOG(ERROR) << "Trying to remove unknown queue, ignoring";
        return;
    }

    auto queue = mId[identity];
    mId.erase(identity);
    mLocks.erase(queue.get());
    mIdentityMap.erase(identity);
    mConnections.erase(identity);
}

bool Switchboard::acceptJsepMessage(std::string identity, std::string message) {
    AutoReadLock lock(mMapLock);
    if (mId.count(identity) == 0 || mConnections.count(identity) == 0) {
        LOG(ERROR) << "Trying to send to unknown identity.";
        return false;
    }
    if (!json::jsonaccept(message)) {
        return false;
    }
    mConnections[identity]->IncomingMessage(json::parse(message));
    return true;
}

bool Switchboard::nextMessage(std::string identity,
                              std::string* nextMessage,
                              System::Duration blockAtMostMs) {
    std::shared_ptr<MessageQueue> queue;
    std::shared_ptr<Lock> lock;
    {
        AutoReadLock readerLock(mMapLock);
        if (mId.find(identity) == mId.end()) {
            LOG(ERROR) << "Unknown identity: " << identity;
            *nextMessage =
                    std::move(std::string("{ \"bye\" : \"disconnected\" }"));
            return false;
        }
        queue = mId[identity];
        lock = mLocks[queue.get()];
        lock->lock();
    }

    System::Duration blockUntil =
            System::get()->getUnixTimeUs() + blockAtMostMs * 1000;
    bool status = queue->popLockedBefore(nextMessage, blockUntil) ==
                  BufferQueueResult::Ok;
    lock->unlock();
    return status;
}

}  // namespace webrtc
}  // namespace emulator
