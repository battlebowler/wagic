#ifndef _CARD_DISPLAY_H_
#define _CARD_DISPLAY_H_

#include "PlayGuiObjectController.h"

class TargetChooser;
class MTGGameZone;
class MTGCardInstance;

class CardDisplay: public PlayGuiObjectController
{
    int mId;
public:
    int x, y, start_item, nb_displayed_items;
    MTGGameZone * zone;
    TargetChooser * tc;
    JGuiListener * listener;
    CardDisplay(GameObserver* game);
    CardDisplay(int id, GameObserver* game, int x, int y, JGuiListener * listener = NULL, TargetChooser * tc = NULL,
            int nb_displayed_items = 7);
    void AddCard(MTGCardInstance * _card);
    // Focus a card by index (e.g. to show a pulled foil big in the pack reveal).
    void setCurrentCard(int i) { if (i >= 0 && i < (int) mObjects.size()) mCurr = i; }
    // Nearest pack-reveal thumbnail to a screen point (-1 if none). Thumbnail x/y are set
    // each frame in Render(norect); used for finger-anchored swipe selection.
    int thumbAtPoint(float px, float py) const;
    // Finger-anchored browse for the in-duel zone card list: move the selection/preview to the
    // card nearest the drag point (like the hand). Does NOT consume the click.
    void hoverAt(float px, float py);
    void rotateLeft();
    void rotateRight();
    bool CheckUserInput(JButton key);
    virtual void Update(float dt);
    void Render(bool norect = false);
    void init(MTGGameZone * zone);
    virtual ostream& toString(ostream& out) const;
};

std::ostream& operator<<(std::ostream& out, const CardDisplay& m);

#endif
