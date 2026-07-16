#include "os_notification.hpp"
#include <algorithm>
#include <cmath>

namespace OS {

// Push Notifications

void NotificationManager::Push(const std::string &title,
                               const std::string &message, float duration) {
  Push(title, message, duration, nullptr, {0});
}

void NotificationManager::Push(const std::string &title,
                               const std::string &message, float duration,
                               Texture2D icon) {
  Push(title, message, duration, nullptr, icon);
}

void NotificationManager::Push(const std::string &title,
                               const std::string &message, float duration,
                               std::function<void()> onClick,
                               Texture2D icon) {
  Notification notif;
  notif.title = title;
  notif.message = message;
  notif.duration = duration;
  notif.icon = icon;
  notif.onClick = std::move(onClick);
  notif.phase = Notification::Phase::SLIDE_IN;
  notif.animTimer = 0.0f;
  notif.elapsed = 0.0f;
  notif.height = CalcNotificationHeight(notif);

  m_notifications.push_back(std::move(notif));

  // Trim excess
  while ((int)m_notifications.size() > m_maxVisible * 2) {
    m_notifications.erase(m_notifications.begin());
  }
}

// Configuration

void NotificationManager::SetFont(Font font) { m_font = font; }

void NotificationManager::SetStyle(const NotificationStyle &style) {
  m_style = style;
}

void NotificationManager::SetMaxVisible(int max) { m_maxVisible = max; }

void NotificationManager::ClearAll() { m_notifications.clear(); }

// Helpers

float NotificationManager::CalcNotificationHeight(
    const Notification &notif) const {
  float h = m_style.padding * 2 + m_style.titleFontSize + 4.0f;

  if (!notif.message.empty()) {
    // Estimate lines based on message length and width
    Vector2 scale = Fumbo::Utils::GetUIScale();
    float availWidth = m_style.width - m_style.padding * 2;
    if (notif.icon.id != 0) {
      availWidth -= 36.0f; // Icon space
    }

    // Simple approximation: each char is about fontSize * 0.55 wide
    float charWidth = m_style.messageFontSize * 0.55f;
    int charsPerLine = std::max(1, (int)(availWidth / charWidth));
    int lines = ((int)notif.message.length() + charsPerLine - 1) / charsPerLine;
    lines = std::min(lines, 4); // Max 4 lines

    h += lines * (m_style.messageFontSize + 2.0f);
  }

  return std::max(h, m_style.minHeight);
}

// Update

bool NotificationManager::Update() {
  float dt = GetFrameTime();
  bool consumed = false;

  Vector2 scale = Fumbo::Utils::GetUIScale();
  Vector2 offset = Fumbo::Utils::GetUIOffset();
  Vector2 mouse = GetMousePosition();
  Vector2 mouseUI = {(mouse.x - offset.x) / scale.x,
                     (mouse.y - offset.y) / scale.y};

  // Update from newest to oldest
  for (int i = (int)m_notifications.size() - 1; i >= 0; i--) {
    auto &notif = m_notifications[i];

    switch (notif.phase) {
    case Notification::Phase::SLIDE_IN:
      notif.animTimer += dt;
      if (notif.animTimer >= m_style.animationDuration) {
        notif.animTimer = m_style.animationDuration;
        notif.phase = Notification::Phase::VISIBLE;
      }
      break;

    case Notification::Phase::VISIBLE:
      notif.elapsed += dt;
      if (notif.elapsed >= notif.duration) {
        notif.phase = Notification::Phase::SLIDE_OUT;
        notif.animTimer = 0.0f;
      }
      break;

    case Notification::Phase::SLIDE_OUT:
      notif.animTimer += dt;
      if (notif.animTimer >= m_style.animationDuration) {
        notif.phase = Notification::Phase::DONE;
      }
      break;

    case Notification::Phase::DONE:
      break;
    }

    // Check click
    if (notif.phase == Notification::Phase::VISIBLE ||
        notif.phase == Notification::Phase::SLIDE_IN) {
      // Calculate position for hit-test (see Draw for matching logic)
      float y = m_style.marginTop;
      for (int j = (int)m_notifications.size() - 1; j > i; j--) {
        if (m_notifications[j].phase != Notification::Phase::DONE) {
          y += m_notifications[j].height + m_style.spacing;
        }
      }

      float x = Fumbo::Utils::UI_WIDTH - m_style.width - m_style.marginRight;
      Rectangle notifRect = {x, y, m_style.width, notif.height};

      if (CheckCollisionPointRec(mouseUI, notifRect)) {
        consumed = true;
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
          if (notif.onClick)
            notif.onClick();
          notif.phase = Notification::Phase::SLIDE_OUT;
          notif.animTimer = 0.0f;
        }
      }
    }
  }

  // Remove done notifications
  m_notifications.erase(
      std::remove_if(m_notifications.begin(), m_notifications.end(),
                     [](const Notification &n) {
                       return n.phase == Notification::Phase::DONE;
                     }),
      m_notifications.end());

  return consumed;
}

// Draw

void NotificationManager::Draw() {
  if (m_notifications.empty())
    return;

  float y = m_style.marginTop;
  int drawn = 0;

  // Draw from newest (back) to oldest: newest at top
  for (int i = (int)m_notifications.size() - 1; i >= 0 && drawn < m_maxVisible;
       i--) {
    auto &notif = m_notifications[i];
    if (notif.phase == Notification::Phase::DONE)
      continue;

    float baseX = Fumbo::Utils::UI_WIDTH - m_style.width - m_style.marginRight;
    float x = baseX;

    // Animation offset
    float animProgress = 1.0f;
    if (notif.phase == Notification::Phase::SLIDE_IN) {
      animProgress = notif.animTimer / m_style.animationDuration;
      // Ease out cubic
      float t = 1.0f - animProgress;
      animProgress = 1.0f - t * t * t;
      x = baseX + m_style.width * (1.0f - animProgress);
    } else if (notif.phase == Notification::Phase::SLIDE_OUT) {
      animProgress = notif.animTimer / m_style.animationDuration;
      // Ease in cubic
      float t = animProgress;
      animProgress = t * t * t;
      x = baseX + m_style.width * animProgress;
    }

    float alpha = (notif.phase == Notification::Phase::SLIDE_OUT)
                      ? (1.0f - animProgress)
                      : std::min(1.0f, animProgress * 2.0f);
    unsigned char a = (unsigned char)(alpha * 240.0f);

    Rectangle notifRect = {x, y, m_style.width, notif.height};

    // Shadow
    Rectangle shadowRect = {notifRect.x + 2.0f, notifRect.y + 2.0f,
                            notifRect.width, notifRect.height};
    Fumbo::Graphic2D::DrawRectangleRounded(
        shadowRect, m_style.cornerRoundness, 4,
        {0, 0, 0, (unsigned char)(a * 0.3f)});

    // Background
    Color bg = m_style.backgroundColor;
    bg.a = a;
    Fumbo::Graphic2D::DrawRectangleRounded(notifRect, m_style.cornerRoundness,
                                           4, bg);

    // Border
    Color border = m_style.borderColor;
    border.a = a;
    Fumbo::Graphic2D::DrawRectangleRoundedLinesEx(
        notifRect, m_style.cornerRoundness, 4, m_style.borderThickness, border);

    // Progress bar at bottom
    if (notif.phase == Notification::Phase::VISIBLE && notif.duration > 0) {
      float progress = 1.0f - (notif.elapsed / notif.duration);
      Rectangle progressRect = {
          notifRect.x + 2.0f,
          notifRect.y + notifRect.height - 3.0f,
          (notifRect.width - 4.0f) * progress, 2.0f};
      Color pColor = m_style.progressColor;
      pColor.a = a;
      Fumbo::Graphic2D::DrawRectangleRec(progressRect, pColor);
    }

    // Content
    float contentX = notifRect.x + m_style.padding;
    float contentY = notifRect.y + m_style.padding;

    // Icon
    if (notif.icon.id != 0) {
      float iconSize = 28.0f;
      Fumbo::Graphic2D::DrawTexture(notif.icon, {contentX, contentY + 2.0f},
                                    {iconSize, iconSize}, 0.0f,
                                    {255, 255, 255, a});
      contentX += iconSize + 8.0f;
    }

    // Title
    Color titleCol = m_style.titleColor;
    titleCol.a = a;
    Fumbo::Graphic2D::DrawText(notif.title, {contentX, contentY}, m_font,
                               m_style.titleFontSize, titleCol);

    // Message
    if (!notif.message.empty()) {
      Color msgCol = m_style.messageColor;
      msgCol.a = a;
      Fumbo::Graphic2D::DrawText(notif.message,
                                 {contentX, contentY + m_style.titleFontSize + 4.0f},
                                 m_font, m_style.messageFontSize, msgCol);
    }

    y += notif.height + m_style.spacing;
    drawn++;
  }
}

} // namespace OS
