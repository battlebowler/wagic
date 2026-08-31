//-------------------------------------------------------------------------------------
//
// JGE++ is a hardware accelerated 2D game SDK for PSP/Windows.
//
// Licensed under the BSD license, see LICENSE in JGE root for details.
//
// Copyright (c) 2007 James Hui (a.k.a. Dr.Watson) <jhkhui@gmail.com>
//
//-------------------------------------------------------------------------------------


#ifndef _JGUI_H
#define _JGUI_H

#include <ostream>
#include "JGE.h"
#include "JSprite.h"

#define MAX_GUIOBJECT           64

#define JGUI_STYLE_LEFTRIGHT    0x01
#define JGUI_STYLE_UPDOWN       0x02
#define JGUI_STYLE_WRAPPING     0x04

#define JGUI_INITIAL_DELAY      0.4
#define JGUI_REPEAT_DELAY       0.2

const int kCancelMenuID = -1;
const int kInfoMenuID = -200;
const int kRandomPlayerMenuID = -11;
const int kRandomAIPlayerMenuID = -12;
const int kEvilTwinMenuID = -14;
const int kCommanderMenuID = -33;

class JGuiListener
{
public:
    virtual ~JGuiListener()
    {
    }
    virtual void ButtonPressed(int controllerId, int controlId) = 0;
};

class JGuiObject
{
protected:
    static JGE* mEngine;

private:
    int mId;

public:
    JGuiObject(int id);
    virtual ~JGuiObject();

    virtual void Render() = 0;
    virtual std::ostream& toString(std::ostream&) const = 0;
    virtual void Update(float dt);

    virtual void Entering(); // when focus is transferring to this obj
    virtual bool Leaving(JButton key); // when focus is transferring away from this obj, true to go ahead
    virtual bool ButtonPressed(); // action button pressed, return false to ignore

    // Used for mouse support so that the GUI engine can found out which Object was selected
    virtual bool getTopLeft(float&, float&)
    {
        return false;
    }
    ;

    // Touch hit-testing: return true if the point (x, y) is within this object's
    // rendered bounds. Default false (unknown bounds) so that a tap on an object that
    // does not implement this is simply ignored rather than activating the wrong item.
    // (Named HitTest rather than Contains to avoid clashing with Pos::Contains in the
    // play-area classes that multiply-inherit both JGuiObject and Pos.)
    virtual bool HitTest(float /*x*/, float /*y*/)
    {
        return false;
    }
    ;

    // Whether HitTest() implements real bounds for this object. When every item in a
    // controller reports true, a tap that misses them all does nothing; otherwise the
    // controller falls back to legacy "nearest object" selection so un-migrated widgets
    // keep working.
    virtual bool hasHitTestBounds()
    {
        return false;
    }
    ;

    int GetId();
};

class JGuiController
{
protected:
    JGE* mEngine;

    int mId;
    bool mActive;

    JButton mActionButton;
    JButton mCancelButton;
    int mCurr;
    int mStyle;

    JSprite* mCursor;
    bool mShowCursor;
    int mCursorX;
    int mCursorY;

    int mBgX;
    int mBgY;
    const JTexture* mBg;
    PIXEL_TYPE mShadingColor;
    JRect* mShadingBg;

    JGuiListener* mListener;
    //int mKeyHoldTime;

public:
    // --- Touch content-scroll (opt-in via mUseScroll) ---
    // When enabled the list scrolls its content by mScrollPx (driven 1:1 by the finger
    // drag) instead of roaming the selection cursor; the drag-emitted up/down become
    // content steps. The owning widget sets mScrollMax each frame and offsets its own
    // rendering/highlight by mScrollPx. Non-scrolling controllers leave mUseScroll false
    // and are completely unaffected.
    bool  mUseScroll;
    // When true, a tap on a not-yet-focused item only selects it (so a detail panel can
    // preview it); a second tap on the focused item commits. Default false = one-tap
    // select+activate (the normal behaviour everywhere else).
    bool  mTapSelectsOnly;
    // When true, a tap NEVER commits — it only moves focus (so a detail panel can preview it);
    // committing must come from an explicit control (e.g. a "Select" button that calls
    // confirmSelection()). Default false.
    bool  mTapNeverConfirms;
    float mScrollPx;
    float mScrollMax;
    float mDragLastY;

    void updateDragScroll();                 // read the finger drag -> mScrollPx (per frame)
    void clampScroll();                      // clamp mScrollPx to [0, mScrollMax]
    bool scrollKey(JButton key, float step); // up/down -> content step; true if consumed
    // Pulsing up/down triangles at x, between topY/botY, shown when more content exists.
    void renderScrollArrows(float x, float topY, float botY, float t);

    vector<JGuiObject*> mObjects;

    vector<JGuiObject*> mButtons;
    int mCount;

    JGuiController(JGE* jge, int id, JGuiListener* listener);
    virtual ~JGuiController();

    virtual void Render();
    virtual void Update(float dt);
    virtual bool CheckUserInput(JButton key);
    // Commit the currently-focused item as if it were activated. For use by external controls
    // (e.g. a "Select" button) when taps are focus-only (see mTapNeverConfirms).
    void confirmSelection();

    virtual void Add(JGuiObject* ctrl, bool isButton = false);
    virtual void RemoveAt(int i, bool isButton = false);
    virtual void Remove(int id);
    virtual void Remove(JGuiObject* ctrl);

    void SetActionButton(JButton button);
    void SetStyle(int style);
    void SetCursor(JSprite* cursor);

    bool IsActive();
    void SetActive(bool flag);

};

ostream& operator<<(ostream &out, const JGuiObject &j);

#endif
