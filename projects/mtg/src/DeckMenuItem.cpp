#include "PrecompiledHeader.h"

#include "DeckMenuItem.h"
#include "Translate.h"
#include "WResourceManager.h"
#include <algorithm>

#define ITEM_PX_WIDTH (SCREEN_WIDTH_F * (190.0f / 480.0f))
#define kItemXOffset ((int)(SCREEN_WIDTH_F * (22.0f / 480.0f)))
#define kItemYHeight ((int)(SCREEN_HEIGHT_F * (30.0f / 272.0f)))

const int kHorizontalScrollSpeed = 30; // higher numbers mean faster scrolling

float DeckMenuItem::mYOffset = 0;

DeckMenuItem::DeckMenuItem(DeckMenu* _parent, int id, int fontId, string text, float x, float y, bool hasFocus, bool autoTranslate, DeckMetaData *deckMetaData)
        : JGuiObject(id), parent(_parent), fontId(fontId), mX(x), mY(y)
{
    WFont * mFont = WResourceManager::Instance()->GetWFont(fontId);
    mMetaData = deckMetaData;
    mText = trim(text);
    mIsValidSelection = false;

    if (autoTranslate)
        mText = _(mText);


    mHasFocus = hasFocus;
    float newImageWidth = 0.0f;
    if (mMetaData && !mMetaData->getGamesPlayed())
    {
        JTexture * tex = WResourceManager::Instance()->RetrieveTexture("new.png");
        if (tex)
        {
            JQuadPtr quad = WResourceManager::Instance()->RetrieveQuad("new.png", 2.0f, 2.0f, tex->mWidth - 4.0f, tex->mHeight - 4.0f); //avoids weird rectangle around the texture because of bilinear filtering
            newImageWidth = quad->mWidth;
        }
    }

    float titleStringWidth = mFont->GetStringWidth( mText.c_str() );
    mTitleResetWidth = (titleStringWidth - newImageWidth )/ 2;
    mScrollEnabled = titleStringWidth  > ( ITEM_PX_WIDTH - newImageWidth );
    mScrollerOffset = 0.0f;

    if (hasFocus)
    {
        mIsValidSelection = true;
        Entering();
    }

    if (mMetaData && mMetaData->getAvatarFilename().size() > 0)
    {
        mImageFilename = mMetaData->getAvatarFilename();
        if(!(WResourceManager::Instance()->RetrieveTexture(mImageFilename)))
            mImageFilename = "baka.jpg"; // if the AI deck has no specific avatar we will display the default "baka.jpg" image.
    }
    else
    {
        // this is a non-deck menu item (ie "Random", "Cancel", etc
        switch(id)
        {
            case kRandomPlayerMenuID:
                mImageFilename = "noavatar.jpg";
                break;
            case kRandomAIPlayerMenuID:
                mImageFilename = "noavatar.jpg";
                break;
            case kCommanderMenuID:
                mImageFilename = "noavatar.jpg";
                break;
            case kEvilTwinMenuID:
            {
                mImageFilename = "avatar.jpg";
                break;
            }
            default:
                mImageFilename = "noavatar.jpg";
                // do nothing.
                break;
        }

    }


    mDisplayInitialized = false;

}

void DeckMenuItem::Update(float dt)
{
    mScrollerOffset += kHorizontalScrollSpeed * dt;
    if ( (mScrollerOffset) > mTitleResetWidth )
        mScrollerOffset = -ITEM_PX_WIDTH;
}


void DeckMenuItem::RenderWithOffset(float yOffset)
{
    mYOffset = yOffset;

    WFont * mFont = WResourceManager::Instance()->GetWFont(fontId);

    if (!( mHasFocus && mScrollEnabled ))
        mScrollerOffset = 0;
    if (!mHasFocus && mScrollEnabled)
        mScrollerOffset = -1 * ( getWidth() - ITEM_PX_WIDTH )/2;
    float offSet = mScrollerOffset;

    if (mHasFocus)
        mFont->SetScale(SCALE_SELECTED);
    else
        mFont->SetScale(SCALE_NORMAL);

    int rowAlign = (parent && parent->usesLeftAlignedList()) ? JGETEXT_LEFT : JGETEXT_CENTER;
    mFont->DrawString(mText.c_str(), mX, mY + yOffset, rowAlign, offSet, ITEM_PX_WIDTH);
    mDisplayInitialized = true;
    //Render a "new" icon for decks that have never been played yet (suppressed on the modernized
    //list-only screens -- deck picker / opponent chooser / deck-editor selection -- where the info
    //panel already shows "Games Played: 0" and the badge is just noise).
    if (mMetaData && !mMetaData->getGamesPlayed() && !(parent && parent->usesLeftAlignedList()))
    {
        JTexture * tex = WResourceManager::Instance()->RetrieveTexture("new.png");
        if (tex)
        {
            JQuadPtr quad = WResourceManager::Instance()->RetrieveQuad("new.png", 2.0f, 2.0f, tex->mWidth - 0.0f, tex->mHeight - 0.0f); //avoids weird rectangle aroudn the texture because of bilinear filtering
            quad->SetHotSpot(quad->mWidth/2.0f, quad->mHeight/2.0f);
            float x = mX + min(ITEM_PX_WIDTH - quad->mWidth, getWidth() )/2 + quad->mWidth/2;
            if (quad) JRenderer::GetInstance()->RenderQuad(quad.get(), x , mY + yOffset + quad->mHeight/2, 0, 16.f / quad->mWidth, 12.f / quad->mHeight);
        }
    }
    mFont->SetScale(SCALE_NORMAL);
}

void DeckMenuItem::Render()
{
    RenderWithOffset(0);
}

void DeckMenuItem::checkUserClick()
{
    int x1 = -1, y1 = -1;
    if (mEngine->GetLeftClickCoordinates(x1, y1))
    {
        mIsValidSelection = false;
        int x2 = kItemXOffset, y2 = static_cast<int>(mY + mYOffset);
        if ( (x1 >= x2) && (x1 <= (x2 + ITEM_PX_WIDTH)) && (y1 >= y2) && (y1 < (y2 + kItemYHeight)))
            mIsValidSelection = true;
    }
    else
        mIsValidSelection = true;
}


void DeckMenuItem::Entering()
{
    checkUserClick();
    mHasFocus = true;
    parent->mSelectionTargetY = mY;
}

bool DeckMenuItem::Leaving(JButton)
{
    // check to see if the user clicked on the object, if so return true.
    checkUserClick();
    mHasFocus = false;
    return true;
}

bool DeckMenuItem::ButtonPressed()
{
    return mIsValidSelection;
}

bool DeckMenuItem::HitTest(float px, float py)
{
    // Same row bounds as checkUserClick(): the item occupies a fixed-width row anchored
    // at kItemXOffset, mY + mYOffset.
    float x2 = (float)kItemXOffset;
    float y2 = mY + mYOffset;
    return (px >= x2 && px <= x2 + (float)ITEM_PX_WIDTH &&
            py >= y2 && py <= y2 + (float)kItemYHeight);
}

void DeckMenuItem::Relocate(float x, float y)
{
    mX = x;
    mY = y;
}

float DeckMenuItem::getWidth() const
{
    WFont * mFont = WResourceManager::Instance()->GetWFont(fontId);
    return mFont->GetStringWidth(mText.c_str());
}

string DeckMenuItem::getDeckName() const
{
    if (mMetaData)
        return mMetaData->getName();

    std::string s;
    std::stringstream out;
    out << mMetaData->getDeckId();
    s = out.str();
    return "[deck" + s + "]";
}

ostream& DeckMenuItem::toString(ostream& out) const
{
    return out << "DeckMenuItem ::: mHasFocus : " << mHasFocus
               << " ; parent : " << parent
               << " ; mText : " << mText
               << " ; mX,mY : " << mX << "," << mY;
}

DeckMenuItem::~DeckMenuItem()
{
    mMetaData = NULL;
}