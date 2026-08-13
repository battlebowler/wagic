/*
* CardGui.cpp
* This class is in charge of rendering Cards on the screen
*/

#include "PrecompiledHeader.h"

#include "JGE.h"
#include "CardGui.h"
#include "ManaCostHybrid.h"
#include "ExtraCost.h"
#include "Subtypes.h"
#include "Translate.h"
#include "MTGDefinitions.h"
#include "GameObserver.h"
#include <Vector2D.h>
#include "Counters.h"
#include "ModRules.h"
#include "CardDescriptor.h"
#include "GameApp.h"



const float CardGui::Width     = 74.0f;
const float CardGui::Height    = 106.0f;
const float CardGui::BigWidth  = 400.0f;   // conservative, ~half screen width
const float CardGui::BigHeight = 570.0f;   // ~80% screen height

static float kCardScale = SCREEN_HEIGHT / 272.0f;
static float kPSPScale  = 16.0f / (SCREEN_HEIGHT / 272.0f);

const float kWidthScaleFactor = 0.8f * (SCREEN_HEIGHT / 272.0f);

// Touch hit-testing. Cards are drawn centered on (actX, actY): CardGui::Render scales
// the card art so its on-screen height is (actZ * 38 * kCardScale) game units (the
// texture dimensions cancel out), and the width follows the standard card aspect
// ratio. We rebuild the same footprint here so a tap only "hits" a card when it lands
// on the card as drawn, instead of snapping to the nearest one.
bool CardGui::Contains(float px, float py) const
{
    if (kCardScale <= 0.0f)
        return false;
    float h = actZ * 38.0f * kCardScale;
    float w = h * (Width / Height);
    if (w <= 0.0f || h <= 0.0f)
        return false;
    float halfW = 0.5f * w;
    float halfH = 0.5f * h;
    return (px >= actX - halfW && px <= actX + halfW &&
            py >= actY - halfH && py <= actY + halfH);
}

map<string, string> CardGui::counterGraphics;

namespace
{
    inline float SineHelperFunction(const float& value)
    {
        return sinf(2 * M_PI * (value) / 256.0f);
    }

    inline float CosineHelperFunction(const float& value)
    {
        return cosf(2 * M_PI * (value - 35) / 256.0f);
    }

}

CardGui::CardGui(MTGCardInstance* card, float x, float y)
    : PlayGuiObject(Height, x, y, 0, false), card(card)
{
}
CardGui::CardGui(MTGCardInstance* card, const Pos& ref)
    : PlayGuiObject(Height, ref, 0, false), card(card)
{
}

float CardView::GetCenterX()
{
    bool largeCard = mHeight == BigHeight;

    float centerX = x + (largeCard ? BigWidth : Width) * 0.5f * zoom;
    return centerX;
}

float CardView::GetCenterY()
{
    bool largeCard = mHeight == BigHeight;

    float centerY = y + (largeCard ? BigHeight : Height) * 0.5f * zoom;
    return centerY;
}


CardView::CardView(const SelectorZone owner, MTGCardInstance* card, float x, float y)
    : CardGui(card, x, y), owner(owner)
{
    const Pos* ref = card->view;
    while (card)
    {
        if (ref == card->view)
            card->view = this;
        card = card->next;
    }
}

CardView::CardView(const SelectorZone owner, MTGCardInstance* card, const Pos& ref)
    : CardGui(card, ref), owner(owner)
{
    const Pos* r = card->view;
    while (card)
    {
        if (r == card->view)
            card->view = this;
        card = card->next;
    }
}

CardView::~CardView()
{
    if (card)
    {
        const Pos* r = this;
        while (card)
        {
            if (r == card->view)
                card->view = NULL;
            card = card->next;
        }
    }
}

void CardGui::Update(float dt)
{
    PlayGuiObject::Update(dt);
}

void CardGui::DrawCard(const Pos& inPosition, int inMode, bool thumb, bool noborder, bool gdv)
{
    // The instance knows its own foil flag (avoids an RTTI dynamic_cast downstream).
    DrawCard(card, inPosition, inMode, thumb, noborder, gdv, card ? card->foil : false);
}

void CardGui::DrawCard(MTGCard* inCard, const Pos& inPosition, int inMode, bool thumb, bool noborder, bool gdv, bool foil)
{
    switch (inMode)
    {
    case DrawMode::kNormal:
        RenderBig(inCard, inPosition, thumb, noborder, gdv, foil);
        break;
    case DrawMode::kText:
        AlternateRender(inCard, inPosition);
        break;
    default:
        break;
    }
}

void CardGui::Render()
{
    if (kCardScale == 0.0f) {
        kCardScale = SCREEN_HEIGHT / 272.0f;
        kPSPScale  = 16.0f / kCardScale;
    }

    GameObserver * game = card->getObserver();
    WFont * mFont = game?game->getResourceManager()->GetWFont(Fonts::MAIN_FONT):WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT);
    JRenderer * renderer = JRenderer::GetInstance();
    TargetChooser * tc = NULL;

    if (game)
        tc = game->getCurrentTargetChooser();

    bool alternate = true;
    JQuadPtr quad = game? game->getResourceManager()->RetrieveCard(card, CACHE_NORMAL):WResourceManager::Instance()->RetrieveCard(card, CACHE_NORMAL);
    if(card && !card->isToken && card->name != card->model->data->name)
    {
        MTGCard * fcard = MTGCollection()->getCardByName(card->name);
        quad = game->getResourceManager()->RetrieveCard(fcard, CACHE_NORMAL);
    }
    if (game && card->hasCopiedToken && !quad.get())
    {
        MTGCard * tcard = MTGCollection()->getCardById(abs(card->copiedID));
        quad = game->getResourceManager()->RetrieveCardToken(tcard, CACHE_NORMAL, 1, abs(card->copiedID));
    }
    if (quad.get())
        alternate = false;
    else
        quad = AlternateThumbQuad(card);

    float cardScale = quad ? (38 * kCardScale) / quad->mHeight : 1;
    //I want the below for melded cards but I dont know how to adjust everything else
    //to look neat and clean. leaving this here incase someone else wants to pretty up the p/t box
    //and line up the position.
   /* if (card->MeldedFrom.size())
        cardScale = cardScale + (10 / quad->mHeight);*/
    float scale = actZ * cardScale;

    JQuadPtr shadow;
    if (actZ > 1)
    {
        shadow = game? game->getResourceManager()->GetQuad("shadow"):WResourceManager::Instance()->GetQuad("shadow");
        if (shadow) 
        {
            shadow->SetColor(ARGB(static_cast<unsigned char>(actA)/2,255,255,255));
            renderer->RenderQuad(shadow.get(), actX + (actZ - 1) * 15, actY + (actZ - 1) * 15, actT, 28 * actZ / kPSPScale, 40 * actZ / kPSPScale);
        }
    }

    JQuadPtr extracostshadow;
    if (card->isExtraCostTarget)
    {
        extracostshadow = card->getObserver()->getResourceManager()->GetQuad("extracostshadow");
        if (extracostshadow) 
        {
            extracostshadow->SetColor(ARGB(static_cast<unsigned char>(actA)/2,100,0,0));
            renderer->RenderQuad(extracostshadow.get(), actX + (actZ - 1) * 15, actY + (actZ - 1) * 15, actT, 28 * actZ / kPSPScale, 40 * actZ / kPSPScale);
        }
    }

    if(game && game->connectRule)
    {
        // Am I a parent of a selected card, or am I a parent and myself being selected?
        bool isActiveConnectedParent = mHasFocus && card->childrenCards.size();
        if (!isActiveConnectedParent)
        {
            for (size_t i = 0; i < card->childrenCards.size(); ++i)
            {
                MTGCardInstance * child = card->childrenCards[i];
                if (CardView* cv = dynamic_cast<CardView*>(child->view))
                {
                    if (cv->mHasFocus)
                    {
                        isActiveConnectedParent = true;
                        break;
                    }
                }
            }
        }
        if (isActiveConnectedParent)
        {
            JQuadPtr white = card->getObserver()->getResourceManager()->GetQuad("white");
            if(white)
            {
                white->SetColor(ARGB(255,230,50,50));
                renderer->RenderQuad(white.get(), actX, actY, actT, 30 * actZ / kPSPScale, 42 * actZ / kPSPScale);
            }
        }

        // Am I a child of a selected card, or am I a child and myself being selected?
        bool isActiveConnectedChild = mHasFocus && card->parentCards.size();
        if (!isActiveConnectedChild)
        {
            for (size_t i = 0; i < card->parentCards.size(); ++i)
            {
                MTGCardInstance * parent = card->parentCards[i];
                if (CardView* cv = dynamic_cast<CardView*>(parent->view))
                {
                    if (cv->mHasFocus)
                    {
                        isActiveConnectedChild = true;
                        break;
                    }
                }
            }
        }
        if (isActiveConnectedChild)
        {
            JQuadPtr white = card->getObserver()->getResourceManager()->GetQuad("white");
            if(white)
            {
                white->SetColor(ARGB(255,0,0,255));
                renderer->RenderQuad(white.get(), actX, actY, actT, 30 * actZ / kPSPScale, 42 * actZ / kPSPScale);
            }
        }
    }
    if (quad)
    {
        quad->SetColor(ARGB(static_cast<unsigned char>(actA),255,255,255));
        //fake border...
        JQuadPtr fakeborder;
        JQuadPtr highlightborder;
        fakeborder = game? game->getResourceManager()->GetQuad("white"):WResourceManager::Instance()->GetQuad("white");
        highlightborder = game? game->getResourceManager()->GetQuad("white"):WResourceManager::Instance()->GetQuad("white");
        if(fakeborder)
        {
            if(game)
            {
                if(card->has(Constants::PAYZERO) ||
                    ((card->has(Constants::CANPLAYFROMGRAVEYARD) || card->has(Constants::TEMPFLASHBACK) || card->getManaCost()->getFlashback() || card->getManaCost()->getRetrace()) && game->isInGrave(card)) ||
                    (((card->has(Constants::FORETELL) && card->foretellTurn > -1 && game->turn > card->foretellTurn) || card->has(Constants::CANPLAYFROMEXILE)) && game->isInExile(card)))
                    fakeborder->SetColor(ARGB((int)(actA),7,235,7)); //green border
                else if(card->isCommander)
                    fakeborder->SetColor(ARGB((int)(actA),255,255,255)); //white border for commanders
                else
                {
                    string cardsetname = setlist[card->setId].c_str();
                    bool whiteBorder = (cardsetname == "2ED" || cardsetname == "RV" || cardsetname == "4ED" ||
                                        cardsetname == "5ED" || cardsetname == "6ED" || cardsetname == "7ED" ||
                                        cardsetname == "8ED" || cardsetname == "9ED" || cardsetname == "S00" ||
                                        cardsetname == "S99" || cardsetname == "PTK" || cardsetname == "BTD" ||
                                        cardsetname == "ATH" || cardsetname == "BRB" || cardsetname == "CHR" ||
                                        cardsetname == "DM") && !options[Options::BLKBORDER].number;
                    fakeborder->SetColor(whiteBorder ? ARGB((int)(actA),248,248,255) : ARGB((int)(actA),15,15,15));
                }
            }
            else
                fakeborder->SetColor(ARGB((int)(actA),15,15,15));

            renderer->RenderQuad(fakeborder.get(), actX, actY, actT, (29 * actZ + 1) / kPSPScale, 42 * actZ / kPSPScale);
        }
        //draw border for highlighting
        if (game)
        {
            if (card && card->forcedBorderA && highlightborder)
            {
                highlightborder->SetColor(ARGB(95,255,0,0));
                renderer->RenderQuad(highlightborder.get(), actX, actY, actT, (30 * actZ + 1) / kPSPScale, 43 * actZ / kPSPScale);
            }
            if (card && card->forcedBorderB && highlightborder)
            {
                highlightborder->SetColor(ARGB(95,0,245,0));
                renderer->RenderQuad(highlightborder.get(), actX, actY, actT, (30 * actZ + 1) / kPSPScale, 43 * actZ / kPSPScale);
            }
            if(card->myPair && card->myPair->isInPlay(game) && highlightborder)
            {
                if(mHasFocus)
                {
                    highlightborder->SetColor(ARGB(200,7,98,248));
                    renderer->RenderQuad(highlightborder.get(), actX, actY, actT, (30 * actZ + 1) / kPSPScale, 43 * actZ / kPSPScale);
                }
                if(CardView* cv = dynamic_cast<CardView*>(card->myPair->view))
                {
                    if(cv->mHasFocus)
                    {
                        highlightborder->SetColor(ARGB(200,57,28,248));
                        renderer->RenderQuad(highlightborder.get(), actX, actY, actT, (30 * actZ + 1) / kPSPScale, 43 * actZ / kPSPScale);
                    }
                }
            }
            if(card->shackled && card->shackled->isInPlay(game) && highlightborder)
            {
                if(mHasFocus)
                {
                    if(card->has(Constants::SHACKLER))
                        highlightborder->SetColor(ARGB(200,7,98,248));
                    else
                        highlightborder->SetColor(ARGB(200,57,28,248));

                    renderer->RenderQuad(highlightborder.get(), actX, actY, actT, (30 * actZ + 1) / kPSPScale, 43 * actZ / kPSPScale);
                }
                if(CardView* cv = dynamic_cast<CardView*>(card->shackled->view))
                {
                    if(cv->mHasFocus)
                    {
                        if(!card->shackled->has(Constants::SHACKLER))
                            highlightborder->SetColor(ARGB(200,7,98,248));
                        else
                            highlightborder->SetColor(ARGB(200,57,28,248));

                        renderer->RenderQuad(highlightborder.get(), actX, actY, actT, (30 * actZ + 1) / kPSPScale, 43 * actZ / kPSPScale);
                    }
                }
            }
        }
        //draw the card image
        renderer->RenderQuad(quad.get(), actX, actY, actT, scale, scale);

        // Foil: an animated holographic sheen shifting over the card art (additive blend).
        if (card->foil)
        {
            float hw = 0.5f * scale * quad->mWidth;
            float hh = 0.5f * scale * quad->mHeight;
            float ph = (float) ((JGEGetTime() / 25) % 360) * 3.14159265f / 180.0f;
            int a = (int) (actA * 0.33f);
            if (a < 0) a = 0;
            PIXEL_TYPE cols[4];
            for (int i = 0; i < 4; i++)
            {
                float d = ph + i * 1.5708f; // 90 deg per corner -> shifting rainbow
                int r = (int) (110 + 110 * sin(d));
                int g = (int) (110 + 110 * sin(d + 2.0944f)); // +120 deg
                int b = (int) (110 + 110 * sin(d + 4.1888f)); // +240 deg
                cols[i] = ARGB(a, r, g, b);
            }
            renderer->SetTexBlend(BLEND_SRC_ALPHA, BLEND_ONE);
            renderer->FillRect(actX - hw, actY - hh, hw * 2, hh * 2, cols);
            renderer->SetTexBlend(BLEND_SRC_ALPHA, BLEND_ONE_MINUS_SRC_ALPHA);
        }
    }

    if (alternate)
    {
        // For image-less cards we draw the name as text. In the hand the cards are
        // compressed and heavily overlapped, so drawing every name turns into an
        // unreadable blob floating over the battlefield. Only show the name of the
        // focused (zoomed) hand card; cards elsewhere (battlefield) always show it.
        bool showName = true;
        if (CardView* cv = dynamic_cast<CardView*>(this))
            if (cv->owner == CardView::handZone && !mHasFocus)
                showName = false;

        if (showName)
        {
            mFont->SetColor(ARGB(static_cast<unsigned char>(actA), 0, 0, 0));
            mFont->SetScale(DEFAULT_MAIN_FONT_SCALE * 0.5f * actZ);
            mFont->DrawString(_(card->getName()), actX - actZ * Width / 2 + 1, actY - actZ * Height / 2 + 1);
            mFont->SetScale(DEFAULT_MAIN_FONT_SCALE);
        }

        JQuadPtr icon;
        if (card->hasSubtype("plains"))
            icon = game?game->getResourceManager()->GetQuad("c_white"):WResourceManager::Instance()->GetQuad("c_white");
        else if (card->hasSubtype("swamp"))
            icon = game?game->getResourceManager()->GetQuad("c_black"):WResourceManager::Instance()->GetQuad("c_black");
        else if (card->hasSubtype("forest"))
            icon = game?game->getResourceManager()->GetQuad("c_green"):WResourceManager::Instance()->GetQuad("c_green");
        else if (card->hasSubtype("mountain"))
            icon = game?game->getResourceManager()->GetQuad("c_red"):WResourceManager::Instance()->GetQuad("c_red");
        else if (card->hasSubtype("island"))
            icon = game?game->getResourceManager()->GetQuad("c_blue"):WResourceManager::Instance()->GetQuad("c_blue");
        if (icon.get())
        {
            icon->SetColor(ARGB(static_cast<unsigned char>(actA),255,255,255));
            renderer->RenderQuad(icon.get(), actX, actY, actT);
            icon->SetColor(ARGB(255,255,255,255)); //Putting color back as this quad is shared
        }

    }
    JQuadPtr mor;
    if((card->isMorphed||(card->name == "Morph" && card->isACopier)) && !alternate)
    {
        mor = card->getObserver()->getResourceManager()->RetrieveTempQuad("morph.jpg");
        if (mor &&  mor->mTex) {
            mor->SetHotSpot(static_cast<float> (mor->mTex->mWidth / 2), static_cast<float> (mor->mTex->mHeight / 2));
            mor->SetColor(ARGB(255,255,255,255));
            renderer->RenderQuad(mor.get(), actX, actY, actT,scale, scale);
        }
    }

    //draw line
    if (game)
    {
        JQuadPtr ssMask = card->getObserver()->getResourceManager()->GetQuad("white");

        //choose attacker mask
        if(game->currentPlayer->hasPossibleAttackers())
        {
            if(card->isInPlay(game) && card->isCreature() 
                && ((!card->canAttack() || (card->attackCost > 0)) && (!card->canAttack(true) || (card->attackPlaneswalkerCost > 0)))
                && !card->isPhased && !card->didattacked )
            {
                if(game->getCurrentGamePhase() > MTG_PHASE_FIRSTMAIN 
                    && game->getCurrentGamePhase() < MTG_PHASE_SECONDMAIN 
                    && card->controller() == game->currentPlayer )
                {
                    if(card->controller()->isHuman() && ssMask)
                    {
                        ssMask->SetColor(ARGB(170,64,64,64));
                        renderer->RenderQuad(ssMask.get(), actX, actY, actT, (27 * actZ + 1) / kPSPScale, 40 * actZ / kPSPScale);
                    }
                }
            }
        }

        if (card && card->forcedBorderA)
        {
            if(card->isTapped())
            {
                if(mHasFocus)
                    renderer->DrawRoundRect(actX - (scale * quad->mWidth / 2)-10,actY - (scale * quad->mHeight / 2)+6.5f, (scale * quad->mHeight)-0.02f, (scale * quad->mWidth)-0.02f, 1.8f,ARGB(250,255,0,0));
                else
                    renderer->DrawRoundRect(actX - (scale * quad->mWidth / 2)-8,actY - (scale * quad->mHeight / 2)+4, (scale * quad->mHeight)-0.02f, (scale * quad->mWidth)-0.02f, 1.8f,ARGB(250,255,0,0));
            }
            else
                renderer->DrawRoundRect(actX - (scale * quad->mWidth / 2)-2,actY - (scale * quad->mHeight / 2)-2, (scale * quad->mWidth)-0.02f, (scale * quad->mHeight)-0.02f, 1.8f,ARGB(250,255,0,0));
        }
        if (card && card->forcedBorderB)
        {
            if(card->isTapped())
            {
                if(mHasFocus)
                    renderer->DrawRoundRect(actX - (scale * quad->mWidth / 2)-10,actY - (scale * quad->mHeight / 2)+6.5f, (scale * quad->mHeight)-0.02f, (scale * quad->mWidth)-0.02f, 1.8f,ARGB(250,0,255,0));
                else
                    renderer->DrawRoundRect(actX - (scale * quad->mWidth / 2)-8,actY - (scale * quad->mHeight / 2)+4, (scale * quad->mHeight)-0.02f, (scale * quad->mWidth)-0.02f, 1.8f,ARGB(250,0,255,0));
            }
            else
                renderer->DrawRoundRect(actX - (scale * quad->mWidth / 2)-2,actY - (scale * quad->mHeight / 2)-2, (scale * quad->mWidth)-0.02f, (scale * quad->mHeight)-0.02f, 1.8f,ARGB(250,0,255,0));
       }
    }

    // Render a mask over the card, if set
    if (mask && quad)
        JRenderer::GetInstance()->FillRect(actX - (scale * quad->mWidth / 2),actY - (scale * quad->mHeight / 2), scale * quad->mWidth, scale* quad->mHeight, mask);

    if ((tc && tc->alreadyHasTarget(card)) || (game && card == game->mLayers->actionLayer()->currentActionCard))//paint targets red.
    {
        JQuadPtr rMask = card->getObserver()->getResourceManager()->GetQuad("white");
        rMask->SetColor(ARGB(128,255,0,0));//red
        renderer->RenderQuad(rMask.get(), actX, actY, actT, (27 * actZ + 1) / kPSPScale, 40 * actZ / kPSPScale);
    }
    if(tc && tc->source && tc->source->view && tc->source->view->actZ >= 1.3 && card == tc->source)//paint the source green while infocus.
    {
        JQuadPtr gMask = card->getObserver()->getResourceManager()->GetQuad("white");
        gMask->SetColor(ARGB(128,0,255,0));//green
        renderer->RenderQuad(gMask.get(), actX, actY, actT, (27 * actZ + 1) / kPSPScale, 40 * actZ / kPSPScale);
    }

    //draws the numbers power/toughness
    if (card->isCreature())
    {
        mFont->SetScale(DEFAULT_MAIN_FONT_SCALE);
        char buffer[200];
        sprintf(buffer, "%i/%i", card->power, card->life);
         //move up the p/t box by increasing ymody
         float ymody = ((card->isAttacker()||card->isDefenser())&&card->isTapped())?-6.0f:0.0f;
        renderer->FillRect(actX - (13 * actZ), actY + ymody + 4 * actZ, 25.5f * actZ, 14 * actZ,
            ARGB(((static_cast<unsigned char>(actA))/2),0,0,0));
        renderer->DrawRect(actX - (13 * actZ), actY + ymody + 4 * actZ, 25.5f * actZ, 14 * actZ,
            ARGB(((static_cast<unsigned char>(actA))),20,20,20));
        //damaged or buffed or powered down        
        if(card->wasDealtDamage > 0 && card->life <= 2)
            mFont->SetColor(ARGB(static_cast<unsigned char>(actA),255,0,0));//red critical and damaged
        else if(card->wasDealtDamage == 0 && card->pbonus < 0)
            mFont->SetColor(ARGB(static_cast<unsigned char>(actA),216,191,216));//thistle powered down
        else if(card->wasDealtDamage == 0 && card->pbonus >= 3)
            mFont->SetColor(ARGB(static_cast<unsigned char>(actA),255,255,0));//yellow buff
        else if(card->hasType("legendary") && card->hasType("eldrazi") && !card->has(Constants::CHANGELING))
            mFont->SetColor(ARGB(static_cast<unsigned char>(actA),238,130,238));//violet legendary eldrazi
        else
            mFont->SetColor(ARGB(static_cast<unsigned char>(actA),255,255,255));//white default
        mFont->SetScale(actZ);
        mFont->SetScale(actZ);
        float halfbufferW = (mFont->GetStringWidth(buffer))/2;
        mFont->DrawString(buffer, actX - halfbufferW, actY + ymody + 7 * actZ);
        mFont->SetScale(1);
    }

    string buff = "";
    string starMark = "";
    if(card->exerted)
        starMark += "exerted";
    if(card->isToken && !card->isACopier)
        buff = "T";
    if(card->isToken && card->isACopier)
        buff = "CT";
    if(!card->isToken && card->isACopier)
        buff = "C";
    buff = starMark + buff;
    //if(card->has(Constants::PAYZERO))
        //buff += "Z";
    if(card->chooseacolor >= 1)
    {
        if(card->chooseacolor == 1)
        {
            renderer->FillRect(actX - 10 * actZ, actY - (1 * actZ), 6.f,6.f,ARGB(255,0,255,0));
            renderer->DrawRect(actX - 10 * actZ, actY - (1 * actZ), 6.f,6.f,ARGB(180,10,10,10));
        }
        else if(card->chooseacolor == 2)
        {
            renderer->FillRect(actX - 10 * actZ, actY - (1 * actZ), 6.f,6.f,ARGB(255,0,206,209));
            renderer->DrawRect(actX - 10 * actZ, actY - (1 * actZ), 6.f,6.f,ARGB(180,10,10,10));
        }
        else if(card->chooseacolor == 3)
        {
            renderer->FillRect(actX - 10 * actZ, actY - (1 * actZ), 6.f,6.f,ARGB(255,255,0,0));
            renderer->DrawRect(actX - 10 * actZ, actY - (1 * actZ), 6.f,6.f,ARGB(180,10,10,10));
        }
        else if(card->chooseacolor == 4)
        {
            renderer->FillRect(actX - 10 * actZ, actY - (1 * actZ), 6.f,6.f,ARGB(255,20,20,20));
            renderer->DrawRect(actX - 10 * actZ, actY - (1 * actZ), 6.f,6.f,ARGB(180,210,210,210));
        }
        else if(card->chooseacolor == 5)
        {
            renderer->FillRect(actX - 10 * actZ, actY - (1 * actZ), 6.f,6.f,ARGB(255,250,235,215));
            renderer->DrawRect(actX - 10 * actZ, actY - (1 * actZ), 6.f,6.f,ARGB(180,10,10,10));
        }
    }
    if(card->chooseasubtype.size() && !alternate && game)
    {
        mFont->SetScale(DEFAULT_MAIN_FONT_SCALE);
        char buffer[200];
        sprintf(buffer, "%s", card->chooseasubtype.c_str());
        mFont->SetColor(ARGB(static_cast<unsigned char>(actA),255,215,0));//Gold indicator
        mFont->SetScale(0.8f);
        mFont->DrawString(buffer, actX - 10 * actZ, actY - (25.3f * actZ));
        mFont->SetScale(1);
    }
    if(card->chooseaname.size() && !alternate && game)
    {
        mFont->SetScale(DEFAULT_MAIN_FONT_SCALE);
        char buffer[200];
        sprintf(buffer, "%s", card->chooseaname.c_str());
        mFont->SetColor(ARGB(static_cast<unsigned char>(actA),255,215,0));//Gold indicator
        mFont->SetScale(0.8f);
        mFont->DrawString(buffer, actX - 10 * actZ, actY - (25.3f * actZ));
        mFont->SetScale(1);
    }
    if(!alternate && buff != "" && game)
    {
        mFont->SetScale(DEFAULT_MAIN_FONT_SCALE);
        char buffer[200];
        sprintf(buffer, "%s", buff.c_str());
        mFont->SetColor(ARGB(static_cast<unsigned char>(actA),255,215,0));//Gold indicator
        mFont->SetScale(actZ);
        mFont->DrawString(buffer, actX - 10 * actZ, actY - (18.3f * actZ));
        mFont->SetScale(1);
    }
#if !defined (PSP)
    if(game && game->gameType() == GAME_TYPE_MOMIR)
    {
        if(game->isInHand(card) && !card->controller()->isAI())
        {
            if ((game->currentPlayer != card->controller()) || (card->controller()->game->playRestrictions->canPutIntoZone(card, card->controller()->game->inPlay) == PlayRestriction::CANT_PLAY))
            {
                mFont->SetScale(0.4f);
                mFont->SetColor(ARGB(static_cast<unsigned char>(actA),255,0,0));
                mFont->SetScale(actZ);
                mFont->DrawString("+", actX - 12 * actZ, actY - (18.8f * actZ));
                mFont->SetScale(1);
            }
            else
            {
                mFont->SetScale(0.4f);
                mFont->SetColor(ARGB(static_cast<unsigned char>(actA),0,255,0));
                mFont->SetScale(actZ);
                mFont->DrawString("+", actX - 12 * actZ, actY - (18.8f * actZ));
                mFont->SetScale(1);
            }
        }
    }
#endif
    if (card->counters->mCount > 0)
    {
        unsigned c = -1;
        for (int i = 0; i < card->counters->mCount; i++)
        {
            if (card->counters->counters[i]->name != "")
                c = i;
            break;
        }
        if (c + 1)
        {
            mFont->SetScale(DEFAULT_MAIN_FONT_SCALE);
            char buffer[200];
            sprintf(buffer, "%i", card->counters->counters[0]->nb);
            mFont->SetColor(ARGB(static_cast<unsigned char>(actA),255,255,255));
            mFont->SetScale(actZ);
            mFont->DrawString(buffer, actX - 10 * actZ, actY - (10.8f * actZ));
            mFont->SetScale(1);
        }
    }

    //shadow that covers the whole card for targetchooser...
    if (tc && !tc->canTarget(card))
    {
        if (!shadow)
            shadow = card->getObserver()->getResourceManager()->GetQuad("shadow");
        if (shadow)
        {
            shadow->SetColor(ARGB(190,255,255,255));
            renderer->RenderQuad(shadow.get(), actX, actY, actT, (28 * actZ + 1) / kPSPScale, 40 * actZ / kPSPScale);
        }
    }
    
    //for necro
    if(game)
    {
        if (!shadow)
            shadow = card->getObserver()->getResourceManager()->GetQuad("shadow");
        if (shadow)
        {
            int myA = 0;
            if(game && card->has(Constants::NECROED))//no peeking...
                myA = 255;
            else
                myA = 0;
            
            shadow->SetColor(ARGB(myA,255,255,255));
            if(myA > 0)
                renderer->RenderQuad(shadow.get(), actX, actY, actT, (28 * actZ + 1) / kPSPScale, 40 * actZ / kPSPScale);
        }
    }

    PlayGuiObject::Render();
}

JQuadPtr CardGui::AlternateThumbQuad(MTGCard * card)
{
    JQuadPtr q;
    vector<ModRulesBackGroundCardGuiItem *>items = gModRules.cardgui.background;
    ModRulesBackGroundCardGuiItem * item;
    int numItems = (int)items.size();
    if (card->data->countColors() > 1)
    {
         item = items[numItems-1];
    }
    else
    {
        item = items[card->data->getColor()];
    }
    
    
    q = WResourceManager::Instance()->RetrieveTempQuad(item->mDisplayThumb);
    items.clear();  
    if (q && q->mTex)
        q->SetHotSpot(static_cast<float> (q->mTex->mWidth / 2), static_cast<float> (q->mTex->mHeight / 2));
    return q;
}

void CardGui::AlternateRender(MTGCard * card, const Pos& pos)
{
    // Draw the "unknown" card model
    JRenderer * renderer = JRenderer::GetInstance();
    JQuadPtr q;
    MTGCardInstance * thiscard = dynamic_cast<MTGCardInstance*> (card);
    int zpos = 0;
    float x = pos.actX;

    vector<ModRulesBackGroundCardGuiItem *>items = gModRules.cardgui.background;
    ModRulesBackGroundCardGuiItem * item;
    int numItems = (int)items.size();
    if (card->data->countColors() > 1)
    {
        item = items[numItems-1];
    }
    else
    {
        item = items[card->data->getColor()];
    }

    q = WResourceManager::Instance()->RetrieveTempQuad(item->mDisplayImg,TEXTURE_SUB_5551);

    // The actual rendered card dimensions in virtual units
    const float kAltCardHeight = 250.0f;
    const float kAltCardWidth  = (q.get() && q->mTex) ? kAltCardHeight * ((float)q->mTex->mWidth / (float)q->mTex->mHeight) : 175.0f;

    items.clear();
    if (q.get() && q->mTex)
    {
        if(thiscard && thiscard->getObserver())
        {
            zpos = thiscard->zpos;
        }

        q->SetHotSpot(static_cast<float> (q->mTex->mWidth / 2), static_cast<float> (q->mTex->mHeight / 2));

        float scale = pos.actZ * kAltCardHeight / q->mHeight;
        q->SetColor(ARGB((int)pos.actA,255,255,255));
        //new border
        renderer->FillRoundRect(pos.actX - (scale * q->mWidth / 2)-5.8f,pos.actY - (scale * q->mHeight / 2)-5.8f, (scale * q->mWidth)-0.02f, (scale * q->mHeight)-0.02f, 5.8f,ARGB(255,5,5,5));
        renderer->DrawRoundRect(pos.actX - (scale * q->mWidth / 2)-5.8f,pos.actY - (scale * q->mHeight / 2)-5.8f, (scale * q->mWidth)-0.02f, (scale * q->mHeight)-0.02f, 5.8f,ARGB(50,240,240,240));
        //end
        renderer->RenderQuad(q.get(), x, pos.actY, pos.actT, scale, scale);
    }

    vector<ModRulesRenderCardGuiItem *>Carditems = gModRules.cardgui.renderbig;

    WFont * font = WResourceManager::Instance()->GetWFont(Fonts::MAGIC_FONT);
    float backup_scale = font->GetScale();
    font->SetColor(ARGB((int)pos.actA, 0, 0, 0));
    font->SetScale(kWidthScaleFactor * pos.actZ);

    float left  = x - (kAltCardWidth / 2 - 10) * pos.actZ;
    float top   = pos.actY - (kAltCardHeight / 2 - 25) * pos.actZ;
    unsigned h = neofont ? 14 : 11;

    // Card name
    font->DrawString(_(card->data->name).c_str(), left, top - 20 * pos.actZ);

    // Types
    if (card->data->types.size())
    {
        string s = "";
        for (unsigned int i = 0; i < card->data->types.size() - 1; i++)
        {
            s += _(MTGAllCards::findType(card->data->types[i]));
            if(!strcmp(_(MTGAllCards::findType(card->data->types[i])).c_str(),"Creature") || !strcmp(_(MTGAllCards::findType(card->data->types[i])).c_str(),"Land"))
                s += _(" - ");
            else
                s += _(" ");
        }
        s += _(MTGAllCards::findType(card->data->types[card->data->types.size()-1]));
        font->DrawString(s.c_str(), left, top + 8 * pos.actZ);
    }

    // Description
    std::vector<string> txt = card->data->getFormattedText();
    for (unsigned int i = 0; i < txt.size(); ++i)
        font->DrawString(_(txt[i].c_str()), left, top + 30 * pos.actZ + h * i * pos.actZ);

    // Power/Toughness
    if (card->data->isCreature())
    {
        char ptbuf[32];
        sprintf(ptbuf, "%i/%i", card->data->power, card->data->toughness);
        float ptw = font->GetStringWidth(ptbuf);
        font->DrawString(ptbuf, left + (kAltCardWidth - 20) * pos.actZ - ptw, top + (kAltCardHeight + 80) * pos.actZ);
    }

    // Expansion / Rarity
    {
        string sRarity;
        switch(card->getRarity())
        {
            case Constants::RARITY_M: sRarity = _("Mythic"); break;
            case Constants::RARITY_R: sRarity = _("Rare"); break;
            case Constants::RARITY_U: sRarity = _("Uncommon"); break;
            case Constants::RARITY_C: sRarity = _("Common"); break;
            case Constants::RARITY_L: sRarity = _("Land"); break;
            case Constants::RARITY_T: sRarity = _("Token"); break;
            default:                  sRarity = _("Special"); break;
        }
        string exprar = string(setlist[card->setId].c_str()) + " " + sRarity;
        font->DrawString(exprar.c_str(), left, top + (kAltCardHeight + 80) * pos.actZ);
    }

    // Mana cost (top right)
    {
        ManaCost* manacost = card->data->getManaCost();
        ManaCostHybrid* mh;
        unsigned int j = 0;
        unsigned int z = 0;
        unsigned char t = (JGE::GetInstance()->GetTime() / 3) & 0xFF;
        unsigned char v = t + 127;
        float manaY = pos.actY - (kAltCardHeight / 2 - 8) * pos.actZ;
        float manaStartX = x + (kAltCardWidth / 2 - 8) * pos.actZ;

        for (int i = Constants::NB_Colors - 2; i >= 1; --i)
        {
            int cost;
            for (cost = manacost->getCost(i); cost > 0; --cost)
            {
                renderer->RenderQuad(manaIcons[i].get(), manaStartX - 12 * j * pos.actZ, manaY, 0, 0.4f * pos.actZ, 0.4f * pos.actZ);
                ++j;
            }
        }
        if (int cost = manacost->getCost(0))
        {
            char buffer[10];
            sprintf(buffer, "%d", cost);
            renderer->RenderQuad(manaIcons[0].get(), manaStartX - 12 * j * pos.actZ, manaY, 0, 0.4f * pos.actZ, 0.4f * pos.actZ);
            float w = font->GetStringWidth(buffer);
            font->DrawString(buffer, manaStartX - 12 * j * pos.actZ - w / 2, manaY - 5 * pos.actZ);
            ++j;
        }
        if (manacost->hasX())
        {
            char buffer[10];
            sprintf(buffer, "X");
            renderer->RenderQuad(manaIcons[0].get(), manaStartX - 12 * j * pos.actZ, manaY, 0, 0.4f * pos.actZ, 0.4f * pos.actZ);
            float w = font->GetStringWidth(buffer);
            font->DrawString(buffer, manaStartX - 12 * j * pos.actZ - w / 2, manaY - 5 * pos.actZ);
        }
        z = 0;
        while ((mh = manacost->getHybridCost(z)))
        {
            float scale = pos.actZ * 0.05f * cosf(2 * M_PI * ((float) t) / 256.0f);
            renderer->RenderQuad(manaIcons[mh->color1].get(), manaStartX - 12 * j * pos.actZ, manaY, 0, 0.4f + scale, 0.4f + scale);
            renderer->RenderQuad(manaIcons[mh->color2].get(), manaStartX - 12 * j * pos.actZ, manaY, 0, 0.4f - scale, 0.4f - scale);
            ++j;
            ++z;
        }
    }

    font->SetScale(backup_scale);

    RenderCountersBig(card, pos, DrawMode::kText);
}

void CardGui::TinyCropRender(MTGCard * card, const Pos& pos, JQuad * quad, bool foil)
{
    if (!quad)
        return;

    JRenderer * renderer = JRenderer::GetInstance();
    JQuadPtr q;

    float x = pos.actX;
    float displayScale = 250 / BigHeight;
   
    vector<ModRulesBackGroundCardGuiItem *>items = gModRules.cardgui.background;
    ModRulesBackGroundCardGuiItem * item;
    int numItems = (int)items.size();
    if (card->data->countColors() > 1)
    {
         item = items[numItems-1];
    }
    else
    {
        item = items[card->data->getColor()];
    }
    
    q = WResourceManager::Instance()->RetrieveTempQuad(item->mDisplayImg,TEXTURE_SUB_5551);
    items.clear();
    if (q.get() && q->mTex)
    {
        q->SetHotSpot(static_cast<float> (q->mTex->mWidth / 2), static_cast<float> (q->mTex->mHeight / 2));

        float scale = pos.actZ * displayScale * BigHeight / q->mHeight;
        q->SetColor(ARGB((int)pos.actA,255,255,255));
        renderer->RenderQuad(q.get(), x, pos.actY, pos.actT, scale, scale);
    }
    
    std::vector<string> txt = card->data->getFormattedText();
    size_t nbTextLines = txt.size();

    //Render the image on top of that
    quad->SetColor(ARGB((int)pos.actA,255,255,255));
    float imgScale = pos.actZ * (displayScale * (BigWidth - 15)) / quad->mWidth;
    float imgY = pos.actY - (20 * imgScale);
    if (nbTextLines > 6)
    {
        imgY -= 10 * imgScale;
        imgScale *= 0.75;
    }
    renderer->RenderQuad(quad, x, imgY, pos.actT, imgScale, imgScale);



    vector<ModRulesRenderCardGuiItem *>Carditems = gModRules.cardgui.rendertinycrop;
    
    WFont * font = WResourceManager::Instance()->GetWFont(Fonts::MAGIC_FONT);
    float backup_scale = font->GetScale();
    string sFormattedData = "";
    for( size_t i =0 ; i < Carditems.size(); i ++)
    {
        ModRulesRenderCardGuiItem * Carditem = Carditems[i];
        if (Carditem->mFilter.length() == 0 || FilterCard(card,Carditem->mFilter.c_str()))
        {

           if (Carditem->mFont) 
            {
                font->SetColor(Carditem->mFontColor);
                font->SetScale(((float)Carditem->mFontSize / 100) * pos.actZ);
                    
            }
            else
            {
                font->SetColor(ARGB((int)pos.actA, 0, 0, 0));
                font->SetScale(kWidthScaleFactor * pos.actZ);

            }
            
            if (Carditem->mName == "description")
            {

                std::vector<string> txt = card->data->getFormattedText();
                float imgBottom = imgY + (imgScale * quad->mHeight / 2);

                unsigned i = 0;
                unsigned h = neofont ? 14 : 11;
                for (std::vector<string>::const_iterator it = txt.begin(); it != txt.end(); ++it, ++i)
                    font->DrawString(it->c_str(), x + (Carditem->mPosX - BigWidth / 2) * pos.actZ, imgBottom + (Carditem->mPosY + h * i) * pos.actZ);
            }
            else if (Carditem->mName == "mana")
            {
                // Mana
                // Need Create a realy generic struct for mana render
                ManaCost* manacost = card->data->getManaCost();
                ManaCostHybrid* h;
                ExtraCost* e;
                unsigned int j = 0;
                unsigned int z = 0;
                unsigned char t = (JGE::GetInstance()->GetTime() / 3) & 0xFF;
                unsigned char v = t + 127;
                float yOffset = (float)Carditem->mPosY;
                
                JQuadPtr quad = WResourceManager::Instance()->RetrieveQuad("menuicons.png", 0, 0, 0, 0, "", RETRIEVE_NORMAL, TEXTURE_SUB_5551, 1);
                if (quad.get())
                    if (quad->mHeight >= 78)
                    while ((e = manacost->getExtraCost(z)))
                    {
                            if(e->mCostRenderString == "Phyrexian Mana")
                            {
                                float _color = (float)card->data->getColor() -1;
                                JQuadPtr ExtraManas = WResourceManager::Instance()->RetrieveQuad("menuicons.png", 2 + _color * 36, 76, 32, 32, "c_extra", RETRIEVE_MANAGE);
                                ExtraManas->SetHotSpot(16, 16);
                                renderer->RenderQuad(ExtraManas.get(), x + (-12 * j + Carditem->mPosX) * pos.actZ, pos.actY + (yOffset) * pos.actZ, 0, 0.4f
                                * pos.actZ, 0.4f * pos.actZ);
                            }
                   
                        ++j;
                        ++z;
                    }
               
                z=0;
                while ((h = manacost->getHybridCost(z)))
                {
                    float scale = pos.actZ * 0.05f * cosf(2 * M_PI * ((float) t) / 256.0f);

                    if (scale < 0)
                    {
                        renderer->RenderQuad(manaIcons[h->color1].get(), x + (-12 * j + Carditem->mPosX + 3 * SineHelperFunction((float) t)) * pos.actZ,
                            pos.actY + (yOffset + 3 * CosineHelperFunction((float) t)) * pos.actZ, 0, 0.4f + scale, 0.4f
                            + scale);
                        renderer->RenderQuad(manaIcons[h->color2].get(), x + (-12 * j + Carditem->mPosX + 3 * SineHelperFunction((float) v)) * pos.actZ,
                            pos.actY + (yOffset + 3 * CosineHelperFunction((float) v)) * pos.actZ, 0, 0.4f - scale, 0.4f
                            - scale);
                    }
                    else
                    {
                        renderer->RenderQuad(manaIcons[h->color2].get(), x + (-12 * j + Carditem->mPosX + 3 * SineHelperFunction((float) v)) * pos.actZ,
                            pos.actY + (yOffset + 3 * CosineHelperFunction((float) v)) * pos.actZ, 0, 0.4f - scale, 0.4f
                            - scale);
                        renderer->RenderQuad(manaIcons[h->color1].get(), x + (-12 * j + Carditem->mPosX + 3 * SineHelperFunction((float) t)) * pos.actZ,
                            pos.actY + (yOffset + 3 * CosineHelperFunction((float) t)) * pos.actZ, 0, 0.4f + scale, 0.4f
                            + scale);
                    }
                    ++j;
                    ++z;
                }
                for (int i = Constants::NB_Colors - 2; i >= 1; --i)
                {
                     int cost;
                    for (cost = manacost->getCost(i); cost > 0; --cost)
                    {
                        renderer->RenderQuad(manaIcons[i].get(), x + (-12 * j + Carditem->mPosX) * pos.actZ, pos.actY + (yOffset) * pos.actZ, 0, 0.4f
                            * pos.actZ, 0.4f * pos.actZ);
                        ++j;
                    }
                    
                }
                // Colorless mana
                if (int cost = manacost->getCost(0))
                {
                    char buffer[10];
                    sprintf(buffer, "%d", cost);
                    renderer->RenderQuad(manaIcons[0].get(), x + (-12 * j + Carditem->mPosX) * pos.actZ, pos.actY + (yOffset) * pos.actZ, 0, 0.4f * pos.actZ,
                        0.4f * pos.actZ);
                    float w = font->GetStringWidth(buffer);
                    font->DrawString(buffer, x + (-12 * j + (Carditem->mPosX +1) - w / 2) * pos.actZ, pos.actY + (yOffset - 5) * pos.actZ);
                    ++j;
                }
                //Has X?
                if (manacost->hasX())
                {
                    char buffer[10];
                    sprintf(buffer, "X");
                    renderer->RenderQuad(manaIcons[0].get(), x + (-12 * j + Carditem->mPosX) * pos.actZ, pos.actY + (yOffset) * pos.actZ, 0, 0.4f * pos.actZ,
                        0.4f * pos.actZ);
                    float w = font->GetStringWidth(buffer);
                    font->DrawString(buffer, x + (-12 * j + (Carditem->mPosX + 1) - w / 2) * pos.actZ, pos.actY + (yOffset - 5) * pos.actZ);
                }
    
            }
            else if (Carditem->mName == "icon")
            {
                float yOffseticon = (float)Carditem->mPosY;
                JQuadPtr ExtraIcons = WResourceManager::Instance()->RetrieveQuad(Carditem->mFileName.c_str(), 2 + (float)(Carditem->mIconPosX - 1) * 36, (float)(Carditem->mIconPosY -1) * 38 , 32, 32, "", RETRIEVE_MANAGE);
                ExtraIcons->SetHotSpot(16,16);
                renderer->RenderQuad(ExtraIcons.get(), x + (Carditem->mPosX) * pos.actZ, pos.actY + (yOffseticon) * pos.actZ, 0, (float)Carditem->mSizeIcon * 0.4f
                * pos.actZ, (float)Carditem->mSizeIcon* 0.4f * pos.actZ);

            }
            else 
            {
                string formattedfield = Carditem->mFormattedData;
                size_t found = Carditem->mName.find("title"); // Write the title
                if (found != string::npos)
                {
                    stringstream st;
                    st << card->data->name;
                    formattedfield = FormattedData(formattedfield, "title", st.str());
                
                }

                found = Carditem->mName.find("power"); // Write the strength
                if (found != string::npos)
                {
                    stringstream st;
                    st << card->data->power;
                    formattedfield = FormattedData(formattedfield, "power", st.str());
                }
                found = Carditem->mName.find("life"); // Write the toughness
                if (found != string::npos)
                {
                    stringstream st;
                    st << card->data->toughness;
                    formattedfield = FormattedData(formattedfield, "life", st.str());

                }

                found = Carditem->mName.find("types"); //types
                if (found != string::npos)
                {
                    string s = "";
                    if (card->data->basicAbilities[(int)Constants::CHANGELING])
                    {
                        // this avoids drawing the list of subtypes on changeling cards.
                        if (card->data->types.size()){
                            s = _(MTGAllCards::findType(card->data->types[0])) + _(" - Shapeshifter");
                        } else {
                            s = _("Shapeshifter");
                        }
                    } else {
                        for (unsigned int i = 0; i < card->data->types.size() - 1; i++)
                        {
                            s += _(MTGAllCards::findType(card->data->types[i]));
                            if(!strcmp(_(MTGAllCards::findType(card->data->types[i])).c_str(),"Creature") || !strcmp(_(MTGAllCards::findType(card->data->types[i])).c_str(),"Land"))
                                s += _(" - ");
                            else
                                s += _(" ");
                        }
                        s += _(MTGAllCards::findType(card->data->types[card->data->types.size()-1]));
                    }
                    if (!card->data->types.size()){
                        DebugTrace("Typeless card: " << setlist[card->setId].c_str() << card->data->getName() << card->getId());
                    }
                    formattedfield = FormattedData(formattedfield, "types", s);
                }

                found = Carditem->mName.find("rarity");
                if (found != string::npos)
                {
                    
                    string sRarity;
                    switch(card->getRarity())
                    {
                    case Constants::RARITY_M:
                        sRarity ="Mythic";
                        break;
                    case Constants::RARITY_R:
                        sRarity ="Rare";
                        break;
                    case Constants::RARITY_U:
                        sRarity ="Uncommon";
                        break;
                    case Constants::RARITY_C:
                        sRarity ="Common";
                        break;
                    case Constants::RARITY_L:
                        sRarity ="Land";
                        break;
                    case Constants::RARITY_T:
                        sRarity ="Token";
                        break;
                    default:
                    case Constants::RARITY_S:
                        sRarity ="Special";
                        break;
                    }
                    formattedfield = FormattedData(formattedfield, "rarity", sRarity);
                }

                 found = Carditem->mName.find("expansion");
                if (found != string::npos)
                {
                    formattedfield = FormattedData(formattedfield, "expansion", setlist[card->setId].c_str());
                }

                if (!Carditem->mFont) 
                {          
                    float w = font->GetStringWidth(formattedfield.c_str()) * kWidthScaleFactor * pos.actZ;
                    if (w > BigWidth - 30)
                        font->SetScale((BigWidth - 30) / w);
                }
                font->DrawString(formattedfield.c_str(), x + (Carditem->mPosX  - BigWidth / 2) * pos.actZ, pos.actY + (Carditem->mPosY - BigHeight / 2) * pos.actZ);
            
            }
         
        }
    }


    
    font->SetScale(backup_scale);

    // Foil holographic sheen for collectible foils, over the composited crop card. This is
    // the path most full-card previews use (pack reveal big card, deck editor, etc.).
    {
        if (foil)
        {
            float ch = 250.0f * pos.actZ;               // composited card frame height
            float cw = ch * (BigWidth / BigHeight);     // width from card aspect
            float ph = (float) ((JGEGetTime() / 25) % 360) * 3.14159265f / 180.0f;
            int a = (int) (pos.actA * 0.42f);
            if (a < 0) a = 0;
            PIXEL_TYPE cols[4];
            for (int i = 0; i < 4; i++)
            {
                float d = ph + i * 1.5708f;
                int r = (int) (110 + 110 * sin(d));
                int g = (int) (110 + 110 * sin(d + 2.0944f));
                int b = (int) (110 + 110 * sin(d + 4.1888f));
                cols[i] = ARGB(a, r, g, b);
            }
            renderer->SetTexBlend(BLEND_SRC_ALPHA, BLEND_ONE);
            renderer->FillRect(pos.actX - cw / 2, pos.actY - ch / 2, cw, ch, cols);
            renderer->SetTexBlend(BLEND_SRC_ALPHA, BLEND_ONE_MINUS_SRC_ALPHA);
        }
    }

    RenderCountersBig(card, pos);
}

//Renders a big card on screen. Defaults to the "alternate" rendering if no image is found
void CardGui::RenderBig(MTGCard* card, const Pos& pos, bool thumb, bool noborder, bool gdv, bool foil)
{
    // Ensure scale factors are initialized even if Render() hasn't been called yet
    if (kCardScale == 0.0f) {
        kCardScale = SCREEN_HEIGHT / 272.0f;
        kPSPScale  = 16.0f / kCardScale;
    }

    JRenderer * renderer = JRenderer::GetInstance();
    float x = pos.actX;
    JQuadPtr alphabeta = WResourceManager::Instance()->RetrieveTempQuad("alphabeta.png");
    JQuadPtr quad = thumb ? WResourceManager::Instance()->RetrieveCard(card, RETRIEVE_THUMB)
                          : WResourceManager::Instance()->RetrieveCard(card);
    MTGCardInstance * kcard =  dynamic_cast<MTGCardInstance*>(card);
    GameObserver * game = NULL;
    //TargetChooser * tc = NULL;
    bool myA = true;
    if(kcard)
    {
        game = kcard->getObserver();
        if(game)
        {
            if(kcard->has(Constants::NECROED))
                myA = false;
            else
                myA = true;
        }
    }
    if(kcard && !kcard->isToken && kcard->name != kcard->model->data->name)
    {
        MTGCard * fcard = MTGCollection()->getCardByName(kcard->name);
        quad = WResourceManager::Instance()->RetrieveCard(fcard);
    }
    if (kcard && kcard->hasCopiedToken && !quad.get())
    {
        MTGCard * tcard = MTGCollection()->getCardById(abs(kcard->copiedID));
        quad = thumb ? WResourceManager::Instance()->RetrieveCardToken(tcard, RETRIEVE_THUMB, 1, abs(kcard->copiedID))
                     : WResourceManager::Instance()->RetrieveCardToken(tcard, RETRIEVE_NORMAL, 1, abs(kcard->copiedID));
    }
    if (quad.get() && myA)
    {
        if (quad->mHeight < quad->mWidth)
        {
            return TinyCropRender(card, pos, quad.get(), foil);
        }
        quad->SetColor(ARGB(255,255,255,255));
        float scale = pos.actZ * (250.f * kCardScale) / quad->mHeight;
        //init setname
        string cardsetname = setlist[card->setId].c_str();
        /*if(!noborder)
        {
            if(cardsetname == "2ED"||cardsetname == "RV"||cardsetname == "4ED"||cardsetname == "5ED"||cardsetname == "6ED"||cardsetname == "7ED"||cardsetname == "8ED"||cardsetname == "9ED"||cardsetname == "S00"||cardsetname == "S99"||cardsetname == "PTK"||cardsetname == "BTD"||cardsetname == "ATH"||cardsetname == "BRB"||cardsetname == "CHR"||cardsetname == "DM")
            {//Draw white border
                renderer->FillRoundRect((pos.actX - (pos.actZ * 84.f))-11.5f,(pos.actY - (pos.actZ * 119.7f))-14.f,pos.actZ * 168.f + 6.5f,pos.actZ * 239.4f + 12.f,8.f,ARGB(255,248,248,255));
                renderer->DrawRoundRect((pos.actX - (pos.actZ * 84.f))-11.5f,(pos.actY - (pos.actZ * 119.7f))-14.f,pos.actZ * 168.f + 6.5f,pos.actZ * 239.4f + 12.f,8.f,ARGB(150,20,20,20));
            }
            else
            {
                if(cardsetname == "LEA")
                {//BETA HAS REGULAR BORDER
                    //Draw more rounder black border
                    renderer->FillRoundRect((pos.actX - (pos.actZ * 84.f))-10.f,(pos.actY - (pos.actZ * 119.7f))-14.f,pos.actZ * 168.f - 0.5f,pos.actZ * 239.4f + 8.f,10.f,ARGB(255,5,5,5));
                    renderer->DrawRoundRect((pos.actX - (pos.actZ * 84.f))-10.f,(pos.actY - (pos.actZ * 119.7f))-14.f,pos.actZ * 168.f - 0.5f,pos.actZ * 239.4f + 8.f,10.f,ARGB(50,240,240,240));
                }
                else
                {//draw black border
                    renderer->FillRoundRect((pos.actX - (pos.actZ * 84.f))-11.5f,(pos.actY - (pos.actZ * 119.7f))-14.f,pos.actZ * 168.f + 6.5f,pos.actZ * 239.4f + 12.f,8.f,ARGB(255,5,5,5));
                    renderer->DrawRoundRect((pos.actX - (pos.actZ * 84.f))-11.5f,(pos.actY - (pos.actZ * 119.7f))-14.f,pos.actZ * 168.f + 6.5f,pos.actZ * 239.4f + 12.f,8.f,ARGB(50,240,240,240));
                }
            }
        }*///disabled this for universal border across game, deck editor, etc...
        //universal border
        if(options[Options::SHOWBORDER].number)
        {
            float borderSize = 4.0f * kCardScale;
            // FillRoundRect/DrawRoundRect render at size (w + 2*radius) x (h + 2*radius)
            // with radius==borderSize, so passing the card size (offset by -borderSize)
            // already yields a uniform borderSize frame on all four sides. (Do NOT add
            // 2*borderSize to w/h — that double-counts and makes the border huge.)
            float bx = pos.actX - (scale * quad->mWidth / 2) - borderSize;
            float by = pos.actY - (scale * quad->mHeight / 2) - borderSize;
            float bw = (scale * quad->mWidth) - 0.02f;
            float bh = (scale * quad->mHeight) - 0.02f;
            if((cardsetname == "2ED"||cardsetname == "RV"||cardsetname == "4ED"||cardsetname == "5ED"||cardsetname == "6ED"||cardsetname == "7ED"||cardsetname == "8ED"||cardsetname == "9ED"||cardsetname == "S00"||cardsetname == "S99"||cardsetname == "PTK"||cardsetname == "BTD"||cardsetname == "ATH"||cardsetname == "BRB"||cardsetname == "CHR"||cardsetname == "DM")
               && !options[Options::BLKBORDER].number)
            {//white border
                renderer->FillRoundRect(bx, by, bw, bh, borderSize, ARGB(255,248,248,255));
                renderer->DrawRoundRect(bx, by, bw, bh, borderSize, ARGB(150,20,20,20));
            }
            else
            {//black border
                renderer->FillRoundRect(bx, by, bw, bh, borderSize, ARGB(255,5,5,5));
                renderer->DrawRoundRect(bx, by, bw, bh, borderSize, ARGB(50,240,240,240));
            }
            //end new border
            //draw inner border
            if(cardsetname == "LEA"||cardsetname == "LEB")
            {
                if(alphabeta.get())
                {
                    alphabeta->SetHotSpot(static_cast<float> (alphabeta->mWidth / 2), static_cast<float> (alphabeta->mHeight / 2));
                    float myscale = pos.actZ * 255 / alphabeta->mHeight;
                    alphabeta->SetColor(ARGB((int)pos.actA,255,255,255));
                    renderer->RenderQuad(alphabeta.get(), x, pos.actY+0.2f, pos.actT, myscale, myscale);
                }
            }
        }
        float modxscale = (cardsetname =="UNH")?0.015f:0.0f;
        float modyscale = (cardsetname =="UNH")?0.010f:0.0f;
        float gdvadd = gdv?0.008f:0.0f;//scale add grid deck view
        renderer->RenderQuad(quad.get(), x, pos.actY, pos.actT, (scale-0.005f)+modxscale+gdvadd, (scale-0.005f)+modyscale+gdvadd);

        // Foil: animated holographic sheen for collectible foils (flag passed in from the
        // instance). This is the path used by the pack reveal, zone lists and previews.
        {
            if (foil)
            {
                float hw = 0.5f * scale * quad->mWidth;
                float hh = 0.5f * scale * quad->mHeight;
                float ph = (float) ((JGEGetTime() / 25) % 360) * 3.14159265f / 180.0f;
                int a = (int) (pos.actA * 0.42f);
                if (a < 0) a = 0;
                PIXEL_TYPE cols[4];
                for (int i = 0; i < 4; i++)
                {
                    float d = ph + i * 1.5708f;
                    int r = (int) (110 + 110 * sin(d));
                    int g = (int) (110 + 110 * sin(d + 2.0944f));
                    int b = (int) (110 + 110 * sin(d + 4.1888f));
                    cols[i] = ARGB(a, r, g, b);
                }
                renderer->SetTexBlend(BLEND_SRC_ALPHA, BLEND_ONE);
                renderer->FillRect(pos.actX - hw, pos.actY - hh, hw * 2, hh * 2, cols);
                renderer->SetTexBlend(BLEND_SRC_ALPHA, BLEND_ONE_MINUS_SRC_ALPHA);
            }
        }

        RenderCountersBig(card, pos);
        return;
    }

    //DebugTrace("Unable to fetch image: " << card->getImageName());

    // If we come here, we do not have the picture.
    if(myA)
        AlternateRender(card, pos);
}

string CardGui::FormattedData(string data, string replace, string value)
{
    size_t found = data.find(replace.c_str());
    if (found != string::npos)
    {
        size_t len = replace.length();
        string teste = data.replace(found,len,value);
        return teste;
    }
    else
    {
        return value;
    }

}

bool CardGui::FilterCard(MTGCard * _card,string filter)
{
    CardDescriptor  cd;
    MTGCardInstance * card = (MTGCardInstance*) _card->data;
    cd.init();
    cd.mode = CardDescriptor::CD_OR;
    while (filter.size())
    {
        
        string typeName;
        //Advanced cards caracteristics ?
         size_t found = filter.find("[");
        if (found != string::npos)
        {
            int nbminuses = 0;
            int end = filter.find("]");
            string attributes = filter.substr(found + 1, end - found - 1);
            
            while (attributes.size())
            {
                size_t found2 = attributes.find(";");
                size_t foundAnd = attributes.find("&");
                string attribute;
                if (found2 != string::npos)
                {
                    cd.mode = CardDescriptor::CD_OR;
                    attribute = attributes.substr(0, found2);
                    attributes = attributes.substr(found2 + 1);
                }
                else if (foundAnd != string::npos)
                {
                    cd.mode = CardDescriptor::CD_AND;
                    attribute = attributes.substr(0, foundAnd);
                    attributes = attributes.substr(foundAnd + 1);
                }
                else
                {
                    attribute = attributes;
                    attributes = "";
                }
                int minus = 0;
                if (attribute[0] == '-')
                {
                    minus = 1;
                    nbminuses++;
                    attribute = attribute.substr(1);
                }
                int comparisonMode = COMPARISON_NONE;
                int comparisonCriterion = 0;
                if (attribute.size() > 1)
                {
                    size_t operatorPosition = attribute.find("=", 1);
                    if (operatorPosition != string::npos)
                    {
                        string numberCD = attribute.substr(operatorPosition + 1, attribute.size() - operatorPosition - 1);
                        
                        switch (attribute[operatorPosition - 1])
                        {
                        case '<':
                            if (minus)
                            {
                                comparisonMode = COMPARISON_GREATER;
                            }
                            else
                            {
                                comparisonMode = COMPARISON_AT_MOST;
                            }
                            operatorPosition--;
                            break;
                        case '>':
                            if (minus)
                            {
                                comparisonMode = COMPARISON_LESS;
                            }
                            else
                            {
                                comparisonMode = COMPARISON_AT_LEAST;
                            }
                            operatorPosition--;
                            break;
                        default:
                            if (minus)
                            {
                                comparisonMode = COMPARISON_UNEQUAL;
                            }
                            else
                            {
                                comparisonMode = COMPARISON_EQUAL;
                            }
                        }
                        attribute = attribute.substr(0, operatorPosition);
                    }
                }

                //Attacker
                if (attribute.find("attacking") != string::npos)
                {
                    if (minus)
                    {
                        cd.attacker = -1;
                    }
                    else
                    {
                        cd.attacker = 1;
                    }
                }
                //Blocker
                else if (attribute.find("blocking") != string::npos)
                {
                    if (minus)
                    {
                        cd.defenser = &MTGCardInstance::NoCard;
                    }
                    else
                    {
                        cd.defenser = &MTGCardInstance::AnyCard;
                    }
                }
                //Tapped, untapped
                else if (attribute.find("tapped") != string::npos)
                {
                    if (minus)
                    {
                        cd.unsecureSetTapped(-1);
                    }
                    else
                    {
                        cd.unsecureSetTapped(1);
                    }
                }
                //Has been foretold
                else if (attribute.find("foretold") != string::npos)
                {
                    cd.foretellTurn = comparisonCriterion;
                    cd.foretoldComparisonMode = comparisonMode;
                }
                //Has been kicked
                else if (attribute.find("kicked") != string::npos)
                {
                    cd.kicked = comparisonCriterion;
                    cd.kickedComparisonMode = comparisonMode;
                }
                //Has kicker cost
                else if (attribute.find("haskicker") != string::npos)
                {
                    if (minus)
                    {
                        cd.unsecureSetHasKickerCost(-1);
                    }
                    else
                    {
                        cd.unsecureSetHasKickerCost(1);
                    }
                }
                //Has convoke cost
                else if (attribute.find("hasconvoke") != string::npos)
                {
                    if (minus)
                    {
                        cd.unsecureSetHasConvokeCost(-1);
                    }
                    else
                    {
                        cd.unsecureSetHasConvokeCost(1);
                    }
                }
                //Has flashback cost
                else if (attribute.find("hasflashback") != string::npos)
                {
                    if (minus)
                    {
                        cd.unsecureSetHasFlashbackCost(-1);
                    }
                    else
                    {
                        cd.unsecureSetHasFlashbackCost(1);
                    }
                }
                //Has backside
                else if (attribute.find("hasbackside") != string::npos)
                {
                    if (minus)
                    {
                        cd.unsecureSetHasBackSide(-1);
                    }
                    else
                    {
                        cd.unsecureSetHasBackSide(1);
                    }
                }
                //Is modified
                else if (attribute.find("modified") != string::npos)
                {
                    if (minus)
                    {
                        cd.unsecureSetModified(-1);
                    }
                    else
                    {
                        cd.unsecureSetModified(1);
                    }
                }
                //Has toxic
                else if (attribute.find("hastoxic") != string::npos)
                {
                    if (minus)
                    {
                        cd.unsecureSetHasToxic(-1);
                    }
                    else
                    {
                        cd.unsecureSetHasToxic(1);
                    }
                }
                //Has partner
                else if (attribute.find("haspartner") != string::npos)
                {
                    if (minus)
                    {
                        cd.unsecureSetHasPartner(-1);
                    }
                    else
                    {
                        cd.unsecureSetHasPartner(1);
                    }
                }
                //Is a permanent
                else if (attribute.find("permanent") != string::npos)
                {
                    if (minus)
                    {
                        cd.unsecureSetIsPermanent(-1);
                    }
                    else
                    {
                        cd.unsecureSetIsPermanent(1);
                    }
                }
                //Token
                else if (attribute.find("token") != string::npos)
                {
                    if (minus)
                    {
                        cd.isToken = -1;
                    }
                    else
                    {
                        cd.isToken = 1;
                    }
                }
                //Has been flipped
                else if (attribute.find("isflipped") != string::npos)
                {
                    if (minus)
                    {
                        cd.isFlipped = -1;
                    }
                    else
                    {
                        cd.isFlipped = 1;
                    }
                }
                //Card is commander
                else if (attribute.find("iscommander") != string::npos)
                {
                    if (minus)
                    {
                        cd.isCommander = -1;
                    }
                    else
                    {
                        cd.isCommander = 1;
                    }
                }
                //Card is Ring bearer
                else if (attribute.find("ringbearer") != string::npos)
                {
                    if (minus)
                    {
                        cd.isRingBearer = -1;
                    }
                    else
                    {
                        cd.isRingBearer = 1;
                    }
                }
                //Has x in cost
                else if (attribute.find("hasx") != string::npos)
                {
                    if (minus)
                    {
                        cd.hasXCost = -1;
                    }
                    else
                    {
                        cd.hasXCost = 1;
                    }
                }
                //has been discarded
                else if (attribute.find("discarded") != string::npos)
                {
                    if (minus)
                    {
                        cd.CDdiscarded = -1;
                    }
                    else
                    {
                        cd.CDdiscarded = 1;
                    }
                }
                //put in its zone this turn
                else if (attribute.find("fresh") != string::npos)
                {
                    if (minus)
                    {
                        cd.unsecuresetfresh(-1);
                    }
                    else
                    {
                        cd.unsecuresetfresh(1);
                    }
                }
                else if (attribute.find("recent") != string::npos)
                {
                    if (minus)
                    {
                        cd.unsecuresetrecent(-1);
                    }
                    else
                    {
                        cd.unsecuresetrecent(1);
                    }
                }
                else if (attribute.find("geared") != string::npos || attribute.find("equipped") != string::npos)
                {
                    if (minus)
                    {
                        cd.CDgeared = -1;
                    }
                    else
                    {
                        cd.CDgeared = 1;
                    }
                }
                else if (attribute.find("attached") != string::npos)
                {
                    if (minus)
                    {
                        cd.CDattached = -1;
                    }
                    else
                    {
                        cd.CDattached = 1;
                    }
                }
                //creature is a level up creature
                else if (attribute.find("leveler") != string::npos)
                {
                    if (minus)
                    {
                        cd.isLeveler = -1;
                    }
                    else
                    {
                        cd.isLeveler = 1;
                    }
                }
                //creature is enchanted
                else if (attribute.find("enchanted") != string::npos)
                {
                    if (minus)
                    {
                        cd.CDenchanted = -1;
                    }
                    else
                    {
                        cd.CDenchanted = 1;
                    }
                }
                //creature was damaged
                else if (attribute.find("damaged") != string::npos)
                {
                    if (minus)
                    {
                        cd.CDdamaged = -1;
                    }
                    else
                    {
                        cd.CDdamaged = 1;
                    }
                }
                //creature dealt damage to opponent
                else if (attribute.find("opponentdamager") != string::npos)
                {
                    if (minus)
                    {
                        cd.CDopponentDamaged = -1;
                    }
                    else
                    {
                        cd.CDopponentDamaged = 1;
                    }
                }
                //creature dealt damage to controller
                else if (attribute.find("controllerdamager") != string::npos)
                {
                    if (minus)
                    {
                        cd.CDcontrollerDamaged = -1;
                    }
                    else
                    {
                        cd.CDcontrollerDamaged = 1;
                    }
                }
                //creature dealt damage to anything
                else if (attribute.find("damager") != string::npos)
                {
                    if (minus)
                    {
                        cd.CDdamager = -1;
                    } 
                    else 
                    {
                        cd.CDdamager = 1;
                    }
                }
                else if (attribute.find("multicolor") != string::npos)
                {
                    //card is multicolored?
                    if (minus)
                    {
                        cd.setisMultiColored(-1);
                        cd.SetExclusionColor(0);//not multicolored is monocolored not colorless, use iscolorless attribute
                        cd.SetExclusionColor(6);//restriction... green, red, blue, black or white colored only
                        cd.mode = CardDescriptor::CD_OR;
                    }
                    else
                    {
                        cd.setisMultiColored(1);
                    }

                }
                else if (attribute.find("numofcols") != string::npos)
                {
                    //Number of color restrictions
                    cd.numofColors = comparisonCriterion;
                    cd.numofColorsComparisonMode = comparisonMode;
                }
                else if (attribute.find("power") != string::npos)
                {
                    //Power restrictions
                    cd.setPower(comparisonCriterion);
                    cd.powerComparisonMode = comparisonMode;
                }
                else if (attribute.find("toughness") != string::npos)
                {
                    //Toughness restrictions
                    cd.setToughness(comparisonCriterion);
                    cd.toughnessComparisonMode = comparisonMode;
                }
                else if (attribute.find("zpos") != string::npos)
                {//using > or < don't have effect unless like this: >= or <= or =
                    //zpos restrictions
                    cd.zposition = comparisonCriterion;
                    cd.zposComparisonMode = comparisonMode;
                }
                else if (attribute.find("manacost") != string::npos)
                {
                    //Manacost restrictions
                    cd.convertedManacost = comparisonCriterion;
                    cd.manacostComparisonMode = comparisonMode;
                }
                else
                {
                    int attributefound = 0;
                    ////Colors - remove Artifact and Land from the loop
                    
                    for (int cid = 1; cid < Constants::NB_Colors - 1; cid++)
                    { 
                        if (attribute.find(Constants::MTGColorStrings[cid]) != string::npos)
                        {
                            attributefound = 1;
                            if (minus)
                                cd.SetExclusionColor(cid);
                            else
                                cd.setColor(cid);
                        }
                    }
                    if (!attributefound)
                    {
                        //Abilities
                        for (int j = 0; j < Constants::NB_BASIC_ABILITIES; j++)
                        {
                            if (attribute.find(Constants::MTGBasicAbilities[j]) != string::npos)
                            {
                                attributefound = 1;
                                if (minus)
                                    cd.mAbilityExclusions.set(j);
                                else
                                    cd.basicAbilities.set(j);
                            }
                        }
                    }

                    if (!attributefound)
                    {
                        //Subtypes
                        if (minus)
                        {
                            cd.setNegativeSubtype(attribute);
                        }
                        else
                        {
                            cd.setSubtype(attribute);
                        }
                    }
                }
            }
            if (nbminuses)
                cd.mode = CardDescriptor::CD_AND;
            filter = filter.substr(0, found);
        }
        else
        {
            
            found = filter.find(",");
            
            if (found != string::npos)
            {
                cd.mode = CardDescriptor::CD_OR;
                typeName = filter.substr(0, found);
                filter = filter.substr(found + 1);
            }
            else
            {
                typeName = filter;
                filter = "";
            }
            
             cd.setSubtype(typeName);
        }

        
     } 
     if(cd.match(card)) 
         return true;
    return false;
    
}

void CardGui::RenderCountersBig(MTGCard * mtgcard, const Pos& pos, int drawMode)
{
    MTGCardInstance * card = dynamic_cast<MTGCardInstance*> (mtgcard);
    if (!card)
        return;

    int ringTemptations = 0;
    if(card->controller())
        ringTemptations = card->controller()->ringTemptations;

    if (!card->counters && !ringTemptations)
        return;
    if (!card->counters->mCount && !ringTemptations)
        return;

    // Write Named Counters
    WFont * font = WResourceManager::Instance()->GetWFont(Fonts::MAGIC_FONT);
    font->SetColor(ARGB((int)pos.actA, 0, 0, 0));
    font->SetScale(kWidthScaleFactor * pos.actZ);

    unsigned i = 0; 
    if (drawMode == DrawMode::kText)
    {
        std::vector<string> txt = card->data->getFormattedText();
        i = txt.size() + 1;
    }
    
    if (ringTemptations > 0 && card->name == "The Ring") // Added a label on Ring to show the number of temptations.
    {
        char buf[512];
        bool renderText = true;
        string gfx = "";
        if (counterGraphics.find("temptations") == counterGraphics.end())
        {
            string gfxRelativeName = "counters/";
            gfxRelativeName.append("temptations");
            gfxRelativeName.append(".png");
            string _gfx = WResourceManager::Instance()->graphicsFile(gfxRelativeName);
            if (!fileExists(_gfx.c_str()))
                _gfx = "";
            counterGraphics["temptations"] = _gfx;
        }
        gfx = counterGraphics["temptations"];
        if (gfx.size())
            renderText = false;
           
        if (renderText)
        {
            std::string s = "temptations";
            s[0] = toupper(s[0]);
            sprintf(buf, _("%s: %i").c_str(), s.c_str(), ringTemptations);
        }
        
        if (!gfx.size())
        {
            gfx = "counters/default.png";
        }
        
        float x = pos.actX + (22 - BigWidth / 2) * pos.actZ;
        float y =  pos.actY + (-BigHeight / 2 + 80 + 11 * i + 21 * 0) * pos.actZ;
        if (y > pos.actY + 105) 
        {
           y =  (-BigHeight / 2 + 80 + 11 * i) * pos.actZ + (y - 105 - 21);
           x +=  (BigWidth / 2) * pos.actZ;
        }

        if (gfx.size())
        {
            JQuadPtr q = WResourceManager::Instance()->RetrieveTempQuad(gfx);

            if (q.get() && q->mTex)
            {
                float scale = 20.f / q->mHeight;
                if (renderText)
                {
                    float scaleX = (font->GetStringWidth(buf) + 20) / q->mWidth;
                    JRenderer::GetInstance()->RenderQuad(q.get(), x, y, 0, scaleX, scale);
                }
                else
                {
                    JRenderer::GetInstance()->RenderQuad(q.get(), x + (scale * q->mWidth * 0), y, 0, scale, scale);
                }
            }
        }

        if (renderText)
        {
            font->SetColor(ARGB(255,0,0,0));
            font->DrawString(buf, x + 5, y + 5);
        }

        return; // No need to check counters on The Ring.
    }

    for (size_t t = 0; t < card->counters->counters.size(); t++)
    {
        Counter * c = card->counters->counters[t];

        if (!c || c->nb <= 0)
            continue;

        char buf[512];
        bool renderText = true;
        string gfx = "";
        //TODO cache the gfx fetch results?
        if (c->name.size()) 
        {
            if (c->nb < 6) //we only render a counter's specific quad if there are 5 counters of this type or less. Otherwise we will use the generic one
            {
                if (counterGraphics.find(c->name) == counterGraphics.end())
                {
                    string gfxRelativeName = "counters/";
                    gfxRelativeName.append(c->name);
                    gfxRelativeName.append(".png");
                    string _gfx = WResourceManager::Instance()->graphicsFile(gfxRelativeName);
                    if (!fileExists(_gfx.c_str()))
                        _gfx = "";
                    counterGraphics[c->name] = _gfx;
                }
                gfx = counterGraphics[c->name];
                if (gfx.size())
                    renderText = false;
            }

            if (renderText)
            {
                std::string s = c->name;
                s[0] = toupper(s[0]);
                sprintf(buf, _("%s: %i").c_str(), s.c_str(), c->nb);
            }
        }
        else
        {
            sprintf(buf, _("%s%i/%s%i").c_str(), ((c->power > 0) ? "+": ""), c->power * c->nb, ((c->toughness > 0) ? "+": ""),c->toughness* c->nb);
        }

        if (!gfx.size())
        {
            gfx = "counters/default.png";
        }
        
        float x = pos.actX + (22 - BigWidth / 2) * pos.actZ;
        float y =  pos.actY + (-BigHeight / 2 + 80 + 11 * i + 21 * t) * pos.actZ;
        if (y > pos.actY + 105) 
        {
           y =  (-BigHeight / 2 + 80 + 11 * i) * pos.actZ + (y - 105 - 21);
           x +=  (BigWidth / 2) * pos.actZ;
        }

        if (gfx.size())
        {
            JQuadPtr q = WResourceManager::Instance()->RetrieveTempQuad(gfx);

            if (q.get() && q->mTex)
            {
                float scale = 20.f / q->mHeight;
                if (renderText)
                {
                    float scaleX = (font->GetStringWidth(buf) + 20) / q->mWidth;
                    JRenderer::GetInstance()->RenderQuad(q.get(), x, y, 0, scaleX, scale);
                }
                else
                {
                    for (int j = 0; j < c->nb; ++j)
                    {
                        JRenderer::GetInstance()->RenderQuad(q.get(), x + (scale * q->mWidth * j), y, 0, scale, scale);
                    }
                }
            }
        }

        if (renderText)
        {
            font->SetColor(ARGB(255,0,0,0));
            font->DrawString(buf, x + 5, y + 5);
        }
    }
    
}

MTGCardInstance* CardView::getCard()
{
    return card;
}

TransientCardView::TransientCardView(MTGCardInstance* card, float x, float y)
    : CardGui(card, x, y)
{
}

TransientCardView::TransientCardView(MTGCardInstance* card, const Pos& ref)
    : CardGui(card, ref)
{
}
;

ostream& CardView::toString(ostream& out) const
{
    return (CardGui::toString(out) << " : CardView ::: card : " << card << ";  actX,actY : " << actX << "," << actY << "; t : "
        << t << " ; actT : " << actT);
}
ostream& CardGui::toString(ostream& out) const
{
    return (out << "CardGui ::: x,y " << x << "," << y);
}


SimpleCardEffectRotate::SimpleCardEffectRotate(float rotation): mRotation(rotation)
{
}
    
void SimpleCardEffectRotate::doEffect(Pos * card)
{
    card->t = mRotation;
}

void SimpleCardEffectRotate::undoEffect(Pos * card)
{
    card->t = 0;
}

SimpleCardEffectMask::SimpleCardEffectMask(PIXEL_TYPE mask): mMask(mask)
{
}
    
void SimpleCardEffectMask::doEffect(Pos * card)
{
    card->mask = mMask;
}

void SimpleCardEffectMask::undoEffect(Pos * card)
{
    card->mask = 0;
}

