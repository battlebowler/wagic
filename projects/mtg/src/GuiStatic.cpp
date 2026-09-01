#include "PrecompiledHeader.h"

#include "Trash.h"
#include "GuiStatic.h"

GuiStatic::GuiStatic(float desiredHeight, float x, float y, bool hasFocus, GuiAvatars* parent) :
    PlayGuiObject(desiredHeight, x, y, 0, hasFocus), parent(parent)
{
}

void GuiStatic::Entering()
{
    parent->Activate(this);
}

bool GuiStatic::Leaving(JButton)
{
    parent->Deactivate(this);
    return false;
}

GuiAvatar::GuiAvatar(float x, float y, bool hasFocus, Player * player, Corner corner, GuiAvatars* parent) :
    GuiStatic(static_cast<float> (GuiAvatar::Height), x, y, hasFocus, parent), avatarRed(255), currentLife(player->life),
            currentpoisonCount(player->poisonCount), corner(corner), player(player)
{
    type = GUI_AVATAR;
}

void GuiAvatar::Render()
{
    GameObserver * game = player->getObserver();
    JRenderer * r = JRenderer::GetInstance();
    int life = player->life;
    int poisonCount = player->poisonCount;
    int energyCount = player->energyCount;
    int experienceCount = player->experienceCount;
    WFont * mFont = WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT);
    mFont->SetScale(DEFAULT_MAIN_FONT_SCALE);
    TargetChooser * tc = NULL;

    if (game)
        tc = game->getCurrentTargetChooser();

    //Avatar
    int lifeDiff = life - currentLife;
    if (lifeDiff < 0 && currentLife > 0)
    {
        avatarRed = 192 + (3 * 255 * lifeDiff) / currentLife / 4;
        if (avatarRed < 0)
            avatarRed = 0;
    }
    int poisonDiff = poisonCount - currentpoisonCount;
    if (poisonDiff < 0 && currentpoisonCount > 0)
    {
        avatarRed = 192 + (3 * 255 * poisonDiff) / currentpoisonCount / 4;
        if (avatarRed < 0)
            avatarRed = 0;
    }
    currentpoisonCount = poisonCount;
    currentLife = life;

    r->FillRect(actX + 2, actY + 2, Width * actZ, Height * actZ, ARGB((int)(actA / 2), 0, 0, 0));

    float x0 = actX;
    float y0 = actY;

    if (player->getIcon().get())
    {
        if (corner == BOTTOM_RIGHT)
        {
            x0 -= Width * actZ;
            y0 -= Height * actZ;
        }
        switch (corner)
        {
        case TOP_LEFT:
            player->getIcon()->SetHotSpot(0, 0);
            break;
        case BOTTOM_RIGHT:
            player->getIcon()->SetHotSpot(player->getIcon()->mWidth, player->getIcon()->mHeight);
            break;
        }
        player->getIcon()->SetColor(ARGB((int)actA, 255, avatarRed, avatarRed));
        if (tc && !tc->canTarget(player))
        {
            player->getIcon()->SetColor(ARGB((int)actA, 50, 50, 50));
        }
        r->RenderQuad(player->getIcon().get(), actX, actY, actT, Width/player->getIcon()->mWidth*actZ, Height/player->getIcon()->mHeight*actZ);
        if (mHasFocus)
        {
            r->FillRect(x0, x0, Width/player->getIcon()->mWidth * actZ, Height/player->getIcon()->mHeight * actZ, ARGB(abs(128 - wave),255,255,255));
        }
    }

    if (avatarRed < 255)
    {
        avatarRed += 3;
        if (avatarRed > 255)
            avatarRed = 255;
    }

    // (Removed the avatar turn/action outlines: green = whose turn, blue = current action
    // player, red = interrupting player. Kept off for a cleaner battlefield.)

    //Life
    char buffer[10];
    int lx = 255, ly = 255, lz = 255;
    if(life > 24) { lx = 127; ly = 255; lz = 212; }
    if(life > 16 && life < 24) { lx = 255; ly = 255; lz = 255; }
    if(life > 12 && life < 17) { lx = 255; ly = 255; lz = 105; }
    if(life > 8 && life < 13) { lx = 255; ly = 255; lz = 13; }
    if(life > 4 && life < 9) { lx = 255; ly = 166; lz = 0; }
    if(life < 5) { lx = 255; ly = 40; lz = 0; }
    sprintf(buffer, "%i", life);
    switch (corner)
    {
    case TOP_LEFT:
        mFont->SetColor(ARGB((int)actA / 4, 0, 0, 0));
        mFont->DrawString(buffer, actX + 2, actY - 2);
        mFont->SetScale(1.5f);
        mFont->SetColor(ARGB((int)actA, lx, ly, lz));
        mFont->DrawString(buffer, actX + 1, actY - 1);
        mFont->SetScale(1);
        break;
    case BOTTOM_RIGHT:
        mFont->SetScale(1.4f);
        mFont->SetColor(ARGB((int)actA, lx, ly, lz));
        mFont->DrawString(buffer, actX, actY - 14, JGETEXT_RIGHT);
        mFont->SetScale(1);
        break;
    }
    //poison
    char poison[10];
    if (poisonCount > 0)
    {
        sprintf(poison, "%i", poisonCount);
        switch (corner)
        {
        case TOP_LEFT:
            mFont->SetColor(ARGB((int)actA / 1, 0, 255, 0));
            mFont->DrawString(poison, actX + 2, actY + 10);
            break;
        case BOTTOM_RIGHT:
            mFont->SetColor(ARGB((int)actA / 1 ,0, 255, 0));
            mFont->DrawString(poison, actX, actY - 20, JGETEXT_RIGHT);
            break;
        }
    }
    //energy
    char energy[15];
    if (energyCount > 0)
    {
        sprintf(energy, "%i", energyCount);
        switch (corner)
        {
        case TOP_LEFT:
            mFont->SetColor(ARGB((int)actA / 1, 255, 255, 0));
            mFont->DrawString(energy, actX + 2, actY + 17);
            break;
        case BOTTOM_RIGHT:
            mFont->SetColor(ARGB((int)actA / 1 ,255, 255, 0));
            mFont->DrawString(energy, actX, actY - 27, JGETEXT_RIGHT);
            break;
        }
    }
    //experience
    char experience[15];
    if (experienceCount > 0)
    {
        sprintf(experience, "%i", experienceCount);
        switch (corner)
        {
        case TOP_LEFT:
            mFont->SetColor(ARGB((int)actA / 1, 255, 0, 255));
            mFont->DrawString(experience, actX + 2, actY + 24);
            break;
        case BOTTOM_RIGHT:
            mFont->SetColor(ARGB((int)actA / 1 ,255, 0, 255));
            mFont->DrawString(experience, actX - 10, actY - 27, JGETEXT_RIGHT);
            break;
        }
    }
    PlayGuiObject::Render();
}

bool GuiAvatar::Contains(float px, float py) const
{
    // The avatar icon is Width x Height (scaled by actZ). A TOP_LEFT avatar anchors
    // its top-left at (actX, actY); a BOTTOM_RIGHT avatar anchors its bottom-right
    // there. Mirror that so taps hit the icon as drawn.
    float w = Width * actZ;
    float h = Height * actZ;
    float left = actX;
    float top = actY;
    if (corner == BOTTOM_RIGHT)
    {
        left = actX - w;
        top = actY - h;
    }
    return (px >= left && px <= left + w && py >= top && py <= top + h);
}

ostream& GuiAvatar::toString(ostream& out) const
{
    return out << "GuiAvatar ::: avatarRed : " << avatarRed << " ; currentLife : " << currentLife << " ; currentpoisonCount : "
            << currentpoisonCount << " ; player : " << player;
}

void GuiGameZone::toggleDisplay()
{
    if (showCards)
    {
        cd->zone->owner->getObserver()->guiOpenDisplay = NULL;
        showCards = 0;
        cd->zone->owner->getObserver()->OpenedDisplay = NULL;
    }
    else if(!cd->zone->owner->getObserver()->OpenedDisplay)//one display at a time please.
    {
        cd->zone->owner->getObserver()->guiOpenDisplay = this;
        showCards = 1;
        cd->init(zone);
        cd->zone->owner->getObserver()->OpenedDisplay = cd;
    }
}

void GuiGameZone::Render()
{
    // Skip hidden zones entirely: the rail is collapsed (alpha ~0) most of the time, and
    // drawing 10 invisible zone buttons every frame was a large slice of the duel render
    // cost. Zero the tap footprint so a hidden zone also can't be tapped.
    if (actA <= 2.0f)
    {
        width = 0.0f;
        height = 0.0f;
        return;
    }
    //Texture
    JQuadPtr quad = WResourceManager::Instance()->GetQuad(kGenericCardThumbnailID);
    // Remember the generic card-back thumbnail so we can tell whether a REAL zone icon / top
    // card actually replaced it below. If nothing did (e.g. the theme is missing the zone
    // icons), we must NOT draw this generic thumbnail — it fills the whole button with a light
    // card-back, which is the "white box with a stray card image" the rail was showing.
    JQuadPtr genericQuad = quad;
    JQuadPtr overlay;
    float scale = defaultHeight / quad->mHeight;
    float scale2 = scale;
    float modx = 0;
    float mody = 0;

    bool replaced = false;
    bool showtop = (zone && zone->owner->game->battlefield->nb_cards && zone->owner->game->battlefield->hasAbility(Constants::SHOWFROMTOPLIBRARY))?true:false;
    bool showopponenttop = (zone && zone->owner->opponent()->game->battlefield->nb_cards && zone->owner->opponent()->game->battlefield->hasAbility(Constants::SHOWOPPONENTTOPLIBRARY))?true:false;

    quad->SetColor(ARGB((int)(actA),255,255,255));
    if(type == GUI_EXILE || type == GUI_COMMANDZONE || type == GUI_SIDEBOARD)
    {
        quad->SetColor(ARGB((int)(actA),255,240,255));
    }
    
    // Zone icon. Fetched once per zone INSTANCE and cached in mIcon (see the header note): a
    // function-local `static` here survived across games, so after swiping back to the menu it kept
    // pointing at the previous game's freed/reused texture — the stale white box / "weird card image"
    // on the very next game. Instances are recreated per game, so mIcon is naturally fresh. Retry
    // while the texture is still absent (art may not be uploaded yet at game start).
    if (!mIcon || !mIcon->mTex)
    {
        const char* iconFile = NULL;
        switch (type)
        {
            case GUI_LIBRARY:      iconFile = "iconlibrary.png";     break;
            case GUI_OPPONENTHAND: iconFile = "iconhand.png";        break;
            case GUI_GRAVEYARD:    iconFile = "iconcard.png";        break;
            case GUI_EXILE:        iconFile = "iconexile.png";       break;
            case GUI_COMMANDZONE:  iconFile = "iconcommandzone.png"; break;
            case GUI_SIDEBOARD:    iconFile = "iconsideboard.png";   break;
            default: break;
        }
        if (iconFile) mIcon = WResourceManager::Instance()->RetrieveTempQuad(iconFile);
    }
    if (mIcon && mIcon->mTex)
    {
        scale2 = defaultHeight / mIcon->mHeight;
        modx = -0.f;
        mody = -2.f;
        mIcon->SetColor(ARGB((int)(actA),255,255,255));
        quad = mIcon;
    }

    if(type == GUI_LIBRARY && zone->nb_cards && !showCards)
    {
        int top = zone->nb_cards - 1;
        if(zone->cards[top] && (zone->cards[top]->canPlayFromLibrary()||showtop||showopponenttop))
        {
            MTGCardInstance * card = zone->cards[top];
            if(card && card->getObserver())
            {
                replaced = true;
                JQuadPtr kquad = WResourceManager::Instance()->RetrieveCard(card, CACHE_THUMB);
                if(kquad)
                {
                    kquad->SetColor(ARGB((int)(actA),255,255,255));
                    scale2 = defaultHeight / kquad->mHeight;
                    modx = (35/4)+1;
                    mody = (50/4)+1;
                    quad = kquad;
                }
                else
                {
                    quad = CardGui::AlternateThumbQuad(card);
                    if(quad)
                    {
                        quad->SetColor(ARGB((int)(actA),255,255,255));
                        scale2 = defaultHeight / quad->mHeight;
                        modx = (35/4)+1;
                        mody = (50/4)+1;
                    }
                }
            }
        }
    }

    // Button background so each zone reads as an explicit, tappable button (mobile). The
    // whole box (not just the icon) is the tap target — width/height feed Contains().
    {
        JRenderer * br = JRenderer::GetInstance();
        float bw = GuiGameZone::Width * actZ;
        float bh = GuiGameZone::Height * actZ;
        // Flat grey panel matching the interrupt dialog (89,89,89) with a light border;
        // focused zone gets a warm highlight border.
        br->FillRect(actX, actY, bw, bh, ARGB((int)actA, 89, 89, 89));
        br->DrawRect(actX, actY, bw, bh, ARGB((int)actA,
                     mHasFocus ? 240 : 210, mHasFocus ? 220 : 210, mHasFocus ? 120 : 210));
        width = bw;
        height = bh;
    }

    //render small card quad — but only a REAL, TEXTURED zone icon or top-card. Skip it when the
    // quad is the generic card-back fallback OR its texture hasn't loaded (a missing icon in a
    // custom core pack, or art not yet uploaded at game/rematch start). A textureless quad renders
    // as a solid white box, often with stale garbage pixels — the "white stack with a weird card
    // image inside". Better to show just the grey button until a real texture is available.
    if(quad && quad->mTex && quad.get() != genericQuad.get())
    {
        JRenderer::GetInstance()->RenderQuad(quad.get(), actX+modx, actY+mody, 0.0, scale2 * actZ, scale2 * actZ);
    }
    /*if(overlay)
        JRenderer::GetInstance()->RenderQuad(overlay.get(), actX, actY, 0.0, scale2 * actZ, scale2 * actZ);*/

    float x0 = actX;
    if (x0 < SCREEN_WIDTH / 2)
    {
        x0 += 7;
    }

    if (mHasFocus)
    {
        if(!replaced)
            JRenderer::GetInstance()->FillRect(actX, actY, quad->mWidth * scale2 * actZ, quad->mHeight * scale2 * actZ,
                ARGB(abs(128 - wave),255,255,255));
    }

    //Number of cards
    WFont * mFont = WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT);
    mFont->SetScale(DEFAULT_MAIN_FONT_SCALE);
    char buffer[11];
    int mAlpha = (int) (actA);
    /*if(type == GUI_GRAVEYARD)
        sprintf(buffer, "%i\ng", zone->nb_cards);
    else if(type == GUI_LIBRARY)
        sprintf(buffer, "%i\nl", zone->nb_cards);
    else if(type == GUI_OPPONENTHAND)
        sprintf(buffer, "%i\nh", zone->nb_cards);
    else if(type == GUI_EXILE)
        sprintf(buffer, "%i\ne", zone->nb_cards);
    else*/
    sprintf(buffer, "%i", zone->nb_cards);
    mFont->SetColor(ARGB(mAlpha,0,0,0));
    mFont->DrawString(buffer, x0 + 1, actY + 1);
    if (actA > 120)
        mAlpha = 255;
    mFont->SetColor(ARGB(mAlpha,255,255,255));
    mFont->DrawString(buffer, x0, actY);
    
    //show top library - big card display
    if(type == GUI_LIBRARY && mHasFocus && zone->nb_cards && !showCards && replaced)
    {
        int top = zone->nb_cards - 1;
        if(zone->cards[top])
        {
            Pos pos = Pos(SCREEN_WIDTH - 35 - CardGui::BigWidth / 2, CardGui::BigHeight / 2 - 15, 0.80f, 0.0, 220);
            pos.actY = 165;
            if (x < (CardGui::BigWidth / 2)) pos.actX = CardGui::BigWidth / 2;
            CardGui::DrawCard(zone->cards[top], pos);
        }
    }

    if (showCards)
        cd->Render();
    for (vector<CardView*>::iterator it = cards.begin(); it != cards.end(); ++it)
        (*it)->Render();

    PlayGuiObject::Render();
}

void GuiGameZone::ButtonPressed(int, int)
{
    zone->owner->getObserver()->ButtonPressed(this);
}

bool GuiGameZone::Contains(float px, float py) const
{
    // Zones (library, graveyard, exile, ...) draw their icon top-left anchored at
    // (actX, actY); width/height are captured during Render.
    if (width <= 0.f || height <= 0.f)
        return false;
    return (px >= actX && px <= actX + width &&
            py >= actY && py <= actY + height);
}

bool GuiGameZone::CheckUserInput(JButton key)
{
    if (showCards)
    {
        // Touch-first: a tap back on this zone's own icon closes the open card list
        // ("toggle the stack the avatar revealed"), mirroring the avatar-rail toggle.
        // CardDisplay otherwise swallows every tap (it snaps selection to the nearest
        // card) and never closes, so intercept an icon tap here before handing off.
        GameObserver * g = zone ? zone->owner->getObserver() : NULL;
        int cx, cy;
        if (g && g->getInput() && g->getInput()->GetLeftClickCoordinates(cx, cy)
            && Contains(static_cast<float>(cx), static_cast<float>(cy)))
        {
            g->getInput()->LeftClickedProcessed();
            toggleDisplay();
            return true;
        }
        return cd->CheckUserInput(key);
    }
    else if(type == GUI_LIBRARY && zone->nb_cards && !showCards && key == JGE_BTN_OK)
    {
        int top = zone->nb_cards - 1;
        MTGCardInstance * card = zone->cards[top];
        GameObserver * game = card->getObserver();

        // Touch-first: when this OK came from a tap, only draw from the library if the
        // tap actually landed on the pile (otherwise let it fall through to the card
        // selector, so tapping empty space does nothing even while the pile is focused).
        // A keyboard/gamepad OK still requires the pile to be the focused element.
        int cx, cy;
        if (game && game->getInput() && game->getInput()->GetLeftClickCoordinates(cx, cy))
        {
            if (!Contains(static_cast<float>(cx), static_cast<float>(cy)))
                return false;
        }
        else if (!mHasFocus)
        {
            return false;
        }

        bool activateclick = true;
        if(game)
        {
            TargetChooser * tc = game->getCurrentTargetChooser();
            if(tc && (tc->canTarget(card) || !tc->done || tc->Owner->isHuman()))
                activateclick = false;
        }

        if(card && activateclick)
        {
            card->getObserver()->cardClick(card);
            return true;
        }
    }
    return false;
}

void GuiGameZone::Update(float dt)
{
    if (showCards)
        cd->Update(dt);
    PlayGuiObject::Update(dt);

    for (vector<CardView*>::iterator it = cards.begin(); it != cards.end(); ++it)
    {
        CardView * c = (*it);
        c->Update(dt);

        //Dirty fix for http://code.google.com/p/wagic/issues/detail?id=113
        if (fabs(c->actX - c->x) < 0.01 && fabs(c->actY - c->y) < 0.01)
        {
            cards.erase(it);
            zone->owner->getObserver()->mTrash->trash(c);
            return;
        }
    }
}

GuiGameZone::GuiGameZone(float x, float y, bool hasFocus, MTGGameZone* zone, GuiAvatars* parent) :
    GuiStatic(static_cast<float> (GuiGameZone::Height), x, y, hasFocus, parent), zone(zone)
{
    
    // Open the card list toward screen-center: right-edge (self) buttons open it to the
    // left, left-edge (opponent) buttons open it to the right, so it never runs off-screen.
    int cdx = (x > SCREEN_WIDTH / 2) ? static_cast<int>(x) - 235 : static_cast<int>(x) + 35;
    cd = NEW CardDisplay(0, zone->owner->getObserver(), cdx, static_cast<int> (y), this);
    cd->zone = zone;
    showCards = 0;
}

GuiGameZone::~GuiGameZone()
{
    if (cd)
        delete cd;
    for (vector<CardView*>::iterator it = cards.begin(); it != cards.end(); ++it)
        delete (*it);
}

ostream& GuiGameZone::toString(ostream& out) const
{
    return out << "GuiGameZone ::: zone : " << zone << " ; cd : " << cd << " ; showCards : " << showCards;
}

GuiGraveyard::GuiGraveyard(float x, float y, bool hasFocus, Player * player, GuiAvatars* parent) :
    GuiGameZone(x, y, hasFocus, player->game->graveyard, parent), player(player)
{
    type = GUI_GRAVEYARD;
}

int GuiGraveyard::receiveEventPlus(WEvent* e)
{
    if (WEventZoneChange* event = dynamic_cast<WEventZoneChange*>(e))
        if (event->to == zone)
        {
            CardView* t;
            if (event->card->view)
                t = NEW CardView(CardView::nullZone, event->card, *(event->card->view));
            else
                t = NEW CardView(CardView::nullZone, event->card, x, y);
            t->x = x + Width / 2;
            t->y = y + Height / 2;
            t->zoom = 0.6f;
            t->alpha = 0;
            cards.push_back(t);
            return 1;
        }
    return 0;
}

int GuiGraveyard::receiveEventMinus(WEvent* e)
{
    if (WEventZoneChange* event = dynamic_cast<WEventZoneChange*>(e))
        if (event->from == zone)
            for (vector<CardView*>::iterator it = cards.begin(); it != cards.end(); ++it)
                if (event->card->previous == (*it)->card)
                {
                    CardView* cv = *it;
                    cards.erase(it);
                    zone->owner->getObserver()->mTrash->trash(cv);
                    return 1;
                }
    return 0;
}

ostream& GuiGraveyard::toString(ostream& out) const
{
    return out << "GuiGraveyard :::";
}

GuiExile::GuiExile(float x, float y, bool hasFocus, Player * player, GuiAvatars* parent) :
    GuiGameZone(x, y, hasFocus, player->game->exile, parent), player(player)
{
    type = GUI_EXILE;
}

int GuiExile::receiveEventPlus(WEvent* e)
{
    if (WEventZoneChange* event = dynamic_cast<WEventZoneChange*>(e))
        if (event->to == zone)
        {
            CardView* t;
            if (event->card->view)
                t = NEW CardView(CardView::nullZone, event->card, *(event->card->view));
            else
                t = NEW CardView(CardView::nullZone, event->card, x, y);
            t->x = x + Width / 2;
            t->y = y + Height / 2;
            t->zoom = 0.6f;
            t->alpha = 0;
            cards.push_back(t);
            return 1;
        }
    return 0;
}

int GuiExile::receiveEventMinus(WEvent* e)
{
    if (WEventZoneChange* event = dynamic_cast<WEventZoneChange*>(e))
        if (event->from == zone)
            for (vector<CardView*>::iterator it = cards.begin(); it != cards.end(); ++it)
                if (event->card->previous == (*it)->card)
                {
                    CardView* cv = *it;
                    cards.erase(it);
                    zone->owner->getObserver()->mTrash->trash(cv);
                    return 1;
                }
    return 0;
}

ostream& GuiExile::toString(ostream& out) const
{
    return out << "GuiExile :::";
}

GuiCommandZone::GuiCommandZone(float x, float y, bool hasFocus, Player * player, GuiAvatars* parent) :
    GuiGameZone(x, y, hasFocus, player->game->commandzone, parent), player(player)
{
    type = GUI_COMMANDZONE;
}

int GuiCommandZone::receiveEventPlus(WEvent* e)
{
    if (WEventZoneChange* event = dynamic_cast<WEventZoneChange*>(e))
        if (event->to == zone)
        {
            CardView* t;
            if (event->card->view)
                t = NEW CardView(CardView::nullZone, event->card, *(event->card->view));
            else
                t = NEW CardView(CardView::nullZone, event->card, x, y);
            t->x = x + Width / 2;
            t->y = y + Height / 2;
            t->zoom = 0.6f;
            t->alpha = 0;
            cards.push_back(t);
            return 1;
        }
    return 0;
}

int GuiCommandZone::receiveEventMinus(WEvent* e)
{
    if (WEventZoneChange* event = dynamic_cast<WEventZoneChange*>(e))
        if (event->from == zone)
            for (vector<CardView*>::iterator it = cards.begin(); it != cards.end(); ++it)
                if (event->card->previous == (*it)->card)
                {
                    CardView* cv = *it;
                    cards.erase(it);
                    zone->owner->getObserver()->mTrash->trash(cv);
                    return 1;
                }
    return 0;
}

ostream& GuiCommandZone::toString(ostream& out) const
{
    return out << "GuiCommandZone :::";
}

GuiSideboard::GuiSideboard(float x, float y, bool hasFocus, Player * player, GuiAvatars* parent) :
    GuiGameZone(x, y, hasFocus, player->game->sideboard, parent), player(player)
{
    type = GUI_SIDEBOARD;
}

int GuiSideboard::receiveEventPlus(WEvent* e)
{
    if (WEventZoneChange* event = dynamic_cast<WEventZoneChange*>(e))
        if (event->to == zone)
        {
            CardView* t;
            if (event->card->view)
                t = NEW CardView(CardView::nullZone, event->card, *(event->card->view));
            else
                t = NEW CardView(CardView::nullZone, event->card, x, y);
            t->x = x + Width / 2;
            t->y = y + Height / 2;
            t->zoom = 0.6f;
            t->alpha = 0;
            cards.push_back(t);
            return 1;
        }
    return 0;
}

int GuiSideboard::receiveEventMinus(WEvent* e)
{
    if (WEventZoneChange* event = dynamic_cast<WEventZoneChange*>(e))
        if (event->from == zone)
            for (vector<CardView*>::iterator it = cards.begin(); it != cards.end(); ++it)
                if (event->card->previous == (*it)->card)
                {
                    CardView* cv = *it;
                    cards.erase(it);
                    zone->owner->getObserver()->mTrash->trash(cv);
                    return 1;
                }
    return 0;
}

ostream& GuiSideboard::toString(ostream& out) const
{
    return out << "GuiSideboard :::";
}

//opponenthand begins
GuiOpponentHand::GuiOpponentHand(float x, float y, bool hasFocus, Player * player, GuiAvatars* parent) :
    GuiGameZone(x, y, hasFocus, player->game->hand, parent), player(player)
{
    type = GUI_OPPONENTHAND;
}

int GuiOpponentHand::receiveEventPlus(WEvent* e)
{
    if (WEventZoneChange* event = dynamic_cast<WEventZoneChange*>(e))
        if (event->to == zone)
        {
            CardView* t;
            if (event->card->view)
                t = NEW CardView(CardView::nullZone, event->card, *(event->card->view));
            else
                t = NEW CardView(CardView::nullZone, event->card, x, y);
            //t->x = x + Width / 2;
            //t->y = y + Height / 2;
            //t->zoom = 0.6f;
            //I set to negative so we don't see the face when the cards move...
            t->x = -400.f;
            t->y = -400.f;
            t->mask = ARGB(0,0,0,0);
            t->zoom = -0.6f;
            t->alpha = 0;
            cards.push_back(t);
            return 1;
        }
    return 0;
}

int GuiOpponentHand::receiveEventMinus(WEvent* e)
{
    if (WEventZoneChange* event = dynamic_cast<WEventZoneChange*>(e))
        if (event->from == zone)
            for (vector<CardView*>::iterator it = cards.begin(); it != cards.end(); ++it)
                if (event->card->previous == (*it)->card)
                {
                    CardView* cv = *it;
                    cards.erase(it);
                    zone->owner->getObserver()->mTrash->trash(cv);
                    return 1;
                }
    return 0;
}

ostream& GuiOpponentHand::toString(ostream& out) const
{
    return out << "GuiOpponentHand :::";
}

GuiLibrary::GuiLibrary(float x, float y, bool hasFocus, Player * player, GuiAvatars* parent) :
    GuiGameZone(x, y, hasFocus, player->game->library, parent), player(player)
{
    type = GUI_LIBRARY;
}

ostream& GuiLibrary::toString(ostream& out) const
{
    return out << "GuiLibrary :::";
}
