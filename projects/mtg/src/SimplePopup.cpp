/*
 * SimplePopup.cpp
 *
 *  Created on: Nov 18, 2010
 *      Author: Michael
 *  The popups all have a default button created to act as the dismiss button, mapped to JGE_BTN_CANCEL (triangle) with "Detailed Info" to use for the button width.
 */

#include "PrecompiledHeader.h"
#include "SimplePopup.h"
#include "JTypes.h"
#include "GameApp.h"
#include "UITheme.h"
#include "DeckStats.h"
#include "DeckManager.h"
#include <iomanip>

SimplePopup::SimplePopup(int id, JGuiListener* listener, const int fontId, const char * _title, DeckMetaData* deckMetaData, MTGAllCards * collection, float cancelX, float cancelY) :
    JGuiController(JGE::GetInstance(), id, listener), mCollection(collection)
{
    mX = 19;
    mY = 66;
    mWidth = 180.0f;
    mTitle = _title;
    mMaxLines = 12;

    mTextFont = WResourceManager::Instance()->GetWFont(fontId);
    this->mCount = 1; // a hack to ensure the menus do book keeping correctly.  Since we aren't adding items to the menu, this is required
    mStatsWrapper = NULL;
    
    JGuiController::Add(NEW InteractiveButton(this, kDismissButtonId, Fonts::MAIN_FONT, "Detailed Info", cancelX, cancelY, JGE_BTN_CANCEL), true);
    Update(deckMetaData);
}

void SimplePopup::Render()
{
    mClosed = false;
    JRenderer *r = JRenderer::GetInstance();
    string detailedInformation = getDetailedInformation(mDeckInformation->getFilename());

    // Drawn glass panel (UITheme), pinned to the TOP-RIGHT and right-aligned to the screen, clear
    // of the deck panel on the left (no overlap). Replaces the old statsholder.png frame texture.
    const float pw = 196.0f, ph = 178.0f;
    const float px = SCREEN_WIDTH_F - 12.0f - pw;
    const float py = 10.0f;
    UITheme::drawPanel(r, px, py, pw, ph);

    mTextFont->SetColor(ARGB(255, 240, 240, 245));
    mTextFont->DrawString(detailedInformation.c_str(), px + 9.0f, py + 12.0f);

    // Deck colour symbols along the bottom of the panel (same icons as the deck screens).
    if (mDeckInformation)
    {
        string colors = mDeckInformation->getColorIndex();
        if (colors.size() >= 6)
        {
            float ix = px + 16.0f;
            float iy = py + ph - 15.0f;
            for (int c = Constants::MTG_COLOR_ARTIFACT; c < Constants::MTG_COLOR_WASTE; ++c)
            {
                if (c < (int) manaIcons.size() && colors.at(c) == '1' && manaIcons[c].get())
                {
                    r->RenderQuad(manaIcons[c].get(), ix, iy, 0, 0.6f, 0.6f);
                    ix += 22.0f;
                }
            }
        }
    }
}

// draws a bounding box around the popup.
void SimplePopup::drawBoundingBox( float x, float y, float width, float height )
{

    //draw the corners
    string topCornerImageName = "top_corner.png";
    string bottomCornerImageName = "bottom_corner.png";
    string verticalBarImageName = "vert_bar.png";
    string horizontalBarImageName = "top_bar.png";

    const float boxWidth    = ( width + 15 ) / 3.0f;
    const float boxHeight = ( height + 15 ) / 3.0f;

    drawHorzPole( horizontalBarImageName, false, false, x, y, boxWidth );
    drawHorzPole( horizontalBarImageName, false, true, x, y + height, boxWidth );
    
    drawVertPole( verticalBarImageName, false, false, x, y, boxHeight );
    drawVertPole( verticalBarImageName, true, false, x + width, y, boxHeight );

    drawCorner( topCornerImageName, false, false, x, y );
    drawCorner( topCornerImageName, true, false, x + width, y );
    drawCorner( bottomCornerImageName, false, false, x, y + height );
    drawCorner( bottomCornerImageName, true, false, x + width, y + height );
}

void SimplePopup::Update(DeckMetaData* selectedDeck)
{
    mDeckInformation = selectedDeck;
    
    // get the information from the cache, if it doesn't exist create an entry
    mStatsWrapper = DeckManager::GetInstance()->getExtendedDeckStats( mDeckInformation, mCollection, (mDeckInformation->getFilename().find("baka") != string::npos) );
    
}


string SimplePopup::getDetailedInformation(string)
{
    ostringstream oss;
    oss
        << "------- Deck Summary -----" << endl
        << "Cards: "<< mStatsWrapper->cardCount << endl
        << "Creatures: "<< setw(2) << mStatsWrapper->countCreatures
        << "  Enchantments: " << mStatsWrapper->countEnchantments << endl
        << "Instants: " << setw(4) << mStatsWrapper->countInstants
        << "   Sorceries:      " << setw(2) << mStatsWrapper->countSorceries << endl
        << "Lands: "
        << "A: " << setw(2) << left  << mStatsWrapper->countLandsPerColor[ Constants::MTG_COLOR_ARTIFACT ] + mStatsWrapper->countBasicLandsPerColor[ Constants::MTG_COLOR_ARTIFACT ] << " "
        << "G: " << setw(2) << left  << mStatsWrapper->countLandsPerColor[ Constants::MTG_COLOR_GREEN ] + mStatsWrapper->countLandsPerColor[ Constants::MTG_COLOR_GREEN ] << " "
        << "R: " << setw(2) << left  << mStatsWrapper->countLandsPerColor[ Constants::MTG_COLOR_RED ] + mStatsWrapper->countBasicLandsPerColor[ Constants::MTG_COLOR_RED ] << " "
        << "U: " << setw(2) << left  << mStatsWrapper->countLandsPerColor[ Constants::MTG_COLOR_BLUE ] + mStatsWrapper->countBasicLandsPerColor[ Constants::MTG_COLOR_BLUE ] << " "
        << "B: " << setw(2) << left  << mStatsWrapper->countLandsPerColor[ Constants::MTG_COLOR_BLACK ] + mStatsWrapper->countBasicLandsPerColor[ Constants::MTG_COLOR_BLACK ] << " "
        << "W: " << setw(2) << left  << mStatsWrapper->countLandsPerColor[ Constants::MTG_COLOR_WHITE ] + mStatsWrapper->countBasicLandsPerColor[ Constants::MTG_COLOR_WHITE ] << endl
        << "  --- Mana Curve ---  " << endl;

    for ( int costIdx = 0; costIdx < Constants::STATS_MAX_MANA_COST+1; ++costIdx )
            if ( mStatsWrapper->countCardsPerCost[ costIdx ] > 0 )
                oss << costIdx << ": " << setw(2) << left << mStatsWrapper->countCardsPerCost[ costIdx ] << "  ";

    oss << endl;

    oss
        << " --- Average Cost --- " << endl
        << "Creature: "<< setprecision(2) << mStatsWrapper->avgCreatureCost << endl
        << "Mana: " << setprecision(2) << mStatsWrapper->avgManaCost << "   "
        << "Spell: " << setprecision(2) << mStatsWrapper->avgSpellCost << endl;

    return oss.str();
}

void SimplePopup::Update(float)
{
    JButton key = mEngine->ReadButton();
    // Touch-friendly: tapping anywhere (including the Info button that opened this popup)
    // dismisses it, instead of requiring a tap on the small dismiss button. The listener's
    // handler for this popup id closes and deletes the popup.
    int x, y;
    if (mEngine->GetLeftClickCoordinates(x, y))
    {
        mEngine->LeftClickedProcessed();
        if (mListener) mListener->ButtonPressed(mId, kInfoMenuID);
        return;
    }
    CheckUserInput(key);
}

// drawing routines
void SimplePopup::drawCorner(string imageName, bool flipX, bool flipY, float x, float y)
{
    LOG(" Drawing a Corner! ");
    JRenderer* r = JRenderer::GetInstance();
    JQuadPtr horizontalBarImage = WResourceManager::Instance()->RetrieveTempQuad( imageName, TEXTURE_SUB_5551);
    horizontalBarImage->SetHFlip(flipX);
    horizontalBarImage->SetVFlip(flipY);

    r->RenderQuad(horizontalBarImage.get(), x, y);
    LOG(" Done Drawing a Corner! ");
}

void SimplePopup::drawHorzPole(string imageName, bool flipX = false, bool flipY = false, float x = 0, float y = 0, float width = SCREEN_WIDTH_F)
{
    LOG(" Drawing a horizontal border! ");
    JRenderer* r = JRenderer::GetInstance();
    JQuadPtr horizontalBarImage = WResourceManager::Instance()->RetrieveTempQuad( imageName, TEXTURE_SUB_5551);
    if ( horizontalBarImage != NULL )
    {
    horizontalBarImage->SetHFlip(flipX);
    horizontalBarImage->SetVFlip(flipY);

    r->RenderQuad(horizontalBarImage.get(), x, y, 0, width);
    }
    else
    {
        LOG ( "ERROR: Error trying to render horizontal edge! ");
    }
    LOG(" Done Drawing a horizontal border! ");
}

void SimplePopup::drawVertPole(string imageName, bool flipX = false, bool flipY = false, float x = 0, float y = 0, float height = SCREEN_HEIGHT_F)
{
    LOG(" Drawing a Vertical border! ");
    JRenderer* r = JRenderer::GetInstance();
    JQuadPtr verticalBarImage = WResourceManager::Instance()->RetrieveTempQuad( imageName, TEXTURE_SUB_5551);
    if ( verticalBarImage != NULL )
    {
        verticalBarImage->SetHFlip(flipX);
        verticalBarImage->SetVFlip(flipY);

        r->RenderQuad(verticalBarImage.get(), x, y, 0, 1.0f, height);
    }
    else
    {
        LOG ( "ERROR: Error trying to render vertical edge! ");
    }
    LOG(" DONE Drawing a horizontal border! ");
}


void SimplePopup::Close()
{
    mClosed = true;
    mCount = 0;
}

SimplePopup::~SimplePopup(void)
{
    mTextFont = NULL;
    mDeckInformation = NULL;
}

