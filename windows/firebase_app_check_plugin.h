#ifndef FLUTTER_PLUGIN_FIREBASE_APP_CHECK_PLUGIN_H_
#define FLUTTER_PLUGIN_FIREBASE_APP_CHECK_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <windows.h>

#include <memory>
#include <mutex>
#include <string>

#include "firebase/app_check.h"

namespace firebase_app_check_windows {

/// Custom AppCheckProvider that returns a token obtained from the Dart side.
/// Registered with the Firebase C++ SDK so that Firestore and other C++ plugins
/// automatically attach the App Check token to their requests.
class DartBridgeAppCheckProvider
    : public firebase::app_check::AppCheckProvider {
 public:
  void GetToken(
      std::function<void(firebase::app_check::AppCheckToken, int,
                         const std::string&)>
          completion_handler) override;

  void SetToken(const std::string& token, int64_t expire_time_millis);

 private:
  std::string cached_token_;
  int64_t cached_expire_time_millis_ = 0;
  std::mutex mutex_;
};

/// Factory that creates DartBridgeAppCheckProvider instances.
class DartBridgeAppCheckProviderFactory
    : public firebase::app_check::AppCheckProviderFactory {
 public:
  firebase::app_check::AppCheckProvider* CreateProvider(
      firebase::App* app) override;

  DartBridgeAppCheckProvider& provider() { return provider_; }

 private:
  DartBridgeAppCheckProvider provider_;
};

class FirebaseAppCheckPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows* registrar);

  FirebaseAppCheckPlugin(HWND hwnd);

  virtual ~FirebaseAppCheckPlugin();

 private:
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

  // Top-level window handle for IInitializeWithWindow
  HWND hwnd_;

  // The provider factory registered with the Firebase C++ SDK.
  // Must outlive the plugin (static so AppCheck can use it after plugin moves).
  static DartBridgeAppCheckProviderFactory provider_factory_;
  static bool provider_registered_;
};

}  // namespace firebase_app_check_windows

#endif  // FLUTTER_PLUGIN_FIREBASE_APP_CHECK_PLUGIN_H_
