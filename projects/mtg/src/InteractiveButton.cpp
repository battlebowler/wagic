//
//  InteractiveButton.cpp
//  wagic
//
//  Created by Michael Nguyen on 1/23/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include "PrecompiledHeader.h"

#include "InteractiveButton.h"
#include "Translate.h"
#include "JTypes.h"
#include "WResourceManager.h"
#include "WFont.h"

const int kButtonHeight = 30;

InteractiveButton::InteractiveButton(JGuiController* _parent, int id, int fontId, string text, float x, float y, JButton actionKey, bool hasFocus, bool autoTranslate) :
SimpleButton( _parent, id, fontId, text, x, y, hasFocus, autoTranslate)
{
    setIsSelectionValid(false); // by default it's turned off since you can't auto select it.
    mActionKey = actionKey;
}

void InteractiveButton::Entering()
{  
}

void InteractiveButton::checkUserClick()
{
    int x1 = -1, y1 = -1;
    if (mEngine->GetLeftClickCoordinates(x1, y1))
    {   
        setIsSelectionValid(false);
        int buttonImageWidth = static_cast<int>(GetWidth());
        int x2 = static_cast<int>(getX()), y2 = static_cast<int>(getY() + mYOffset);
        int buttonHeight = kButtonHeight;
        if ( (x1 >= x2) && (x1 <= (x2 + buttonImageWidth)) && (y1 >= y2) && (y1 < (y2 + buttonHeight)))
            setIsSelectionValid( true );
    }
    else
        setIsSelectionValid( false );
}

bool InteractiveButton::ButtonPressed()
{
    checkUserClick();
    if (isSelectionValid())
    {
        mEngine->ReadButton();
        mEngine->LeftClickedProcessed();
        mEngine->HoldKey_NoRepeat( mActionKey );
        setIsSelectionValid(false);
        return true;
    }
    
    return false;
}

void InteractiveButton::Render()
{
    // Always render the button and its label. Previously the label only appeared while
    // the button was being pressed (a hover-style reveal), which on touch meant the
    // button names were never visible. isSelectionValid() is now only used to highlight
    // the currently-pressed button.
    bool pressed = isSelectionValid();
    JRenderer *renderer = JRenderer::GetInstance();
    WFont *mainFont = WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT);
    // Pin the shared font to scale 1.0 while measuring/drawing so every standard button is the
    // SAME size regardless of whatever scale the surrounding screen left the font at. Restored
    // at the end so we don't disturb later text.
    float _btnOldScale = mainFont->GetScale();
    mainFont->SetScale(1.0f);
    const string detailedInfoString = _(getText());
    float stringWidth = mainFont->GetStringWidth(detailedInfoString.c_str());
    float pspIconsSize = 0.5;
    float mainFontHeight = mainFont->GetHeight();
    float boxStartX =  getX() - 4;
    mXOffset = 0;
    mYOffset = 0;

#ifndef TOUCH_ENABLED
    renderer->FillRoundRect(boxStartX, getY(), stringWidth - 3, mainFontHeight - 9, 5, ARGB(0, 0, 0, 0));
#else
    renderer->FillRoundRect(boxStartX+1, getY()+1, stringWidth - 3, mainFontHeight - 4, 5, ARGB(220, 5, 5, 5));
    renderer->FillRoundRect(boxStartX, getY(), stringWidth - 3, mainFontHeight - 4, 5, ARGB(255, 140, 23, 23));
    renderer->DrawRoundRect(boxStartX, getY(), stringWidth - 3, mainFontHeight - 4, 5, ARGB(255, 5, 5, 5));
    mYOffset += 2;
#endif

    float buttonXOffset = getX() - mXOffset;
    float buttonYOffset = getY() + mYOffset;
    if (buttonImage != NULL)
    {
        renderer->RenderQuad(buttonImage.get(), buttonXOffset - buttonImage.get()->mWidth/2, buttonYOffset + mainFontHeight/2, 0, pspIconsSize, pspIconsSize);
    }
    mainFont->SetColor(ARGB(255, 255, 255, 255));
    (void)pressed;
    mainFont->DrawString(detailedInfoString, buttonXOffset, buttonYOffset);
    mainFont->SetScale(_btnOldScale);
}

void InteractiveButton::layoutRowRight(const std::vector<InteractiveButton*>& btns, float rightEdge, float y, float gap)
{
    WFont * f = WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT);
    if (!f) return;
    float oldScale = f->GetScale();
    f->SetScale(1.0f);
    float rightX = rightEdge;   // pill right edge available for the current (rightmost-first) button
    for (int i = (int) btns.size() - 1; i >= 0; --i)
    {
        InteractiveButton * b = btns[i];
        if (!b) continue;
        // Render draws the pill spanning [getX-4 .. getX + sw + 3] (FillRoundRect adds 2*radius=10
        // to the sw-3 width), so place getX so the pill's right edge lands on rightX.
        float sw = f->GetStringWidth(_(b->getText()).c_str());
        float gx = rightX - sw - 3.0f;
        b->setX(gx);
        b->setY(y);
        rightX = (gx - 4.0f) - gap;   // next (leftward) button's pill right edge
    }
    f->SetScale(oldScale);
}

void InteractiveButton::setImage( const JQuadPtr imagePtr )
{
    buttonImage = imagePtr;
    float imageXOffset = getX() - buttonImage.get()->mWidth;
    
    if (imageXOffset < 0)
        setX( getX() - imageXOffset/2 + 5 );
}

/* Accessors */

ostream& InteractiveButton::toString(ostream& out) const
{
    return out << "InteractiveButton ::: mHasFocus : " << hasFocus()
    << " ; parent : " << getParent()
    << " ; mText : " << getText()
    << " ; mScale : " << getScale()
    << " ; mTargetScale : " << getTargetScale()
    << " ; mX,mY : " << getX() << "," << getY();
}
