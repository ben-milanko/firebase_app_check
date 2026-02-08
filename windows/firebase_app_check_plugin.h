#ifndef FLUTTER_PLUGIN_FIREBASE_APP_CHECK_PLUGIN_H_
#define FLUTTER_PLUGIN_FIREBASE_APP_CHECK_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <memory>

#include "firebase/app_check.h"

namespace firebase_app_check_windows {

class WindowsAppCheckProvider : public firebase::app_check::AppCheckProvider {
 public:
  WindowsAppCheckProvider();
  virtual ~WindowsAppCheckProvider();

  // Fetches an App Check token.
  virtual void GetToken(std::function<void(firebase::app_check::AppCheckToken,
                                           firebase::app_check::AppCheckError,
                                           const std::string&)>
                            completion_callback) override;
};

class WindowsAppCheckProviderFactory
    : public firebase::app_check::AppCheckProviderFactory {
 public:
  WindowsAppCheckProviderFactory();
  virtual ~WindowsAppCheckProviderFactory();

  virtual firebase::app_check::AppCheckProvider* CreateProvider(
      firebase::App* app) override;
};

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

#endif  // FLUTTER_PLUGIN_FIREBASE_APP_CHECK_PLUGIN_H_
