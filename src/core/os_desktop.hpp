#pragma once

#include "os_context_menu.hpp"
#include "os_notification.hpp"
#include "os_taskbar.hpp"
#include "os_window_manager.hpp"

// OSDesktop: IGameState that combines all OS simulation components

namespace OS {

// Desktop icon
struct DesktopIcon {
  std::string label;
  Texture2D icon = {0};
  Texture2D bgIcon = {0};
  std::function<void()> onDoubleClick;
  Vector2 position = {0, 0};
  bool selected = false;
};

// Desktop style
struct DesktopStyle {
  Color backgroundColor = {0, 128, 128, 255}; // Classic Windows 95/98 Teal
  Color iconTextColor = WHITE;
  Color iconTextShadowColor = BLACK;
  Color iconSelectedColor = {0, 0, 128, 100}; // Windows 95/98 dark blue selection outline/tint
  float iconSize = 48.0f;
  float iconSpacingX = 90.0f;
  float iconSpacingY = 90.0f;
  float iconMarginX = 30.0f;
  float iconMarginY = 45.0f; // Shift down to avoid being overdrawn by top bar (height 30.0f)
  int iconFontSize = 11;
  float iconLabelMaxWidth = 80.0f;
};

class OSDesktop : public IGameState {
public:
  OSDesktop() = default;
  ~OSDesktop() override = default;

  // IGameState interface
  void Init() override;
  void Cleanup() override;
  void Update() override;
  void DrawClean() override;
  void DrawDirty() override;
  // Set the wallpaper texture (drawn in clean layer)
  void SetWallpaper(Texture2D wallpaper);

  // Set wallpaper from solid color
  void SetWallpaperColor(Color color);

  // Set the font used across all OS components
  void SetFont(Font font);

  // Set desktop style
  void SetDesktopStyle(const DesktopStyle &style);

  // Set window style for all windows
  void SetWindowStyle(const WindowStyle &style);

  // Set taskbar style
  void SetTaskbarStyle(const TaskbarStyle &style);

  // Set notification style
  void SetNotificationStyle(const NotificationStyle &style);
  void AddDesktopIcon(const std::string &label, Texture2D icon,
                      std::function<void()> onDoubleClick);
  void AddDesktopIcon(const std::string &label, Texture2D icon, Texture2D bgIcon,
                      std::function<void()> onDoubleClick);

  void ClearDesktopIcons();
  // Open a new window with content callback and return its ID
  int OpenWindow(const std::string &title, Rectangle bounds,
                 WindowContentCallback content, Texture2D icon = {0});

  // Push a notification
  void Notify(const std::string &title, const std::string &message,
              float duration = 4.0f);

  // Access components directly for advanced use
  WindowManager &GetWindowManager();
  Taskbar &GetTaskbar();
  NotificationManager &GetNotificationManager();
  ContextMenu &GetDesktopContextMenu();
  void AddStartMenuItem(const std::string &label,
                        std::function<void()> callback,
                        Texture2D icon = {0});
  void AddTrayItem(const std::string &id,
                   std::function<void(Rectangle area)> drawCallback,
                   std::function<void()> clickCallback = nullptr,
                   float width = 30.0f);

private:
  Font m_font = {0};
  Texture2D m_wallpaper = {0};
  bool m_ownsWallpaper = false; // If we created the wallpaper from color

  DesktopStyle m_desktopStyle;
  Taskbar m_taskbar;
  ContextMenu m_desktopContextMenu;

  std::vector<DesktopIcon> m_icons;
  int m_selectedIcon = -1;

  // Double-click tracking
  double m_lastClickTime = 0.0;
  int m_lastClickIcon = -1;
  const double DOUBLE_CLICK_TIME = 0.35; // seconds

  void HandleDesktopClick();
  void ArrangeIcons();
};

} // namespace OS
