#include "PrecompiledHeader.h"

#include "DeckMenu.h"
#include "DeckMenuItem.h"
#include "DeckMetaData.h"
#include "DeckManager.h"
#include "InteractiveButton.h"
#include "JTypes.h"
#include "GameApp.h"
#include "Translate.h"
#include "TextScroller.h"
#include "Tasks.h"
#include "UITheme.h"

#include <iomanip>

namespace DeckMenuConst
{
    // All spatial constants expressed as fractions of screen dimensions
    // so they track the background image stretch on any device/aspect ratio.
    // Original PSP values in comments for reference.

    const float kVerticalMargin = 16.0f / 272.0f;                  // was 16
    const float kHorizontalMargin = 20.0f / 480.0f;                // was 20
    const float kLineHeight = 25.0f / 272.0f;                      // was 25
    const float kDescriptionVerticalBoxPadding = -5.0f / 272.0f;   // was -5
    const float kDescriptionHorizontalBoxPadding = 5.0f / 480.0f;  // was 5

    const float kVerticalScrollSpeed = 7.0f;

    const int DETAILED_INFO_THRESHOLD = 20;
    const int kDetailedInfoButtonId = 10000;
}

hgeParticleSystem* DeckMenu::stars = NULL;

//
//  For the additional info window, maximum characters per line is roughly 30 characters across.
//  TODO:
//    *** Need to make this configurable in a file somewhere to allow for class reuse

DeckMenu::DeckMenu(int id, JGuiListener* listener, int fontId, const string _title, const int&, bool showDetailsOverride, bool chooseOpponent) :
        JGuiController(JGE::GetInstance(), id, listener), fontId(fontId), mShowDetailsScreen( showDetailsOverride )
{

    backgroundName = "DeckMenuBackdrop";
    mAlwaysShowDetailsButton = false;
    mSelectedDeck = NULL;

    // Touch content-scroll: the finger drags the list, the highlight stays on the selected
    // deck (see updateDragScroll / CheckUserInput / Render).
    mUseScroll = true;

    // All positions as fractions of screen dimensions.
    // Original PSP values (for 480x272) in comments.
    // Layout matched to the deck editor (DeckEditorMenu) so the Play deck picker reads 1-to-1 with
    // the editor: title top-left, avatar + stats to its right, list below, one panel behind it.
    mY = SCREEN_HEIGHT_F * (70.0f / 272.0f);        // editor value (was 50)
    mWidth = SCREEN_WIDTH_F * (176.0f / 480.0f);    // was 176
    mX = SCREEN_WIDTH_F * (123.0f / 480.0f);        // editor value (was 115)

    titleX = SCREEN_WIDTH_F / 6.5f;                 // editor value (was 110/480)
    titleY = SCREEN_HEIGHT_F * (25.0f / 272.0f);     // editor value (was 15)
    titleWidth = SCREEN_WIDTH_F * (180.0f / 480.0f); // was 180

    descX = SCREEN_WIDTH_F * (260.0f / 480.0f) + SCREEN_WIDTH_F * DeckMenuConst::kDescriptionHorizontalBoxPadding;
    descY = SCREEN_HEIGHT_F * (100.0f / 272.0f) + SCREEN_HEIGHT_F * DeckMenuConst::kDescriptionVerticalBoxPadding;
    descHeight = SCREEN_HEIGHT_F * (145.0f / 272.0f);   // was 145
    descWidth = SCREEN_WIDTH_F * (195.0f / 480.0f);     // was 195

    detailedInfoBoxX = SCREEN_WIDTH_F * (400.0f / 480.0f);  // was 400
    detailedInfoBoxY = SCREEN_HEIGHT_F * (235.0f / 272.0f);  // was 235
    starsOffsetX = SCREEN_WIDTH_F * (50.0f / 480.0f);        // was 50

    statsX = SCREEN_WIDTH_F * (280.0f / 480.0f);    // editor value (was 300)
    statsY = SCREEN_HEIGHT_F * (12.0f / 272.0f);     // editor value (was 15)
    statsHeight = SCREEN_HEIGHT_F * (50.0f / 272.0f); // was 50
    statsWidth = SCREEN_WIDTH_F * (185.0f / 480.0f);  // editor value (was 227)

    avatarX = SCREEN_WIDTH_F * (222.0f / 480.0f);   // editor value (was 232)
    avatarY = SCREEN_HEIGHT_F * (8.0f / 272.0f);      // editor value (was 11)

    menuInitialized = false;

    float scrollerWidth = SCREEN_WIDTH_F * (200.0f / 480.0f);   // was 200
    float scrollerHeight = SCREEN_HEIGHT_F * (28.0f / 272.0f);   // was 28
    float scrollerX = SCREEN_WIDTH_F * (14.0f / 480.0f);         // was 14
    float scrollerY = SCREEN_HEIGHT_F * (235.0f / 272.0f);       // was 235
    mScroller = NEW VerticalTextScroller(Fonts::MAIN_FONT, scrollerX, scrollerY, scrollerWidth, scrollerHeight, DeckMenuConst::kVerticalScrollSpeed);

    mAutoTranslate = true;
    maxItems = 6;
    mHeight = SCREEN_HEIGHT_F * (2 * DeckMenuConst::kVerticalMargin + (maxItems * DeckMenuConst::kLineHeight));

    // we want to cap the deck titles to 15 characters to avoid overflowing deck names
    title = _(_title);
    displayTitle = title;
    mFont = WResourceManager::Instance()->GetWFont(fontId);

    startId = 0;
    selectionT = 0;
    timeOpen = 0;
    mClosed = false;
    isOpponent = chooseOpponent;

    if (mFont->GetStringWidth(title.c_str()) > titleWidth)
        titleFontScale = SCALE_SHRINK;
    else
        titleFontScale = SCALE_NORMAL;

    mSelectionTargetY = selectionY = mY + SCREEN_HEIGHT_F * DeckMenuConst::kVerticalMargin;

    if (NULL == stars)
        stars = NEW hgeParticleSystem(WResourceManager::Instance()->RetrievePSI("stars.psi", WResourceManager::Instance()->GetQuad("stars").get()));
    stars->FireAt(mX, mY);

    const string detailedInfoString = _("Info");
    WFont *descriptionFont = WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT);

    float stringWidth = descriptionFont->GetStringWidth(detailedInfoString.c_str());
    float boxStartX = detailedInfoBoxX - stringWidth / 2 + SCREEN_WIDTH_F * (20.0f / 480.0f);
    //dismiss button?
#if defined PSP
    dismissButton = NEW InteractiveButton( this, DeckMenuConst::kDetailedInfoButtonId, Fonts::MAIN_FONT, detailedInfoString, boxStartX + SCREEN_WIDTH_F * (25.0f / 480.0f), detailedInfoBoxY - SCREEN_HEIGHT_F * (10.0f / 272.0f), JGE_BTN_CANCEL);
#else
    dismissButton = NEW InteractiveButton( this, DeckMenuConst::kDetailedInfoButtonId, Fonts::MAIN_FONT, detailedInfoString, boxStartX + SCREEN_WIDTH_F * (30.0f / 480.0f), detailedInfoBoxY + SCREEN_HEIGHT_F * (4.5f / 272.0f), JGE_BTN_CANCEL);
#endif
    JGuiController::Add(dismissButton, true);

    updateScroller();
}

void DeckMenu::RenderDeckManaColors()
{
    // display the deck mana colors if known
    bool displayDeckMana = backgroundName.find("DeckMenuBackdrop") != string::npos;
    float manaIconX = SCREEN_WIDTH_F * (288.0f / 480.0f);   // was 288
    float manaIconY = SCREEN_HEIGHT_F * (248.0f / 272.0f);   // was 248
    float manaIconSpacing = SCREEN_WIDTH_F * (26.0f / 480.0f); // was 26
    if (mSelectedDeck && displayDeckMana)
    {
        string deckManaColors = mSelectedDeck->getColorIndex().c_str();
        if (deckManaColors.size() >= 6)
        {
            // due to space constraints don't display icons for colorless mana.
            for( int colorIdx = Constants::MTG_COLOR_ARTIFACT; colorIdx < Constants::MTG_COLOR_WASTE; ++colorIdx )
            {
                if ( (deckManaColors.at(colorIdx) == '1') != 0)
                {
                    JRenderer::GetInstance()->RenderQuad(manaIcons[colorIdx].get(), manaIconX, manaIconY, 0, 0.6f, 0.6f);
                    manaIconX += manaIconSpacing;
                }
            }
        }
        else if (deckManaColors.compare("") != 0 )
            DebugTrace("Error with color index string for "<< mSelectedDeck->getName() << ". [" << deckManaColors << "].");
    }
}

void DeckMenu::RenderBackground()
{
    ostringstream bgFilename;
#if !defined (PSP)
    if(backgroundName.find("menubgdeckeditor") != string::npos)
        bgFilename << backgroundName << ".jpg";
    else
        bgFilename << backgroundName << ".png";
#else
    if(backgroundName == "menubgdeckeditor")
        bgFilename << "pspmenubgdeckeditor.jpg";
    else
        bgFilename << "pspmenubgdeckeditor.png";
#endif

    static bool loadBackground = true;
    if (loadBackground)
    {
        JQuadPtr background = WResourceManager::Instance()->RetrieveTempQuad(bgFilename.str(), TEXTURE_SUB_5551);
        if (background.get())
        {
            float scaleX = SCREEN_WIDTH_F / background.get()->mWidth;
            float scaleY = SCREEN_HEIGHT_F / background.get()->mHeight;
            JRenderer::GetInstance()->RenderQuad(background.get(), 0, 0, 0, scaleX, scaleY);
        }
        else
            loadBackground = false;
    }
}

DeckMetaData * DeckMenu::getSelectedDeck()
{
    if (mSelectedDeck) return mSelectedDeck;

    return NULL;
}

void DeckMenu::enableDisplayDetailsOverride()
{
    mAlwaysShowDetailsButton = true;
}

bool DeckMenu::showDetailsScreen()
{
    DeckMetaData * currentMenuItem = getSelectedDeck();
    if (currentMenuItem)
    {
        if (mAlwaysShowDetailsButton) return true;
        if (mShowDetailsScreen && currentMenuItem->getVictories() > DeckMenuConst::DETAILED_INFO_THRESHOLD) return true;
    }

    return false;
}

void DeckMenu::initMenuItems()
{
    float sY = mY + SCREEN_HEIGHT_F * DeckMenuConst::kVerticalMargin;
    for (int i = startId; i < mCount; ++i)
    {
        float y = mY + SCREEN_HEIGHT_F * DeckMenuConst::kVerticalMargin + i * SCREEN_HEIGHT_F * DeckMenuConst::kLineHeight;
        DeckMenuItem * currentMenuItem = static_cast<DeckMenuItem*> (mObjects[i]);
        currentMenuItem->Relocate(mX, y);
        if (currentMenuItem->hasFocus()) sY = y;
    }
    mSelectionTargetY = selectionY = sY;

#ifndef TOUCH_ENABLED
    //Grab a texture in VRAM.
    pspIconsTexture = WResourceManager::Instance()->RetrieveTexture("iconspsp.png", RETRIEVE_MANAGE);

    char buf[512];
    for (int i = 0; i < 8; i++)
    {
        sprintf(buf, "iconspsp%d", i);
        pspIcons[i] = WResourceManager::Instance()->RetrieveQuad("iconspsp.png", (float) i * 32, 0, 32, 32, buf);
        pspIcons[i]->SetHotSpot(16, 16);
    }
    dismissButton->setImage(pspIcons[5]);
#endif
}

void DeckMenu::selectRandomDeck(bool isAi)
{
    DeckManager *deckManager = DeckManager::GetInstance();
    vector<DeckMetaData *> *deckList = isAi ? deckManager->getAIDeckOrderList() : deckManager->getPlayerDeckOrderList();
    int random = rand() % (int)deckList->size();
    selectDeck( random, isAi );
}

void DeckMenu::selectDeck(int deckId, bool isAi)
{
    DeckManager *deckManager = DeckManager::GetInstance();
    vector<DeckMetaData *> *deckList = isAi ? deckManager->getAIDeckOrderList() : deckManager->getPlayerDeckOrderList();
    mSelectedDeck = deckList->at(deckId);
}

void DeckMenu::Render()
{
    JRenderer * renderer = JRenderer::GetInstance();
    float height = mHeight;
    JQuadPtr avatarholder;
    JQuadPtr menupanel;
    JQuadPtr menuholder;
#if defined (PSP)
    avatarholder = WResourceManager::Instance()->RetrieveTempQuad("pspavatarholder.png");
    menupanel = WResourceManager::Instance()->RetrieveTempQuad("pspmenupanel.jpg");
    menuholder = WResourceManager::Instance()->RetrieveTempQuad("pspmenuholder.png");
#else
    avatarholder = WResourceManager::Instance()->RetrieveTempQuad("avatarholder.png");
    if(isOpponent){
        menupanel = WResourceManager::Instance()->RetrieveTempQuad("menupanel2.jpg");
        if(!menupanel.get())
            menupanel = WResourceManager::Instance()->RetrieveTempQuad("menupanel.jpg");
    } else
        menupanel = WResourceManager::Instance()->RetrieveTempQuad("menupanel.jpg");
    menuholder = WResourceManager::Instance()->RetrieveTempQuad("menuholder.png");
#endif
    // Avatar/stats use the deck-editor layout in BOTH modes now (title + avatar + stats stacked on
    // the left, list below), so the Play screen matches the editor 1-to-1.
    float modAvatarX = SCREEN_WIDTH_F * (-76.0f / 480.0f);
    float modAvatarY = SCREEN_HEIGHT_F * (-1.5f / 272.0f);
    if (!menuInitialized)
    {
        initMenuItems();
        stars->Fire();
        timeOpen = 0;
        menuInitialized = true;
    }

    RenderBackground();//background deck menu

    // Drawn translucent panels behind the menu content, replacing the old menupanel / menuholder /
    // avatarholder frame textures (low-res bitmap frames that didn't match). The editor layout
    // stacks title + stats + list on the left; the Play layout splits them (list left, avatar +
    // stats top-right), so each gets matching panels sized from the live layout values.
    {
        const float ry = SCREEN_HEIGHT_F * (4.0f / 272.0f);
        // Bottom of the deck-list viewport (all maxItems rows), so the panel always backs the list.
        const float listBottom = mY + SCREEN_HEIGHT_F * (DeckMenuConst::kVerticalMargin + maxItems * DeckMenuConst::kLineHeight);
        const float rh = (listBottom + SCREEN_HEIGHT_F * (4.0f / 272.0f)) - ry;
        UITheme::drawPanel(renderer, SCREEN_WIDTH_F * (6.0f / 480.0f), ry, SCREEN_WIDTH_F * (282.0f / 480.0f), rh, ARGB(195, 14, 16, 22));
    }

    // Credits / task scroller removed: that info already lives on the task board.

    float lineHeight = SCREEN_HEIGHT_F * DeckMenuConst::kLineHeight;
    if (timeOpen < 1) height *= timeOpen > 0 ? timeOpen : -timeOpen;

    for (int i = 0; i < mCount; i++)
    {
        DeckMenuItem *currentMenuItem = static_cast<DeckMenuItem*> (mObjects[i]);
        float renderedY = currentMenuItem->getY() - mScrollPx;
        // Only draw rows that fit ENTIRELY within the list window, so nothing spills past it
        // (no scissor clip on this target). The list scrolls under fixed top/bottom edges.
        float listTopY = mY + SCREEN_HEIGHT_F * DeckMenuConst::kVerticalMargin;
        float listBotY = listTopY + maxItems * lineHeight;
        if (renderedY < listTopY - 1.0f) continue;              // top not fully inside
        if (renderedY + lineHeight > listBotY + 1.0f) break;    // bottom not fully inside (rows ordered)
        {
            // only load stats for visible items in the list
            DeckMetaData* metaData = currentMenuItem->getMetaData();
            if (metaData && !metaData->mStatsLoaded)
            {
                metaData->LoadStats();
            }

            if (currentMenuItem->hasFocus())
            {
                mSelectedDeck = metaData;
                WFont *descriptionFont = WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT);

                // display the "more info" button if special condition is met
                if (showDetailsScreen())
                {
                    dismissButton->setIsSelectionValid(true);
                    dismissButton->Render();
                }
                else
                {
                    dismissButton->setIsSelectionValid(false);
                }
                // display the avatar image
                string currentAvatarImageName = currentMenuItem->getImageFilename();
                if (currentAvatarImageName.size() > 0)
                {
                    JQuadPtr quad = WResourceManager::Instance()->RetrieveTempQuad(currentAvatarImageName, TEXTURE_SUB_AVATAR);
                    if(quad.get())
                    {
                        float avatarW = SCREEN_WIDTH_F * (37.0f / 480.0f);   // was 37
                        float avatarH = SCREEN_HEIGHT_F * (50.0f / 272.0f);   // was 50
                        float xscale = avatarW / quad->mWidth;
                        float yscale = avatarH / quad->mHeight;
                        if (currentMenuItem->getText() == "Evil Twin")
                        {
                            JQuad * evil = quad.get();
                            evil->SetHFlip(true);

                            renderer->RenderQuad(quad.get(), avatarX+modAvatarX, avatarY+modAvatarY, 0, xscale, yscale);
                            renderer->DrawRect(avatarX+modAvatarX, avatarY+modAvatarY, avatarW, avatarH, ARGB(200,3,3,3));
                            evil = NULL;
                        }
                        else
                        {

                            renderer->RenderQuad(quad.get(), avatarX+modAvatarX, avatarY+modAvatarY, 0, xscale, yscale);
                            renderer->DrawRect(avatarX+modAvatarX, avatarY+modAvatarY, avatarW, avatarH, ARGB(200,3,3,3));
                        }
                    }
                }

                // fill in the description part of the screen (skipped on the opponent chooser,
                // where the long mode explainers just cluttered the screen).
                if (!isOpponent)
                {
                    string text = wordWrap(_(currentMenuItem->getDescription()), descWidth, descriptionFont->mFontID );
                    descriptionFont->SetColor(ARGB(255,255,255,255));
                    descriptionFont->DrawString(text.c_str(), descX, descY);
                }

                // fill in the statistical portion
                if (currentMenuItem->hasMetaData())
                {
                    ostringstream oss;
                    oss << _("Deck: ") << currentMenuItem->getDeckName() << endl;
                    oss << currentMenuItem->getDeckStatsSummary();
                    descriptionFont->SetColor(ARGB(255,255,255,255));
                    // Editor stats position in both modes (see modAvatarX above), so Play matches.
                    descriptionFont->DrawString(oss.str(), statsX - SCREEN_WIDTH_F * (86.0f / 480.0f), statsY - SCREEN_HEIGHT_F * (4.0f / 272.0f));
                }

                // change the font color of the current menu item
                mFont->SetColor(ARGB(255,255,255,255));
            }
            else // reset the font color to be slightly muted
                mFont->SetColor(ARGB(150,255,255,255));
            currentMenuItem->RenderWithOffset(-mScrollPx);
        }
    }

    // Pulsing up/down indicators at the right edge of the deck column when there is more
    // of the list to scroll to.
    {
        float listTop = mY + SCREEN_HEIGHT_F * DeckMenuConst::kVerticalMargin;
        float viewportH = maxItems * lineHeight;
        mScrollMax = mCount * lineHeight - viewportH; // recompute here so the arrows are right on frame 1
        if (mScrollMax < 0.0f) mScrollMax = 0.0f;
        clampScroll();
        renderScrollArrows(SCREEN_WIDTH_F * (216.0f / 480.0f), listTop + 3.0f, listTop + viewportH - 3.0f, selectionT);
    }
    //psp
    RenderDeckManaColors();

    if (!title.empty())
    {
        mFont->SetColor(ARGB(255,255,255,255));
        mFont->DrawString(title.c_str(), titleX, titleY, JGETEXT_CENTER);
    }

    // stars->Render() removed: the roaming selection sparkle is no longer used on lists.

}

void DeckMenu::Update(float dt)
{
    updateDragScroll();        // finger drag -> mScrollPx (content follows the finger)
    JGuiController::Update(dt); // reads one button -> CheckUserInput (scroll step or tap)
    UpdateVisuals(dt);
}

bool DeckMenu::CheckUserInput(JButton key)
{
    // Up/down (from a swipe or the d-pad) scroll the content instead of roaming the cursor.
    float step = SCREEN_HEIGHT_F * DeckMenuConst::kLineHeight;
    if (scrollKey(key, step)) return true;
    // Everything else (taps, cancel, action) uses the normal controller handling; taps
    // hit-test against the scrolled row positions (DeckMenuItem::HitTest tracks the offset).
    return JGuiController::CheckUserInput(key);
}


void DeckMenu::UpdateVisuals(float dt)
{
    float lineHeight = SCREEN_HEIGHT_F * DeckMenuConst::kLineHeight;

    // Content-scroll: the finger (updateDragScroll) drives mScrollPx; the cursor no longer
    // pages the list. Clamp to the overflow past the visible window.
    mScrollMax = mCount * lineHeight - maxItems * lineHeight;
    if (mScrollMax < 0.0f) mScrollMax = 0.0f;
    clampScroll();

    stars->Update(dt);
    selectionT += 3 * dt;
    selectionY += (mSelectionTargetY - selectionY) * 8 * dt;

    float horizMargin = SCREEN_WIDTH_F * DeckMenuConst::kHorizontalMargin;
    float starsX = starsOffsetX + ((mWidth - 2 * horizMargin) * (1 + cos(selectionT)) / 2);
    // The highlight/stars ride with the selected item as the list scrolls.
    float starsY = selectionY + SCREEN_HEIGHT_F * (5.0f / 272.0f) * cos(selectionT * 2.35f) + lineHeight / 2 - mScrollPx;
    stars->MoveTo(starsX, starsY);

    //

    if (timeOpen < 0)
    {
        timeOpen += dt * 10;
        if (timeOpen >= 0)
        {
            timeOpen = 0;
            mClosed = true;
            stars->FireAt(mX, mY);
        }
    }
    else
    {
        mClosed = false;
        timeOpen += dt * 10;
    }
    if (mScroller)
        mScroller->Update(dt);

}

void DeckMenu::Add(int id, const string& text, const string& desc, bool forceFocus, DeckMetaData * deckMetaData)
{
    float lineHeight = SCREEN_HEIGHT_F * DeckMenuConst::kLineHeight;
    float vertMargin = SCREEN_HEIGHT_F * DeckMenuConst::kVerticalMargin;
    DeckMenuItem * menuItem = NEW DeckMenuItem(this, id, fontId, text, 0,
                                               mY + vertMargin + mCount * lineHeight, (mCount == 0), mAutoTranslate, deckMetaData);
    Translator * t = Translator::GetInstance();
    map<string, string>::iterator it = t->deckValues.find(text);
    string deckDescription = "";

    if (it != t->deckValues.end()) //translate decks desc
        deckDescription = it->second;
    else
        deckDescription = deckMetaData ? deckMetaData->getDescription() : desc;

    if(deckMetaData && deckMetaData->isCommanderDeck())
        deckDescription = deckDescription + " (" + _("CMD") + ")";

    menuItem->setDescription(deckDescription);

    JGuiController::Add(menuItem);
    if (mCount <= maxItems) mHeight += lineHeight;

    if (forceFocus)
    {
        mObjects[mCurr]->Leaving(JGE_BTN_DOWN);
        mCurr = mCount - 1;
        menuItem->Entering();
    }
}

void DeckMenu::updateScroller()
{
    // add all the items from the Tasks db.
    TaskList taskList;
    mScroller->Reset();

    for (vector<Task*>::iterator it = taskList.tasks.begin(); it != taskList.tasks.end(); it++)
    {
        ostringstream taskDescription;
        taskDescription << "Credits: " << setw(4) << (*it)->getReward() << " / "
                        << "Days Left: " << (*it)->getExpiration() << endl
                        << (*it)->getDesc() << endl << endl;
        mScroller->Add(taskDescription.str());
    }

}

void DeckMenu::Close()
{
    timeOpen = -1.0;
    stars->Stop(true);
}

void DeckMenu::destroy()
{
    SAFE_DELETE(DeckMenu::stars);
}

DeckMenu::~DeckMenu()
{
    WResourceManager::Instance()->Release(pspIconsTexture);
    SAFE_DELETE(mScroller);
    dismissButton = NULL;
}