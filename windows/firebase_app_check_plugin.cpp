#include "firebase_app_check_plugin.h"

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <memory>
#include <string>

#include <Shobjidl.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Services.Store.h>

namespace firebase_app_check_windows {

void FirebaseAppCheckPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows *registrar) {
  auto channel =
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          registrar->messenger(), "plugins.flutter.io/firebase_app_check",
          &flutter::StandardMethodCodec::GetInstance());

  // Get the top-level window handle for IInitializeWithWindow
  HWND hwnd = nullptr;
  if (registrar->GetView()) {
    hwnd = GetAncestor(registrar->GetView()->GetNativeWindow(), GA_ROOT);
  }

  auto plugin = std::make_unique<FirebaseAppCheckPlugin>(hwnd);

  channel->SetMethodCallHandler(
      [plugin_pointer = plugin.get()](const auto &call, auto result) {
        plugin_pointer->HandleMethodCall(call, std::move(result));
      });

  registrar->AddPlugin(std::move(plugin));
}

FirebaseAppCheckPlugin::FirebaseAppCheckPlugin(HWND hwnd) : hwnd_(hwnd) {}

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

    try {
      // Get the StoreContext on the main thread (required for packaged apps)
      auto context =
          winrt::Windows::Services::Store::StoreContext::GetDefault();

      // Desktop Bridge apps must associate the StoreContext with a window
      // handle via IInitializeWithWindow, otherwise Store API calls fail
      // with E_INVALIDARG.
      if (hwnd_) {
        auto init_window = context.as<IInitializeWithWindow>();
        init_window->Initialize(hwnd_);
      }

      auto ticket_hstring = winrt::to_hstring(*service_ticket);

      // Start the async operation on the main thread, then use the
      // Completed callback so we don't block the Flutter UI thread.
      auto async_op = context.GetCustomerCollectionsIdAsync(
          ticket_hstring, L"trax-windows-user");

      // Move result ownership into the completion handler via raw pointer.
      auto *result_raw = result.release();

      async_op.Completed(
          [result_raw](
              winrt::Windows::Foundation::IAsyncOperation<winrt::hstring>
                  const &op,
              winrt::Windows::Foundation::AsyncStatus status) {
            auto result_ptr =
                std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>>(
                    result_raw);

            if (status ==
                winrt::Windows::Foundation::AsyncStatus::Completed) {
              auto store_id_key = op.GetResults();
              result_ptr->Success(
                  flutter::EncodableValue(winrt::to_string(store_id_key)));
            } else if (status ==
                       winrt::Windows::Foundation::AsyncStatus::Error) {
              try {
                op.GetResults();  // re-throws the original exception
              } catch (const winrt::hresult_error &ex) {
                result_ptr->Error("store-error",
                                  winrt::to_string(ex.message()));
              } catch (...) {
                result_ptr->Error("store-error",
                                  "Store async operation failed");
              }
            } else {
              result_ptr->Error("store-cancelled",
                                "Store operation was cancelled");
            }
          });

    } catch (const winrt::hresult_error &ex) {
      result->Error("store-error", winrt::to_string(ex.message()));
    } catch (const std::exception &ex) {
      result->Error("store-error", ex.what());
    } catch (...) {
      result->Error("store-error",
                    "Unknown error accessing Windows Store API");
    }

  } else if (method_call.method_name() ==
             "AppCheck#setTokenAutoRefreshEnabled") {
    result->Success(nullptr);

  } else {
    result->NotImplemented();
  }
}

}  // namespace firebase_app_check_windows
