//-------------------------------------------------------------------------------------
//
// JGE++ is a hardware accelerated 2D game SDK for PSP/Windows.
//
// Licensed under the BSD license, see LICENSE in JGE root for details.
//
// Copyright (c) 2007 James Hui (a.k.a. Dr.Watson) <jhkhui@gmail.com>
//
//-------------------------------------------------------------------------------------

#include "../include/JGE.h"
#include "../include/JGui.h"
#include "../include/JRenderer.h"
#include <cmath>

JGE* JGuiObject::mEngine = NULL;

JGuiObject::JGuiObject(int id) :
    mId(id)
{
    mEngine = JGE::GetInstance();
}

JGuiObject::~JGuiObject()
{
    //    JGERelease();
}

bool JGuiObject::Leaving(JButton key __attribute__((unused)))
{
    return true;
}

bool JGuiObject::ButtonPressed()
{
    return false;
}

void JGuiObject::Entering()
{

}

int JGuiObject::GetId()
{
    return mId;
}

void JGuiObject::Update(float dt __attribute__((unused)))
{
}

ostream& operator<<(ostream &out, const JGuiObject &j)
{
    return j.toString(out);
}

JGuiController::JGuiController(JGE* jge, int id, JGuiListener* listener) :
    mEngine(jge), mId(id), mListener(listener)
{
    mBg = NULL;
    mShadingBg = NULL;

    mCount = 0;
    mCurr = 0;

    mCursorX = SCREEN_WIDTH / 2;
    mCursorY = SCREEN_HEIGHT / 2;
    mShowCursor = false;

    mActionButton = JGE_BTN_OK;
    mCancelButton = JGE_BTN_MENU;

    // No wrapping: lists clamp at the first/last item instead of jumping back to the
    // other end. This matches touch scrolling expectations (a swipe past the end stops).
    mStyle = 0;

    mActive = true;

    mUseScroll = false;
    mTapSelectsOnly = false;
    mScrollPx = 0.0f;
    mScrollMax = 0.0f;
    mDragLastY = -9999.0f;
}

void JGuiController::clampScroll()
{
    if (mScrollPx > mScrollMax) mScrollPx = mScrollMax;
    if (mScrollPx < 0.0f) mScrollPx = 0.0f;
}

void JGuiController::updateDragScroll()
{
    if (!mUseScroll || !mEngine) return;
    int dgx = 0, dgy = 0;
    if (mEngine->GetDragCoordinates(dgx, dgy))
    {
        // Content follows the finger 1:1 (like a normal touch surface).
        if (mDragLastY > -9000.0f) mScrollPx -= (float) (dgy - mDragLastY);
        mDragLastY = (float) dgy;
    }
    else
        mDragLastY = -9999.0f; // finger up / idle: next drag starts fresh (no jump)
    clampScroll();
}

bool JGuiController::scrollKey(JButton key, float step)
{
    if (!mUseScroll) return false;
    if (key != JGE_BTN_UP && key != JGE_BTN_DOWN) return false;
    // A live drag already scrolled this frame; swallow the emitted key so we don't double.
    int dgx = 0, dgy = 0;
    if (mEngine && mEngine->GetDragCoordinates(dgx, dgy)) return true;
    // Reversed to match the drag direction / user preference: up -> further down the list.
    if (key == JGE_BTN_UP) mScrollPx += step;
    else                   mScrollPx -= step;
    clampScroll();
    return true;
}

void JGuiController::renderScrollArrows(float x, float topY, float botY, float t)
{
    if (!mUseScroll) return;
    JRenderer* r = JRenderer::GetInstance();
    float pulse = 0.5f + 0.5f * sinf(t * 4.0f); // 0..1
    int a = 120 + (int) (pulse * 135.0f);       // 120..255 alpha (clearly visible)
    const float w = 5.0f, h = 6.0f;
    // Both triangles use the SAME vertex winding so neither is dropped by back-face culling
    // (FillPolygon draws a GL_TRIANGLE_FAN with a vertical flip; opposite winding = one culled,
    // which is why the down arrow was missing).
    if (mScrollPx > 1.0f) // more above -> up triangle (apex, bottom-left, bottom-right)
    {
        float vx[3] = { x, x - w, x + w };
        float vy[3] = { topY - h, topY + h, topY + h };
        r->FillPolygon(vx, vy, 3, ARGB(a, 255, 255, 255));
    }
    if (mScrollPx < mScrollMax - 1.0f) // more below -> down triangle (apex, top-right, top-left)
    {
        float vx[3] = { x, x + w, x - w };
        float vy[3] = { botY + h, botY - h, botY - h };
        r->FillPolygon(vx, vy, 3, ARGB(a, 255, 255, 255));
    }
}

JGuiController::~JGuiController()
{
    for (int i = 0; i < mCount; i++)
        if (mObjects[i] != NULL) delete mObjects[i];
    for (size_t i = 0; i < mButtons.size(); i++)
        if (mButtons[i] != NULL) delete mButtons[i];

}

void JGuiController::Render()
{
    for (int i = 0; i < mCount; i++)
        if (mObjects[i] != NULL) mObjects[i]->Render();
}

bool JGuiController::CheckUserInput(JButton key)
{
    if (!mCount) return false;

    // Reverse the vertical scroll direction on all menu lists (per user preference): a
    // swipe/drag up moves DOWN the list and vice versa. Applied centrally so every
    // JGuiController-based menu (SimpleMenu, DeckMenu, options, in-game menu, ...) is
    // consistent. Horizontal menus navigate with LEFT/RIGHT and are unaffected.
    if (key == JGE_BTN_UP) key = JGE_BTN_DOWN;
    else if (key == JGE_BTN_DOWN) key = JGE_BTN_UP;

    // Touch-first one-tap: if a tap coordinate is pending, act on the object the tap
    // actually lands on and consume it (along with the OK that accompanies the tap),
    // so selecting and activating happen together in a single tap instead of the old
    // "tap to select, tap again to confirm" behaviour.
    {
        int x = -1, y = -1;
        if (mEngine->GetLeftClickCoordinates(x, y))
        {
            // Buttons (position-aware) get first crack at the tap.
            for (size_t i = 0; i < mButtons.size(); i++)
            {
                if (mButtons[i]->ButtonPressed())
                {
                    mEngine->LeftClickedProcessed();
                    return true;
                }
            }

            if (mObjects.size())
            {
                // Prefer real bounds hit-testing: find the item whose bounds contain the
                // tap (nearest anchor wins if several overlap).
                int n = -1;
                float best = 0.0f;
                bool anyBounds = false;
                for (int i = 0; i < mCount; i++)
                {
                    if (mObjects[i] == NULL)
                        continue;
                    if (mObjects[i]->hasHitTestBounds())
                        anyBounds = true;
                    if (mObjects[i]->HitTest((float)x, (float)y))
                    {
                        float top = 0.0f, left = 0.0f;
                        mObjects[i]->getTopLeft(top, left);
                        float d = (top - y) * (top - y) + (left - x) * (left - x);
                        if (n < 0 || d < best)
                        {
                            best = d;
                            n = i;
                        }
                    }
                }

                // Fallback for controllers whose items don't report hit-test bounds yet:
                // pick the nearest object by anchor (legacy behaviour) so they keep
                // working. When items DO report bounds, a miss deliberately selects
                // nothing (so tapping empty space no longer activates a far-off item).
                if (n < 0 && !anyBounds)
                {
                    unsigned int minDistance2 = (unsigned int)-1;
                    for (int i = 0; i < mCount; i++)
                    {
                        if (mObjects[i] == NULL)
                            continue;
                        float top = 0.0f, left = 0.0f;
                        if (mObjects[i]->getTopLeft(top, left))
                        {
                            unsigned int d = (unsigned int)((top - y) * (top - y) + (left - x) * (left - x));
                            if (d < minDistance2)
                            {
                                minDistance2 = d;
                                n = i;
                            }
                        }
                    }
                }

                if (n >= 0)
                {
                    bool wasFocused = (n == mCurr);
                    if (n != mCurr && mObjects[mCurr] != NULL && mObjects[mCurr]->Leaving(JGE_BTN_DOWN))
                    {
                        mCurr = n;
                        mObjects[mCurr]->Entering();
                    }
                    // Activate the tapped item. With mTapSelectsOnly, a tap on a not-yet-
                    // focused item only selects it (letting a detail panel preview it); a
                    // second tap on the now-focused item commits.
                    if ((!mTapSelectsOnly || wasFocused) && mObjects[mCurr] != NULL && mObjects[mCurr]->ButtonPressed())
                    {
                        if (mListener != NULL)
                            mListener->ButtonPressed(mId, mObjects[mCurr]->GetId());
                    }
                }

                mEngine->LeftClickedProcessed();
                mEngine->ResetInput();
                return true;
            }
            mEngine->LeftClickedProcessed();
        }
    }

    if (key == mActionButton)
    {
        if (!mObjects.empty() && mObjects[mCurr] != NULL && mObjects[mCurr]->ButtonPressed())
        {
            if (mListener != NULL) mListener->ButtonPressed(mId, mObjects[mCurr]->GetId());
            return true;
        }
    }
    else if (key == mCancelButton)
    {
        if (mListener != NULL)
        {
            mListener->ButtonPressed(mId, kCancelMenuID);
        }
    }
    else if (JGE_BTN_CANCEL == key)
    {
        if (mListener != NULL) mListener->ButtonPressed(mId, kInfoMenuID);
        return true;
    }
    else if ((JGE_BTN_LEFT == key) || (JGE_BTN_UP == key)) // || mEngine->GetAnalogY() < 64 || mEngine->GetAnalogX() < 64)
    {
        int n = mCurr;
        n--;
        if (n < 0)
        {
            if ((mStyle & JGUI_STYLE_WRAPPING))
                n = mCount - 1;
            else
                n = 0;
        }

        if (n != mCurr && mObjects[mCurr] != NULL && mObjects[mCurr]->Leaving(JGE_BTN_UP))
        {
            mCurr = n;
            mObjects[mCurr]->Entering();
        }
        return true;
    }
    else if ((JGE_BTN_RIGHT == key) || (JGE_BTN_DOWN == key)) // || mEngine->GetAnalogY()>192 || mEngine->GetAnalogX()>192)
    {
        int n = mCurr;
        n++;
        if (n > mCount - 1)
        {
            if ((mStyle & JGUI_STYLE_WRAPPING))
                n = 0;
            else
                n = mCount - 1;
        }

        if (n != mCurr && mObjects[mCurr] != NULL && mObjects[mCurr]->Leaving(JGE_BTN_DOWN))
        {
            mCurr = n;
            mObjects[mCurr]->Entering();
        }
        return true;
    }
    // Taps (click coordinates) are handled up-front at the top of this function as a
    // one-tap select+activate, so there is no separate "closest object" handling here.
    return false;
}

void JGuiController::Update(float dt)
{
    for (int i = 0; i < mCount; i++)
        if (mObjects[i] != NULL)
            mObjects[i]->Update(dt);
    
    for (size_t i = 0; i < mButtons.size(); i++ )
        mButtons[i]->Update(dt);

    if(mEngine)
    {
        JButton key = mEngine->ReadButton();
        CheckUserInput(key);
    }
}

void JGuiController::Add(JGuiObject* ctrl, bool isButton)
{
    if (!isButton)
    {
        mObjects.push_back(ctrl);
        mCount++;
    }
    else
    {
        mButtons.push_back(ctrl);
    }
}

void JGuiController::RemoveAt(int i, bool isButton)
{
    if (isButton)
    {
        if (!mButtons[i]) return;
        mButtons.erase(mButtons.begin() + i);
        delete mButtons[i];
        
        return;
    }
    
    if (!mObjects[i]) return;
    mObjects.erase(mObjects.begin() + i);
    delete mObjects[i];
    mCount--;
    if (mCurr == mCount) mCurr = 0;
    return;
}

void JGuiController::Remove(int id)
{
    for (int i = 0; i < mCount; i++)
    {
        if (mObjects[i] != NULL && mObjects[i]->GetId() == id)
        {
            RemoveAt(i);
            return;
        }
    }
    
    for (size_t i = 0; i < mButtons.size(); i++)
    {
        if (mButtons[i] != NULL && mButtons[i]->GetId() == id)
        {
            RemoveAt(i, true);
            return;
        }
    }
}

void JGuiController::Remove(JGuiObject* ctrl)
{
    for (int i = 0; i < mCount; i++)
    {
        if (mObjects[i] != NULL && mObjects[i] == ctrl)
        {
            RemoveAt(i);
            return;
        }
    }

    
    for (size_t i = 0; i < mButtons.size(); i++)
    {
        if (mButtons[i] != NULL && mButtons[i] == ctrl)
        {
            RemoveAt(i, true);
            return;
        }
    }
}

void JGuiController::SetActionButton(JButton button)
{
    mActionButton = button;
}
void JGuiController::SetStyle(int style)
{
    mStyle = style;
}
void JGuiController::SetCursor(JSprite* cursor)
{
    mCursor = cursor;
}
bool JGuiController::IsActive()
{
    return mActive;
}
void JGuiController::SetActive(bool flag)
{
    mActive = flag;
}
