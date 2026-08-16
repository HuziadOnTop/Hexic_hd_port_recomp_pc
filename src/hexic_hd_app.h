// hexic_hd - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <cfenv>
#include <rex/rex_app.h>

class HexicHdApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<HexicHdApp>(new HexicHdApp(ctx, "hexic_hd",
        PPCImageConfig));
  }

  void OnPreSetup(rex::RuntimeConfig& config) override {
    _controlfp(_MCW_EM, _MCW_EM);
  }

  void OnPostSetup() override {
    _controlfp(_MCW_EM, _MCW_EM);
  }
};
