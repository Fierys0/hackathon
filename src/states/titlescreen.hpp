#include "fumbo.hpp"
#include "raylib.h"

class TitleScreen : public IGameState {
public:
  void Init() override;
  void Cleanup() override;
  void Update() override;
  void DrawClean() override;
  void DrawDirty() override;

private:
  // Textures
  Texture2D m_titlecard;
  Texture2D m_background;

  // Buttons
  Fumbo::UI::Button m_btnstart;
  Fumbo::UI::Button m_btnload;
  Fumbo::UI::Button m_btnsettings;
  Fumbo::UI::Button m_btnexit;

  // Settings modal
  bool m_showSettings = false;
  bool m_draggingVolume = false;
  void DrawSettingsModal();
};
