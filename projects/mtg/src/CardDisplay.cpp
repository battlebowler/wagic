#include "PrecompiledHeader.h"

#include "CardDisplay.h"
#include "CardGui.h"
#include "CardSelector.h"
#include "TargetChooser.h"
#include "MTGGameZones.h"
#include "GameObserver.h"

CardDisplay::CardDisplay(GameObserver* game) :
    PlayGuiObjectController(game), mId(0)
{
    tc = NULL;
    listener = NULL;
    nb_displayed_items = 7;
    start_item = 0;
    x = 0;
    y = 0;
    zone = NULL;
}

CardDisplay::CardDisplay(int id, GameObserver* game, int _x, int _y, JGuiListener * _listener, TargetChooser * _tc,
                int _nb_displayed_items) :
    PlayGuiObjectController(game), mId(id), x(_x), y(_y)
{
    tc = _tc;
    listener = _listener;
    nb_displayed_items = _nb_displayed_items;
    start_item = 0;
    if (x + nb_displayed_items * 30 + 25 > SCREEN_WIDTH) x = SCREEN_WIDTH - (nb_displayed_items * 30 + 25);
    if (y + 55 > SCREEN_HEIGHT) y = SCREEN_HEIGHT - 55;
    zone = NULL;
}

void CardDisplay::AddCard(MTGCardInstance * _card)
{
    CardGui * card = NEW CardView(CardView::nullZone, _card, static_cast<float> (x + 20 + (mObjects.size() - start_item) * 30),
                    static_cast<float> (y + 25));
    Add(card);
}

int CardDisplay::thumbAtPoint(float px, float /*py*/) const
{
    // The pack-reveal thumbnails form one horizontal strip, so browse by X alone: a swipe
    // anywhere across the reveal selects the card whose column the finger is over.
    int best = -1;
    float bestDx = 1e9f;
    for (size_t i = 0; i < mObjects.size(); i++)
    {
        CardGui * cg = (CardGui *) mObjects[i];
        if (!cg) continue;
        float dx = fabs(cg->x - px);
        if (dx < bestDx) { bestDx = dx; best = (int) i; }
    }
    return best;
}

void CardDisplay::init(MTGGameZone * zone)
{
    resetObjects();
    if (!zone) return;
    start_item = 0;
    vector<MTGCardInstance*> newCD (zone->cards.rbegin(), zone->cards.rend());
    for (int i = 0; i < zone->nb_cards; i++)//invert display so the top will always be the first one to show
    {
        //AddCard(zone->cards[i]);
        AddCard(newCD[i]);
    }
    if (mObjects.size()) mObjects[0]->Entering();
}

void CardDisplay::rotateLeft()
{
    if (start_item == 0) return;
    for (size_t i = 0; i < mObjects.size(); i++)
    {
        CardGui * cardg = (CardGui *) mObjects[i];
        cardg->x += 30;
    }
    start_item--;
}

void CardDisplay::rotateRight()
{
    if (start_item == (int)(mObjects.size()) - 1) return;
    for (size_t i = 0; i < mObjects.size(); i++)
    {
        CardGui * cardg = (CardGui *) mObjects[i];
        cardg->x -= 30;
    }
    start_item++;
}

void CardDisplay::Update(float dt)
{
    bool update = false;

    if (zone)
    {//invert display so the top will always be the first one to show
        vector<MTGCardInstance*> newCD (zone->cards.rbegin(), zone->cards.rend());
        int size = zone->cards.size();
        for (int i = start_item; i < start_item + nb_displayed_items && i < (int)(mObjects.size()); i++)
        {
            if (i > size - 1)
            {
                update = true;
                break;
            }
            CardGui * cardg = (CardGui *) mObjects[i];
            if (cardg->card != newCD[i]) update = true;
        }
    }
    PlayGuiObjectController::Update(dt);
    if (update)
        init(zone);
}

bool CardDisplay::CheckUserInput(JButton key)
{
    if (JGE_BTN_SEC == key || JGE_BTN_PRI == key || JGE_BTN_UP == key || JGE_BTN_DOWN == key)
    {
        if (listener)
        {
            listener->ButtonPressed(mId, 0);
            return true;
        }
    }
    if (!mObjects.size()) return false;

    if (mActionButton == key)
    {
        if (mObjects[mCurr] && mObjects[mCurr]->ButtonPressed())
        {
            CardGui * cardg = (CardGui *) mObjects[mCurr];
            if (tc)
            {
                tc->toggleTarget(cardg->card);
                return true;
            }
            else
            {
                if (observer) observer->ButtonPressed(cardg);
                return true;
            }
        }
        return true;
    }

    switch (key)
    {
    case JGE_BTN_LEFT:
    {
        int n = mCurr;
        n--;
        if (n < start_item)
        {
            if (n < 0)
            {
                n = 0;
            }
            else
            {
                rotateLeft();
            }
        }
        if (n != mCurr && mObjects[mCurr] != NULL && mObjects[mCurr]->Leaving(JGE_BTN_LEFT))
        {
            mCurr = n;
            mObjects[mCurr]->Entering();
        }
        return true;
    }
    case JGE_BTN_RIGHT:
    {
        int n = mCurr;
        n++;
        if (n >= (int) (mObjects.size()))
        {
            n = mObjects.size() - 1;
        }
        if (n >= start_item + nb_displayed_items)
        {
            rotateRight();
        }
        if (n != mCurr && mObjects[mCurr] != NULL && mObjects[mCurr]->Leaving(JGE_BTN_RIGHT))
        {
            mCurr = n;
            mObjects[mCurr]->Entering();
        }
        return true;
    }
    default:
    {
      bool result = false;
      unsigned int distance2;
      unsigned int minDistance2 = -1;
      int n = mCurr;
      int x1,y1;
      JButton key;
      JGE* jge = observer?observer->getInput():JGE::GetInstance();
      if(jge)
      {
          if (jge->GetLeftClickCoordinates(x1, y1))
          {
              for (size_t i = 0; i < mObjects.size(); i++)
              {
                  float top, left;
                  if (mObjects[i]->getTopLeft(top, left))
                  {
                      distance2 = static_cast<unsigned int>((top - y1) * (top - y1) + (left - x1) * (left - x1));
                      if (distance2 < minDistance2)
                      {
                          minDistance2 = distance2;
                          n = i;
                      }
                  }
              }

              if (n < mCurr)
                  key = JGE_BTN_LEFT;
              else
                  key = JGE_BTN_RIGHT;

              if (n < start_item)
              {
                  rotateLeft();
              }
              else if (n >= (int)(mObjects.size()) && mObjects.size())
              {
                  n = mObjects.size() - 1;
              }
              if (n >= start_item + nb_displayed_items)
              {
                  rotateRight();
              }

              if (n != mCurr && mObjects[mCurr] != NULL && mObjects[mCurr]->Leaving(key))
              {
                  mCurr = n;
                  mObjects[mCurr]->Entering();
                  result = true;
              }
              jge->LeftClickedProcessed();
          }
      }
      return result;
    }
    }

    return false;
}

void CardDisplay::hoverAt(float px, float py)
{
    if (!mObjects.size()) return;

    // Nearest card to the finger (same 2D metric the tap handler uses), then move the
    // selection there so its big preview follows the finger. No click is consumed.
    unsigned int minDistance2 = (unsigned int) -1;
    int n = mCurr;
    for (size_t i = 0; i < mObjects.size(); i++)
    {
        float top, left;
        if (mObjects[i]->getTopLeft(top, left))
        {
            unsigned int distance2 = static_cast<unsigned int>((top - py) * (top - py) + (left - px) * (left - px));
            if (distance2 < minDistance2)
            {
                minDistance2 = distance2;
                n = (int) i;
            }
        }
    }

    JButton key = (n < mCurr) ? JGE_BTN_LEFT : JGE_BTN_RIGHT;
    if (n < start_item)
        rotateLeft();
    else if (n >= start_item + nb_displayed_items)
        rotateRight();

    if (n != mCurr && mObjects[mCurr] != NULL && mObjects[mCurr]->Leaving(key))
    {
        mCurr = n;
        mObjects[mCurr]->Entering();
    }
}


void CardDisplay::Render(bool norect)
{
    //norect - code shop
    WFont * mFont = WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT);
    JRenderer * r = JRenderer::GetInstance();
    //if(norect)
       // r->FillRect(0,0,SCREEN_WIDTH_F,SCREEN_HEIGHT_F,ARGB(180,5,5,5));

    if (norect)
    {
        // ---- Booster pack reveal (redesigned) ----
        // Dim the shop background so the opened cards stand out.
        r->FillRect(0, 0, SCREEN_WIDTH_F, SCREEN_HEIGHT_F, ARGB(224, 8, 8, 12));
        if (!mObjects.size()) return;
        int n = (int) mObjects.size();

        // Thumbnail row of every card in the pack, centered along the bottom (above the
        // credits bar). Tap one to inspect it; the selected one is highlighted.
        float cellW = (SCREEN_WIDTH_F - 24.0f) / n;
        if (cellW > 30.0f) cellW = 30.0f;
        float rowY = SCREEN_HEIGHT_F - 44.0f;
        float startX = (SCREEN_WIDTH_F - n * cellW) / 2.0f + cellW / 2.0f;
        for (int i = 0; i < n; i++)
        {
            if (!mObjects[i]) continue;
            CardGui * cg = (CardGui *) mObjects[i];
            cg->x = startX + i * cellW;
            cg->y = rowY;
            // Only the selected thumbnail is enlarged. Forcing zoom from mCurr every frame
            // avoids stale Entering() zoom (tapping/dragging away always shrinks the rest).
            cg->zoom = (i == mCurr) ? 1.4f : 1.0f;
            if (i == mCurr)
                r->FillRect(cg->x - cellW / 2 + 1, rowY - 24, cellW - 2, 48, ARGB(120, 235, 205, 120));
            cg->Render();
        }

        // Big selected card on the left, with its name + rules text to the right.
        if (mObjects[mCurr])
        {
            CardGui * cardg = (CardGui *) mObjects[mCurr];
            Pos pos = Pos(SCREEN_WIDTH_F * 0.26f, SCREEN_HEIGHT_F * 0.40f, 0.72f, 0.0f, 255);
            int drawMode = observer ? observer->getCardSelector()->GetDrawMode() : DrawMode::kNormal;
            cardg->DrawCard(pos, drawMode);

            float tx = SCREEN_WIDTH_F * 0.52f;
            float colW = SCREEN_WIDTH_F * 0.42f; // right-hand text column width
            mFont->SetColor(ARGB(255, 240, 230, 140));
            mFont->SetScale(1.3f);
            mFont->DrawString(cardg->card->data->name.c_str(), tx, 30);
            mFont->SetScale(0.85f);
            mFont->SetColor(ARGB(255, 235, 235, 235));

            // Skip rules text for basic lands (they only show a mana letter like "w").
            if (cardg->card->getRarity() != Constants::RARITY_L)
            {
                // Rebuild the full text, then word-wrap it manually to the column width
                // (the raw formatted lines are pre-wrapped narrow for the tiny card box).
                string raw;
                const std::vector<string>& txt = cardg->card->data->getFormattedText(true);
                for (std::vector<string>::const_iterator it = txt.begin(); it != txt.end(); ++it)
                {
                    if (!raw.empty()) raw.append(" ");
                    raw.append(*it);
                }
                float lineY = 52.0f;
                float lineH = mFont->GetHeight() + 1.0f;
                size_t p = 0;
                string line;
                while (p < raw.size())
                {
                    size_t sp = raw.find(' ', p);
                    string word = (sp == string::npos) ? raw.substr(p) : raw.substr(p, sp - p);
                    p = (sp == string::npos) ? raw.size() : sp + 1;
                    if (word.empty()) continue;
                    string test = line.empty() ? word : line + " " + word;
                    if (!line.empty() && mFont->GetStringWidth(test.c_str()) > colW)
                    {
                        mFont->DrawString(line.c_str(), tx, lineY);
                        lineY += lineH;
                        line = word;
                    }
                    else
                        line = test;
                }
                if (!line.empty())
                    mFont->DrawString(line.c_str(), tx, lineY);
            }
            mFont->SetScale(1.0f);
            mFont->SetColor(ARGB(255, 255, 255, 255));
        }

        mFont->SetColor(ARGB(180, 205, 205, 205));
        mFont->DrawString("Tap a card to inspect  -  Back to close", SCREEN_WIDTH_F / 2, 8, JGETEXT_CENTER);
        mFont->SetColor(ARGB(255, 255, 255, 255));
        return;
    }

    // No outline box for the in-game zone view — the cards themselves show the boundaries,
    // so the old white rectangle (drawn even for an empty zone) is redundant.
    if (!mObjects.size()) return;
    for (int i = start_item; i < start_item + nb_displayed_items && i < (int)(mObjects.size()); i++)
    {
        if (mObjects[i])
        {
            mObjects[i]->Render();
            if (tc)
            {
                CardGui * cardg = (CardGui *) mObjects[i];
                if (tc->alreadyHasTarget(cardg->card))
                {
                    r->DrawCircle(cardg->x + 5, cardg->y + 5, 5, ARGB(255,255,0,0));
                }
                else if (!tc->canTarget(cardg->card))
                {
                    r->FillRect(cardg->x, cardg->y, 30, 40, ARGB(200,0,0,0));
                }
            }
        }
    }

    //TODO: CardSelector should handle the graveyard and the library in the future...
    if (mObjects.size() && mObjects[mCurr] != NULL)
    {
        mObjects[mCurr]->Render();
        CardGui * cardg = ((CardGui *) mObjects[mCurr]);
        //Pos pos = Pos(CardGui::BigWidth / 2, CardGui::BigHeight / 2 - 10, 1.0, 0.0, 220);
        Pos pos = Pos((CardGui::BigWidth / 2), CardGui::BigHeight / 2 - 10, 0.80f, 0.0, 220);
        
        if(norect)
            pos = Pos((CardGui::BigWidth / 2), CardGui::BigHeight / 2 - 7, 1.0, 0.0, 220);

        int drawMode = DrawMode::kNormal;
        if (observer)
        {
            //pos.actY = 145;
            pos.actY = 142;//reduce y a little
            if (x < (CardGui::BigWidth / 2)) pos.actX = SCREEN_WIDTH - 10 - CardGui::BigWidth / 2;
            drawMode = observer->getCardSelector()->GetDrawMode();
        }
        if(norect)
        {
            mFont->SetColor(ARGB(255,240,230,140));
            mFont->SetScale(1.5f);
            mFont->DrawString(cardg->card->data->name.c_str(),SCREEN_WIDTH_F/2,20);
            mFont->SetColor(ARGB(255,255,255,255));
            mFont->SetScale(1.0f);
            string details = "";
            std::vector<string> txt = cardg->card->data->getFormattedText(true);

            for (std::vector<string>::const_iterator it = txt.begin(); it != txt.end(); ++it)
            {
                details.append("\n");
                details.append(it->c_str());
            }
            mFont->DrawString(details.c_str(),SCREEN_WIDTH_F/2,25);
        }
        cardg->DrawCard(pos, drawMode);
    }
}

ostream& CardDisplay::toString(ostream& out) const
{
    return (out << "CardDisplay ::: x,y : " << x << "," << y << " ; start_item : " << start_item << " ; nb_displayed_items "
                    << nb_displayed_items << " ; tc : " << tc << " ; listener : " << listener);
}

std::ostream& operator<<(std::ostream& out, const CardDisplay& m)
{
    return m.toString(out);
}
