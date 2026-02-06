#include "firebase_app_check_plugin.h"

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <memory>
#include <string>

#include "firebase/app.h"
#include "firebase/app_check.h"
#include "firebase/future.h"

namespace firebase_app_check_windows {

using firebase::app_check::AppCheck;
using firebase::app_check::AppCheckToken;

void FirebaseAppCheckPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows *registrar) {
  auto channel =
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          registrar->messenger(), "plugins.flutter.io/firebase_app_check",
          &flutter::StandardMethodCodec::GetInstance());

  auto plugin = std::make_unique<FirebaseAppCheckPlugin>();

  channel->SetMethodCallHandler(
      [plugin_pointer = plugin.get()](const auto &call, auto result) {
        plugin_pointer->HandleMethodCall(call, std::move(result));
      });

  registrar->AddPlugin(std::move(plugin));
}

FirebaseAppCheckPlugin::FirebaseAppCheckPlugin() {}

FirebaseAppCheckPlugin::~FirebaseAppCheckPlugin() {}

void FirebaseAppCheckPlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue> &method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  
  if (method_call.method_name() == "AppCheck#activate") {
    // In C++, GetInstance(app) initializes it.
    // The plugin_platform_interface usually handles the logic of which provider to use.
    // For now, we return success to allow the app to proceed.
    result->Success(flutter::EncodableValue(true));
  } else if (method_call.method_name() == "AppCheck#getToken") {
    const auto* arguments = std::get_if<flutter::EncodableMap>(method_call.arguments());
    bool force_refresh = false;
    if (arguments) {
      auto it = arguments->find(flutter::EncodableValue("forceRefresh"));
      if (it != arguments->end() && std::holds_alternative<bool>(it->second)) {
        force_refresh = std::get<bool>(it->second);
      }
    }

    firebase::App* app = firebase::App::GetInstance();
    if (!app) {
      result->Error("no-app", "Firebase App not initialized");
      return;
    }

    AppCheck* app_check = AppCheck::GetInstance(app);
    firebase::Future<AppCheckToken> future = app_check->GetAppCheckToken(force_refresh);

    future.OnCompletion([result_ptr = result.release()](const firebase::Future<AppCheckToken>& completed_future) {
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result(result_ptr);
      if (completed_future.error() == 0) {
        const AppCheckToken* token = completed_future.result();
        flutter::EncodableMap response;
        response[flutter::EncodableValue("token")] = flutter::EncodableValue(token->token);
        // Platform interface expects expireTimeMillis
        response[flutter::EncodableValue("expireTimeMillis")] = flutter::EncodableValue(token->expire_time_millis);
        result->Success(flutter::EncodableValue(response));
      } else {
        result->Error("app-check-error", completed_future.error_message());
      }
    });
  } else if (method_call.method_name() == "AppCheck#setTokenAutoRefreshEnabled") {
    const auto* is_enabled = std::get_if<bool>(method_call.arguments());
    if (is_enabled) {
      firebase::App* app = firebase::App::GetInstance();
      if (app) {
        AppCheck::GetInstance(app)->SetTokenAutoRefreshEnabled(*is_enabled);
      }
    }
    result->Success(nullptr);
  } else {
    result->NotImplemented();
  }
}

}  // namespace firebase_app_check_windows
