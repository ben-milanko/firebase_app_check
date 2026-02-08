// ignore_for_file: public_member_api_docs
import 'package:firebase_app_check_platform_interface/firebase_app_check_platform_interface.dart';
import 'package:flutter_web_plugins/flutter_web_plugins.dart';

class FirebaseAppCheckWeb extends FirebaseAppCheckPlatform {
  static void registerWith(Registrar registrar) {
    FirebaseAppCheckPlatform.instance = FirebaseAppCheckWeb();
  }
}
