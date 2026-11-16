/*
A class for very simple menus structure
*/
#ifndef _SIMPLEMENU_H_
#define _SIMPLEMENU_H_

#include <string>
#include <JGui.h>
#include "WFont.h"
#include "hge/hgeparticle.h"
#include "WResourceManager.h"
#include "WResource_Fwd.h"

class SimpleMenu: public JGuiController
{
private:
    float mHeight, mWidth, mX, mY;
    int fontId;
    std::string title;
    int displaytitle;
    int maxItems, startId;
    float selectionT, selectionY;
    float timeOpen;
    bool mClosed;
    // Touch selection flash: touch popups have no hover cursor, so no row is highlighted at rest.
    // When a row is tapped it briefly flashes the selection colour as confirmation. mSelectFlash is a
    // 0..1 countdown (drives the fade); mSelectFlashIndex is the row to flash.
    float mSelectFlash;
    int mSelectFlashIndex;

    bool mCenterHorizontal;
    bool mCenterVertical;

    static JQuadPtr spadeR, spadeL, jewel, side;
    static JTexture *spadeRTex, *spadeLTex, *jewelTex, *sideTex;
    hgeParticleSystem* stars;

    inline void MogrifyJewel();
    void drawHorzPole(float x, float y, float width);
    void drawVertPole(float x, float y, float height);

    // Vertical "interrupt-style" dialog (bottom-centered panel with a grey title bar and stacked
    // bordered buttons, matching the interrupt dialog). Renders it and records each button's tap
    // rectangle below.
    void renderInterruptStyle();
    std::vector<float> mBtnLeft, mBtnRight, mBtnTop, mBtnBottom; // per-item button tap rectangles

public:
    bool autoTranslate;
    bool isMultipleChoice;
    // When set, the menu draws as a horizontal interrupt-style button bar (see renderInterruptStyle).
    bool mInterruptStyle;
    // When set (the default), the menu draws in the shared UITheme "glass panel" style — a drawn
    // rounded panel with a clean title bar and a highlighted focused row — instead of the legacy
    // double-border black box. Turned OFF for in-duel menus, which keep the old look.
    bool mPanelStyle;
    SimpleMenu(JGE*, WResourceManager*, int id, JGuiListener* listener, int fontId, float x, float y, const char * _title = "", int _maxItems = 7, bool centerHorizontal = true, bool centerVertical = true);
    virtual ~SimpleMenu();
    virtual void Render();
    virtual bool CheckUserInput(JButton key);
    virtual void Update(float dt);
    using JGuiController::Add;
    virtual void Add(int id, const string &Text, string desc = "", bool forceFocus = false);
    int getmCurr(){return mCurr;}
    float getWidth(){return mWidth; }
    void setTitle(const std::string& t) { title = t; }
    virtual void Close();

    void RecenterMenu();

    float selectionTargetY;
    virtual bool isClosed() const
    {
        return mClosed;
    }
    static void destroy();
};

#endif
