#ifndef _GAME_STATE_AWARDS_H_
#define _GAME_STATE_AWARDS_H_

#include <JGE.h>
#include <vector>
#include <string>
#include <utility>
#include "GameState.h"
#include "SimpleMenu.h"

class WGuiList;
class WGuiMenu;
class WSrcCards;
class GridDeckView;
class DeckDataWrapper;
class MTGDeck;
class MTGCard;

class GameStateAwards: public GameState, public JGuiListener
{
private:
    WGuiList * listview;
    WGuiMenu * detailview;
    WSrcCards * setSrc;
    SimpleMenu * menu;
    bool showMenu;
    bool saveMe;
    int mState;
    int mDetailItem;

    // --- Collection completion (custom-rendered tabs) ---
    int mTab;                          // TAB_CARDS(Sets) / TAB_TROPHIES / TAB_STATS
    std::vector<int> mUnlockedSets;    // setIds of unlocked sets (list order)
    std::vector<int> mSetOwned;        // distinct cards owned per set (parallel to mUnlockedSets)
    std::vector<int> mSetTotal;        // distinct cards in each set  (parallel to mUnlockedSets)
    std::vector<std::pair<std::string, std::string> > mStats; // Stats tab: (label, value) rows
    std::vector<std::pair<std::string, std::string> > mAchv;  // Trophies tab: (name, description)
    std::vector<bool> mAchvEarned;                            // parallel to mAchv: is each trophy earned?
    std::vector<std::string> mAchvImage;                      // parallel to mAchv: trophy image filename ("" = none)
    int mAchvFocus;                                           // Trophies tab: which trophy's image is previewed
    float mAchvDescScroll;                                    // auto-scroll offset for a long focused description
    bool mFoilMode;                                           // Sets tab: show foil ownership (owned foil vs missing)
    std::vector<std::pair<std::string, bool> > mDetailRows;   // set detail: (card name, owned?)
    std::vector<MTGCard*> mDetailCards;                        // parallel card ptrs (for preview)
    std::vector<bool> mDetailFoil;                             // parallel: owned as foil?
    int mDetailSel;                    // selected card row in the set detail (drives the preview)
    float mScrollPx;                   // smooth pixel scroll offset for the active custom list
    float mDragLastY;                  // last drag Y for smooth-scroll delta (< -9000 = idle)

    void buildSetCompletion();         // fill mSetOwned/mSetTotal from the player collection
    void buildStatsLines();            // fill mStats from the player collection
    void buildStatsInto(WGuiMenu * v); // (legacy award path) same stats into a WGui list
    void buildAchievements();          // fill mAchv (Trophies tab)
    void renderTabBar();               // Sets / Trophies / Stats tab buttons
    void renderCollectionList();       // Sets tab: set-completion list with progress bars
    void renderStats();                // Stats tab: label/value rows, same look as Sets
    void renderAchievements();         // Trophies tab: achievement rows, same look
    void renderSetDetail();            // a set's owned/missing card list + preview (custom)
    int  collectionRowAtPoint(int cx, int cy); // Sets-list row under a tap, or -1
    int  detailRowAtPoint(int cx, int cy);      // set-detail card row under a tap, or -1
    bool handleTopBarTap(int cx, int cy); // returns true if a tab button consumed the tap

public:
    GameStateAwards(GameApp* parent);
    bool enterSet(int setid);
    bool enterStats(int option);
    virtual ~GameStateAwards();

    virtual void Start();
    virtual void End();
    virtual void Create();
    virtual void Destroy();
    virtual void Update(float dt);
    virtual void Render();
    virtual void ButtonPressed(int controllerId, int controlId);
    virtual void OnScroll(int inXVelocity, int inYVelocity);
};

#endif
