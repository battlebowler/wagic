#include "PrecompiledHeader.h"

#include "CardSelector.h"
#include "GameApp.h"
#include "GuiAvatars.h"
#include "GameObserver.h"

#define LIB_GRAVE_OFFSET 230

GuiAvatars::GuiAvatars(DuelLayers* duelLayers) :
        GuiLayer(duelLayers), active(NULL)
{
    const float inset = 18.0f;

    const float ZW = (float) GuiGameZone::Width;   // zone button size
    const float ZH = (float) GuiGameZone::Height;
    const float ZG = 2.0f;                          // gap between zone buttons

    // Zones are hidden until you tap your avatar (see Activate/Deactivate); when revealed
    // they appear as a vertical column on the RIGHT edge (yours) / LEFT edge (opponent's),
    // clear of the center card preview and the bottom hand.
    const float railGap = ZH + ZG;

    // ---- You (bottom-right avatar) ----
    Add(self = NEW GuiAvatar(SCREEN_WIDTH - inset, SCREEN_HEIGHT - inset, false, mpDuelLayers->getRenderedPlayer(), GuiAvatar::BOTTOM_RIGHT, this));
    self->zoom = 0.9f;
    {
        float sX  = SCREEN_WIDTH - ZW - 4.0f;   // far right edge
        float sY0 = 30.0f;                       // below the top-right phase-tap zone
        Add(selfLibrary     = NEW GuiLibrary    (sX, sY0 + 0 * railGap, false, mpDuelLayers->getRenderedPlayer(), this));
        Add(selfGraveyard   = NEW GuiGraveyard  (sX, sY0 + 1 * railGap, false, mpDuelLayers->getRenderedPlayer(), this));
        Add(selfExile       = NEW GuiExile      (sX, sY0 + 2 * railGap, false, mpDuelLayers->getRenderedPlayer(), this));
        Add(selfCommandZone = NEW GuiCommandZone(sX, sY0 + 3 * railGap, false, mpDuelLayers->getRenderedPlayer(), this));
        Add(selfSideboard   = NEW GuiSideboard  (sX, sY0 + 4 * railGap, false, mpDuelLayers->getRenderedPlayer(), this));
    }

    // ---- Opponent (top-left avatar) ----
    Add(opponent = NEW GuiAvatar(inset, inset, false, mpDuelLayers->getRenderedPlayerOpponent(), GuiAvatar::TOP_LEFT, this));
    opponent->zoom = 0.9f;
    {
        float oX  = 2.0f;                                       // far left edge
        float oY0 = inset + GuiAvatar::Height + 12.0f;          // below the (enlarged) opponent avatar
        Add(opponentLibrary     = NEW GuiLibrary    (oX, oY0 + 0 * railGap, false, mpDuelLayers->getRenderedPlayerOpponent(), this));
        Add(opponentGraveyard   = NEW GuiGraveyard  (oX, oY0 + 1 * railGap, false, mpDuelLayers->getRenderedPlayerOpponent(), this));
        Add(opponentExile       = NEW GuiExile      (oX, oY0 + 2 * railGap, false, mpDuelLayers->getRenderedPlayerOpponent(), this));
        Add(opponentCommandZone = NEW GuiCommandZone(oX, oY0 + 3 * railGap, false, mpDuelLayers->getRenderedPlayerOpponent(), this));
        Add(opponentHand        = NEW GuiOpponentHand(oX, oY0 + 4 * railGap, false, mpDuelLayers->getRenderedPlayerOpponent(), this));
    }

    observer->getCardSelector()->Add(self);
    observer->getCardSelector()->Add(selfGraveyard);
    observer->getCardSelector()->Add(selfExile);
    observer->getCardSelector()->Add(selfCommandZone);
    observer->getCardSelector()->Add(selfLibrary);
    observer->getCardSelector()->Add(selfSideboard);
    observer->getCardSelector()->Add(opponent);
    observer->getCardSelector()->Add(opponentGraveyard);
    observer->getCardSelector()->Add(opponentExile);
    observer->getCardSelector()->Add(opponentCommandZone);
    observer->getCardSelector()->Add(opponentLibrary);
    observer->getCardSelector()->Add(opponentHand);
    // Hidden until the owning avatar is tapped (Activate reveals the whole column at 230,
    // Deactivate hides it again). alpha 0 also makes them un-tappable (Closest.cpp skips
    // actA<32), so they don't intercept taps while hidden.
    selfLibrary->alpha = selfGraveyard->alpha = selfExile->alpha = selfCommandZone->alpha = selfSideboard->alpha = 0.0f;
    opponentLibrary->alpha = opponentGraveyard->alpha = opponentExile->alpha = opponentCommandZone->alpha = opponentHand->alpha = 0.0f;
}

float GuiAvatars::LeftBoundarySelf()
{
    return SCREEN_WIDTH - 10;
}

GuiAvatars::~GuiAvatars()
{
}

void GuiAvatars::Activate(PlayGuiObject* c)
{
    c->zoom = 1.2f;
    c->mHasFocus = true;

    // Tapping an avatar (or one of its zones) reveals that player's whole zone column.
    if ((opponentGraveyard == c) || (opponentExile == c) || (opponentCommandZone == c) || (opponentLibrary == c) || (opponent == c) || (opponentHand == c))
    {
        opponentLibrary->alpha = opponentGraveyard->alpha = opponentExile->alpha = opponentCommandZone->alpha = opponentHand->alpha = 230.0f;
        active = opponent;
        opponent->zoom = 1.2f;
    }
    else if ((selfGraveyard == c) || (selfExile == c) || (selfCommandZone == c) || (selfSideboard == c) || (selfLibrary == c) || (self == c))
    {
        selfLibrary->alpha = selfGraveyard->alpha = selfExile->alpha = selfCommandZone->alpha = selfSideboard->alpha = 230.0f;
        self->zoom = 1.0f;
        active = self;
    }
    if (opponent != c && self != c)
        c->alpha = 255.0f; // highlight the focused zone button
}
void GuiAvatars::Deactivate(PlayGuiObject* c)
{
    c->zoom = 1.0;
    c->mHasFocus = false;
    // Tapping away from the rail also closes any open zone card-list and clears the big
    // card preview, so it doesn't linger after the zone view is dismissed.
    if (observer)
    {
        if (observer->getCardSelector())
            observer->getCardSelector()->ClearPreview();
        if (observer->guiOpenDisplay && c == observer->guiOpenDisplay)
            observer->guiOpenDisplay->toggleDisplay();
    }
    // Leaving the avatar/its zones hides that player's whole column again. Clear mHasFocus on
    // EVERY zone in the column, not just `c`: otherwise a zone that was focused stays flagged
    // (its pulsing white highlight in GuiGameZone::Render never clears) when the rail is hidden
    // via the avatar toggle (which deactivates the avatar, not the focused zone) and then shown
    // again.
    if ((opponentGraveyard == c) || (opponentExile == c) || (opponentCommandZone == c) || (opponentLibrary == c) || (opponentHand == c) || (opponent == c))
    {
        opponentLibrary->alpha = opponentGraveyard->alpha = opponentExile->alpha = opponentCommandZone->alpha = opponentHand->alpha = 0.0f;
        opponentLibrary->mHasFocus = opponentGraveyard->mHasFocus = opponentExile->mHasFocus =
            opponentCommandZone->mHasFocus = opponentHand->mHasFocus = opponent->mHasFocus = false;
        opponent->zoom = 0.9f;
        active = NULL;
    }
    else if ((selfGraveyard == c) || (selfExile == c) || (selfCommandZone == c) || (selfSideboard == c) || (selfLibrary == c) || (self == c))
    {
        selfLibrary->alpha = selfGraveyard->alpha = selfExile->alpha = selfCommandZone->alpha = selfSideboard->alpha = 0.0f;
        selfLibrary->mHasFocus = selfGraveyard->mHasFocus = selfExile->mHasFocus =
            selfCommandZone->mHasFocus = selfSideboard->mHasFocus = self->mHasFocus = false;
        self->zoom = 0.5f;
        active = NULL;
    }
}

int GuiAvatars::receiveEventPlus(WEvent* e)
{
    return selfGraveyard->receiveEventPlus(e) | selfExile->receiveEventPlus(e) | selfSideboard->receiveEventPlus(e) | selfCommandZone->receiveEventPlus(e) | opponentExile->receiveEventPlus(e) | opponentCommandZone->receiveEventPlus(e) | opponentGraveyard->receiveEventPlus(e) | opponentHand->receiveEventPlus(e);
}

int GuiAvatars::receiveEventMinus(WEvent* e)
{
    selfGraveyard->receiveEventMinus(e);
    selfExile->receiveEventMinus(e);
    selfCommandZone->receiveEventMinus(e);
    selfSideboard->receiveEventMinus(e);
    opponentGraveyard->receiveEventMinus(e);
    opponentExile->receiveEventMinus(e);
    opponentCommandZone->receiveEventMinus(e);
    opponentHand->receiveEventMinus(e);
    return 1;
}

bool GuiAvatars::CheckUserInput(JButton key)
{
    if (self->CheckUserInput(key))
        return true;
    if (opponent->CheckUserInput(key))
        return true;
    if (selfGraveyard->CheckUserInput(key))
        return true;
    if (selfExile->CheckUserInput(key))
        return true;
    if (selfCommandZone->CheckUserInput(key))
        return true;
    if (selfSideboard->CheckUserInput(key))
        return true;
    if (opponentGraveyard->CheckUserInput(key))
        return true;
    if (opponentExile->CheckUserInput(key))
        return true;
    if (opponentCommandZone->CheckUserInput(key))
        return true;
    if (opponentHand->CheckUserInput(key))
        return true;
    if (selfLibrary->CheckUserInput(key))
        return true;
    if (opponentLibrary->CheckUserInput(key))
        return true;
    return false;
}

void GuiAvatars::Update(float dt)
{
    // All zone buttons stay visible at all times (empty ones included) — their fixed
    // positions come from the constructor and their alpha stays at 230.
    self->Update(dt);
    opponent->Update(dt);
    selfGraveyard->Update(dt);
    selfExile->Update(dt);
    selfCommandZone->Update(dt);
    selfSideboard->Update(dt);
    opponentHand->Update(dt);
    opponentGraveyard->Update(dt);
    opponentExile->Update(dt);
    opponentCommandZone->Update(dt);
    selfLibrary->Update(dt);
    opponentLibrary->Update(dt);
}

void GuiAvatars::Render()
{
    JRenderer * r = JRenderer::GetInstance();
    float w = 54;
    float h = 54;
    if (opponent == active)
    {
        r->FillRect(opponent->actX, opponent->actY, 40 * opponent->actZ, h+25 * opponent->actZ, ARGB(200,0,0,0));
        r->FillRect(opponent->actX, opponent->actY, w * opponent->actZ, h+25 * opponent->actZ, ARGB(200,0,0,0));
    }
    else if (self == active)
    {
        r->FillRect(self->actX - w * self->actZ - 4.5f, self->actY - h-28 * self->actZ, 24 * self->actZ + 35, h+28 * self->actZ, ARGB(200,0,0,0));
        r->FillRect(self->actX - w * self->actZ - 4.5f, self->actY - h * self->actZ, w * self->actZ, h * self->actZ, ARGB(200,0,0,0));
    }
    GuiLayer::Render();

}

GuiAvatar* GuiAvatars::GetSelf()
{
    return self;
}

GuiAvatar* GuiAvatars::GetOpponent()
{
    return opponent;
}
