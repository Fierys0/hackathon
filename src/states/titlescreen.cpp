#include "titlescreen.hpp"
#include "demoDesktop.hpp"
#include "../core/globals.hpp"
#include "fumbo.hpp"
#include <memory>

void TitleScreen::Init() {
  // Preload Assets
  m_titlecard = Fumbo::Assets::LoadTexture("assets/images/titlecard.png");
  m_background = Fumbo::Assets::LoadTexture("assets/images/background.png");

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
    Fumbo::Instance().ChangeState(std::make_shared<DemoDesktop>());
  }

  if (m_btnsettings.IsPressed()) {
    // No-op or notification
  }

  if (m_btnexit.IsPressed()) {
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
}
