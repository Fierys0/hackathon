#include "os_window.hpp"
#include <algorithm>
#include <cmath>

namespace OS {

// Setup

void OSWindow::Create(const std::string &title, Rectangle bounds,
                      Texture2D icon) {
  m_title = title;
  m_bounds = bounds;
  m_priorBounds = bounds;
  m_icon = icon;
  m_state = WindowState::NORMAL;
  m_visible = true;
  m_focused = false;
}

void OSWindow::SetContent(WindowContentCallback callback) {
  m_contentCallback = std::move(callback);
}

void OSWindow::SetCloseCallback(WindowCloseCallback callback) {
  m_closeCallback = std::move(callback);
}

void OSWindow::SetStyle(const WindowStyle &style) { m_style = style; }

void OSWindow::SetFont(Font font) { m_font = font; }

// State Management

void OSWindow::Show() {
  if (m_state == WindowState::CLOSED) {
    m_state = WindowState::NORMAL;
  }
  m_visible = true;
}

void OSWindow::Hide() { m_visible = false; }

void OSWindow::Minimize() {
  if (m_state == WindowState::NORMAL || m_state == WindowState::MAXIMIZED) {
    m_state = WindowState::MINIMIZED;
    m_visible = false;
  }
}

void OSWindow::Maximize() {
  if (m_state == WindowState::NORMAL) {
    m_priorBounds = m_bounds;
    // Fullscreen within UI space (leave room for taskbar at bottom)
    m_bounds = {0, 0, Fumbo::Utils::UI_WIDTH, Fumbo::Utils::UI_HEIGHT - 40.0f};
    m_state = WindowState::MAXIMIZED;
  } else if (m_state == WindowState::MAXIMIZED) {
    Restore();
  }
}

void OSWindow::Restore() {
  if (m_state == WindowState::MINIMIZED) {
    m_state = WindowState::NORMAL;
    m_visible = true;
  } else if (m_state == WindowState::MAXIMIZED) {
    m_bounds = m_priorBounds;
    m_state = WindowState::NORMAL;
  }
}

void OSWindow::Close() {
  m_state = WindowState::CLOSED;
  m_visible = false;
  if (m_closeCallback)
    m_closeCallback();
}

void OSWindow::Focus() { m_focused = true; }
void OSWindow::Unfocus() { m_focused = false; }

void OSWindow::SetTitle(const std::string &title) { m_title = title; }

void OSWindow::SetBounds(Rectangle bounds) { m_bounds = bounds; }

void OSWindow::SetPosition(float x, float y) {
  m_bounds.x = x;
  m_bounds.y = y;
}

void OSWindow::SetSize(float w, float h) {
  m_bounds.width = std::max(w, m_style.minWidth);
  m_bounds.height = std::max(h, m_style.minHeight);
}

// Queries

bool OSWindow::IsVisible() const { return m_visible && m_state != WindowState::CLOSED; }
bool OSWindow::IsFocused() const { return m_focused; }
bool OSWindow::IsMinimized() const { return m_state == WindowState::MINIMIZED; }
bool OSWindow::IsMaximized() const { return m_state == WindowState::MAXIMIZED; }
bool OSWindow::IsClosed() const { return m_state == WindowState::CLOSED; }

bool OSWindow::IsHovered() const {
  Vector2 mouseUI = ScreenToUI(GetMousePosition());
  return CheckCollisionPointRec(mouseUI, m_bounds);
}

Rectangle OSWindow::GetBounds() const { return m_bounds; }

Rectangle OSWindow::GetContentArea() const {
  return {m_bounds.x, m_bounds.y + m_style.titleBarHeight,
          m_bounds.width,
          m_bounds.height - m_style.titleBarHeight};
}

const std::string &OSWindow::GetTitle() const { return m_title; }

WindowState OSWindow::GetState() const { return m_state; }

// Helpers

Vector2 OSWindow::ScreenToUI(Vector2 screenPos) const {
  Vector2 scale = Fumbo::Utils::GetUIScale();
  Vector2 offset = Fumbo::Utils::GetUIOffset();
  return {(screenPos.x - offset.x) / scale.x,
          (screenPos.y - offset.y) / scale.y};
}

Rectangle OSWindow::GetTitleBarRect() const {
  return {m_bounds.x, m_bounds.y, m_bounds.width, m_style.titleBarHeight};
}

Rectangle OSWindow::GetCloseButtonRect() const {
  float sz = m_style.buttonSize;
  float margin = m_style.buttonMarginRight;
  float cx = m_bounds.x + m_bounds.width - margin - sz;
  float cy = m_bounds.y + (m_style.titleBarHeight - sz) * 0.5f;
  return {cx, cy, sz, sz};
}

Rectangle OSWindow::GetMaxButtonRect() const {
  float sz = m_style.buttonSize;
  float margin = m_style.buttonMarginRight;
  float spacing = m_style.buttonSpacing;
  float cx = m_bounds.x + m_bounds.width - margin - sz * 2 - spacing;
  float cy = m_bounds.y + (m_style.titleBarHeight - sz) * 0.5f;
  return {cx, cy, sz, sz};
}

Rectangle OSWindow::GetMinButtonRect() const {
  float sz = m_style.buttonSize;
  float margin = m_style.buttonMarginRight;
  float spacing = m_style.buttonSpacing;
  float cx = m_bounds.x + m_bounds.width - margin - sz * 3 - spacing * 2;
  float cy = m_bounds.y + (m_style.titleBarHeight - sz) * 0.5f;
  return {cx, cy, sz, sz};
}

int OSWindow::HitTestEdges(Vector2 mouseUI) const {
  if (!m_style.resizable || m_state == WindowState::MAXIMIZED)
    return 0;

  const float edgeSize = 6.0f;
  int edge = 0;

  if (mouseUI.x >= m_bounds.x - edgeSize &&
      mouseUI.x <= m_bounds.x + edgeSize &&
      mouseUI.y >= m_bounds.y && mouseUI.y <= m_bounds.y + m_bounds.height)
    edge |= 1; // left

  if (mouseUI.x >= m_bounds.x + m_bounds.width - edgeSize &&
      mouseUI.x <= m_bounds.x + m_bounds.width + edgeSize &&
      mouseUI.y >= m_bounds.y && mouseUI.y <= m_bounds.y + m_bounds.height)
    edge |= 2; // right

  if (mouseUI.y >= m_bounds.y - edgeSize &&
      mouseUI.y <= m_bounds.y + edgeSize &&
      mouseUI.x >= m_bounds.x && mouseUI.x <= m_bounds.x + m_bounds.width)
    edge |= 4; // top

  if (mouseUI.y >= m_bounds.y + m_bounds.height - edgeSize &&
      mouseUI.y <= m_bounds.y + m_bounds.height + edgeSize &&
      mouseUI.x >= m_bounds.x && mouseUI.x <= m_bounds.x + m_bounds.width)
    edge |= 8; // bottom

  return edge;
}

// Update

bool OSWindow::Update() {
  if (!IsVisible())
    return false;

  Vector2 mouseUI = ScreenToUI(GetMousePosition());
  bool consumed = false;

  if (m_resizing) {
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
      float dx = mouseUI.x - m_resizeStart.x;
      float dy = mouseUI.y - m_resizeStart.y;

      Rectangle newBounds = m_resizeStartBounds;

      if (m_resizeEdge & 2) { // right
        newBounds.width = std::max(m_resizeStartBounds.width + dx, m_style.minWidth);
      }
      if (m_resizeEdge & 8) { // bottom
        newBounds.height = std::max(m_resizeStartBounds.height + dy, m_style.minHeight);
      }
      if (m_resizeEdge & 1) { // left
        float newW = m_resizeStartBounds.width - dx;
        if (newW >= m_style.minWidth) {
          newBounds.x = m_resizeStartBounds.x + dx;
          newBounds.width = newW;
        }
      }
      if (m_resizeEdge & 4) { // top
        float newH = m_resizeStartBounds.height - dy;
        if (newH >= m_style.minHeight) {
          newBounds.y = m_resizeStartBounds.y + dy;
          newBounds.height = newH;
        }
      }

      m_bounds = newBounds;
      consumed = true;
    } else {
      m_resizing = false;
    }
    return consumed;
  }

  if (m_dragging) {
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
      m_bounds.x = mouseUI.x - m_dragOffset.x;
      m_bounds.y = mouseUI.y - m_dragOffset.y;

      // Clamp so title bar stays on screen
      m_bounds.x = std::max(m_bounds.x, -m_bounds.width + 50.0f);
      m_bounds.y = std::max(m_bounds.y, 0.0f);
      m_bounds.x = std::min(m_bounds.x, Fumbo::Utils::UI_WIDTH - 50.0f);

      consumed = true;
    } else {
      m_dragging = false;
    }
    return consumed;
  }

  bool overWindow = CheckCollisionPointRec(mouseUI, m_bounds);
  int edges = HitTestEdges(mouseUI);

  Rectangle closeR = GetCloseButtonRect();
  Rectangle maxR = GetMaxButtonRect();
  Rectangle minR = GetMinButtonRect();

  m_closeHover = m_style.closable && CheckCollisionPointRec(mouseUI, closeR);
  m_maxHover = m_style.maximizable && CheckCollisionPointRec(mouseUI, maxR);
  m_minHover = m_style.minimizable && CheckCollisionPointRec(mouseUI, minR);

  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
    // Check title bar buttons first
    if (m_closeHover) {
      Close();
      return true;
    }
    if (m_maxHover) {
      Maximize();
      return true;
    }
    if (m_minHover) {
      Minimize();
      return true;
    }

    // Check resize edges
    if (edges && m_style.resizable) {
      m_resizing = true;
      m_resizeEdge = edges;
      m_resizeStart = mouseUI;
      m_resizeStartBounds = m_bounds;
      return true;
    }

    // Check title bar drag
    Rectangle titleBar = GetTitleBarRect();
    if (m_style.draggable && CheckCollisionPointRec(mouseUI, titleBar)) {
      m_dragging = true;
      m_dragOffset = {mouseUI.x - m_bounds.x, mouseUI.y - m_bounds.y};

      // Double-click on title bar to maximize/restore
      // (not implemented yet: can be added later)

      return true;
    }

    // Clicked inside window body
    if (overWindow) {
      consumed = true;
    }
  }

  // Mouse is over the window: consume to prevent pass-through
  if (overWindow || edges) {
    consumed = true;
  }

  return consumed;
}

// Draw

void OSWindow::Draw() {
  if (!IsVisible())
    return;

  if (m_style.enableShadow) {
    Rectangle shadowRect = {m_bounds.x + m_style.shadowOffset.x,
                            m_bounds.y + m_style.shadowOffset.y,
                            m_bounds.width, m_bounds.height};
    Fumbo::Graphic2D::DrawRectangleRounded(shadowRect, m_style.cornerRoundness, 6,
                                           m_style.shadowColor);
  }

  Fumbo::Graphic2D::DrawRectangleRounded(m_bounds, m_style.cornerRoundness, 6,
                                         m_style.bodyColor);

  Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(
      m_bounds, m_style.cornerRoundness, 6, m_style.borderThickness,
      m_focused ? m_style.titleBarFocusedColor : m_style.borderColor);

  Rectangle titleBar = GetTitleBarRect();
  Color tbColor =
      m_focused ? m_style.titleBarFocusedColor : m_style.titleBarColor;

  // Draw title bar with rounded top corners only (approximate with full rect +
  // clip)
  Fumbo::Graphic2D::DrawRectangleRounded(titleBar, m_style.cornerRoundness * 2, 6,
                                         tbColor);
  // Bottom part of title bar (straight edge)
  Rectangle titleBottom = {titleBar.x, titleBar.y + titleBar.height * 0.5f,
                           titleBar.width, titleBar.height * 0.5f};
  Fumbo::Graphic2D::DrawRectangleRec(titleBottom, tbColor);

  Fumbo::Graphic2D::DrawLineEx(
      {titleBar.x, titleBar.y + titleBar.height},
      {titleBar.x + titleBar.width, titleBar.y + titleBar.height}, 1.0f,
      m_style.borderColor);

  float textStartX = m_bounds.x + 10.0f;
  if (m_icon.id != 0) {
    float iconSize = m_style.titleBarHeight - 8.0f;
    Fumbo::Graphic2D::DrawTexture(
        m_icon, {m_bounds.x + 6.0f, m_bounds.y + 4.0f},
        {iconSize, iconSize});
    textStartX = m_bounds.x + 6.0f + iconSize + 4.0f;
  }

  float titleY = m_bounds.y + (m_style.titleBarHeight - m_style.titleFontSize) * 0.5f;
  Fumbo::Graphic2D::DrawText(m_title, {textStartX, titleY}, m_font,
                             m_style.titleFontSize, m_style.titleTextColor);

  // Close button (red circle)
  if (m_style.closable) {
    Rectangle cr = GetCloseButtonRect();
    Color cc = m_closeHover ? m_style.closeButtonHoverColor
                            : m_style.closeButtonColor;
    float radius = cr.width * 0.5f;
    Fumbo::Graphic2D::DrawCircleV(
        {cr.x + radius, cr.y + radius}, radius, cc);
  }

  // Maximize button (green circle)
  if (m_style.maximizable) {
    Rectangle mr = GetMaxButtonRect();
    Color mc =
        m_maxHover ? m_style.maximizeButtonHoverColor : m_style.maximizeButtonColor;
    float radius = mr.width * 0.5f;
    Fumbo::Graphic2D::DrawCircleV(
        {mr.x + radius, mr.y + radius}, radius, mc);
  }

  // Minimize button (yellow circle)
  if (m_style.minimizable) {
    Rectangle minR = GetMinButtonRect();
    Color minC = m_minHover ? m_style.minimizeButtonHoverColor
                            : m_style.minimizeButtonColor;
    float radius = minR.width * 0.5f;
    Fumbo::Graphic2D::DrawCircleV(
        {minR.x + radius, minR.y + radius}, radius, minC);
  }

  if (m_contentCallback) {
    Rectangle contentArea = GetContentArea();
    // Begin scissor to clip content to window area
    Rectangle screenContent = Fumbo::Utils::UISpaceToScreen(contentArea);
    Vector2 uiOffset = Fumbo::Utils::GetUIOffset();
    BeginScissorMode((int)(screenContent.x + uiOffset.x),
                     (int)(screenContent.y + uiOffset.y),
                     (int)screenContent.width, (int)screenContent.height);
    m_contentCallback(contentArea);
    EndScissorMode();
  }
}

} // namespace OS
