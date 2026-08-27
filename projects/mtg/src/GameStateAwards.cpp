/*
 This is where the player views their awards, etc.
 */
#include "PrecompiledHeader.h"

#include <JRenderer.h>
#include "GameStateAwards.h"
#include "GameApp.h"
#include "MTGDeck.h"
#include "Translate.h"
#include "OptionItem.h"
#include "DeckDataWrapper.h"
#include "Credits.h"
#include "WResourceManager.h"
#include "WFont.h"
#include "CardGui.h"
#include "GridDeckView.h"
#include "DeckDataWrapper.h"

// A "Back to Main Menu" list entry styled like the game's standard buttons: a filled red
// box with white centred text (the default WGuiItem renders plain text only).
class WGuiBackButton : public WGuiItem
{
public:
    WGuiBackButton(string s) : WGuiItem(s) {}
    virtual void Render()
    {
        JRenderer * r = JRenderer::GetInstance();
        WFont * mFont = WResourceManager::Instance()->GetWFont(Fonts::OPTION_FONT);
        float bw = getWidth();
        float bh = getHeight();
        r->FillRect(x + 3, y + 3, bw - 6, bh - 5, ARGB(220, 5, 5, 5));
        r->FillRect(x + 2, y + 2, bw - 6, bh - 5, ARGB(255, 140, 23, 23));
        r->DrawRect(x + 2, y + 2, bw - 6, bh - 5, ARGB(255, 20, 20, 20));
        DWORD old = mFont->GetColor();
        mFont->SetColor(ARGB(255, 255, 255, 255));
        float fH = (bh - mFont->GetHeight()) / 2;
        mFont->DrawString(_(displayValue), x + bw / 2, y + fH, JGETEXT_CENTER);
        mFont->SetColor(old);
    }
};

// A set-spoiler row that colours the card name by ownership: bright white if the player owns
// the card, dim grey if it's still missing from the collection. Mirrors WGuiItem::Render so it
// lines up with any plain rows, only swapping the text colour.
class WGuiCardEntry : public WGuiItem
{
public:
    bool owned;
    WGuiCardEntry(string s, bool _owned) : WGuiItem(s), owned(_owned) {}
    virtual void Render()
    {
        WFont * mFont = WResourceManager::Instance()->GetWFont(Fonts::OPTION_FONT);
        DWORD oldcolor = mFont->GetColor();
        mFont->SetColor(owned ? ARGB(255, 240, 240, 240) : ARGB(255, 96, 96, 96));
        float fH = (height - mFont->GetHeight()) / 2;
        string trans = _(displayValue);
        float fW = mFont->GetStringWidth(trans.c_str());
        float boxW = getWidth();
        float oldS = mFont->GetScale();
        if (fW > boxW)
            mFont->SetScale(boxW / fW);
        mFont->DrawString(trans, x + (width / 2), y + fH, JGETEXT_CENTER);
        mFont->SetScale(oldS);
        mFont->SetColor(oldcolor);
    }
};

namespace
{
    // On-screen Back button for touch (the trophy room otherwise only exits via hardware
    // MENU/PREV/SEC). Emits JGE_BTN_SEC: detail -> list, list -> main menu. Sized to the game's
    // standard red pill so it matches every other Back button (see back-button-uniformity):
    // text-fitted visible box = inner(sw-3 x fh-4) + 2*radius. Runtime-computed (SCREEN_*_F).
    inline void getAwardsBackRect(float& x, float& y, float& w, float& h)
    {
        WFont * f = WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT);
        float sw = 30.0f, fh = 16.0f;
        if (f)
        {
            f->SetScale(1.0f);
            sw = f->GetStringWidth(_("Back").c_str());
            fh = f->GetHeight();
        }
        w = (sw - 3.0f) + 10.0f;
        h = (fh - 4.0f) + 10.0f;
        x = SCREEN_WIDTH_F - w - 4.0f;   // bottom-right
        y = SCREEN_HEIGHT_F - h - 4.0f;
    }
}

enum ENUM_AWARDS_STATE
{
    STATE_LISTVIEW,
    STATE_DETAILS,
    EXIT_AWARDS_MENU = -102,
    GUI_AWARD_BUTTON = -103,
    SET_MENU_ID = -104,   // set-selection popup for the Cards tab

};

// Top-of-screen tabs for the redesigned trophy room / collection browser.
enum ENUM_AWARDS_TAB
{
    TAB_CARDS = 0,   // visual grid of the owned collection (default)
    TAB_TROPHIES,    // achievements + set list (the original list view)
    TAB_STATS        // collection statistics
};

namespace
{
    // Tab bar sits along the top-left; the set-filter button sits just right of the tabs
    // (Cards tab only). All dimensions are VISIBLE pixels; FillRoundRect inflates a box by
    // 2*radius per axis, so the draw helper subtracts that. Computed at runtime because
    // SCREEN_WIDTH_F is only valid once the real screen size is known.
    const float kTabY = 4.0f;
    const float kTabVisH = 22.0f;
    const float kTabVisW = 74.0f;
    const float kTabGap = 4.0f;
    inline float tabX(int i) { return 6.0f + i * (kTabVisW + kTabGap); }
    const float kFilterVisW = 152.0f;
    inline float filterX() { return tabX(3) + 6.0f; }

    // Draw one of the game's standard red pill buttons covering the VISIBLE box
    // (x, y, visW, visH). active=true keeps it bright; inactive dims it so the current tab
    // stands out.
    void drawTabButton(float x, float y, float visW, float visH, const string& label, bool active)
    {
        JRenderer * r = JRenderer::GetInstance();
        const float rad = 5.0f;
        float iw = visW - 2 * rad, ih = visH - 2 * rad;
        int a = active ? 255 : 150;
        r->FillRoundRect(x + 1, y + 1, iw, ih, rad, ARGB(active ? 220 : 150, 5, 5, 5));
        r->FillRoundRect(x, y, iw, ih, rad, ARGB(a, 140, 23, 23));
        r->DrawRoundRect(x, y, iw, ih, rad, ARGB(a, 5, 5, 5));
        WFont * f = WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT);
        if (f)
        {
            f->SetScale(1.0f);
            f->SetColor(ARGB(active ? 255 : 210, 255, 255, 255));
            f->DrawString(_(label), x + visW * 0.5f, y + (visH - f->GetHeight()) * 0.5f, JGETEXT_CENTER);
        }
    }
}

namespace GameStateAwardsConst
{
    const int kBackToTrophiesID = 2;
    const int kBackToMainMenuID = 1;
}

static std::string kAwardFile = "";

GameStateAwards::GameStateAwards(GameApp* parent) :
    GameState(parent, "trophies"),
    listview(NULL), detailview(NULL), setSrc(NULL), menu(NULL),
    showMenu(false), saveMe(false), mState(STATE_LISTVIEW), mDetailItem(0),
    mTab(TAB_CARDS), mDetailSel(0), mScrollPx(0.0f), mDragLastY(-9999.0f)
{

}

GameStateAwards::~GameStateAwards()
{
    kAwardFile = ""; //Reset the chosen background.
}

void GameStateAwards::End()
{
    SAFE_DELETE(menu);
    SAFE_DELETE(detailview);
    SAFE_DELETE(listview);
    SAFE_DELETE(setSrc);
    mUnlockedSets.clear();
    mSetOwned.clear();
    mSetTotal.clear();
    mStats.clear();
    mAchv.clear();
    mDetailRows.clear();
    mDetailCards.clear();

    if (saveMe)
        options.save();

    kAwardFile = ""; //Reset the chosen background.
}
void GameStateAwards::Start()
{
    mParent->DoAnimation(TRANSITION_FADE_IN);
    char buf[256];
    mState = STATE_LISTVIEW;
    options.checkProfile();

    menu = NULL;
    saveMe = options.newAward();

    // Collection-completion state (rebuilt fresh each entry; End() frees the previous set).
    mTab = TAB_CARDS;
    mScrollPx = 0.0f;
    mDragLastY = -9999.0f;
    mUnlockedSets.clear();
    mSetOwned.clear();
    mSetTotal.clear();
    mStats.clear();

    // Trophies tab is custom-rendered now (renderAchievements) -- no WGui list, no highlight.
    listview = NULL;
    detailview = NULL;
    mAchv.clear();
    mDetailRows.clear();
    mDetailCards.clear();
    buildAchievements();

    vector<pair<string, string> > orderedSet;
    for(int i = 0; i < setlist.size(); i++){
        sprintf(buf, "%s", setlist[i].c_str());
        if (options[Options::SORTINGSETS].number == 2) // Now sets can be sorted by sector(orderindex) or name or release date.
            orderedSet.push_back(pair<string, string> (setlist.getInfo(i)->getDate(), buf));
        else if (options[Options::SORTINGSETS].number == 1)
            orderedSet.push_back(pair<string, string> (setlist.getInfo(i)->getName(), buf));
        else
            orderedSet.push_back(pair<string, string> (setlist.getInfo(i)->getOrderIndex(), buf));
    }
    sort(orderedSet.begin(),orderedSet.end());
    for (unsigned int i = 0; i < orderedSet.size(); i++)
    {
        int sid = setlist.findSet(orderedSet.at(i).second);
        MTGSetInfo * si = setlist.getInfo(sid);
        if (!si) continue;
        if (!options[Options::optionSet(sid)].number) continue; // locked set: not shown
        mUnlockedSets.push_back(sid);   // Sets tab lists these
    }

    setSrc = NULL;
    showMenu = false;

    // Tally the player's collection per set for the completion bars (default Sets tab).
    buildSetCompletion();

#if !defined (PSP)
    GameApp::playMusic("Track4.mp3"); // Added music for trophies.
#endif
}

void GameStateAwards::Create()
{
}
void GameStateAwards::Destroy()
{
}

void GameStateAwards::renderTabBar()
{
    // Solid toolbar strip so the tabs read as chrome instead of floating over the card art
    // (the grid draws its top row right up to the top edge).
    JRenderer::GetInstance()->FillRect(0, 0, SCREEN_WIDTH_F, kTabY + kTabVisH + 4, ARGB(210, 8, 8, 12));

    const char * labels[3] = { "Sets", "Trophies", "Stats" };
    for (int i = 0; i < 3; i++)
        drawTabButton(tabX(i), kTabY, kTabVisW, kTabVisH, labels[i], mTab == i);
}

// Returns true if a tap on the tab bar was consumed.
bool GameStateAwards::handleTopBarTap(int cx, int cy)
{
    // Forgiving vertical band (covers the toolbar strip) so a slightly-low tap on a tab still
    // registers instead of falling through to the content underneath.
    if (cy < 0 || cy > kTabY + kTabVisH + 4)
        return false;

    for (int i = 0; i < 3; i++)
    {
        float x = tabX(i);
        if (cx >= x && cx <= x + kTabVisW)
        {
            if (mTab != i)
            {
                mTab = i;
                mScrollPx = 0.0f;        // reset scroll for the newly-shown list
                mDragLastY = -9999.0f;
                // Close any open set-detail spoiler so it doesn't leak into the new tab.
                if (mState == STATE_DETAILS)
                {
                    mState = STATE_LISTVIEW;
                    SAFE_DELETE(detailview);
                    SAFE_DELETE(setSrc);
                }
                if (mTab == TAB_STATS && mStats.empty())
                    buildStatsLines();   // computed once, lazily
            }
            return true;
        }
    }
    return false;
}

namespace
{
    // Shared list geometry for the custom Sets / Stats tabs, so they look identical and scroll
    // the same way. The toolbar strip (renderTabBar, drawn on top afterwards) hides the sliver
    // of the first row that scrolls up under it.
    const float kListX = 8.0f;
    inline float kListW()   { return SCREEN_WIDTH_F - 16.0f; }
    inline float kListTop() { return kTabY + kTabVisH + 8.0f; }
    inline float kListBot() { return SCREEN_HEIGHT_F - 6.0f; }
    const float kRowH = 24.0f;
}

// The Sets tab: a scrolling list of every unlocked set with a progress bar showing how much of
// that set the player owns (owned / total) -- a collection-completion view. Smooth pixel scroll.
void GameStateAwards::renderCollectionList()
{
    JRenderer * r = JRenderer::GetInstance();
    WFont * f = WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT);
    f->SetScale(1.0f);
    float fh = f->GetHeight();
    const float listW = kListW(), top = kListTop(), bottom = kListBot();

    // Solid backdrop so the busy trophy-room art doesn't bleed through the gaps between rows.
    r->FillRect(0, top - 2, SCREEN_WIDTH_F, bottom - top + 4, ARGB(238, 12, 14, 18));

    int n = (int) mUnlockedSets.size();
    float maxScrollPx = n * kRowH - (bottom - top);
    if (maxScrollPx < 0.0f) maxScrollPx = 0.0f;
    if (mScrollPx > maxScrollPx) mScrollPx = maxScrollPx;
    if (mScrollPx < 0.0f) mScrollPx = 0.0f;

    int firstRow = (int) (mScrollPx / kRowH);
    float pixOff = mScrollPx - firstRow * kRowH;

    for (int vis = 0; ; vis++)
    {
        int i = firstRow + vis;
        if (i >= n) break;
        float ry = top - pixOff + vis * kRowH;
        if (ry >= bottom) break;

        int owned = mSetOwned[i];
        int total = mSetTotal[i];
        float frac = (total > 0) ? (float) owned / (float) total : 0.0f;
        if (frac > 1.0f) frac = 1.0f;

        // Progress bar drawn as the row background: dark track + filled portion.
        float barH = kRowH - 4.0f;
        r->FillRect(kListX, ry, listW, barH, ARGB(205, 18, 22, 28));
        r->FillRect(kListX, ry, listW * frac, barH, ARGB(230, 42, 110, 168));
        r->DrawRect(kListX, ry, listW, barH, ARGB(120, 130, 130, 130));

        // Set name on the left, owned/total on the right.
        MTGSetInfo * si = setlist.getInfo(mUnlockedSets[i]);
        string label = si ? si->getName() : setlist[mUnlockedSets[i]];
        float ty = ry + (barH - fh) * 0.5f;
        f->SetColor(ARGB(255, 255, 255, 255));
        f->DrawString(label, kListX + 5, ty);
        char buf[48];
        sprintf(buf, "%d / %d", owned, total);
        f->DrawString(buf, kListX + listW - 5, ty, JGETEXT_RIGHT);
    }
}

// The Stats tab: same row look as Sets (same font, alignment, dark row background) so the two
// tabs are visually uniform -- just label on the left, value on the right, no progress bar.
void GameStateAwards::renderStats()
{
    JRenderer * r = JRenderer::GetInstance();
    WFont * f = WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT);
    f->SetScale(1.0f);
    float fh = f->GetHeight();
    const float listW = kListW(), top = kListTop(), bottom = kListBot();

    // Solid backdrop so the busy trophy-room art doesn't bleed through the gaps between rows.
    r->FillRect(0, top - 2, SCREEN_WIDTH_F, bottom - top + 4, ARGB(238, 12, 14, 18));

    int n = (int) mStats.size();
    float maxScrollPx = n * kRowH - (bottom - top);
    if (maxScrollPx < 0.0f) maxScrollPx = 0.0f;
    if (mScrollPx > maxScrollPx) mScrollPx = maxScrollPx;
    if (mScrollPx < 0.0f) mScrollPx = 0.0f;

    int firstRow = (int) (mScrollPx / kRowH);
    float pixOff = mScrollPx - firstRow * kRowH;

    for (int vis = 0; ; vis++)
    {
        int i = firstRow + vis;
        if (i >= n) break;
        float ry = top - pixOff + vis * kRowH;
        if (ry >= bottom) break;

        float barH = kRowH - 4.0f;
        r->FillRect(kListX, ry, listW, barH, ARGB(205, 18, 22, 28));
        r->DrawRect(kListX, ry, listW, barH, ARGB(120, 130, 130, 130));

        float ty = ry + (barH - fh) * 0.5f;
        f->SetColor(ARGB(255, 235, 235, 235));
        f->DrawString(mStats[i].first, kListX + 5, ty);
        f->SetColor(ARGB(255, 150, 210, 255));   // value tinted to echo the completion bar
        f->DrawString(mStats[i].second, kListX + listW - 5, ty, JGETEXT_RIGHT);
    }
}

// Maps a tap to a Sets-list row index (or -1). Layout MUST match renderCollectionList.
int GameStateAwards::collectionRowAtPoint(int cx, int cy)
{
    (void) cx;
    const float top = kListTop();
    if (cy < top) return -1;
    int firstRow = (int) (mScrollPx / kRowH);
    float pixOff = mScrollPx - firstRow * kRowH;
    int vis = (int) ((cy - (top - pixOff)) / kRowH);
    float ry = top - pixOff + vis * kRowH;
    if (cy > ry + (kRowH - 4.0f)) return -1; // tap landed in the gap between rows
    int i = firstRow + vis;
    if (i < 0 || i >= (int) mUnlockedSets.size()) return -1;
    return i;
}

void GameStateAwards::buildAchievements()
{
    mAchv.clear();
    mAchv.push_back(std::make_pair(std::string("Difficulty Modes"),
        std::string(options[Options::DIFFICULTY_MODE_UNLOCKED].number ? "Unlocked" : "Reach a 66% victory ratio")));
    for (std::map<std::string, Unlockable *>::iterator it = Unlockable::unlockables.begin();
         it != Unlockable::unlockables.end(); ++it)
    {
        Unlockable * award = it->second;
        if (award)
            mAchv.push_back(std::make_pair(award->getValue("name"), award->getValue("trophyroom_text")));
    }
    mAchv.push_back(std::make_pair(std::string("Evil Twin Mode"),
        std::string(options[Options::EVILTWIN_MODE_UNLOCKED].number ? "Unlocked" : "Win with the same army size")));
    mAchv.push_back(std::make_pair(std::string("Random Deck Mode"),
        std::string(options[Options::RANDOMDECK_MODE_UNLOCKED].number ? "Unlocked" : "Win against a higher difficulty")));
    mAchv.push_back(std::make_pair(std::string("Valuable Collection"),
        std::string(options[Options::AWARD_COLLECTOR].number ? "Achieved" : "Own a collection worth over 10,000c")));
}

// Trophies tab: achievement rows (name + status/goal), same look/scroll as Sets & Stats.
void GameStateAwards::renderAchievements()
{
    JRenderer * r = JRenderer::GetInstance();
    WFont * f = WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT);
    f->SetScale(1.0f);
    float fh = f->GetHeight();
    const float listW = kListW(), top = kListTop(), bottom = kListBot();

    r->FillRect(0, top - 2, SCREEN_WIDTH_F, bottom - top + 4, ARGB(238, 12, 14, 18));

    int n = (int) mAchv.size();
    float maxScrollPx = n * kRowH - (bottom - top);
    if (maxScrollPx < 0.0f) maxScrollPx = 0.0f;
    if (mScrollPx > maxScrollPx) mScrollPx = maxScrollPx;
    if (mScrollPx < 0.0f) mScrollPx = 0.0f;

    int firstRow = (int) (mScrollPx / kRowH);
    float pixOff = mScrollPx - firstRow * kRowH;

    for (int vis = 0; ; vis++)
    {
        int i = firstRow + vis;
        if (i >= n) break;
        float ry = top - pixOff + vis * kRowH;
        if (ry >= bottom) break;

        float barH = kRowH - 4.0f;
        r->FillRect(kListX, ry, listW, barH, ARGB(205, 18, 22, 28));
        r->DrawRect(kListX, ry, listW, barH, ARGB(120, 130, 130, 130));

        float ty = ry + (barH - fh) * 0.5f;
        f->SetColor(ARGB(255, 235, 235, 235));
        f->DrawString(mAchv[i].first, kListX + 5, ty);
        f->SetColor(ARGB(255, 150, 210, 255));
        f->DrawString(mAchv[i].second, kListX + listW - 5, ty, JGETEXT_RIGHT);
    }
}

namespace { const float kDetailListX = 0.40f; } // fraction of width where the detail list starts

// A set's card list: owned (bright) vs missing (grey) card rows on the right, the selected
// card's art previewed on the left. Smooth scroll + precise per-row taps like the other tabs.
void GameStateAwards::renderSetDetail()
{
    JRenderer * r = JRenderer::GetInstance();
    WFont * f = WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT);
    f->SetScale(1.0f);
    float fh = f->GetHeight();

    const float top = kListTop(), bottom = kListBot();
    const float listX = SCREEN_WIDTH_F * kDetailListX;
    const float listW = SCREEN_WIDTH_F - listX - 8.0f;

    r->FillRect(0, top - 2, SCREEN_WIDTH_F, bottom - top + 4, ARGB(238, 12, 14, 18));

    int n = (int) mDetailRows.size();
    float maxScrollPx = n * kRowH - (bottom - top);
    if (maxScrollPx < 0.0f) maxScrollPx = 0.0f;
    if (mScrollPx > maxScrollPx) mScrollPx = maxScrollPx;
    if (mScrollPx < 0.0f) mScrollPx = 0.0f;

    int firstRow = (int) (mScrollPx / kRowH);
    float pixOff = mScrollPx - firstRow * kRowH;

    for (int vis = 0; ; vis++)
    {
        int i = firstRow + vis;
        if (i >= n) break;
        float ry = top - pixOff + vis * kRowH;
        if (ry >= bottom) break;

        float barH = kRowH - 4.0f;
        bool owned = mDetailRows[i].second;
        bool foil = (i < (int) mDetailFoil.size()) && mDetailFoil[i];
        bool sel = (i == mDetailSel);
        r->FillRect(listX, ry, listW, barH, ARGB(205, sel ? 40 : 18, sel ? 52 : 22, sel ? 70 : 28));
        r->DrawRect(listX, ry, listW, barH, ARGB(120, 130, 130, 130));
        float ty = ry + (barH - fh) * 0.5f;
        // Name coloured by REGULAR ownership (white = own the normal copy, grey = missing), so
        // the normal copy still reads as owned; a gold "(foil)" tag then marks the owned foil copy.
        f->SetColor(owned ? ARGB(255, 240, 240, 240) : ARGB(255, 96, 96, 96));
        f->DrawString(mDetailRows[i].first, listX + 5, ty);
        if (foil)
        {
            float nameW = f->GetStringWidth(mDetailRows[i].first.c_str());
            f->SetColor(ARGB(255, 255, 210, 90));
            f->DrawString(std::string(" (foil)"), listX + 5 + nameW, ty);
        }
    }

    // Preview of the selected card in the left column.
    if (mDetailSel >= 0 && mDetailSel < (int) mDetailCards.size() && mDetailCards[mDetailSel])
    {
        MTGCard * c = mDetailCards[mDetailSel];
        float pw = listX - 16.0f;
        float pcx = 8.0f + pw * 0.5f;
        float pcy = (top + bottom) * 0.5f;
        // Render the preview exactly like the in-game card via RenderBig: it draws the real card
        // frame (rounded outer corners, squared interior, white/black by set) and fills an owned
        // foil beneath it, and falls back to text when art is missing. q is fetched only for the
        // aspect ratio / to warm the cache.
        JQuadPtr q = WResourceManager::Instance()->RetrieveCard(c, RETRIEVE_EXISTING);
        if (!q.get())
            q = WResourceManager::Instance()->RetrieveCard(c);

        const float kcs = SCREEN_HEIGHT_F / 272.0f;         // RenderBig's kCardScale
        float aspect = (q.get() && q->mHeight > 0) ? (q->mWidth / q->mHeight)
                                                   : (CardGui::BigWidth / CardGui::BigHeight);
        float cardH = (bottom - top) * 0.72f;               // leave headroom for the drawn border
        if (cardH * aspect > pw * 0.86f) cardH = pw * 0.86f / aspect;
        float actZ = cardH / (250.0f * kcs);                // RenderBig card height = actZ*250*kCardScale

        Pos pos(pcx, pcy, actZ, 0.0f, 255.0f);
        bool isFoil = (mDetailSel < (int) mDetailFoil.size()) && mDetailFoil[mDetailSel];
        // Force the in-game card border on for the preview regardless of the player's SHOWBORDER
        // setting, then restore it. DrawCard(kNormal) is the public path into RenderBig, which now
        // draws the universal card frame (rounded outer corners, squared interior, white/black by
        // set) and fills an owned foil edge-to-edge beneath that frame.
        int savedBorder = options[Options::SHOWBORDER].number;
        options[Options::SHOWBORDER].number = 1;
        CardGui::DrawCard(c, pos, DrawMode::kNormal, false, false, false, isFoil);
        options[Options::SHOWBORDER].number = savedBorder;
    }
}

// Maps a tap to a set-detail card row (right-hand list), or -1. Layout matches renderSetDetail.
int GameStateAwards::detailRowAtPoint(int cx, int cy)
{
    const float top = kListTop();
    const float listX = SCREEN_WIDTH_F * kDetailListX;
    if (cx < listX || cy < top) return -1;
    int firstRow = (int) (mScrollPx / kRowH);
    float pixOff = mScrollPx - firstRow * kRowH;
    int vis = (int) ((cy - (top - pixOff)) / kRowH);
    float ry = top - pixOff + vis * kRowH;
    if (cy > ry + (kRowH - 4.0f)) return -1;
    int i = firstRow + vis;
    if (i < 0 || i >= (int) mDetailRows.size()) return -1;
    return i;
}

void GameStateAwards::Render()
{
    JRenderer * r = JRenderer::GetInstance();
    r->ClearScreen(ARGB(0,0,0,0));

#if defined (PSP)
    JQuadPtr background = WResourceManager::Instance()->RetrieveTempQuad("pspawardback.jpg", TEXTURE_SUB_5551);
#else
    //Now it's possibile to randomly use up to 10 background images for trophies room (if random index is 0, it will be rendered the default "awardback.jpg" image).
    JQuadPtr background;
    if(kAwardFile == ""){
        char temp[4096];
        sprintf(temp, "awardback%i.jpg", std::rand() % 10);
        kAwardFile.assign(temp);
        JQuadPtr background = WResourceManager::Instance()->RetrieveTempQuad(kAwardFile, TEXTURE_SUB_5551);
        if (!background.get())
            kAwardFile = "awardback.jpg"; //Fallback to default background image for trophies room.
    }
    background = WResourceManager::Instance()->RetrieveTempQuad(kAwardFile, TEXTURE_SUB_5551);
#endif

    if (background.get())
        r->RenderQuad(background.get(), 0, 0, 0, SCREEN_WIDTH_F / background->mWidth, SCREEN_HEIGHT_F / background->mHeight);

    switch (mTab)
    {
    case TAB_CARDS:
        if (mState == STATE_DETAILS)
            renderSetDetail();      // a set's owned/missing card list + preview
        else
            renderCollectionList();
        break;
    case TAB_TROPHIES:
        renderAchievements();
        break;
    case TAB_STATS:
        renderStats();
        break;
    }

    // Tabs + set-filter across the top (hidden while the popup menu is up).
    if (!(showMenu && menu))
        renderTabBar();

    if (showMenu && menu)
    {
        // Dim the whole screen so the popup (set picker / exit menu) reads as a clear modal
        // over the busy card grid.
        r->FillRect(0, 0, SCREEN_WIDTH_F, SCREEN_HEIGHT_F, ARGB(170, 0, 0, 0));
        menu->Render();
    }
    else
    {
        // On-screen Back button — the standard red pill (drawTabButton), so it's the same
        // size/style as every other Back button in the game.
        float ex, ey, ew, eh;
        getAwardsBackRect(ex, ey, ew, eh);
        drawTabButton(ex, ey, ew, eh, "Back", true);
    }
}

void GameStateAwards::Update(float dt)
{
    if (mEngine->GetButtonClick(JGE_BTN_CANCEL))
        options[Options::DISABLECARDS].number = !options[Options::DISABLECARDS].number;

    if (showMenu)
    {
        menu->Update(dt);
    }
    else
    {
        int cx = -1, cy = -1;

        // Taps: tab bar first, then the back button. The Sets list is scroll-only (swipes
        // arrive below as directional keys); it has no per-row tap action.
        if (mEngine->GetLeftClickCoordinates(cx, cy))
        {
            if (handleTopBarTap(cx, cy))
            {
                mEngine->ResetInput();
            }
            else
            {
                float ex, ey, ew, eh;
                getAwardsBackRect(ex, ey, ew, eh);
                if (cx >= ex && cx <= ex + ew && cy >= ey && cy <= ey + eh)
                {
                    mEngine->ResetInput();
                    mEngine->HoldKey_NoRepeat(JGE_BTN_SEC);
                }
                else if (mTab == TAB_CARDS && mState != STATE_DETAILS)
                {
                    // Tap a set row -> open its owned/missing card list.
                    int row = collectionRowAtPoint(cx, cy);
                    if (row >= 0)
                    {
                        mEngine->ResetInput();
                        if (enterSet(mUnlockedSets[row]))
                            mState = STATE_DETAILS;
                    }
                }
                else if (mTab == TAB_CARDS && mState == STATE_DETAILS)
                {
                    // Tap a card row -> select it so its art shows in the preview.
                    int row = detailRowAtPoint(cx, cy);
                    if (row >= 0)
                    {
                        mEngine->ResetInput();
                        mDetailSel = row;
                    }
                }
            }
        }

        // Smooth touch scrolling for ALL the custom lists (Sets, Stats, Trophies, and the set
        // detail): the list follows the finger 1:1 like a normal touch surface.
        bool customList = true;
        bool draggedThisFrame = false;
        if (customList)
        {
            int dgx = 0, dgy = 0;
            if (mEngine->GetDragCoordinates(dgx, dgy))
            {
                if (mDragLastY > -9000.0f)
                    mScrollPx -= (float) (dgy - mDragLastY);   // content follows the finger
                mDragLastY = (float) dgy;
                draggedThisFrame = true;
            }
            else
                mDragLastY = -9999.0f; // finger up / idle: next drag starts fresh (no jump)
        }

        JButton key = JGE_BTN_NONE;

        while ((key = JGE::GetInstance()->ReadButton()))
        {
            switch (key)
            {
            case JGE_BTN_MENU:
                showMenu = true;
                SAFE_DELETE(menu);
                menu = NEW SimpleMenu(JGE::GetInstance(), WResourceManager::Instance(), EXIT_AWARDS_MENU, this, Fonts::MENU_FONT, 50, 170);
                if (mTab == TAB_TROPHIES && mState == STATE_DETAILS)
                    menu->Add(GameStateAwardsConst::kBackToTrophiesID, "Back to Trophies");
                menu->Add(GameStateAwardsConst::kBackToMainMenuID, "Back to Main Menu");
                menu->Add(kCancelMenuID, "Cancel");
                break;
            case JGE_BTN_PREV:
                mParent->DoTransition(TRANSITION_FADE, GAME_STATE_MENU);
                break;
            case JGE_BTN_SEC:
                if (mState == STATE_DETAILS)
                {
                    mState = STATE_LISTVIEW; // card list -> back to the Sets list
                    mDetailRows.clear();
                    mDetailCards.clear();
                    mScrollPx = 0.0f;
                    mDragLastY = -9999.0f;
                }
                else
                    mParent->DoTransition(TRANSITION_FADE, GAME_STATE_MENU);
                break;
            default:
            {
                // All views scroll smoothly. A live drag already scrolled this frame; a flick
                // (no drag) steps one row (reversed: swipe up -> further down the list).
                (void) dt;
                if (!draggedThisFrame)
                {
                    if (key == JGE_BTN_UP) mScrollPx += kRowH;
                    else if (key == JGE_BTN_DOWN) mScrollPx -= kRowH;
                }
                break;
            }
            }
        }

    }
    if (setSrc)
        setSrc->Update(dt);
}

void GameStateAwards::buildSetCompletion()
{
    mSetOwned.assign(mUnlockedSets.size(), 0);
    mSetTotal.assign(mUnlockedSets.size(), 0);

    // Foils count toward 100% for sets that can have them (foil era, ~1999+): each card then
    // counts twice — the regular copy and the foil copy — so the target doubles for those sets.
    const int kFoilFromYear = 1999;
    std::vector<bool> setHasFoils(mUnlockedSets.size(), false);

    // Denominator: distinct cards in each set (as present in the game data). setId -> row.
    std::map<int, int> setIndex;
    for (size_t i = 0; i < mUnlockedSets.size(); i++)
    {
        MTGSetInfo * si = setlist.getInfo(mUnlockedSets[i]);
        int total = si ? si->totalCards() : 0;
        bool foilEra = si && si->year >= kFoilFromYear;
        setHasFoils[i] = foilEra;
        mSetTotal[i] = foilEra ? total * 2 : total;
        setIndex[mUnlockedSets[i]] = (int) i;
    }

    // Numerator: distinct cards the player owns, tallied per set from the collection (each
    // getCard() is one distinct owned card). For foil-era sets, an owned foil copy of a card
    // (collection foilCount > 0) counts as a second card toward that set's completion.
    MTGDeck * coll = NEW MTGDeck(options.profileFile(PLAYER_COLLECTION).c_str(), MTGCollection());
    DeckDataWrapper * view = NEW DeckDataWrapper(coll);
    for (int t = 0; t < view->Size(); t++)
    {
        MTGCard * c = view->getCard(t);
        if (!c) continue;
        std::map<int, int>::iterator it = setIndex.find(c->setId);
        if (it == setIndex.end()) continue;
        int row = it->second;
        mSetOwned[row]++;                                          // the regular copy
        if (setHasFoils[row] && coll->getFoilCount(c->getId()) > 0)
            mSetOwned[row]++;                                      // plus an owned foil copy
    }
    SAFE_DELETE(view);
    SAFE_DELETE(coll);
}

// Build the custom owned/missing card list for one set (rows + parallel card pointers for the
// preview), replacing the old WGui spoiler so the detail scrolls/taps like the rest of the room.
bool GameStateAwards::enterSet(int setid)
{
    MTGSetInfo * si = setlist.getInfo(setid);
    if (!si)
        return false;

    // Every card in the set (from the game database), in collector order.
    WSrcCards * src = NEW WSrcCards();
    src->addFilter(NEW WCFilterSet(setid));
    src->loadMatches(MTGCollection());
    src->bakeFilters();
    src->Sort(WSrcCards::SORT_COLLECTOR);

    // The player's collection (cardId -> owned count) to mark each card owned vs missing.
    MTGDeck * coll = NEW MTGDeck(options.profileFile(PLAYER_COLLECTION).c_str(), MTGCollection());

    mDetailRows.clear();
    mDetailCards.clear();
    mDetailFoil.clear();
    const bool foilEra = si->year >= 1999;   // only these sets can have foils
    for (int t = 0; t < src->Size(); t++)
    {
        MTGCard * c = src->getCard(t);
        if (!c || !c->data) continue;
        bool have = coll->cards.find(c->getId()) != coll->cards.end() && coll->cards[c->getId()] > 0;
        bool foil = foilEra && coll->getFoilCount(c->getId()) > 0;
        mDetailRows.push_back(std::make_pair(c->data->name, have));
        mDetailCards.push_back(c);   // database pointer (persists this session) -> preview art
        mDetailFoil.push_back(foil);
    }
    SAFE_DELETE(coll);
    SAFE_DELETE(src);               // frees the source list; the card pointers stay valid

    mDetailSel = 0;
    mScrollPx = 0.0f;
    mDragLastY = -9999.0f;
    return !mDetailRows.empty();
}
// Compute the collection-statistics rows (label, value) into mStats, for the Stats tab.
void GameStateAwards::buildStatsLines()
{
    mStats.clear();
    DeckDataWrapper* ddw = NEW DeckDataWrapper(NEW MTGDeck(options.profileFile(PLAYER_COLLECTION).c_str(), MTGCollection()));

    if (setlist.size() > 0)
    {
        int * counts = (int*) calloc(setlist.size(), sizeof(int));
        int setid = -1;
        int dupes = 0;
        MTGCard * many = NULL;
        MTGCard * costly = NULL;
        MTGCard * strong = NULL;
        MTGCard * tough = NULL;

        for (int t = 0; t < ddw->Size(); t++)
        {
            MTGCard * c = ddw->getCard(t);
            if (!c)
                continue;
            int count = ddw->count(c);
            if (!c->data->isLand() && (many == NULL || count > dupes))
            {
                many = c;
                dupes = count;
            }
            counts[c->setId] += count;
            if (costly == NULL || c->data->getManaCost()->getConvertedCost() > costly->data->getManaCost()->getConvertedCost())
                costly = c;
            if (c->data->isCreature() && (strong == NULL || c->data->getPower() > strong->data->getPower()))
                strong = c;
            if (c->data->isCreature() && (tough == NULL || c->data->getToughness() > tough->data->getToughness()))
                tough = c;
        }
        for (int i = 0; i < setlist.size(); i++)
            if (setid < 0 || counts[i] > counts[setid])
                setid = i;
        free(counts);

        char buf[512];
        sprintf(buf, "%ic", ddw->totalPrice());
        mStats.push_back(std::make_pair(std::string("Total Value"), std::string(buf)));
        sprintf(buf, "%i", ddw->getCount(WSrcDeck::UNFILTERED_COPIES));
        mStats.push_back(std::make_pair(std::string("Total Cards"), std::string(buf)));
        sprintf(buf, "%i", ddw->getCount(WSrcDeck::UNFILTERED_UNIQUE));
        mStats.push_back(std::make_pair(std::string("Unique Cards"), std::string(buf)));
        if (many)
        {
            sprintf(buf, "%i (%s)", dupes, many->data->getName().c_str());
            mStats.push_back(std::make_pair(std::string("Most Duplicates"), std::string(buf)));
        }
        if (setid >= 0)
            mStats.push_back(std::make_pair(std::string("Favorite Set"), std::string(setlist[setid].c_str())));
        if (costly)
        {
            sprintf(buf, "%i (%s)", costly->data->getManaCost()->getConvertedCost(), costly->data->getName().c_str());
            mStats.push_back(std::make_pair(std::string("Highest Mana Cost"), std::string(buf)));
        }
        if (strong)
        {
            sprintf(buf, "%i (%s)", strong->data->getPower(), strong->data->getName().c_str());
            mStats.push_back(std::make_pair(std::string("Most Powerful"), std::string(buf)));
        }
        if (tough)
        {
            sprintf(buf, "%i (%s)", tough->data->getToughness(), tough->data->getName().c_str());
            mStats.push_back(std::make_pair(std::string("Toughest"), std::string(buf)));
        }
    }

    SAFE_DELETE(ddw->parent);
    SAFE_DELETE(ddw);
}

// (Trophies "Valuable Collection" award path) render the same stats as a plain WGui list.
void GameStateAwards::buildStatsInto(WGuiMenu * detailview)
{
    buildStatsLines();
    detailview->Add(NEW WGuiHeader("Collection Stats"));
    for (size_t i = 0; i < mStats.size(); i++)
    {
        string line = mStats[i].first + ": " + mStats[i].second;
        detailview->Add(NEW WGuiItem(line.c_str(), WGuiItem::NO_TRANSLATE));
    }
    detailview->Entering(JGE_BTN_NONE);
}

bool GameStateAwards::enterStats(int option)
{
    if (option != Options::AWARD_COLLECTOR)
        return false;
    SAFE_DELETE(detailview);
    detailview = NEW WGuiList("Details");
    buildStatsInto(detailview);
    return true;
}

void GameStateAwards::ButtonPressed(int controllerId, int controlId)
{
    if (controllerId == EXIT_AWARDS_MENU)
        switch (controlId)
        {
        case GameStateAwardsConst::kBackToMainMenuID:
            mParent->DoTransition(TRANSITION_FADE, GAME_STATE_MENU);
            showMenu = false;
            break;
        case GameStateAwardsConst::kBackToTrophiesID:
            mState = STATE_LISTVIEW;
            SAFE_DELETE(detailview);
            showMenu = false;
            break;
        case kCancelMenuID:
            showMenu = false;
            break;
        }
    else if (controllerId == GUI_AWARD_BUTTON)
    {
        int setid = controlId - Options::SET_UNLOCKS;

        if (controlId >= Options::SET_UNLOCKS && enterSet(setid))
        {
            mState = STATE_DETAILS;
            mDetailItem = controlId;

        }
        else if (controlId == Options::AWARD_COLLECTOR && enterStats(controlId))
        {
            mState = STATE_DETAILS;
        }
    }
}

void GameStateAwards::OnScroll(int, int inYVelocity)
{
    if (abs(inYVelocity) > 300)
    {
        bool flickUpwards = (inYVelocity < 0);
        int velocity = (inYVelocity < 0) ? (-1 * inYVelocity) : inYVelocity;
        while(velocity > 0)
        {
            mEngine->HoldKey_NoRepeat(flickUpwards ? JGE_BTN_DOWN : JGE_BTN_UP);
            velocity -= 100;
        }
    }
}
