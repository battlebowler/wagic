#include "PrecompiledHeader.h"

#include "MTGRules.h"
#include "CardSelector.h"
#include "GuiCombat.h"
#include "GuiBackground.h"
#include "GuiFrame.h"
#include "GuiPhaseBar.h"
#include "GuiAvatars.h"
#include "GuiHand.h"
#include "GuiPlay.h"
#include "GuiMana.h"
#include "Trash.h"
#include "DuelLayers.h"
#include "GameOptions.h"
#include "WResourceManager.h"
#include "WFont.h"
#include "Translate.h"
#include "JRenderer.h"

namespace
{
    // The phase-advance tap target is the top-right phase text ("(your turn) Main
    // phase 1", drawn by GuiPhaseBar). It has no button/visual of its own: tapping that
    // corner advances the phase. Computed at runtime because SCREEN_WIDTH_F is only
    // correct once the real screen size is known.
    inline void getNextPhaseRect(float& x, float& y, float& w, float& h)
    {
        w = SCREEN_WIDTH_F * 0.40f;   // right ~40%, covering the phase text
        h = 26.0f;                    // top strip where the phase text sits
        x = SCREEN_WIDTH_F - w;
        y = 0.0f;
    }

    // On-screen "Cancel" button, shown only while choosing a target (so an accidental cast
    // can be backed out of). Sits in the right-hand margin, clear of the board and hand.
    inline void getCancelRect(float& x, float& y, float& w, float& h)
    {
        w = 62.0f;
        h = 20.0f;
        x = SCREEN_WIDTH_F - w - 4.0f;
        y = SCREEN_HEIGHT_F * 0.44f;
    }

}

void DuelLayers::CheckUserInput(int isAI)
{
    JButton key;
    int x, y;
    JGE* jge = observer->getInput();
    if(!jge) return;

    // Finger-anchored browsing: while the finger is dragging, move the selection to the
    // card/element under it (with its preview) instead of stepping through with the
    // directional presses the swipe also emits. Drain those presses so they don't fight
    // the drag. A tap (no drag) still activates via the normal path below.
    if (!isAI)
    {
        int dragX, dragY;
        if (jge->GetDragCoordinates(dragX, dragY))
        {
            // If a zone card-list is open (graveyard / exile / library / ...), browse THAT
            // with the finger -- its big preview follows the card under the finger, like the
            // hand does -- instead of hovering the battlefield behind it.
            if (observer->OpenedDisplay)
                observer->OpenedDisplay->hoverAt((float)dragX, (float)dragY);
            else if (mCardSelector)
                mCardSelector->HoverAt((float)dragX, (float)dragY);
            while (jge->ReadButton()) {}
            jge->LeftClickedProcessed();
            return;
        }
    }

    while ((key = jge->ReadButton()) || jge->GetLeftClickCoordinates(x, y))
    {
        if ((!isAI) && ((0 != key) ||  jge->GetLeftClickCoordinates(x, y)))
        {
            // A fresh tap dismisses any lingering big card preview right away. If the tap
            // lands on a battlefield card, CardSelector re-shows that card's preview during
            // dispatch below; a tap on anything else (hand, avatar, phase button, empty
            // space) leaves it cleared.
            if (mCardSelector && jge->GetLeftClickCoordinates(x, y))
                mCardSelector->ClearPreview();
            // On-screen Cancel button: while choosing a target, a tap here aborts the action
            // (so an accidental cast can be backed out of). Takes priority over other taps.
            // Show it for a spell cast from hand too: that targeting is driven by the game's
            // targetChooser with no ActionElement waiting, so action->canCancel() is false
            // there even though cancelCurrentAction() (which deletes the targetChooser) works.
            if (observer->getCurrentTargetChooser() && action &&
                (action->canCancel() || !action->isWaitingForAnswer()) &&
                jge->GetLeftClickCoordinates(x, y))
            {
                float ccx, ccy, ccw, cch;
                getCancelRect(ccx, ccy, ccw, cch);
                if (x >= ccx && x <= ccx + ccw && y >= ccy && y <= ccy + cch)
                {
                    jge->LeftClickedProcessed();
                    observer->cancelCurrentAction();
                    break;
                }
            }
            // (Game menu / return to main menu is opened by the system Back gesture —
            // an edge swipe — which maps to JGE_BTN_MENU; no on-screen button needed.)
            // On-screen Next Phase button: a tap inside its bounds advances the phase.
            float npx, npy, npw, nph;
            getNextPhaseRect(npx, npy, npw, nph);
            if (jge->GetLeftClickCoordinates(x, y) &&
                x >= npx && x <= npx + npw &&
                y >= npy && y <= npy + nph)
            {
                JButton phaseTrigger = options[Options::REVERSETRIGGERS].number ? JGE_BTN_NEXT : JGE_BTN_PREV;
                jge->LeftClickedProcessed();
                // Dispatch the phase trigger immediately on this tap (stack first so a
                // pending interrupt still gets priority, then the phase handler).
                if (!stack->CheckUserInput(phaseTrigger))
                    action->CheckUserInput(phaseTrigger);
                break;
            }
            // Hand handle: a tap on the right-edge grip collapses/restores the player's hand.
            // Checked before card dispatch so it wins over any card the grip overlaps.
            if (hand && jge->GetLeftClickCoordinates(x, y))
            {
                float hgx, hgy, hgw, hgh;
                hand->getHandleRect(hgx, hgy, hgw, hgh);
                if (x >= hgx && x <= hgx + hgw && y >= hgy && y <= hgy + hgh)
                {
                    jge->LeftClickedProcessed();
                    hand->ToggleHidden();
                    break;
                }
            }
            if (stack->CheckUserInput(key)) {
                jge->LeftClickedProcessed();
                break;
            }
            if (combat->CheckUserInput(key)) {
                jge->LeftClickedProcessed();
                break;
            }
            if (avatars->CheckUserInput(key)) {
                jge->LeftClickedProcessed();
                break; //avatars need to check their input before action (CTRL_CROSS)
            }
            if (action->CheckUserInput(key)) {
                jge->LeftClickedProcessed();
                break;
            }
            if (hand->CheckUserInput(key)) {
                jge->LeftClickedProcessed();
                break;
            }
            if (mCardSelector->CheckUserInput(key)) {
                jge->LeftClickedProcessed();
                break;
            }
        }
        jge->LeftClickedProcessed();
    }
}

void DuelLayers::Update(float dt, Player * currentPlayer)
{
    for (int i = 0; i < nbitems; ++i)
        objects[i]->Update(dt);

    int isAI = currentPlayer->isAI() || currentPlayer != getObserver()->players[mPlayerViewIndex]; // Fix for 2 players hand.
    if (isAI && !currentPlayer->getObserver()->isLoading())
        currentPlayer->Act(dt);

    CheckUserInput(isAI);
}

ActionStack * DuelLayers::stackLayer()
{
    return stack;
}

GuiCombat * DuelLayers::combatLayer()
{
    return combat;
}

ActionLayer * DuelLayers::actionLayer()
{
    return action;
}

GuiAvatars * DuelLayers::GetAvatars()
{
    return avatars;
}

DuelLayers::DuelLayers(GameObserver* go, int playerViewIndex) :
    nbitems(0), mPlayerViewIndex(playerViewIndex)
{
    observer = go;
    observer->mLayers = this;
    mCardSelector = NEW CardSelector(go, this);
    //1 Action Layer
    action = NEW ActionLayer(go);
    action->Add(phaseHandler = NEW MTGGamePhase(go, action->getMaxId())); //Phases handler
    action->Add(NEW OtherAbilitiesEventReceiver(go, -1)); //autohand, etc... handler
    //Other display elements
    action->Add(NEW HUDDisplay(go, -1));

    Add(NEW GuiMana(20, 20, getRenderedPlayerOpponent()));
    // Player's mana pool anchored to the top-right, just under the phase name, so it no
    // longer sits on top of the big card preview (which occupies the lower right). Kept
    // ~45px in from the right edge because the pool renders slightly right of its anchor.
    Add(NEW GuiMana(SCREEN_WIDTH_F - 45.0f, 20.0f, getRenderedPlayer()));
    Add(stack = NEW ActionStack(go));
    Add(combat = NEW GuiCombat(go));
    Add(action);
    Add(mCardSelector);
    Add(hand = NEW GuiHandSelf(go, getRenderedPlayer()->game->hand));
    Add(avatars = NEW GuiAvatars(this));
    Add(NEW GuiHandOpponent(go, getRenderedPlayerOpponent()->game->hand));
    Add(NEW GuiPlay(this));
    Add(NEW GuiPhaseBar(this));
    Add(NEW GuiFrame(go));
    Add(NEW GuiBackground(go));
}

DuelLayers::~DuelLayers()
{
    int _nbitems = nbitems;
    nbitems = 0;
    for (int i = 0; i < _nbitems; ++i)
    {
        if (objects[i] != mCardSelector)
        {
            SAFE_DELETE(objects[i]);
            objects[i] = NULL;
        }
    }

    for (size_t i = 0; i < waiters.size(); ++i)
        delete (waiters[i]);
    observer->mTrash->cleanup();

    SAFE_DELETE(mCardSelector);
}

void DuelLayers::Add(GuiLayer * layer)
{
    objects.push_back(layer);
    nbitems++;
}

void DuelLayers::Remove()
{
    --nbitems;
}

void DuelLayers::Render()
{
    bool focusMakesItThrough = true;
    for (int i = 0; i < nbitems; ++i)
    {
        objects[i]->hasFocus = focusMakesItThrough;
        if (objects[i]->modal)
            focusMakesItThrough = false;
    }
    for (int i = nbitems - 1; i >= 0; --i)
    {
        objects[i]->Render();
    }
    // (No on-screen Next Phase button: tapping the top-right phase text advances the
    // phase. No on-screen menu button: the system Back gesture / edge swipe opens the
    // game menu. See CheckUserInput.)

    // Cancel button: shown while choosing a target so an accidental cast/ability can be
    // backed out of. A spell cast from hand targets via the game's targetChooser with no
    // ActionElement waiting (canCancel() is false there), so also show it whenever a
    // targetChooser is active and nothing is forcing the target (isWaitingForAnswer NULL).
    if (observer && observer->getCurrentTargetChooser() && action &&
        (action->canCancel() || !action->isWaitingForAnswer()))
    {
        JRenderer * r = JRenderer::GetInstance();
        WFont * f = WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT);
        float cx, cy, cw, ch;
        getCancelRect(cx, cy, cw, ch);
        r->FillRoundRect(cx, cy, cw, ch, 3.0f, ARGB(255, 140, 23, 23));
        r->DrawRoundRect(cx, cy, cw, ch, 3.0f, ARGB(255, 255, 255, 255));
        if (f)
        {
            f->SetScale(1.0f);
            f->SetColor(ARGB(255, 255, 255, 255));
            float ty = cy + (ch - f->GetHeight()) * 0.5f; // vertically centered in the button
            f->DrawString(_("Cancel"), cx + cw * 0.5f, ty, JGETEXT_CENTER);
        }
    }
}

int DuelLayers::receiveEvent(WEvent * e)
{

#if 0
#define PRINT_IF(type) { type *foo = dynamic_cast<type*>(e); if (foo) cout << "Is a " #type " " << *foo << endl; }
    cout << "Received event " << e << " ";
    PRINT_IF(WEventZoneChange);
    PRINT_IF(WEventDamage);
    PRINT_IF(WEventPhaseChange);
    PRINT_IF(WEventCardUpdate);
    PRINT_IF(WEventCardTap);
    PRINT_IF(WEventCreatureAttacker);
    PRINT_IF(WEventCreatureBlocker);
    PRINT_IF(WEventCreatureBlockerRank);
    PRINT_IF(WEventCombatStepChange);
    PRINT_IF(WEventEngageMana);
    PRINT_IF(WEventConsumeMana);
    PRINT_IF(WEventEmptyManaPool);
#endif

    int used = 0;
    for (int i = 0; i < nbitems; ++i)
        used |= objects[i]->receiveEventPlus(e);
    if (!used)
    {
        Pos* p;
        if (WEventZoneChange *event = dynamic_cast<WEventZoneChange*>(e))
        {
            MTGCardInstance* card = event->card;
            if (card->view)
                waiters.push_back(p = NEW Pos(*(card->view)));
            else
                waiters.push_back(p = NEW Pos(0, 0, 0, 0, 255));
            const Pos* ref = card->view;
            while (card)
            {
                if (ref == card->view)
                    card->view = p;
                card = card->next;
            }
        }
    }
    for (int i = 0; i < nbitems; ++i)
        objects[i]->receiveEventMinus(e);

    if (WEventPhaseChange *event = dynamic_cast<WEventPhaseChange*>(e))
        if (MTG_PHASE_BEFORE_BEGIN == event->to->id)
            observer->mTrash->cleanup();

    return 1;
}

float DuelLayers::RightBoundary()
{
    return MIN (hand->LeftBoundary(), avatars->LeftBoundarySelf());
}

Player* DuelLayers::getRenderedPlayer()
{
    return observer->players[mPlayerViewIndex]; 
};

Player* DuelLayers::getRenderedPlayerOpponent()
{ 
    return observer->players[mPlayerViewIndex]->opponent(); 
};
