#pragma once

#include "os_window.hpp"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

// WindowManager: Manages all OS windows, z-ordering, and focus

namespace OS {

class WindowManager {
public:
  static WindowManager &Instance() {
    static WindowManager instance;
    return instance;
  }
  // Create a window and return its ID. The window is immediately shown.
  int CreateWindow(const std::string &title, Rectangle bounds,
                   WindowContentCallback content = nullptr,
                   Texture2D icon = {0});

  // Create a window with a custom style
  int CreateWindow(const std::string &title, Rectangle bounds,
                   const WindowStyle &style,
                   WindowContentCallback content = nullptr,
                   Texture2D icon = {0});
  // Get a window by ID (nullptr if not found)
  OSWindow *GetWindow(int id);
  void SetWindowContent(int id, WindowContentCallback content);
  void SetWindowCloseCallback(int id, WindowCloseCallback callback);
  void CloseWindow(int id);
  void MinimizeWindow(int id);
  void MaximizeWindow(int id);
  void RestoreWindow(int id);
  void FocusWindow(int id);
  void ShowWindow(int id);
  void HideWindow(int id);
  bool IsWindowOpen(int id) const;
  int GetFocusedWindowId() const;
  std::vector<int> GetOpenWindowIds() const;
  // Set the default font for all newly created windows
  void SetDefaultFont(Font font);

  // Set the default style for all newly created windows
  void SetDefaultStyle(const WindowStyle &style);
  // Update all windows (input handling, z-ordering).
  // Returns true if any window consumed the mouse.
  bool Update();

  // Draw all windows (back to front by z-order).
  void Draw();

  // Remove closed windows from the list
  void Cleanup();

  // Remove all windows
  void Clear();

private:
  WindowManager() = default;
  ~WindowManager() = default;
  WindowManager(const WindowManager &) = delete;
  WindowManager &operator=(const WindowManager &) = delete;

  int m_nextId = 1;
  Font m_defaultFont = {0};
  WindowStyle m_defaultStyle;

  // Z-order: back of vector = top (drawn last, receives input first)
  std::vector<int> m_zOrder;

  // All windows by ID
  std::map<int, std::unique_ptr<OSWindow>> m_windows;

  // Bring a window to the top of z-order
  void BringToFront(int id);
};

} // namespace OS
