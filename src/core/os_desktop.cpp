#include "os_desktop.hpp"
#include <cmath>
#include <ctime>

namespace OS {

// IGameState Interface

void OSDesktop::Init() {
  // Setup default taskbar
  m_taskbar.SetFont(m_font);

  // Setup default context menu
  m_desktopContextMenu.SetFont(m_font);

  // Setup window manager defaults
  WindowManager::Instance().SetDefaultFont(m_font);

  // Setup notification manager defaults
  NotificationManager::Instance().SetFont(m_font);

  // Arrange icons
  ArrangeIcons();
}

void OSDesktop::Cleanup() {
  WindowManager::Instance().Clear();
  NotificationManager::Instance().ClearAll();

  if (m_ownsWallpaper && m_wallpaper.id != 0) {
    UnloadTexture(m_wallpaper);
    m_wallpaper = {0};
    m_ownsWallpaper = false;
  }
}

void OSDesktop::Update() {
  // Order of input handling matters:
  // 1. Context menu (always on top)
  // 2. Notifications (overlay)
  // 3. Start menu (part of taskbar)
  // 4. Windows (z-ordered)
  // 5. Taskbar
  // 6. Desktop icons and right-click

  bool consumed = false;

  // 1. Context menu
  if (m_desktopContextMenu.IsOpen()) {
    consumed = m_desktopContextMenu.Update();
    if (consumed)
      return;
  }

  // 2. Notifications
  consumed = NotificationManager::Instance().Update();
  if (consumed)
    return;

  // 3+5. Taskbar (includes start menu)
  consumed = m_taskbar.Update();
  if (consumed)
    return;

  // 4. Windows
  consumed = WindowManager::Instance().Update();
  if (consumed) {
    // Close start menu when interacting with windows
    m_taskbar.CloseStartMenu();
    return;
  }

  // 6. Desktop area: icons and right-click
  HandleDesktopClick();

  // Cleanup closed windows
  WindowManager::Instance().Cleanup();
}

void OSDesktop::DrawClean() {
  // Draw wallpaper
  if (m_wallpaper.id != 0) {
    Fumbo::Graphic2D::DrawTexture(
        m_wallpaper, {0, 0},
        {Fumbo::Utils::UI_WIDTH, Fumbo::Utils::UI_HEIGHT});
  } else {
    // Default gradient background
    Fumbo::Graphic2D::DrawRectangleGradientV(
        0, 0, (int)Fumbo::Utils::UI_WIDTH, (int)Fumbo::Utils::UI_HEIGHT,
        m_desktopStyle.backgroundColor,
        {30, 30, 60, 255});
  }
}

void OSDesktop::DrawDirty() {
  // 1. Desktop icons
  for (int i = 0; i < (int)m_icons.size(); i++) {
    auto &icon = m_icons[i];

    float cx = icon.position.x;
    float cy = icon.position.y;
    float iconSz = m_desktopStyle.iconSize;

    // Selection highlight
    if (icon.selected) {
      Rectangle selRect = {cx - 4.0f, cy - 4.0f, iconSz + 8.0f,
                           iconSz + m_desktopStyle.iconFontSize + 16.0f};
      Fumbo::Graphic2D::DrawRectangleRounded(selRect, 0.1f, 4,
                                             m_desktopStyle.iconSelectedColor);
    }

    // Icon texture
    if (icon.icon.id != 0) {
      Fumbo::Graphic2D::DrawTexture(icon.icon, {cx, cy}, {iconSz, iconSz});
    } else {
      // Default folder-like icon
      Fumbo::Graphic2D::DrawRectangleRounded(
          {cx, cy, iconSz, iconSz}, 0.15f, 4,
          {70, 100, 200, 200});
      Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(
          {cx, cy, iconSz, iconSz}, 0.15f, 4, 1.0f,
          {100, 130, 230, 255});
    }

    // Label (with shadow for readability)
    float labelX = cx + iconSz * 0.5f;
    float labelY = cy + iconSz + 4.0f;

    // Center the label
    Vector2 scale = Fumbo::Utils::GetUIScale();
    float fontSize = m_desktopStyle.iconFontSize * scale.y;
    Vector2 textSize = MeasureTextEx(m_font, icon.label.c_str(), fontSize, 1.0f);
    float textOffsetX = textSize.x / (2.0f * scale.x);

    // Text shadow
    Fumbo::Graphic2D::DrawText(icon.label, {labelX - textOffsetX + 1.0f, labelY + 1.0f},
                               m_font, m_desktopStyle.iconFontSize,
                               m_desktopStyle.iconTextShadowColor);
    // Text
    Fumbo::Graphic2D::DrawText(icon.label, {labelX - textOffsetX, labelY},
                               m_font, m_desktopStyle.iconFontSize,
                               m_desktopStyle.iconTextColor);
  }

  // 2. Windows
  WindowManager::Instance().Draw();

  // 3. Taskbar (draws on top of windows)
  m_taskbar.Draw();

  // 4. Notifications (overlay everything)
  NotificationManager::Instance().Draw();

  // 5. Context menu (on very top)
  m_desktopContextMenu.Draw();
}

// Desktop Configuration

void OSDesktop::SetWallpaper(Texture2D wallpaper) {
  if (m_ownsWallpaper && m_wallpaper.id != 0) {
    UnloadTexture(m_wallpaper);
  }
  m_wallpaper = wallpaper;
  m_ownsWallpaper = false;
  Fumbo::Engine::Instance().InvalidateCleanLayer();
}

void OSDesktop::SetWallpaperColor(Color color) {
  if (m_ownsWallpaper && m_wallpaper.id != 0) {
    UnloadTexture(m_wallpaper);
  }
  m_wallpaper = Fumbo::Utils::ColorToTexture(color, {64, 64});
  m_ownsWallpaper = true;
  Fumbo::Engine::Instance().InvalidateCleanLayer();
}

void OSDesktop::SetFont(Font font) {
  m_font = font;
  m_taskbar.SetFont(font);
  m_desktopContextMenu.SetFont(font);
  WindowManager::Instance().SetDefaultFont(font);
  NotificationManager::Instance().SetFont(font);
}

void OSDesktop::SetDesktopStyle(const DesktopStyle &style) {
  m_desktopStyle = style;
}

void OSDesktop::SetWindowStyle(const WindowStyle &style) {
  WindowManager::Instance().SetDefaultStyle(style);
}

void OSDesktop::SetTaskbarStyle(const TaskbarStyle &style) {
  m_taskbar.SetStyle(style);
}

void OSDesktop::SetNotificationStyle(const NotificationStyle &style) {
  NotificationManager::Instance().SetStyle(style);
}

// Desktop Icons

void OSDesktop::AddDesktopIcon(const std::string &label, Texture2D icon,
                               std::function<void()> onDoubleClick) {
  DesktopIcon di;
  di.label = label;
  di.icon = icon;
  di.onDoubleClick = std::move(onDoubleClick);
  m_icons.push_back(std::move(di));
  ArrangeIcons();
}

void OSDesktop::ClearDesktopIcons() {
  m_icons.clear();
  m_selectedIcon = -1;
}

void OSDesktop::ArrangeIcons() {
  float x = m_desktopStyle.iconMarginX;
  float y = m_desktopStyle.iconMarginY;
  float maxY = Fumbo::Utils::UI_HEIGHT - m_taskbar.GetHeight() - m_desktopStyle.iconSpacingY;

  for (auto &icon : m_icons) {
    icon.position = {x, y};
    y += m_desktopStyle.iconSpacingY;

    if (y > maxY) {
      y = m_desktopStyle.iconMarginY;
      x += m_desktopStyle.iconSpacingX;
    }
  }
}

// Convenience API

int OSDesktop::OpenWindow(const std::string &title, Rectangle bounds,
                          WindowContentCallback content, Texture2D icon) {
  return WindowManager::Instance().CreateWindow(title, bounds,
                                                std::move(content), icon);
}

void OSDesktop::Notify(const std::string &title, const std::string &message,
                       float duration) {
  NotificationManager::Instance().Push(title, message, duration);
}

WindowManager &OSDesktop::GetWindowManager() {
  return WindowManager::Instance();
}

Taskbar &OSDesktop::GetTaskbar() { return m_taskbar; }

NotificationManager &OSDesktop::GetNotificationManager() {
  return NotificationManager::Instance();
}

ContextMenu &OSDesktop::GetDesktopContextMenu() {
  return m_desktopContextMenu;
}

void OSDesktop::AddStartMenuItem(const std::string &label,
                                 std::function<void()> callback,
                                 Texture2D icon) {
  m_taskbar.AddStartMenuItem(label, std::move(callback), icon);
}

void OSDesktop::AddTrayItem(const std::string &id,
                            std::function<void(Rectangle area)> drawCallback,
                            std::function<void()> clickCallback, float width) {
  m_taskbar.AddTrayItem(id, std::move(drawCallback), std::move(clickCallback),
                        width);
}

// Desktop Click Handling

void OSDesktop::HandleDesktopClick() {
  Vector2 scale = Fumbo::Utils::GetUIScale();
  Vector2 offset = Fumbo::Utils::GetUIOffset();
  Vector2 mouse = GetMousePosition();
  Vector2 mouseUI = {(mouse.x - offset.x) / scale.x,
                     (mouse.y - offset.y) / scale.y};

  // Right-click → context menu
  if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
    m_desktopContextMenu.ShowAtMouse();
    return;
  }

  // Left-click on desktop
  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
    // Close start menu
    m_taskbar.CloseStartMenu();

    // Check if clicking on an icon
    int clickedIcon = -1;
    for (int i = 0; i < (int)m_icons.size(); i++) {
      Rectangle iconRect = {m_icons[i].position.x, m_icons[i].position.y,
                             m_desktopStyle.iconSize, m_desktopStyle.iconSize +
                             m_desktopStyle.iconFontSize + 8.0f};
      if (CheckCollisionPointRec(mouseUI, iconRect)) {
        clickedIcon = i;
        break;
      }
    }

    // Deselect all first
    for (auto &icon : m_icons) {
      icon.selected = false;
    }

    if (clickedIcon >= 0) {
      m_icons[clickedIcon].selected = true;

      // Double-click detection
      double now = GetTime();
      if (m_lastClickIcon == clickedIcon &&
          (now - m_lastClickTime) < DOUBLE_CLICK_TIME) {
        // Double-click!
        if (m_icons[clickedIcon].onDoubleClick)
          m_icons[clickedIcon].onDoubleClick();
        m_lastClickIcon = -1;
        m_lastClickTime = 0.0;
      } else {
        m_lastClickIcon = clickedIcon;
        m_lastClickTime = now;
      }

      m_selectedIcon = clickedIcon;
    } else {
      m_selectedIcon = -1;
      m_lastClickIcon = -1;
    }
  }
}

} // namespace OS
