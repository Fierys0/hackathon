#include "titlescreen.hpp"
#include "demoDesktop.hpp"
#include "../core/globals.hpp"
#include "fumbo.hpp"
#include <memory>
#include <string>
#include <algorithm>

void TitleScreen::Init() {
  // Preload Assets
  m_titlecard = Fumbo::Assets::LoadTexture("assets/images/titlecard.png");
  m_background = Fumbo::Assets::LoadTexture("assets/images/background.png");

  // Load Audio (Sound Effects Only)
  auto &am = Fumbo::Engine::Instance().GetAudioManager();
  am.LoadAudio("click", "assets/music/click.mp3", Fumbo::Audio::AudioType::SOUND);
  am.LoadAudio("notification", "assets/music/notification.mp3", Fumbo::Audio::AudioType::SOUND);
  am.LoadAudio("warning", "assets/music/warning.mp3", Fumbo::Audio::AudioType::SOUND);

  // Center buttons horizontally at x = 540, width = 200, height = 40.
  // Y positions: 400, 455, 510, 565
  m_btnstart = Fumbo::UI::Button({540, 400, 200, 40});
  m_btnstart.AddText("START SIMULATION", OS::GlobalFont, 12, BLACK);

  m_btnload = Fumbo::UI::Button({540, 455, 200, 40});
  m_btnload.AddText("LOAD SHIFT", OS::GlobalFont, 12, BLACK);
  m_btnload.SetInteractable(false);

  m_btnsettings = Fumbo::UI::Button({540, 510, 200, 40});
  m_btnsettings.AddText("SETTINGS", OS::GlobalFont, 12, BLACK);

  m_btnexit = Fumbo::UI::Button({540, 565, 200, 40});
  m_btnexit.AddText("SHUT DOWN", OS::GlobalFont, 12, BLACK);
}

void TitleScreen::Cleanup() {
  // Unload Assets After Gamestate changes
  UnloadTexture(m_titlecard);
  UnloadTexture(m_background);
}

void TitleScreen::Update() {
  if (m_btnstart.IsPressed()) {
    Fumbo::Engine::Instance().GetAudioManager().PlaySound("click");
    Fumbo::Instance().ChangeState(std::make_shared<DemoDesktop>());
  }

  if (m_btnsettings.IsPressed()) {
    Fumbo::Engine::Instance().GetAudioManager().PlaySound("click");
    m_showSettings = !m_showSettings;
  }

  if (m_btnexit.IsPressed()) {
    Fumbo::Engine::Instance().GetAudioManager().PlaySound("click");
    Fumbo::Instance().Quit();
  }
}

void TitleScreen::DrawClean() {
  // Draw classic Windows 95 teal solid background
  Fumbo::Graphic2D::DrawRectangleRec(
      {0.0f, 0.0f, 1280.0f, 720.0f},
      {0, 128, 128, 255});

  // Center the titlecard horizontally and scale it to exactly 50% of the screen width
  float scaledW = 1280.0f * 0.50f;
  float scaleFactor = scaledW / (float)m_titlecard.width;
  float scaledH = (float)m_titlecard.height * scaleFactor;
  float tcX = (1280.0f - scaledW) * 0.5f;
  Fumbo::Graphic2D::DrawTexture(
      m_titlecard, {tcX, 70.0f},
      {scaledW, scaledH});
}

void TitleScreen::DrawDirty() {
  m_btnstart.Draw();
  m_btnload.Draw();
  m_btnsettings.Draw();
  m_btnexit.Draw();

  // Helper lambda to draw classic Windows 95/98 3D borders over the flat buttons
  auto DrawWin95ButtonBorder = [](Rectangle r, bool pressed) {
    if (pressed) {
      Fumbo::Graphic2D::DrawLineEx({r.x, r.y}, {r.x + r.width, r.y}, 1.0f, {128, 128, 128, 255});
      Fumbo::Graphic2D::DrawLineEx({r.x, r.y}, {r.x, r.y + r.height}, 1.0f, {128, 128, 128, 255});
      Fumbo::Graphic2D::DrawLineEx({r.x + r.width - 1.0f, r.y}, {r.x + r.width - 1.0f, r.y + r.height}, 1.0f, WHITE);
      Fumbo::Graphic2D::DrawLineEx({r.x, r.y + r.height - 1.0f}, {r.x + r.width, r.y + r.height - 1.0f}, 1.0f, WHITE);
    } else {
      Fumbo::Graphic2D::DrawLineEx({r.x, r.y}, {r.x + r.width, r.y}, 1.0f, WHITE);
      Fumbo::Graphic2D::DrawLineEx({r.x, r.y}, {r.x, r.y + r.height}, 1.0f, WHITE);
      Fumbo::Graphic2D::DrawLineEx({r.x + r.width - 1.0f, r.y}, {r.x + r.width - 1.0f, r.y + r.height}, 1.0f, {128, 128, 128, 255});
      Fumbo::Graphic2D::DrawLineEx({r.x, r.y + r.height - 1.0f}, {r.x + r.width, r.y + r.height - 1.0f}, 1.0f, {128, 128, 128, 255});
    }
  };

  DrawWin95ButtonBorder({540, 400, 200, 40}, m_btnstart.IsHover() && IsMouseButtonDown(MOUSE_LEFT_BUTTON));
  DrawWin95ButtonBorder({540, 455, 200, 40}, false); // Disabled Load button is never pressed
  DrawWin95ButtonBorder({540, 510, 200, 40}, m_btnsettings.IsHover() && IsMouseButtonDown(MOUSE_LEFT_BUTTON));
  DrawWin95ButtonBorder({540, 565, 200, 40}, m_btnexit.IsHover() && IsMouseButtonDown(MOUSE_LEFT_BUTTON));

  if (m_showSettings) {
    DrawSettingsModal();
  }
}

void TitleScreen::DrawSettingsModal() {
  const float PANEL_W = 360.0f;
  const float PANEL_H = 200.0f;
  const float PANEL_X = (1280.0f - PANEL_W) * 0.5f;
  const float PANEL_Y = (720.0f - PANEL_H) * 0.5f;

  // ----- scaled mouse -----
  Vector2 scale  = Fumbo::Utils::GetUIScale();
  Vector2 offset = Fumbo::Utils::GetUIOffset();
  Vector2 rawMouse = GetMousePosition();
  Vector2 mouse = { (rawMouse.x - offset.x) / scale.x,
                    (rawMouse.y - offset.y) / scale.y };

  // Semi-transparent dim overlay
  Fumbo::Graphic2D::DrawRectangleRec({0, 0, 1280, 720}, {0, 0, 0, 120});

  // Win95-style panel background (grey)
  Fumbo::Graphic2D::DrawRectangleRec({PANEL_X, PANEL_Y, PANEL_W, PANEL_H}, {192, 192, 192, 255});

  // Outer 3D border (raised)
  // top/left bright
  Fumbo::Graphic2D::DrawLineEx({PANEL_X, PANEL_Y}, {PANEL_X + PANEL_W, PANEL_Y}, 2.0f, WHITE);
  Fumbo::Graphic2D::DrawLineEx({PANEL_X, PANEL_Y}, {PANEL_X, PANEL_Y + PANEL_H}, 2.0f, WHITE);
  // bottom/right dark
  Fumbo::Graphic2D::DrawLineEx({PANEL_X + PANEL_W, PANEL_Y}, {PANEL_X + PANEL_W, PANEL_Y + PANEL_H}, 2.0f, {64,64,64,255});
  Fumbo::Graphic2D::DrawLineEx({PANEL_X, PANEL_Y + PANEL_H}, {PANEL_X + PANEL_W, PANEL_Y + PANEL_H}, 2.0f, {64,64,64,255});

  // Title bar
  Rectangle titleBar = {PANEL_X, PANEL_Y, PANEL_W, 24.0f};
  Fumbo::Graphic2D::DrawRectangleRec(titleBar, {0, 0, 128, 255});
  Fumbo::Graphic2D::DrawText("Settings", {PANEL_X + 8.0f, PANEL_Y + 5.0f}, OS::GlobalFont, 12, WHITE);

  // Close button [X]
  Rectangle closeRect = {PANEL_X + PANEL_W - 22.0f, PANEL_Y + 2.0f, 20.0f, 20.0f};
  bool closeHover = CheckCollisionPointRec(mouse, closeRect);
  Fumbo::Graphic2D::DrawRectangleRec(closeRect, closeHover ? Color{180,180,180,255} : Color{192,192,192,255});
  // 3D border for close button
  Fumbo::Graphic2D::DrawLineEx({closeRect.x, closeRect.y}, {closeRect.x + closeRect.width, closeRect.y}, 1.0f, WHITE);
  Fumbo::Graphic2D::DrawLineEx({closeRect.x, closeRect.y}, {closeRect.x, closeRect.y + closeRect.height}, 1.0f, WHITE);
  Fumbo::Graphic2D::DrawLineEx({closeRect.x + closeRect.width, closeRect.y}, {closeRect.x + closeRect.width, closeRect.y + closeRect.height}, 1.0f, {64,64,64,255});
  Fumbo::Graphic2D::DrawLineEx({closeRect.x, closeRect.y + closeRect.height}, {closeRect.x + closeRect.width, closeRect.y + closeRect.height}, 1.0f, {64,64,64,255});
  Fumbo::Graphic2D::DrawText("x", {closeRect.x + 5.0f, closeRect.y + 4.0f}, OS::GlobalFont, 10, BLACK);

  if (closeHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    m_showSettings = false;
    Fumbo::Engine::Instance().GetAudioManager().PlaySound("click");
    return;
  }

  float contentY = PANEL_Y + 34.0f;
  float contentX = PANEL_X + 16.0f;

  // ----- Display row -----
  Fumbo::Graphic2D::DrawText("Display:", {contentX, contentY + 4.0f}, OS::GlobalFont, 11, BLACK);

  bool isFullscreen = IsWindowFullscreen();

  // Windowed button
  Rectangle btnWindowed = {contentX + 90.0f, contentY, 100.0f, 22.0f};
  bool btnWHover = CheckCollisionPointRec(mouse, btnWindowed);
  Color btnWColor = !isFullscreen ? Color{0, 0, 128, 255} : (btnWHover ? Color{130,130,180,255} : Color{192,192,192,255});
  Fumbo::Graphic2D::DrawRectangleRec(btnWindowed, btnWColor);
  Fumbo::Graphic2D::DrawLineEx({btnWindowed.x, btnWindowed.y}, {btnWindowed.x + btnWindowed.width, btnWindowed.y}, 1.0f, WHITE);
  Fumbo::Graphic2D::DrawLineEx({btnWindowed.x, btnWindowed.y}, {btnWindowed.x, btnWindowed.y + btnWindowed.height}, 1.0f, WHITE);
  Fumbo::Graphic2D::DrawLineEx({btnWindowed.x + btnWindowed.width, btnWindowed.y}, {btnWindowed.x + btnWindowed.width, btnWindowed.y + btnWindowed.height}, 1.0f, {64,64,64,255});
  Fumbo::Graphic2D::DrawLineEx({btnWindowed.x, btnWindowed.y + btnWindowed.height}, {btnWindowed.x + btnWindowed.width, btnWindowed.y + btnWindowed.height}, 1.0f, {64,64,64,255});
  Fumbo::Graphic2D::DrawText("Windowed", {btnWindowed.x + 12.0f, btnWindowed.y + 5.0f}, OS::GlobalFont, 10, !isFullscreen ? WHITE : BLACK);
  if (btnWHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && isFullscreen) {
    ToggleFullscreen();
    Fumbo::Engine::Instance().GetAudioManager().PlaySound("click");
  }

  // Fullscreen button
  Rectangle btnFull = {contentX + 200.0f, contentY, 100.0f, 22.0f};
  bool btnFHover = CheckCollisionPointRec(mouse, btnFull);
  Color btnFColor = isFullscreen ? Color{0, 0, 128, 255} : (btnFHover ? Color{130,130,180,255} : Color{192,192,192,255});
  Fumbo::Graphic2D::DrawRectangleRec(btnFull, btnFColor);
  Fumbo::Graphic2D::DrawLineEx({btnFull.x, btnFull.y}, {btnFull.x + btnFull.width, btnFull.y}, 1.0f, WHITE);
  Fumbo::Graphic2D::DrawLineEx({btnFull.x, btnFull.y}, {btnFull.x, btnFull.y + btnFull.height}, 1.0f, WHITE);
  Fumbo::Graphic2D::DrawLineEx({btnFull.x + btnFull.width, btnFull.y}, {btnFull.x + btnFull.width, btnFull.y + btnFull.height}, 1.0f, {64,64,64,255});
  Fumbo::Graphic2D::DrawLineEx({btnFull.x, btnFull.y + btnFull.height}, {btnFull.x + btnFull.width, btnFull.y + btnFull.height}, 1.0f, {64,64,64,255});
  Fumbo::Graphic2D::DrawText("Fullscreen", {btnFull.x + 10.0f, btnFull.y + 5.0f}, OS::GlobalFont, 10, isFullscreen ? WHITE : BLACK);
  if (btnFHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !isFullscreen) {
    ToggleFullscreen();
    Fumbo::Engine::Instance().GetAudioManager().PlaySound("click");
  }

  contentY += 40.0f;

  // ----- Volume row -----
  Fumbo::Graphic2D::DrawText("Volume:", {contentX, contentY + 8.0f}, OS::GlobalFont, 11, BLACK);

  float currentVolume = Fumbo::Engine::Instance().GetAudioManager().GetMasterVolume();
  float sliderX = contentX + 90.0f;
  float sliderW = PANEL_W - 170.0f;
  Rectangle sliderTrack = {sliderX, contentY + 14.0f, sliderW, 4.0f};

  // Sunken track border
  Fumbo::Graphic2D::DrawLineEx({sliderTrack.x, sliderTrack.y}, {sliderTrack.x + sliderTrack.width, sliderTrack.y}, 1.0f, {64,64,64,255});
  Fumbo::Graphic2D::DrawLineEx({sliderTrack.x, sliderTrack.y}, {sliderTrack.x, sliderTrack.y + sliderTrack.height}, 1.0f, {64,64,64,255});
  Fumbo::Graphic2D::DrawRectangleRec(sliderTrack, {128, 128, 128, 255});
  Rectangle sliderFill = {sliderTrack.x, sliderTrack.y, sliderTrack.width * currentVolume, sliderTrack.height};
  Fumbo::Graphic2D::DrawRectangleRec(sliderFill, {0, 0, 128, 255});

  Rectangle handle = {sliderTrack.x + sliderTrack.width * currentVolume - 5.0f, contentY + 6.0f, 10.0f, 20.0f};
  Rectangle sliderHit = {sliderTrack.x - 8.0f, contentY + 4.0f, sliderTrack.width + 16.0f, 28.0f};

  if (CheckCollisionPointRec(mouse, sliderHit) && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    float newVal = (mouse.x - sliderTrack.x) / sliderTrack.width;
    newVal = std::clamp(newVal, 0.0f, 1.0f);
    Fumbo::Engine::Instance().GetAudioManager().SetMasterVolume(newVal);
  }

  bool handleHover = CheckCollisionPointRec(mouse, handle);
  Color handleColor = (handleHover || (CheckCollisionPointRec(mouse, sliderHit) && IsMouseButtonDown(MOUSE_BUTTON_LEFT))) ? Color{0,0,128,255} : Color{192,192,192,255};
  Fumbo::Graphic2D::DrawRectangleRec(handle, handleColor);
  // Handle 3D border
  Fumbo::Graphic2D::DrawLineEx({handle.x, handle.y}, {handle.x + handle.width, handle.y}, 1.0f, WHITE);
  Fumbo::Graphic2D::DrawLineEx({handle.x, handle.y}, {handle.x, handle.y + handle.height}, 1.0f, WHITE);
  Fumbo::Graphic2D::DrawLineEx({handle.x + handle.width, handle.y}, {handle.x + handle.width, handle.y + handle.height}, 1.0f, {64,64,64,255});
  Fumbo::Graphic2D::DrawLineEx({handle.x, handle.y + handle.height}, {handle.x + handle.width, handle.y + handle.height}, 1.0f, {64,64,64,255});

  // Volume % label
  std::string volLabel = std::to_string((int)(currentVolume * 100)) + "%";
  Fumbo::Graphic2D::DrawText(volLabel.c_str(), {sliderTrack.x + sliderTrack.width + 8.0f, contentY + 8.0f}, OS::GlobalFont, 10, BLACK);

  contentY += 50.0f;

  // ----- OK button -----
  Rectangle btnOK = {PANEL_X + PANEL_W * 0.5f - 40.0f, contentY + 8.0f, 80.0f, 24.0f};
  bool okHover = CheckCollisionPointRec(mouse, btnOK);
  Fumbo::Graphic2D::DrawRectangleRec(btnOK, okHover ? Color{180,180,180,255} : Color{192,192,192,255});
  Fumbo::Graphic2D::DrawLineEx({btnOK.x, btnOK.y}, {btnOK.x + btnOK.width, btnOK.y}, 1.0f, WHITE);
  Fumbo::Graphic2D::DrawLineEx({btnOK.x, btnOK.y}, {btnOK.x, btnOK.y + btnOK.height}, 1.0f, WHITE);
  Fumbo::Graphic2D::DrawLineEx({btnOK.x + btnOK.width, btnOK.y}, {btnOK.x + btnOK.width, btnOK.y + btnOK.height}, 1.0f, {64,64,64,255});
  Fumbo::Graphic2D::DrawLineEx({btnOK.x, btnOK.y + btnOK.height}, {btnOK.x + btnOK.width, btnOK.y + btnOK.height}, 1.0f, {64,64,64,255});
  Fumbo::Graphic2D::DrawText("OK", {btnOK.x + 30.0f, btnOK.y + 6.0f}, OS::GlobalFont, 11, BLACK);

  if (okHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    m_showSettings = false;
    Fumbo::Engine::Instance().GetAudioManager().PlaySound("click");
  }
}
