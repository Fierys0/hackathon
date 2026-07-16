#pragma once

#include "fumbo.hpp"
#include <functional>
#include <string>

// OSWindow: A draggable, resizable, styleable window for OS simulation

namespace OS {

// Window state flags
enum class WindowState { NORMAL, MINIMIZED, MAXIMIZED, CLOSED };

// Window style configuration
struct WindowStyle {
  // Title bar
  Color titleBarColor = {45, 45, 60, 255};
  Color titleBarFocusedColor = {60, 60, 120, 255};
  Color titleTextColor = WHITE;
  int titleFontSize = 14;

  // Body
  Color bodyColor = {30, 30, 42, 240};
  Color borderColor = {80, 80, 120, 200};
  float borderThickness = 1.5f;
  float cornerRoundness = 0.05f;

  // Title bar buttons
  Color closeButtonColor = {220, 60, 60, 255};
  Color closeButtonHoverColor = {255, 80, 80, 255};
  Color maximizeButtonColor = {60, 180, 60, 255};
  Color maximizeButtonHoverColor = {80, 220, 80, 255};
  Color minimizeButtonColor = {220, 180, 40, 255};
  Color minimizeButtonHoverColor = {255, 210, 60, 255};

  // Dimensions
  float titleBarHeight = 30.0f;
  float buttonSize = 12.0f;
  float buttonSpacing = 8.0f;
  float buttonMarginRight = 10.0f;

  // Shadow
  bool enableShadow = true;
  Color shadowColor = {0, 0, 0, 80};
  Vector2 shadowOffset = {4.0f, 4.0f};

  // Constraints
  float minWidth = 150.0f;
  float minHeight = 100.0f;

  // Behavior
  bool resizable = true;
  bool closable = true;
  bool minimizable = true;
  bool maximizable = true;
  bool draggable = true;
};

// Content callback: receives the content area rectangle (in UI space) and draws
// whatever the user wants inside the window.
using WindowContentCallback = std::function<void(Rectangle contentArea)>;

// Close callback: called when the window is closed.
using WindowCloseCallback = std::function<void()>;

class OSWindow {
public:
  OSWindow() = default;
  ~OSWindow() = default;

  // Move-only (buttons contain Fumbo::UI::Button which is move-only)
  OSWindow(const OSWindow &) = delete;
  OSWindow &operator=(const OSWindow &) = delete;
  OSWindow(OSWindow &&) = default;
  OSWindow &operator=(OSWindow &&) = default;
  // Create a window with title, bounds in UI space, and optional icon
  void Create(const std::string &title, Rectangle bounds,
              Texture2D icon = {0});

  // Set the content drawing callback
  void SetContent(WindowContentCallback callback);

  // Set close callback
  void SetCloseCallback(WindowCloseCallback callback);

  // Apply a custom style
  void SetStyle(const WindowStyle &style);

  // Set the font used for the title bar (must be called before Draw)
  void SetFont(Font font);
  void Show();
  void Hide();
  void Minimize();
  void Maximize();
  void Restore();          // Restore from minimized or maximized
  void Close();
  void Focus();
  void Unfocus();

  void SetTitle(const std::string &title);
  void SetBounds(Rectangle bounds);
  void SetPosition(float x, float y);
  void SetSize(float w, float h);
  bool IsVisible() const;
  bool IsFocused() const;
  bool IsMinimized() const;
  bool IsMaximized() const;
  bool IsClosed() const;
  bool IsHovered() const;
  Rectangle GetBounds() const;
  Rectangle GetContentArea() const;
  const std::string &GetTitle() const;
  WindowState GetState() const;
  // Returns true if this window consumed the mouse input (prevents pass-through)
  bool Update();
  void Draw();

  // Internal ID (set by WindowManager)
  int id = -1;

private:
  std::string m_title;
  Texture2D m_icon = {0};
  Rectangle m_bounds = {100, 50, 400, 300};       // Current bounds in UI space
  Rectangle m_priorBounds = {100, 50, 400, 300};   // Bounds before maximize
  WindowState m_state = WindowState::CLOSED;
  WindowStyle m_style;
  Font m_font = {0};
  bool m_focused = false;
  bool m_visible = false;

  // Content
  WindowContentCallback m_contentCallback;
  WindowCloseCallback m_closeCallback;

  // Drag state
  bool m_dragging = false;
  Vector2 m_dragOffset = {0, 0};

  // Resize state
  bool m_resizing = false;
  int m_resizeEdge = 0; // bitmask: 1=left, 2=right, 4=top, 8=bottom
  Vector2 m_resizeStart = {0, 0};
  Rectangle m_resizeStartBounds = {0, 0, 0, 0};

  // Title bar button hover states
  bool m_closeHover = false;
  bool m_maxHover = false;
  bool m_minHover = false;

  // Helpers
  Rectangle GetTitleBarRect() const;
  Rectangle GetCloseButtonRect() const;
  Rectangle GetMaxButtonRect() const;
  Rectangle GetMinButtonRect() const;
  int HitTestEdges(Vector2 mouseUI) const;
  Vector2 ScreenToUI(Vector2 screenPos) const;
};

} // namespace OS
