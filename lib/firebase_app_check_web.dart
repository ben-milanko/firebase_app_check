// ignore_for_file: public_member_api_docs
import 'dart:async';

import 'package:firebase_app_check_platform_interface/firebase_app_check_platform_interface.dart';
import 'package:firebase_core/firebase_core.dart';
import 'package:flutter_web_plugins/flutter_web_plugins.dart';

class FirebaseAppCheckWeb extends FirebaseAppCheckPlatform {
  FirebaseAppCheckWeb({this.app}) : super(app?.name);

  final FirebaseApp? app;

  static void registerWith(Registrar registrar) {
    FirebaseAppCheckPlatform.instance = FirebaseAppCheckWeb();
  }

  @override
  FirebaseAppCheckPlatform delegateFor({required FirebaseApp app}) {
    return FirebaseAppCheckWeb(app: app);
  }

  @override
  Future<void> activate({
    String? webProvider,
    AndroidProvider? androidProvider,
    AppleProvider? appleProvider,
    AndroidAppCheckProvider? providerAndroid,
    AppleAppCheckProvider? providerApple,
    WebProvider? providerWeb,
  }) async {
    // No-op for the stub
  }

  @override
  Future<String?> getToken(bool forceRefresh) async {
    return null;
  }

  @override
  Future<void> setTokenAutoRefreshEnabled(bool isTokenAutoRefreshEnabled) async {
    // No-op
  }

  @override
  Future<String> getLimitedUseToken() async {
    return '';
  }

  @override
  Stream<String?> get onTokenChange => const Stream.empty();
}
