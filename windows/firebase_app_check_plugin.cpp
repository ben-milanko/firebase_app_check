#include "firebase_app_check_plugin.h"

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <memory>
#include <string>

#include <Shobjidl.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Services.Store.h>

#include "firebase/app.h"
#include "firebase/app_check.h"

namespace firebase_app_check_windows {

// ---------------------------------------------------------------------------
// DartBridgeAppCheckProvider — feeds Dart-obtained tokens into the C++ SDK
// ---------------------------------------------------------------------------

void DartBridgeAppCheckProvider::GetToken(
    std::function<void(firebase::app_check::AppCheckToken, int,
                       const std::string&)>
        completion_handler) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (cached_token_.empty()) {
    // Return an empty token with success (0). Returning an error code (-1)
    // causes the Firebase C++ SDK to treat ALL requests (Auth, Firestore,
    // Storage) as failed, blocking login on builds where no App Check token
    // is available (e.g. non-packaged dev builds). With an empty token the
    // backend decides: if App Check is not enforced the request succeeds;
    // if enforced the backend returns a clear rejection.
    completion_handler({}, 0, "");
    return;
  }
  firebase::app_check::AppCheckToken token;
  token.token = cached_token_;
  token.expire_time_millis = cached_expire_time_millis_;
  completion_handler(token, 0, "");
}

void DartBridgeAppCheckProvider::SetToken(const std::string& token,
                                          int64_t expire_time_millis) {
  std::lock_guard<std::mutex> lock(mutex_);
  cached_token_ = token;
  cached_expire_time_millis_ = expire_time_millis;
}

// ---------------------------------------------------------------------------
// DartBridgeAppCheckProviderFactory
// ---------------------------------------------------------------------------

firebase::app_check::AppCheckProvider*
DartBridgeAppCheckProviderFactory::CreateProvider(firebase::App* /*app*/) {
  return &provider_;
}

// ---------------------------------------------------------------------------
// Static members
// ---------------------------------------------------------------------------

DartBridgeAppCheckProviderFactory FirebaseAppCheckPlugin::provider_factory_;
bool FirebaseAppCheckPlugin::provider_registered_ = false;

// ---------------------------------------------------------------------------
// Plugin registration
// ---------------------------------------------------------------------------

void FirebaseAppCheckPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows* registrar) {
  // Register the provider factory BEFORE firebase::App::Create().
  // The Firebase C++ SDK requires SetAppCheckProviderFactory to be called
  // before any other Firebase SDK is initialized, so that Firestore, Storage,
  // etc. know to request App Check tokens for their gRPC/REST requests.
  if (!provider_registered_) {
    firebase::app_check::AppCheck::SetAppCheckProviderFactory(
        &provider_factory_);
    provider_registered_ = true;
  }

  auto channel =
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          registrar->messenger(), "plugins.flutter.io/firebase_app_check",
          &flutter::StandardMethodCodec::GetInstance());

  HWND hwnd = nullptr;
  if (registrar->GetView()) {
    hwnd = GetAncestor(registrar->GetView()->GetNativeWindow(), GA_ROOT);
  }

  auto plugin = std::make_unique<FirebaseAppCheckPlugin>(hwnd);

  channel->SetMethodCallHandler(
      [plugin_pointer = plugin.get()](const auto& call, auto result) {
        plugin_pointer->HandleMethodCall(call, std::move(result));
      });

  registrar->AddPlugin(std::move(plugin));
}

FirebaseAppCheckPlugin::FirebaseAppCheckPlugin(HWND hwnd) : hwnd_(hwnd) {}

FirebaseAppCheckPlugin::~FirebaseAppCheckPlugin() {}

// ---------------------------------------------------------------------------
// Method call handler
// ---------------------------------------------------------------------------

void FirebaseAppCheckPlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue>& method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {

  if (method_call.method_name() == "AppCheck#activate") {
    // Provider factory is already registered in RegisterWithRegistrar
    // (before firebase::App::Create). Nothing to do here.
    result->Success(flutter::EncodableValue(true));

  } else if (method_call.method_name() == "AppCheck#getToken") {
    // Return the cached token from our provider.
    auto& provider = provider_factory_.provider();
    provider.GetToken(
        [&result](firebase::app_check::AppCheckToken token, int error_code,
                  const std::string& error_message) {
          flutter::EncodableMap response;
          response[flutter::EncodableValue("token")] =
              flutter::EncodableValue(token.token);
          response[flutter::EncodableValue("expireTimeMillis")] =
              flutter::EncodableValue(token.expire_time_millis);
          result->Success(flutter::EncodableValue(response));
        });

  } else if (method_call.method_name() == "AppCheck#setToken") {
    const auto* args =
        std::get_if<flutter::EncodableMap>(method_call.arguments());
    if (args) {
      auto token_it = args->find(flutter::EncodableValue("token"));
      auto expire_it = args->find(flutter::EncodableValue("expireTimeMillis"));
      if (token_it != args->end() && expire_it != args->end()) {
        // Dart's StandardMessageCodec encodes small ints as 32-bit int,
        // so handle both int and int64_t variants.
        int64_t expire_ms = 0;
        if (auto* val = std::get_if<int64_t>(&expire_it->second)) {
          expire_ms = *val;
        } else if (auto* val32 = std::get_if<int32_t>(&expire_it->second)) {
          expire_ms = static_cast<int64_t>(*val32);
        }
        provider_factory_.provider().SetToken(
            std::get<std::string>(token_it->second),
            expire_ms);
      }
    }
    result->Success(nullptr);

  } else if (method_call.method_name() == "AppCheck#getPackageIdentity") {
    try {
      auto package = winrt::Windows::ApplicationModel::Package::Current();
      auto id = package.Id();

      flutter::EncodableMap response;
      response[flutter::EncodableValue("packageFamilyName")] =
          flutter::EncodableValue(winrt::to_string(id.FamilyName()));
      response[flutter::EncodableValue("publisherId")] =
          flutter::EncodableValue(winrt::to_string(id.PublisherId()));
      response[flutter::EncodableValue("packageFullName")] =
          flutter::EncodableValue(winrt::to_string(id.FullName()));
      response[flutter::EncodableValue("publisherDisplayName")] =
          flutter::EncodableValue(
              winrt::to_string(package.PublisherDisplayName()));
      response[flutter::EncodableValue("version")] =
          flutter::EncodableValue(
              std::to_string(id.Version().Major) + "." +
              std::to_string(id.Version().Minor) + "." +
              std::to_string(id.Version().Build) + "." +
              std::to_string(id.Version().Revision));

      result->Success(flutter::EncodableValue(response));
    } catch (const winrt::hresult_error& ex) {
      result->Error("package-error", winrt::to_string(ex.message()));
    } catch (...) {
      result->Error(
          "package-error",
          "Failed to read package identity (app may not be packaged)");
    }

  } else if (method_call.method_name() == "AppCheck#getStoreIdKey") {
    const auto* service_ticket =
        std::get_if<std::string>(method_call.arguments());
    if (!service_ticket || service_ticket->empty()) {
      result->Error("invalid-argument", "Missing or empty service ticket");
      return;
    }

    try {
      auto context =
          winrt::Windows::Services::Store::StoreContext::GetDefault();

      if (hwnd_) {
        auto init_window = context.as<IInitializeWithWindow>();
        init_window->Initialize(hwnd_);
      }

      auto ticket_hstring = winrt::to_hstring(*service_ticket);
      auto async_op = context.GetCustomerCollectionsIdAsync(
          ticket_hstring, L"trax-windows-user");

      auto* result_raw = result.release();

      async_op.Completed(
          [result_raw](
              winrt::Windows::Foundation::IAsyncOperation<winrt::hstring>
                  const& op,
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
                op.GetResults();
              } catch (const winrt::hresult_error& ex) {
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

    } catch (const winrt::hresult_error& ex) {
      result->Error("store-error", winrt::to_string(ex.message()));
    } catch (const std::exception& ex) {
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
