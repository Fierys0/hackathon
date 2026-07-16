#pragma once

#include "os_window_manager.hpp"
#include <functional>
#include <string>
#include <vector>

// Taskbar: Bottom bar with start button, open window list, and system tray

namespace OS {

// System tray item
struct TrayItem {
  std::string id;
  std::function<void(Rectangle area)> drawCallback;
  std::function<void()> clickCallback;
  float width = 30.0f; // Width allocated in the tray
};

// Start menu item
struct StartMenuItem {
  std::string label;
  Texture2D icon = {0};
  std::function<void()> callback;
};

// Taskbar style configuration
struct TaskbarStyle {
  Color backgroundColor = {20, 20, 35, 240};
  Color borderColor = {60, 60, 100, 200};
  Color itemColor = {40, 40, 60, 220};
  Color itemHoverColor = {60, 60, 100, 255};
  Color itemActiveColor = {70, 70, 140, 255};
  Color textColor = {220, 220, 240, 255};
  Color startButtonColor = {50, 80, 180, 255};
  Color startButtonHoverColor = {70, 100, 220, 255};

  // Start menu
  Color startMenuBg = {25, 25, 40, 245};
  Color startMenuItemHover = {50, 50, 90, 255};
  Color startMenuBorder = {60, 60, 100, 200};

  float height = 36.0f;
  float startButtonWidth = 80.0f;
  float windowItemWidth = 130.0f;
  float windowItemHeight = 28.0f;
  float itemSpacing = 4.0f;
  int fontSize = 12;
  int startMenuFontSize = 13;
  float startMenuItemHeight = 32.0f;
  float startMenuWidth = 200.0f;
};

class Taskbar {
public:
  Taskbar() = default;
  ~Taskbar() = default;
  void SetFont(Font font);
  void SetStyle(const TaskbarStyle &style);
  void AddStartMenuItem(const std::string &label, std::function<void()> callback,
                        Texture2D icon = {0});
  void ClearStartMenu();
  void AddTrayItem(const std::string &id,
                   std::function<void(Rectangle area)> drawCallback,
                   std::function<void()> clickCallback = nullptr,
                   float width = 30.0f);
  void RemoveTrayItem(const std::string &id);
  float GetHeight() const;
  Rectangle GetBounds() const;
  bool IsStartMenuOpen() const;
  void CloseStartMenu();
  // Returns true if the taskbar consumed the mouse
  bool Update();
  void Draw();

private:
  TaskbarStyle m_style;
  Font m_font = {0};

  // Start menu
  bool m_startMenuOpen = false;
  bool m_startHover = false;
  std::vector<StartMenuItem> m_startMenuItems;
  int m_startMenuHoverIndex = -1;

  // System tray
  std::vector<TrayItem> m_trayItems;

  // Window items hover
  int m_windowItemHoverId = -1;

  // Helpers
  Rectangle GetStartButtonRect() const;
  Rectangle GetWindowListArea() const;
  Rectangle GetTrayArea() const;
  Rectangle GetStartMenuRect() const;
};

} // namespace OS
