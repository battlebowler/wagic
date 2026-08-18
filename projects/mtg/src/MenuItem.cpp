#include "PrecompiledHeader.h"

#include "MenuItem.h"
#include "Translate.h"
#include "WResourceManager.h"
#include "ModRules.h"

MenuItem::MenuItem(int id, WFont *font, string text, float x, float y, JQuad * _off, JQuad * _on, const char * particle,
                   JQuad * particleTex, bool hasFocus) :
        JGuiObject(id), mFont(font), mX(x), mY(y)
{
    mText = _(text);
    updatedSinceLastRender = 1;
    mParticleSys = NULL;
    hgeParticleSystemInfo * psi = WResourceManager::Instance()->RetrievePSI(particle, particleTex);
    if (psi)
    {
        mParticleSys = NEW hgeParticleSystem(psi);
        mParticleSys->MoveTo(mX, mY);
    }

    mHasFocus = hasFocus;
    lastDt = 0.001f;
    mScale = 1.0f;
    mTargetScale = 1.0f;
    mHitHalfWidth = 22.0f; // default tap half-width for non-row items
    mRowSelect = false;

    onQuad = _on;
    offQuad = _off;

    if (hasFocus)
        Entering();
}

void MenuItem::Render()
{
    JRenderer * renderer = JRenderer::GetInstance();

    if (mHasFocus)
    {
        // The old "fire" particle effect that indicated the focused menu item has been
        // removed; the per-icon label + scale is enough of a focus cue on touch.
        mFont->SetColor(ARGB(255,255,255,255));
        offQuad->SetColor(ARGB(60,255,255,255));
        renderer->RenderQuad(offQuad, SCREEN_WIDTH, SCREEN_HEIGHT / 2, 0, 8, 8);//big icon main menu right side
        offQuad->SetColor(ARGB(255,255,255,255));
        onQuad->SetColor(ARGB(255,255,255,255));
        // (The per-icon label below now shows the name; no central hover label.)
        renderer->RenderQuad(onQuad, mX, mY, 0, mScale, mScale);

    }
    else
    {
        renderer->RenderQuad(offQuad, mX, mY, 0, mScale, mScale);
    }

    // Always show each item's name beneath its icon. Touch has no hover, so the old
    // behaviour (only the focused item's name shown) left the icons unlabelled.
    {
        float oldScale = mFont->GetScale();
        mFont->SetScale(0.6f);
        mFont->SetColor(mHasFocus ? ARGB(255, 255, 255, 255) : ARGB(255, 200, 200, 200));
        mFont->DrawString(mText.c_str(), mX, mY + 22.0f, JGETEXT_CENTER);
        mFont->SetColor(ARGB(255, 255, 255, 255));
        mFont->SetScale(oldScale);
    }

    updatedSinceLastRender = 0;
}

void MenuItem::Update(float dt)
{
    updatedSinceLastRender = 1;
    lastDt = dt;
    if (mScale < mTargetScale)
    {
        mScale += 8.0f * dt;
        if (mScale > mTargetScale)
            mScale = mTargetScale;
    }
    else if (mScale > mTargetScale)
    {
        mScale -= 8.0f * dt;
        if (mScale < mTargetScale)
            mScale = mTargetScale;
    }

    if (mParticleSys)
        mParticleSys->Update(dt);
}

void MenuItem::Entering()
{
    if (mParticleSys)
        mParticleSys->Fire();
    mHasFocus = true;
    mTargetScale = 1.2f;
}

bool MenuItem::Leaving(JButton)
{
    if (mParticleSys)
        mParticleSys->Stop(true);
    mHasFocus = false;
    mTargetScale = 1.0f;
    return true;
}

bool MenuItem::ButtonPressed()
{
    return true;
}

bool MenuItem::HitTest(float px, float py)
{
    // Touch-friendly hit zone. The icon is drawn centred on (mX, mY) with its label just
    // below. The vertical band is generous so a tap anywhere in the icon row engages.
    float s = (mScale > 0.0f) ? mScale : 1.0f;
    float halfIcon = 18.0f * s;
    float top = mY - halfIcon - 24.0f;
    float bottom = mY + 34.0f; // covers the label rendered at mY + 22
    if (py < top || py > bottom)
        return false;
    // Row icons: accept any tap in the band and let the controller pick the horizontally
    // nearest icon. This avoids dead zones (e.g. the right half of the last/rightmost icon,
    // which has no neighbour to its right) and wrong-neighbour selection between icons.
    if (mRowSelect)
        return true;
    float halfW = (mHitHalfWidth > halfIcon) ? mHitHalfWidth : halfIcon;
    return (px >= mX - halfW && px <= mX + halfW);
}

MenuItem::~MenuItem()
{
    SAFE_DELETE(mParticleSys);
}

ostream& MenuItem::toString(ostream& out) const
{
    return out << "MenuItem ::: mHasFocus : " << mHasFocus << " ; mFont : " << mFont << " ; mText : " << mText << " ; mX,mY : "
               << mX << "," << mY << " ; updatedSinceLastRender : " << updatedSinceLastRender << " ; lastDt : " << lastDt
               << " ; mScale : " << mScale << " ; mTargetScale : " << mTargetScale << " ; onQuad : " << onQuad
               << " ; offQuad : " << offQuad << " ; mParticleSys : " << mParticleSys;
}

OtherMenuItem::OtherMenuItem(int id, WFont *font, string text, float x, float y, JQuad * _off, JQuad * _on, JButton _key, bool hasFocus) :
        MenuItem(id, font, text, x, y, _off, _on, "", WResourceManager::Instance()->GetQuad("particles").get(), hasFocus), mKey(_key), mTimeIndex(0)
{

}

OtherMenuItem::~OtherMenuItem()
{

}

void OtherMenuItem::Render()
{
    int alpha = 255;
    if (GetId() == MENUITEM_TROPHIES && options.newAward())
        alpha = (int) (sin(mTimeIndex) * 255);

    float olds = mFont->GetScale();
    float xPos = SCREEN_WIDTH - 64;
    float xTextPos = xPos + 54;
    float yPos = SCREEN_HEIGHT_F-26.f;
    int textAlign = JGETEXT_RIGHT;
    //onQuad->SetHFlip(false);

    switch(mKey)
    {
        case JGE_BTN_PREV:
            xPos = 5;
            xTextPos = xPos + 10;
            textAlign = JGETEXT_LEFT;
            //onQuad->SetHFlip(true);
            break;
        default:
            break;
    }

    //onQuad->SetColor(ARGB(abs(alpha),255,255,255));
    mFont->SetScale(1.0f);
    mFont->SetScale(50.0f / mFont->GetStringWidth(mText.c_str()));
    //JRenderer::GetInstance()->RenderQuad(onQuad, xPos, yPos+2, 0, mScale, mScale);
    //JRenderer::GetInstance()->FillRoundRect(xPos,yPos+2,mFont->GetStringWidth(mText.c_str()),mFont->GetHeight(),2,ARGB(abs(alpha),255,255,255));
    JRenderer::GetInstance()->FillRoundRect(xPos+1, yPos+6, mFont->GetStringWidth(mText.c_str()) - 3, mFont->GetHeight() - 10, 5, ARGB(abs(alpha), 5, 5, 5));
    if(!mHasFocus)
    {
        mFont->SetColor(ARGB(abs(alpha),255,255,255));
        JRenderer::GetInstance()->FillRoundRect(xPos, yPos+5, mFont->GetStringWidth(mText.c_str()) - 3, mFont->GetHeight() - 10, 5, ARGB(abs(alpha), 140, 23, 23));
    }
    else
    {
        mFont->SetColor(ARGB(abs(alpha),5,5,5));
        JRenderer::GetInstance()->FillRoundRect(xPos, yPos+5, mFont->GetStringWidth(mText.c_str()) - 3, mFont->GetHeight() - 10, 5, ARGB(abs(alpha), 140, 140, 140));
    }
    JRenderer::GetInstance()->DrawRoundRect(xPos, yPos+5, mFont->GetStringWidth(mText.c_str()) - 3, mFont->GetHeight() - 10, 5, ARGB(abs(alpha-20), 5, 5, 5));
    mFont->DrawString(mText, xTextPos, yPos+9, textAlign);
    mFont->SetScale(olds);
}

void OtherMenuItem::Update(float dt)
{
    MenuItem::Update(dt);
    mTimeIndex += 2*dt;
}

bool OtherMenuItem::HitTest(float px, float py)
{
    // The corner pills render across the bottom edge (Trophy Room bottom-right at
    // SCREEN_WIDTH-64.., PREV bottom-left at 5..). Accept the whole corner band so both
    // words are tappable, rather than MenuItem's narrow mX±22 zone (only the first word).
    if (py < SCREEN_HEIGHT_F - 30.0f)
        return false;
    if (mKey == JGE_BTN_PREV)
        return px <= 78.0f;               // bottom-left corner
    return px >= SCREEN_WIDTH_F - 78.0f;  // bottom-right corner (e.g. Trophy Room)
}