prebuilt(WEBRTC)

android_add_executable(
  TARGET goldfish-webrtc-bridge
  LICENSE Apache-2.0 INSTALL .
  SRC # cmake-format: sortable
      emulator/avd/FakeAvd.cpp
      emulator/main/GoldfishBridge.cpp
      emulator/net/EmulatorForwarder.cpp
      emulator/net/EmulatorGrpcClient.cpp
      emulator/net/SocketForwarder.cpp
      emulator/webrtc/capture/AudioSource.cpp
      emulator/webrtc/capture/GrpcAudioSource.cpp
      emulator/webrtc/capture/GrpcVideoSource.cpp
      emulator/webrtc/capture/MediaSourceLibrary.cpp
      emulator/webrtc/capture/VideoCapturerFactory.cpp
      emulator/webrtc/capture/VideoShareCapturer.cpp
      emulator/webrtc/capture/VideoTrackReceiver.cpp
      emulator/webrtc/capture/VideoTrackSource.cpp
      emulator/webrtc/Participant.cpp
      emulator/webrtc/RtcConnection.cpp
      emulator/webrtc/StandaloneConnection.cpp
      emulator/webrtc/Switchboard.cpp)

android_target_compile_options(goldfish-webrtc-bridge darwin-x86_64
                               PRIVATE "-fvisibility=hidden")
android_target_compile_definitions(goldfish-webrtc-bridge linux-x86_64 PRIVATE
                                   "-DWEBRTC_POSIX")
android_target_compile_definitions(goldfish-webrtc-bridge windows_msvc-x86_64
                                   PRIVATE "-DWEBRTC_WIN")
android_target_compile_definitions(
  goldfish-webrtc-bridge darwin-x86_64 PRIVATE "-DWEBRTC_MAC" "-DWEBRTC_POSIX"
  "-D_GLIBCXX_USE_CXX11_ABI=0")

# WARNING! We are linking against a large webrtc prebuilt. This archive has
# everything needed for webrtc support This includes SSL, LIBYUV, etc.. Be very
# careful linking any of the android emulator libraries against it
target_link_libraries(
  goldfish-webrtc-bridge
  PRIVATE WebRTC::WebRTC
          Threads::Threads
          android-net
          android-emu
          android-grpc
          android-webrtc
          nlohmann_json::nlohmann_json
          ssl)
target_include_directories(goldfish-webrtc-bridge
                           PRIVATE . ${CMAKE_CURRENT_BINARY_DIR})

# The linker dependencies, mainly needed to keep the encoders/decoders happy..
android_target_link_libraries(goldfish-webrtc-bridge linux-x86_64
                              PRIVATE dl X11 rt)

android_target_link_libraries(
  goldfish-webrtc-bridge windows_msvc-x86_64
  PRIVATE # cmake-format: sortable
          amstrmid::amstrmid dmoguids::dmoguids msdmo::msdmo winmm::winmm
          wmcodecdspuuid::wmcodecdspuuid)
android_target_link_libraries(
  goldfish-webrtc-bridge darwin-x86_64
  PRIVATE # cmake-format: sortable
          "-framework AVFoundation"
          "-framework AudioToolbox"
          "-framework CoreAudio"
          "-framework CoreFoundation"
          "-framework CoreGraphics"
          "-framework CoreMedia"
          "-framework CoreVideo"
          "-framework Foundation")
