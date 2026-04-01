#ifndef FLUTTER_PLUGIN_FIREBASE_APP_CHECK_PLUGIN_H_
#define FLUTTER_PLUGIN_FIREBASE_APP_CHECK_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <windows.h>
#include <memory>
#include <mutex>
#include <string>

namespace firebase_app_check_windows {

class FirebaseAppCheckPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

  FirebaseAppCheckPlugin(HWND hwnd);

  virtual ~FirebaseAppCheckPlugin();

 private:
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

  // Top-level window handle, needed by IInitializeWithWindow for StoreContext
  HWND hwnd_;

  // Cached App Check token and expiry (set via setToken, returned by getToken)
  std::string cached_token_;
  int64_t cached_expire_time_millis_ = 0;
  std::mutex token_mutex_;
};

}  // namespace firebase_app_check_windows

#endif  // FLUTTER_PLUGIN_FIREBASE_APP_CHECK_PLUGIN_H_
