#include "include/firebase_app_check/firebase_app_check_plugin_c_api.h"

#include <flutter/plugin_registrar_windows.h>

#include "firebase_app_check_plugin.h"

void FirebaseAppCheckPluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  firebase_app_check_windows::FirebaseAppCheckPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
