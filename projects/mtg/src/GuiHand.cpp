#include "PrecompiledHeader.h"

#include "CardSelector.h"
#include "CardSelector.h"
#include "GameApp.h"
#include "Trash.h"
#include "GuiHand.h"
#include "OptionItem.h"

const float GuiHand::ClosedRowX = 459;
const float GuiHand::LeftRowX = 420;
const float GuiHand::RightRowX = 460;

const float GuiHand::OpenX = 394;
const float GuiHand::ClosedX = 494;
const float GuiHand::OpenY = SCREEN_HEIGHT - 50;
const float GuiHand::ClosedY = SCREEN_HEIGHT;

bool HandLimitor::select(Target* t)
{
    if (CardView* c = dynamic_cast<CardView*>(t))
        return hand->isInHand(c);
    else
        return false;
}
bool HandLimitor::greyout(Target*)
{
    return true;
}
HandLimitor::HandLimitor(GuiHand* hand) :
    hand(hand)
{
}

GuiHand::GuiHand(GameObserver* observer, MTGHand* hand) :
    GuiLayer(observer), hand(hand)
{
    if(observer->getResourceManager())
    {
        back = observer->getResourceManager()->RetrieveTempQuad("handback.png");
        if (back.get())
            back->SetTextureRect(1, 0, 100, 250);
        else
            GameApp::systemError = "Error loading hand texture : " __FILE__;
    }
}

GuiHand::~GuiHand()
{
        for (vector<CardView*>::iterator it = cards.begin(); it != cards.end(); ++it)
            delete (*it);
}

void GuiHand::Update(float dt)
{
    for (vector<CardView*>::iterator it = cards.begin(); it != cards.end(); ++it)
        (*it)->Update(dt);
}

bool GuiHand::isInHand(CardView* card)
{
    vector<CardView*>::iterator it;
    it = find(cards.begin(), cards.end(), card);
    return (it != cards.end());
}

GuiHandOpponent::GuiHandOpponent(GameObserver* observer, MTGHand* hand) :
    GuiHand(observer, hand)
{
    vector<MTGCardInstance *>::iterator ite;
    for(ite = hand->cards.begin(); ite != hand->cards.end(); ite++)
    {
        WEventZoneChange event(*ite, NULL, hand);
        receiveEventPlus(&event);
    }
}

void GuiHandOpponent::Render()
{
    JQuadPtr quad = WResourceManager::Instance()->GetQuad(kGenericCardThumbnailID);

    float x = 45;
    for (vector<CardView*>::iterator it = cards.begin(); it != cards.end(); ++it)
    {
        (*it)->x = x;
        (*it)->y = 2;
        (*it)->zoom = 0.3f;
        (*it)->Render(quad.get());
        if(cards.size() > 12)
            x += 240/cards.size();
        else
            x += 18;
    }
}

GuiHandSelf::GuiHandSelf(GameObserver* observer, MTGHand* hand) :
    GuiHand(observer, hand), state(Closed), mHidden(false), mRowCenterY(SCREEN_HEIGHT_F - 34.0f), backpos(ClosedX, SCREEN_HEIGHT - 250, 1.0, 0, 255)
{
    limitor = NEW HandLimitor(this);
    if (OptionHandDirection::HORIZONTAL == options[Options::HANDDIRECTION].number)
    {
        backpos.t = M_PI / 2;
        backpos.y = ClosedY;
        backpos.x = SCREEN_WIDTH - 30 * 7 - 14;
        backpos.UpdateNow();
    }

    vector<MTGCardInstance *>::iterator ite;
    for(ite = hand->cards.begin(); ite != hand->cards.end(); ite++)
    {
        WEventZoneChange event(*ite, NULL, hand);
        receiveEventPlus(&event);
    }
}

GuiHandSelf::~GuiHandSelf()
{
    SAFE_DELETE(limitor);
}

void GuiHandSelf::Repos()
{
    float y = 48.0;

    // Handle-collapsed: park every card off the right edge so it neither draws nor hit-tests,
    // freeing the battlefield behind the hand. UpdateNow so the tap hit-test sees it immediately.
    if (mHidden)
    {
        for (vector<CardView*>::iterator it = cards.begin(); it != cards.end(); ++it)
        {
            (*it)->x = SCREEN_WIDTH + 300.0f;
            (*it)->UpdateNow();
        }
        return;
    }

    if (Closed == state && OptionClosedHand::VISIBLE == options[Options::CLOSEDHAND].number)
    {
        if (OptionHandDirection::HORIZONTAL == options[Options::HANDDIRECTION].number)
        {
            float spacing = 40.0f;   // widened with kSmallCardArtH (larger cards, 50)

            // Shift entire hand left by one card width
            float startX = SCREEN_WIDTH - 30.0f - CardGui::Width;

            // Push up by half card height
            //float yPos = SCREEN_HEIGHT - 30.0f - (CardGui::Height * 0.5f);
            float yPos = SCREEN_HEIGHT - 30.0f;

            for (vector<CardView*>::reverse_iterator it = cards.rbegin(); it != cards.rend(); ++it)
            {
                (*it)->x = startX;
                (*it)->y = yPos;
                startX -= spacing;
            }
        }
        else
        {
            float dist = 236.0f / cards.size();   // widened with kSmallCardArtH (larger cards, 50)
            if (dist > 26)
                dist = 26.0;
            else
                y = 40.0;

            for (vector<CardView*>::iterator it = cards.begin(); it != cards.end(); ++it)
            {
                (*it)->x = ClosedRowX;
                (*it)->y = y;
                y += dist;
            }
        }
    }
    else
    {
        bool q = (Closed == state);

        if (OptionHandDirection::HORIZONTAL == options[Options::HANDDIRECTION].number)
        {
            float cardWidth = CardGui::Width;
            float cardHeight = CardGui::Height;

            // Push rightmost card left by one card width
            float xPos = SCREEN_WIDTH - 30.0f - cardWidth;

            float dist = 316.0f / cards.size();   // widened with kSmallCardArtH so the open row
            if (dist > 40)                         // stays border-to-border at the larger card size (50)
                dist = 40;
            else
                xPos = SCREEN_WIDTH - 15.0f - cardWidth;

            // Baseline hand position near the bottom. A FOCUSED card is lifted separately in
            // GuiHandSelf::Update (so it clears the bottom edge without leaving a big gap under the
            // rest of the hand).
            float yPos = SCREEN_HEIGHT - 34.0f - (cardHeight * 0.5f);

            for (vector<CardView*>::reverse_iterator it = cards.rbegin(); it != cards.rend(); ++it)
            {
                (*it)->x = xPos;
                (*it)->y = yPos;
                xPos -= dist;
                (*it)->alpha = static_cast<float>(q ? 0 : 255);
            }

            backpos.x = xPos + SCREEN_HEIGHT - 14.0f - cardWidth;
        }
        else
        {
            float dist = 294.0f / ((cards.size() + 1) / 2);   // widened with kSmallCardArtH (50)
            if (dist > 85)
                dist = 85;

            bool flip = false;
            for (vector<CardView*>::iterator it = cards.begin(); it != cards.end(); ++it)
            {
                (*it)->x = flip ? RightRowX : LeftRowX;
                (*it)->y = y;
                if (flip)
                    y += dist;
                flip = !flip;
                (*it)->alpha = static_cast<float>(q ? 0 : 255);
            }
        }
    }
}

void GuiHandSelf::getHandleRect(float& x, float& y, float& w, float& h) const
{
    // A tall vertical bar running along the LEFT side of the hand cards -- the seam between the
    // battlefield/avatar and where the hand begins -- so it reads as the hand's own grab edge.
    // Placed from fixed layout constants (not live card x) so it stays put when the hand is
    // collapsed off-screen.
    w = SCREEN_WIDTH_F * (14.0f / 480.0f);
    if (OptionHandDirection::HORIZONTAL == options[Options::HANDDIRECTION].number)
    {
        // Hand runs along the bottom edge; drop a vertical bar matching the hand cards' visible
        // height into the gap between the rightmost hand card and the bottom-right avatar. Wide
        // enough for two rotated (sideways) text lines: "CLOSE"/"OPEN" and "HAND".
        // Sit a bit inside the hand cards' top/bottom, centered on the sampled card center.
        // FillRoundRect draws from y down to y+h+2*radius.
        w = 32.0f;
        h = 42.0f;
        y = mRowCenterY - 23.0f;
        x = SCREEN_WIDTH_F - 92.0f;
    }
    else
    {
        // Vertical hand on the right: a tall bar along the hand column's left edge.
        const float handLeft = LeftRowX - CardGui::Width * 0.5f; // left edge of the hand column
        x = handLeft - w - 2.0f;
        y = 40.0f;
        h = SCREEN_HEIGHT_F - 40.0f - y;
    }
}

void GuiHandSelf::ToggleHidden()
{
    mHidden = !mHidden;
    Repos(); // park cards off-screen (hidden) or lay them back out (shown)
}

bool GuiHandSelf::CheckUserInput(JButton key)
{
    JButton trigger = (options[Options::REVERSETRIGGERS].number ? JGE_BTN_PREV : JGE_BTN_NEXT);
    if (trigger == key)
    {
        state = (Open == state ? Closed : Open);
        if (Open == state)
            observer->getCardSelector()->Push();
        observer->getCardSelector()->Limit(Open == state ? limitor : NULL, CardView::handZone);
        if (Closed == state)
            observer->getCardSelector()->Pop();
        if (OptionHandDirection::HORIZONTAL == options[Options::HANDDIRECTION].number)
            backpos.y = Open == state ? OpenY : ClosedY;
        else
            backpos.x = Open == state ? OpenX : ClosedX;
        if (Open == state && OptionClosedHand::INVISIBLE == options[Options::CLOSEDHAND].number)
        {
            if (OptionHandDirection::HORIZONTAL == options[Options::HANDDIRECTION].number)
                for (vector<CardView*>::iterator it = cards.begin(); it != cards.end(); ++it)
                {
                    (*it)->y = SCREEN_HEIGHT + 30;
                    (*it)->UpdateNow();
                }
            else
                for (vector<CardView*>::iterator it = cards.begin(); it != cards.end(); ++it)
                {
                    (*it)->x = SCREEN_WIDTH + 30;
                    (*it)->UpdateNow();
                }
        }
        Repos();
        return true;
    }
    return false;
}

void GuiHandSelf::Update(float dt)
{
    backpos.Update(dt);
    GuiHand::Update(dt);
    // Lift the focused (enlarged) card so its zoomed body + P/T box clears the bottom screen edge.
    // Derived from the card's STABLE target y and its current zoom (actZ animates 1 -> 1.4), so the
    // card rises smoothly and both the render AND the tap hit-test (CardGui::Contains uses actY)
    // stay in sync. Only the focused card moves; the rest of the hand keeps its baseline position.
    for (vector<CardView*>::iterator it = cards.begin(); it != cards.end(); ++it)
    {
        CardView* cv = *it;
        if (cv && cv->actZ > 1.05f)
            cv->actY = cv->y - (cv->actZ - 1.0f) * 40.0f;
    }

    // Sample the row center from a non-enlarged, on-screen card so the handle bar tracks the
    // cards' exact vertical center (the card frame renders centered on the card's y).
    if (!mHidden)
        for (vector<CardView*>::iterator it = cards.begin(); it != cards.end(); ++it)
            if ((*it) && (*it)->actZ < 1.05f && (*it)->x < SCREEN_WIDTH_F)
            {
                mRowCenterY = (*it)->actY; // the card frame renders centered on actY
                break;
            }
}

void GuiHandSelf::Render()
{
    // The tap-to-toggle handle is always drawn (on top of the hand / battlefield) so the player
    // can collapse the hand to reach the cards behind it, then bring it back. No auto-hide.
    {
        JRenderer* r = JRenderer::GetInstance();
        float hx, hy, hw, hh;
        getHandleRect(hx, hy, hw, hh);
        // Match the phase bar: dark fill + warm gold outline.
        r->FillRoundRect(hx, hy, hw - 2.0f, hh, 2.0f, ARGB(235, 16, 16, 20));
        r->DrawRoundRect(hx, hy, hw - 2.0f, hh, 2.0f, ARGB(255, 176, 148, 84));
        WFont* hf = WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT);
        if (hf)
        {
            // Normal (unrotated) text, two lines stacked and centered in the box.
            const char* w1 = mHidden ? "Open" : "Close";
            const char* w2 = "Hand";
            hf->SetRotation(0.0f);
            hf->SetScale(1.0f);
            const float fH = hf->GetHeight();
            const float longer = MAX(hf->GetStringWidth(w1), hf->GetStringWidth(w2));
            float sc = (longer > 0.0f) ? ((hw - 4.0f) * 0.9f / longer) : 1.0f;
            if (sc > 0.85f) sc = 0.85f;
            hf->SetScale(sc);
            hf->SetColor(ARGB(255, 220, 230, 245));

            const float lineH = fH * sc;
            const float cx = hx + (hw - 2.0f) * 0.5f + 2.5f;   // nudge right
            const float top = hy + (hh - 2.0f * lineH) * 0.5f + 1.0f; // nudge down
            hf->DrawString(w1, cx, top, JGETEXT_CENTER);
            hf->DrawString(w2, cx, top + lineH, JGETEXT_CENTER);

            hf->SetScale(1.0f);
        }
    }

    if (mHidden)
        return; // hand collapsed: draw only the handle, nothing else

    //Empty hand
    if (state == Open && cards.size() == 0)
    {
        WFont * mFont = WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT);
        mFont->SetColor(ARGB(255,255,0,0));
        if (OptionHandDirection::HORIZONTAL == options[Options::HANDDIRECTION].number)
        {
            back->SetColor(ARGB(255,255,0,0));
            JRenderer::GetInstance()->RenderQuad(back.get(), backpos.actX, backpos.actY, backpos.actT, backpos.actZ, backpos.actZ);
            back->SetColor(ARGB(255,255,255,255));
            mFont->DrawString("0", SCREEN_WIDTH - 10, backpos.actY);
        }
        else
            backpos.Render(back.get());
        return;
    }

    backpos.Render(back.get());
    if (OptionClosedHand::VISIBLE == options[Options::CLOSEDHAND].number || state == Open)
        for (vector<CardView*>::iterator it = cards.begin(); it != cards.end(); ++it)
            (*it)->Render();
}

float GuiHandSelf::LeftBoundary()
{
    float min = SCREEN_WIDTH + 10;
    if (OptionClosedHand::VISIBLE == options[Options::CLOSEDHAND].number || state == Open)
        for (vector<CardView*>::iterator it = cards.begin(); it != cards.end(); ++it)
            if ((*it)->x - CardGui::Width / 2 < min)
                min = (*it)->x - CardGui::Width / 2;
    return min;
}

int GuiHandSelf::receiveEventPlus(WEvent* e)
{
    if (WEventZoneChange* ev = dynamic_cast<WEventZoneChange*>(e))
        if (hand == ev->to)
        {
            CardView* card;
            if (ev->card->view)
            {

                //fix for http://code.google.com/p/wagic/issues/detail?id=462.
                // We don't want a card in the hand to have an alpha of 0
                ev->card->view->alpha = 255;

                card = NEW CardView(CardView::handZone, ev->card, *(ev->card->view));
            }
            else
                card = NEW CardView(CardView::handZone, ev->card, ClosedRowX, 0);
            card->t = 6 * M_PI;
            cards.push_back(card);
            observer->getCardSelector()->Add(card);
            Repos();
            return 1;
        }
    return 0;
}
int GuiHandSelf::receiveEventMinus(WEvent* e)
{
    if (WEventZoneChange* event = dynamic_cast<WEventZoneChange*>(e))
    {
        if (hand == event->from)
            for (vector<CardView*>::iterator it = cards.begin(); it != cards.end(); ++it)
                if (event->card->previous == (*it)->card)
                {
                    CardView* cv = *it;
                    observer->getCardSelector()->Remove(cv);
                    cards.erase(it);
                    Repos();
                    observer->mTrash->trash(cv);
                    return 1;
                }
        return 1;
    }
    return 0;
}

int GuiHandOpponent::receiveEventPlus(WEvent* e)
{
    if (WEventZoneChange* event = dynamic_cast<WEventZoneChange*>(e))
        if (hand == event->to)
        {
            CardView* card;
            if (event->card->view)
                card = NEW CardView(CardView::handZone, event->card, *(event->card->view));
            else
                card = NEW CardView(CardView::handZone, event->card, ClosedRowX, 0);
            card->alpha = 255;
            card->t = -4 * M_PI;
            cards.push_back(card);
            return 1;
        }
    return 0;
}
int GuiHandOpponent::receiveEventMinus(WEvent* e)
{
    if (WEventZoneChange* event = dynamic_cast<WEventZoneChange*>(e))
    {
        if (hand == event->from)
            for (vector<CardView*>::iterator it = cards.begin(); it != cards.end(); ++it)
                if (event->card->previous == (*it)->card)
                {
                    CardView* cv = *it;
                    cards.erase(it);
                    observer->mTrash->trash(cv);
                    return 1;
                }
        return 0;
    }
    return 0;
}

// I wanna write it like that. GCC doesn't want me to without -O.
// I'm submitting a bug report.
//      it->x = (it->x + (flip ? RightRowX : LeftRowX)) / 2;
