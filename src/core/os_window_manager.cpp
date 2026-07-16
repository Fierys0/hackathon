#include "os_window_manager.hpp"
#include <algorithm>

namespace OS {

// Window Creation

int WindowManager::CreateWindow(const std::string &title, Rectangle bounds,
                                WindowContentCallback content,
                                Texture2D icon) {
  return CreateWindow(title, bounds, m_defaultStyle, std::move(content), icon);
}

int WindowManager::CreateWindow(const std::string &title, Rectangle bounds,
                                const WindowStyle &style,
                                WindowContentCallback content,
                                Texture2D icon) {
  int id = m_nextId++;
  auto win = std::make_unique<OSWindow>();
  win->Create(title, bounds, icon);
  win->SetStyle(style);
  win->SetFont(m_defaultFont);
  win->id = id;

  if (content) {
    win->SetContent(std::move(content));
  }

  m_windows[id] = std::move(win);
  m_zOrder.push_back(id);
  FocusWindow(id);

  return id;
}

// Window Access

OSWindow *WindowManager::GetWindow(int id) {
  auto it = m_windows.find(id);
  if (it != m_windows.end()) {
    return it->second.get();
  }
  return nullptr;
}

// Window Control

void WindowManager::SetWindowContent(int id, WindowContentCallback content) {
  if (auto *w = GetWindow(id))
    w->SetContent(std::move(content));
}

void WindowManager::SetWindowCloseCallback(int id, WindowCloseCallback callback) {
  if (auto *w = GetWindow(id))
    w->SetCloseCallback(std::move(callback));
}

void WindowManager::CloseWindow(int id) {
  if (auto *w = GetWindow(id))
    w->Close();
}

void WindowManager::MinimizeWindow(int id) {
  if (auto *w = GetWindow(id))
    w->Minimize();
}

void WindowManager::MaximizeWindow(int id) {
  if (auto *w = GetWindow(id))
    w->Maximize();
}

void WindowManager::RestoreWindow(int id) {
  if (auto *w = GetWindow(id)) {
    w->Restore();
    BringToFront(id);
    FocusWindow(id);
  }
}

void WindowManager::FocusWindow(int id) {
  // Unfocus all
  for (auto &pair : m_windows) {
    pair.second->Unfocus();
  }
  // Focus the target
  if (auto *w = GetWindow(id)) {
    w->Focus();
    BringToFront(id);
  }
}

void WindowManager::ShowWindow(int id) {
  if (auto *w = GetWindow(id)) {
    w->Show();
    BringToFront(id);
    FocusWindow(id);
  }
}

void WindowManager::HideWindow(int id) {
  if (auto *w = GetWindow(id))
    w->Hide();
}

// Window Queries

bool WindowManager::IsWindowOpen(int id) const {
  auto it = m_windows.find(id);
  if (it != m_windows.end()) {
    return !it->second->IsClosed();
  }
  return false;
}

int WindowManager::GetFocusedWindowId() const {
  for (auto &pair : m_windows) {
    if (pair.second->IsFocused() && pair.second->IsVisible()) {
      return pair.first;
    }
  }
  return -1;
}

std::vector<int> WindowManager::GetOpenWindowIds() const {
  std::vector<int> ids;
  for (auto &pair : m_windows) {
    if (!pair.second->IsClosed()) {
      ids.push_back(pair.first);
    }
  }
  return ids;
}

// Global Settings

void WindowManager::SetDefaultFont(Font font) { m_defaultFont = font; }

void WindowManager::SetDefaultStyle(const WindowStyle &style) {
  m_defaultStyle = style;
}

// Z-Order

void WindowManager::BringToFront(int id) {
  auto it = std::find(m_zOrder.begin(), m_zOrder.end(), id);
  if (it != m_zOrder.end()) {
    m_zOrder.erase(it);
  }
  m_zOrder.push_back(id);
}

// Core Loop

bool WindowManager::Update() {
  bool consumed = false;

  // Process from top (front) to back: top window gets input first
  for (int i = (int)m_zOrder.size() - 1; i >= 0; i--) {
    int id = m_zOrder[i];
    auto *w = GetWindow(id);
    if (!w || !w->IsVisible())
      continue;

    if (!consumed) {
      bool windowConsumed = w->Update();
      if (windowConsumed) {
        consumed = true;
        // If mouse was pressed on this window, focus it
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
          FocusWindow(id);
        }
      }
    }
  }

  return consumed;
}

void WindowManager::Draw() {
  // Draw from back (bottom of stack) to front (top of stack)
  for (int id : m_zOrder) {
    auto *w = GetWindow(id);
    if (w && w->IsVisible()) {
      w->Draw();
    }
  }
}

void WindowManager::Cleanup() {
  // Remove closed windows
  std::vector<int> toRemove;
  for (auto &pair : m_windows) {
    if (pair.second->IsClosed()) {
      toRemove.push_back(pair.first);
    }
  }
  for (int id : toRemove) {
    m_windows.erase(id);
    auto it = std::find(m_zOrder.begin(), m_zOrder.end(), id);
    if (it != m_zOrder.end())
      m_zOrder.erase(it);
  }
}

void WindowManager::Clear() {
  m_windows.clear();
  m_zOrder.clear();
}

} // namespace OS
