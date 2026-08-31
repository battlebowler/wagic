#ifndef _GAME_STATE_OPTIONS_H_
#define _GAME_STATE_OPTIONS_H_

#include <JGE.h>
#include <JGui.h>
#include "GameState.h"
#include <vector>
#include <string>

class GameApp;
class WGuiTabMenu;
class WGuiBase;
class SimpleMenu;
class SimplePad;

struct KeybGrabber
{
    virtual void KeyPressed(LocalKeySym) = 0;
};

class GameStateOptions: public GameState, public JGuiListener
{
private:
    enum
    {
        SHOW_OPTIONS,
        SHOW_OPTIONS_MENU,
        SAVE
    };
    float timer;
    bool mReload;
    KeybGrabber* grabber;
    WGuiBase* mCreditsTab; // the Credits tab list; the scrolling credits render only when it's active

    // --- User-tab Profiles manager: a self-contained, full-screen touch modal (drag-to-scroll list
    // of profile rows, tap a row to open its options). Built custom because the WGui settings
    // framework is d-pad oriented and can't cleanly do per-row taps + drag scrolling.
    struct ProfileRow
    {
        std::string name;   // profile name ("Default", "Patrick", ...)
        std::string theme;  // that profile's saved Theme (folder under themes/)
        std::string stats;  // "<credits> credits  -  <cards> cards  -  <sets> sets"
    };
    std::vector<ProfileRow> mProfileRows;
    int   mProfilesDetail;       // index of the profile whose options panel is open; -1 = list view
    float mProfilesScroll;       // vertical scroll offset of the list, in virtual pixels
    WGuiBase* mUserTab;          // the User tab; while it's the active tab its body IS the manager
    void  buildProfileRows();    // (re)gather name/theme/stats for every profile
    void  renderProfilesModal();
    void  updateProfilesModal(float dt);
    void  renderProfileCard(const ProfileRow& row, float x, float y, float w, float h);
    void  cycleProfileTheme(int idx); // advance idx's profile to the next theme and apply it

public:
    SimpleMenu * optionsMenu;
    WGuiTabMenu * optionsTabs;
    int mState;

    GameStateOptions(GameApp* parent);
    virtual ~GameStateOptions();

    virtual void Start();
    virtual void End();
    virtual void Update(float dt);
    virtual void Render();
    virtual void GrabKeyboard(KeybGrabber*);
    virtual void UngrabKeyboard(const KeybGrabber*);
    void ButtonPressed(int controllerId, int ControlId);

    string newProfile;

};

#endif
