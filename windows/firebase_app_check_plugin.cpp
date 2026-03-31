#include "firebase_app_check_plugin.h"

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <memory>
#include <string>
#include <thread>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Services.Store.h>

namespace firebase_app_check_windows {

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
    result->Success(flutter::EncodableValue(true));

  } else if (method_call.method_name() == "AppCheck#getToken") {
    std::lock_guard<std::mutex> lock(token_mutex_);
    flutter::EncodableMap response;
    response[flutter::EncodableValue("token")] =
        flutter::EncodableValue(cached_token_);
    response[flutter::EncodableValue("expireTimeMillis")] =
        flutter::EncodableValue(cached_expire_time_millis_);
    result->Success(flutter::EncodableValue(response));

  } else if (method_call.method_name() == "AppCheck#setToken") {
    const auto *args =
        std::get_if<flutter::EncodableMap>(method_call.arguments());
    if (args) {
      auto token_it = args->find(flutter::EncodableValue("token"));
      auto expire_it = args->find(flutter::EncodableValue("expireTimeMillis"));
      if (token_it != args->end() && expire_it != args->end()) {
        std::lock_guard<std::mutex> lock(token_mutex_);
        cached_token_ = std::get<std::string>(token_it->second);
        cached_expire_time_millis_ = std::get<int64_t>(expire_it->second);
      }
    }
    result->Success(nullptr);

  } else if (method_call.method_name() == "AppCheck#getStoreIdKey") {
    const auto *service_ticket =
        std::get_if<std::string>(method_call.arguments());
    if (!service_ticket || service_ticket->empty()) {
      result->Error("invalid-argument", "Missing or empty service ticket");
      return;
    }

    // Move result ownership to the background thread via raw pointer.
    // Flutter's MethodResult supports cross-thread responses.
    auto *result_raw = result.release();
    auto ticket_copy = *service_ticket;

    std::thread([ticket_copy, result_raw]() {
      auto result_ptr =
          std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>>(
              result_raw);
      try {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);

        auto context =
            winrt::Windows::Services::Store::StoreContext::GetDefault();
        // GetCustomerCollectionsIdAsync(serviceTicket, publisherUserId)
        // publisherUserId is an anonymous user identifier; empty string is
        // acceptable when user-level tracking is not needed.
        auto store_id_key = context.GetCustomerCollectionsIdAsync(
            winrt::to_hstring(ticket_copy), L"").get();

        result_ptr->Success(
            flutter::EncodableValue(winrt::to_string(store_id_key)));

        winrt::uninit_apartment();
      } catch (const winrt::hresult_error &ex) {
        result_ptr->Error("store-error", winrt::to_string(ex.message()));
        try { winrt::uninit_apartment(); } catch (...) {}
      } catch (const std::exception &ex) {
        result_ptr->Error("store-error", ex.what());
        try { winrt::uninit_apartment(); } catch (...) {}
      } catch (...) {
        result_ptr->Error("store-error",
                          "Unknown error accessing Windows Store API");
        try { winrt::uninit_apartment(); } catch (...) {}
      }
    }).detach();

  } else if (method_call.method_name() ==
             "AppCheck#setTokenAutoRefreshEnabled") {
    result->Success(nullptr);

  } else {
    result->NotImplemented();
  }
}

}  // namespace firebase_app_check_windows
