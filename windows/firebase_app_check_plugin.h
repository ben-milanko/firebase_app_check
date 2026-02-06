#ifndef FLUTTER_PLUGIN_FIREBASE_APP_CHECK_PLUGIN_H_
#define FLUTTER_PLUGIN_FIREBASE_APP_CHECK_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <memory>

namespace firebase_app_check_windows {

class FirebaseAppCheckPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

  FirebaseAppCheckPlugin();

  virtual ~FirebaseAppCheckPlugin();

 private:
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
};

}  // namespace firebase_app_check_windows

// Registers the plugin with the Flutter engine.
void FirebaseAppCheckPluginRegisterWithRegistrar(
    flutter::PluginRegistrarWindows *registrar);

#endif  // FLUTTER_PLUGIN_FIREBASE_APP_CHECK_PLUGIN_H_
