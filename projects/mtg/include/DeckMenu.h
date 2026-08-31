/*
 A class for menus with a fixed layout
 */
#ifndef _DeckMenu_H_
#define _DeckMenu_H_

#include <string>
#include "WFont.h"
#include "hge/hgeparticle.h"
#include "DeckMetaData.h"
#include "TextScroller.h"
#include "InteractiveButton.h"

class DeckMenu: public JGuiController
{
private:
    InteractiveButton *dismissButton;
    
protected:

    float mHeight, mWidth, mX, mY;
    float titleX, titleY, titleWidth;
    float descX, descY, descHeight, descWidth;
    float statsX, statsY, statsHeight, statsWidth;
    float avatarX, avatarY;
    float detailedInfoBoxX, detailedInfoBoxY;
    float starsOffsetX;

    bool menuInitialized;
    string backgroundName;

    int fontId;
    string title;
    string displayTitle;
    WFont * mFont;
    float titleFontScale;

    int maxItems, startId;

    float selectionT, selectionY;
    float timeOpen;

    static hgeParticleSystem* stars;

    void initMenuItems();
    string getDescription();
    string getMetaInformation();
    DeckMetaData *mSelectedDeck;
    bool mShowDetailsScreen;
    bool mAlwaysShowDetailsButton;
    bool mClosed;
    bool isOpponent;
    // When true, the screen is split into a narrow list panel on the left with an info panel on
    // the right (deck picker / opponent chooser / deck-editor screens); there is no Info popup
    // button. The deck editor's card-builder view keeps the legacy single-panel layout (false).
    bool mListOnlyLayout;
    // When true (default), the list-only layout draws its generic info panel (previewing the
    // focused row). A subclass can set this false to draw its own info-panel content instead
    // (e.g. DeckEditorMenu's deck-builder variant shows the edited deck's stats, not the focused
    // option's description) while still using the list-only frames + list.
    bool mDrawInfoPanel;

public:
    VerticalTextScroller * mScroller;
    bool mAutoTranslate;
    float mSelectionTargetY;
    
    int getSelectedDeckId() const 
    {
        return mSelectedDeck->getDeckId();
    }
    
    void selectDeck(int deckId, bool isAi);
    void selectRandomDeck(bool isAi);
    
    //used for detailed info button
    JQuadPtr pspIcons[8];
    JTexture * pspIconsTexture;

    DeckMenu(int id, JGuiListener* listener, int fontId, const string _title = "", const int& startIndex = 0, bool alwaysShowDetailsButton = false, bool chooseOpponent = false);
    ~DeckMenu();

    DeckMetaData * getSelectedDeck();
    void enableDisplayDetailsOverride();
    bool showDetailsScreen();
    // Move focus to the first row that is a real deck (has metadata), skipping leading action rows
    // like "Random" or "New Deck...". Keeps the always-on info panel and the Select Deck button
    // operating on an actual deck instead of an action row. Call after the list is fully built.
    void focusFirstDeck();

    // True when the list should be left-aligned (the narrow list-only Play picker). Read by
    // DeckMenuItem when it draws its row text.
    bool usesLeftAlignedList() const { return mListOnlyLayout; }

    // True on the "Choose Opponent" chooser. Read by DeckMenuItem to suppress the "new" badge on
    // newly-unlocked opponent decks.
    bool isOpponentChooser() const { return isOpponent; }

    // Build the multi-line deck stats text (play record + composition) shown in the list-only
    // info panel. Uses the cached extended deck stats, so it is cheap to call per frame.
    string buildDeckStatsText(DeckMetaData* deck);
    
    virtual bool isClosed() const { return mClosed; }

    virtual void Render();
    virtual void Update(float dt);
    virtual bool CheckUserInput(JButton key);
    // Advance only the visuals (particles/selection animation) without processing input.
    // Used when a modal popup is displayed on top of the menu so the background keeps
    // animating instead of freezing.
    void UpdateVisuals(float dt);
    using JGuiController::Add;
    virtual void Add(int id, const string& Text, const string& desc = "", bool forceFocus = false, DeckMetaData *deckMetaData = NULL);
    virtual void Close();
    void updateScroller();
    void RenderBackground();
    void RenderDeckManaColors();
    
    static void destroy();
};

#endif
