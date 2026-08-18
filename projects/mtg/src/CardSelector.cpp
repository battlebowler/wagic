#include "PrecompiledHeader.h"

#include "PlayGuiObject.h"
#include "CardGui.h"
#include "CardSelector.h"
#include "GuiHand.h"
#include "Closest.cpp"
#include "GameObserver.h"

// Frames the big battlefield card preview stays up after the last interaction with it.
// Kept short so it reads as a quick peek and doesn't linger over the board (was 800).
static const float kPreviewFrames = 90.0f;

struct CardSelectorLeft: public Exp
{
    static inline bool test(CardSelector::Target* ref, CardSelector::Target* test)
    {
        return ref->x - test->x > fabs(ref->y - test->y);
    }
};
struct CardSelectorRight: public Exp
{
    static inline bool test(CardSelector::Target* ref, CardSelector::Target* test)
    {
        return test->x - ref->x > fabs(ref->y - test->y);
    }
};
struct CardSelectorUp: public Exp
{
    static inline bool test(CardSelector::Target* ref, CardSelector::Target* test)
    {
        return ref->y - test->y > fabs(ref->x - test->x);
    }
};
struct CardSelectorDown: public Exp
{
    static inline bool test(CardSelector::Target* ref, CardSelector::Target* test)
    {
        return test->y - ref->y > fabs(ref->x - test->x);
    }
};
struct CardSelectorDiff: public Exp
{
    static inline bool test(CardSelector::Target* ref, CardSelector::Target* test)
    {
        return ref != test;
    }
};
struct CardSelectorTrue: public Exp
{
    static inline bool test(CardSelector::Target*, CardSelector::Target*)
    {
        return true;
    }
};

CardSelector::SelectorMemory::SelectorMemory(PlayGuiObject* object) :
        object(object)
{
    if (object)
    {
        x = object->x;
        y = object->y;
    }
}
CardSelector::SelectorMemory::SelectorMemory()
{
    object = NULL;
    x = y = 0;
}

CardSelector::CardSelector(GameObserver *observer, DuelLayers* duel) :
        CardSelectorBase(observer), active(NULL), duel(duel), limitor(NULL), bigpos(300, 145, 1.0, 0.0, 220), timer(0.0f)
{
}

void CardSelector::Add(CardSelector::Target* target)
{
    if (NULL == active)
        if (NULL == limitor || limitor->select(active))
            active = target;
    CardView* c = dynamic_cast<CardView*> (target);
    if (c)
        c->zoom = 1.0f;
    c = dynamic_cast<CardView*> (active);
    if (c)
        c->zoom = 1.4f;
    cards.push_back(target);
}

void CardSelector::Remove(CardSelector::Target* card)
{
    for (vector<Target*>::iterator it = cards.begin(); it != cards.end(); ++it)
        if (card == *it)
        {
            if (active == *it)
            {
                CardView* c = dynamic_cast<CardView*> (active);
                if (c)
                    c->zoom = 1.0f;
                active = closest<CardSelectorDiff> (cards, limitor, active);
                c = dynamic_cast<CardView*> (active);
                if (c)
                    c->zoom = 1.4f;
            }
            if (active == *it)
                active = NULL;
            cards.erase(it);
            return;
        }
}

CardSelector::Target* CardSelector::fetchMemory(SelectorMemory& memory)
{
    if (NULL == memory.object)
        return NULL;
    for (vector<Target*>::iterator it = cards.begin(); it != cards.end(); ++it)
        if (*it == memory.object)
        {
            if ((NULL == limitor) || (limitor->select(memory.object)))
                return memory.object;
            else
                break;
        }
    // We come here if the card is not in the selector any more, or if
    // it is there but it is now refused by the limitor.
    return closest<CardSelectorTrue> (cards, limitor, memory.x, memory.y);
}

void CardSelector::Push()
{
    memoryStack.push(SelectorMemory(active));
}

void CardSelector::Pop()
{
    Target* oldactive = active;
    if (!memoryStack.empty())
    {
        active = fetchMemory(memoryStack.top());
        memoryStack.pop();
        CardView::SelectorZone oldowner;
        if (CardView *q = dynamic_cast<CardView*>(oldactive))
            oldowner = q->owner;
        else
            oldowner = CardView::nullZone;
        if (CardView::nullZone != oldowner)
            lasts[oldowner] = SelectorMemory(oldactive);
    }
    if (active != oldactive)
    {
        {
            CardView* c = dynamic_cast<CardView*> (oldactive);
            if (c)
                c->zoom = 1.0f;
        } //Is this needed, I think it is one in Leaving(0) ?
        {
            CardView* c = dynamic_cast<CardView*> (active);
            if (c)
                c->zoom = 1.4f;
        } //Is this needed, I think it is one in Entering() ?
        if (oldactive)
            oldactive->Leaving(JGE_BTN_NONE);
        if (active)
            active->Entering();
    }
}

bool CardSelector::CheckUserInput(JButton key)
{
    if (!active)
    {
        for (vector<Target*>::iterator it = cards.begin(); it != cards.end(); ++it)
            if ((NULL == limitor) || (limitor->select(*it)))
            {
                active = *it;
                active->Entering();
                return true;
            }
        return true;
    }
    Target* oldactive = active;
    int x,y;
    JGE* jge = observer->getInput();
    if(!jge) return false;

    if(jge->GetLeftClickCoordinates(x, y))
    {
        // Touch-first: a tap acts only on the element it actually lands on, rather
        // than snapping selection to the nearest card. Empty-space taps do nothing.
        Target* tapped = hitTest<Target> (cards, limitor, static_cast<float> (x), static_cast<float> (y));
        // Consume the tap BEFORE dispatching. Rules like MTGAttackRule::reactToClick
        // re-enter CheckUserInput (e.g. with JGE_BTN_RIGHT) from inside ButtonPressed;
        // if the click coordinates were still pending, that re-entrant call would
        // re-dispatch the same tap and recurse forever (stack overflow / crash).
        jge->LeftClickedProcessed();
        if (tapped)
        {
            // one-tap direct action: select the tapped card and activate it now.
            active = tapped;
            observer->ButtonPressed(active);
            goto switch_active;
        }
        return true; // consumed the tap; nothing was under the finger.
    }

    switch (key)
    {
        case JGE_BTN_SEC:
            observer->cancelCurrentAction();
            goto switch_active;
            break;
        case JGE_BTN_OK:
            observer->ButtonPressed(active);
            goto switch_active;
            break;
        case JGE_BTN_LEFT:
            active = closest<CardSelectorLeft> (cards, limitor, active);
            break;
        case JGE_BTN_RIGHT:
            active = closest<CardSelectorRight> (cards, limitor, active);
            break;
        case JGE_BTN_UP:
            active = closest<CardSelectorUp> (cards, limitor, active);
            break;
        case JGE_BTN_DOWN:
            active = closest<CardSelectorDown> (cards, limitor, active);
            break;
        case JGE_BTN_CANCEL:
            mDrawMode = (mDrawMode + 1) % DrawMode::kNumDrawModes;
            if (mDrawMode == DrawMode::kText)
                options[Options::DISABLECARDS].number = 1;
            else
                options[Options::DISABLECARDS].number = 0;
            return true;
        default:
            // Taps are handled above via hit-testing; any other key we don't
            // recognise here is not ours to consume.
            return false;
    }
    if(key != JGE_BTN_NONE)
    {
        if (active != oldactive)
        {
            CardView::SelectorZone oldowner, owner;
            if (CardView *q = dynamic_cast<CardView*>(oldactive))
                oldowner = q->owner;
            else
                oldowner = CardView::nullZone;
            if (CardView *q = dynamic_cast<CardView*>(active))
                owner = q->owner;
            else
                owner = CardView::nullZone;
            if (oldowner != owner)
            {
                if (CardView::nullZone != owner)
                {
                    if (PlayGuiObject* old = fetchMemory(lasts[owner]))
                        switch (key)
                        {
                            case JGE_BTN_LEFT:
                                if (old->x < oldactive->x)
                                    active = old;
                                break;
                            case JGE_BTN_RIGHT:
                                if (old->x > oldactive->x)
                                    active = old;
                                break;
                            case JGE_BTN_UP:
                                if (old->y < oldactive->y)
                                    active = old;
                                break;
                            case JGE_BTN_DOWN:
                                if (old->y > oldactive->y)
                                    active = old;
                                break;
                            default:
                                if (old)
                                    active = old;
                                break;
                        }
                }
                lasts[oldowner] = SelectorMemory(oldactive);
            }
        }
        else
        {
            // active card hasn't changed - that means we're probably at an edge of the battlefield.
            // check if we're not already a selected avatar - if not, select one depending whether we're going up/down.
            GuiAvatar* avatar = dynamic_cast<GuiAvatar*> (active);
            if (!avatar)
            {
                if (key == JGE_BTN_DOWN)
                {
                    active = duel->GetAvatars()->GetSelf();
                }
                else if (key == JGE_BTN_UP)
                {
                    active = duel->GetAvatars()->GetOpponent();
                }
            }
        }
    }

    switch_active:
    if (active != oldactive)
    {
        {
            PlayGuiObject* c = dynamic_cast<PlayGuiObject*> (oldactive);
            if (c)
                c->zoom = 1.0f;
        }
        {
            PlayGuiObject* c = dynamic_cast<PlayGuiObject*> (active);
            if (c)
                c->zoom = 1.4f;
        }
        if (oldactive)
            oldactive->Leaving(JGE_BTN_NONE);
        if (active)
            active->Entering();
        timer = kPreviewFrames; // briefly preview the newly focused card
    }
    else
    {
        // Re-tapping the already-focused avatar toggles its zone rail: hide it if it's shown
        // (a second tap "backs out"), reveal it again on the next tap. mHasFocus tracks the
        // revealed state (GuiAvatars::Activate/Deactivate set it). Non-avatars just preview.
        // Not while targeting: there a tap on an avatar targets that player, so don't steal it.
        if (GuiAvatar * av = dynamic_cast<GuiAvatar*> (active))
        {
            if (!observer || !observer->getCurrentTargetChooser())
            {
                if (av->mHasFocus)
                    av->Leaving(JGE_BTN_NONE); // -> GuiAvatars::Deactivate (hide the rail)
                else
                    av->Entering();            // -> GuiAvatars::Activate (reveal the rail)
            }
        }
        timer = kPreviewFrames;
    }
    return true;
}

void CardSelector::HoverAt(float x, float y)
{
    // Finger-anchored browsing: move the selection to the card/element under the finger
    // as it drags, showing its preview, without activating it (tapping activates).
    Target* hit = hitTest<Target> (cards, limitor, x, y);
    if (!hit)
        return;

    timer = kPreviewFrames; // keep the preview alive while the finger hovers a card
    if (hit == active)
        return;

    Target* oldactive = active;
    active = hit;
    {
        PlayGuiObject* c = dynamic_cast<PlayGuiObject*> (oldactive);
        if (c)
            c->zoom = 1.0f;
    }
    {
        PlayGuiObject* c = dynamic_cast<PlayGuiObject*> (active);
        if (c)
            c->zoom = 1.4f;
    }
    if (oldactive)
        oldactive->Leaving(JGE_BTN_NONE);
    if (active)
        active->Entering();
}

void CardSelector::ClearPreview()
{
    timer = 0; // stop drawing the big card preview
}

void CardSelector::Update(float dt)
{
    // Scale for the big card preview (0.50 = slightly smaller than the old 0.65)
    float scale = 0.85f;

    // DrawCard() applies an internal displayScale (250/BigHeight) on top of zoom,
    // so the actual rendered size is: zoom * 250 tall, zoom * 250 * BigWidth/BigHeight wide
    float renderedH = scale * 250.0f;
    float renderedW = scale * 250.0f * CardGui::BigWidth / CardGui::BigHeight;

    // Position the preview just above the horizontal hand. Shifted left of the old 0.75 so
    // it clears the always-visible zone button rail now on the right edge.
    float handTop = SCREEN_HEIGHT_F - 30.0f - CardGui::Height;
    bigpos.x = SCREEN_WIDTH_F * 0.60f;
    bigpos.y = handTop - 3.0f - renderedH / 11.0f;

    bigpos.zoom = scale;

    bigpos.Update(dt);

    if (timer > 0)
        timer -= 1;
}

void CardSelector::Render()
{
    if (active)
    {
        active->Render();
        if (CardView* card = dynamic_cast<CardView*>(active) )
        {
            //if(timer > 0)
            //card->DrawCard(bigpos, mDrawMode);
            if(timer > 0)
            {
                float modx = 0.f;
                Pos npos = Pos(bigpos.x+modx,bigpos.y-4.f,bigpos.zoom-(bigpos.zoom/5),bigpos.t,bigpos.alpha);
                card->DrawCard(npos, mDrawMode);
            }
        }
    }
}

void CardSelector::Limit(LimitorFunctor<PlayGuiObject>* limitor, CardView::SelectorZone destzone)
{
    this->limitor = limitor;
    if (limitor && !limitor->select(active))
    {
        PlayGuiObject* oldactive = active;
        CardView::SelectorZone oldowner;
        if (CardView *q = dynamic_cast<CardView*>(oldactive))
            oldowner = q->owner;
        else
            oldowner = CardView::nullZone;
        if (oldowner != destzone)
        {
            if (CardView::nullZone != destzone)
                if (PlayGuiObject* old = fetchMemory(lasts[destzone]))
                    active = old;
            lasts[oldowner] = SelectorMemory(oldactive);
        }

        if (limitor && !limitor->select(active))
        {
            active = NULL;
            for (vector<PlayGuiObject*>::iterator it = cards.begin(); it != cards.end(); ++it)
                if (limitor->select(*it))
                {
                    active = *it;
                    break;
                }
        }

        if (active != oldactive)
        {
            {
                CardView* c = dynamic_cast<CardView*> (oldactive);
                if (c)
                    c->zoom = 1.0f;
            }
            {
                CardView* c = dynamic_cast<CardView*> (active);
                if (c)
                    c->zoom = 1.4f;
            }
            if (oldactive)
                oldactive->Leaving(JGE_BTN_NONE);
            if (active)
                active->Entering();
        }
    }
}

void CardSelector::PushLimitor()
{
    if (NULL == limitor)
        return;
    CardView::SelectorZone owner;
    if (CardView *q = dynamic_cast<CardView*>(active))
        owner = q->owner;
    else
        owner = CardView::nullZone;
    limitorStack.push(make_pair(limitor, owner));
}

void CardSelector::PopLimitor()
{
    if (limitorStack.empty())
        return;
    Limit(limitorStack.top().first, limitorStack.top().second);
    limitorStack.pop();
}