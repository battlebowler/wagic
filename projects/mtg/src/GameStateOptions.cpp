#include "PrecompiledHeader.h"

#include "GameStateOptions.h"
#include "GameStateMenu.h"
#include "GameApp.h"
#include "OptionItem.h"
#include "StyleManager.h"
#include "PlayerData.h"
#include "SimpleMenu.h"
#include "SimplePad.h"
#include "Translate.h"

namespace GameStateOptionsConst
{
    const int kSaveAndBackToMainMenuID = 1;
    const int kBackToMainMenuID = 2;
    const int kNewProfileID = 4;
    const int kReloadID = 5;
    const int kManageDataID = 6;
    const int kManageProfilesID = 7; // opens the custom touch Profiles manager from the User tab
}

static std::string kBgFile = "";

GameStateOptions::GameStateOptions(GameApp* parent) :
    GameState(parent, "options"), mReload(false), grabber(NULL), mCreditsTab(NULL),
    mProfilesDetail(-1), mProfilesScroll(0), mUserTab(NULL), optionsMenu(NULL), optionsTabs(NULL)
{
}

GameStateOptions::~GameStateOptions()
{
    kBgFile = ""; //Reset the chosen background.
}

void GameStateOptions::Start()
{
    newProfile = "";
    timer = 0;
    mState = SHOW_OPTIONS;
    JRenderer::GetInstance()->EnableVSync(true);

    WGuiList * optionsList;

    optionsList = NEW WGuiList("Settings");

    optionsList->Add(NEW WGuiHeader("General Options"));
    if (GameApp::HasMusic)
        optionsList->Add(NEW WDecoEnum(NEW OptionInteger(Options::MUSICVOLUME, "Music volume", 100, 10, 100),
                        OptionVolume::getInstance()));
    optionsList->Add(NEW WDecoEnum(NEW OptionInteger(Options::SFXVOLUME, "SFX volume", 100, 10, 100), OptionVolume::getInstance()));
    if (options[Options::DIFFICULTY_MODE_UNLOCKED].number)
    {
        optionsList->Add(NEW WDecoEnum(NEW OptionInteger(Options::DIFFICULTY, "Difficulty", 3, 1, 0),
                        OptionDifficulty::getInstance()));
        optionsList->Add(NEW WDecoEnum(NEW OptionInteger(Options::ECON_DIFFICULTY, "Economic Difficuly", Constants::ECON_EASY)));
    }
    optionsList->Add(NEW OptionInteger(Options::INTERRUPT_SECONDS, "Seconds to pause for an Interrupt", 20, 1));
    optionsList->Add(NEW OptionInteger(Options::INTERRUPTMYSPELLS, "Interrupt my spells"));
   // optionsList->Add(NEW OptionInteger(Options::INTERRUPTMYABILITIES, "Interrupt my abilities"));
    //this is a dev option, not meant for standard play. uncomment if you need to see abilities you own hitting the stack.
    optionsList->Add(NEW OptionInteger(Options::INTERRUPT_SECONDMAIN, "Interrupt opponent's end of turn"));
#if defined (ANDROID)
    // Import / export / download data. Replaces the old swipe-out admin menu.
    optionsList->Add(NEW WGuiButton(NEW WGuiHeader("Import / Export / Download Data"), -102, GameStateOptionsConst::kManageDataID, this));
#endif
    optionsList->Add(NEW WGuiButton(NEW WGuiHeader("Back to Main Menu"), -102, GameStateOptionsConst::kBackToMainMenuID, this));
    optionsList->Add(NEW WGuiButton(NEW WGuiHeader("Save And Exit"), -102, GameStateOptionsConst::kSaveAndBackToMainMenuID, this));
    optionsTabs = NEW WGuiTabMenu();
    optionsList->scaleItemsHeight(1.4f);
    optionsTabs->Add(optionsList);

    optionsList = NEW WGuiList("Misc");
    optionsList->Add(NEW WGuiHeader("Card Display Options"));
    optionsList->Add(NEW OptionInteger(Options::SHOWBORDER, "Show Borders"));
    //black border
    optionsList->Add(NEW OptionInteger(Options::BLKBORDER, "All Black Borders"));
    //Sort deck by date
    optionsList->Add(NEW OptionInteger(Options::SORTINGDECKS, "Sort decks by date"));
    //show tokens in editor
    optionsList->Add(NEW OptionInteger(Options::SHOWTOKENS, "Show Tokens in Editor"));
    WDecoStyled * wMisc = NEW WDecoStyled(NEW WGuiHeader("Warning!!!"));
    wMisc->mStyle = WDecoStyled::DS_STYLE_ALERT;
    optionsList->Add(wMisc);
    //show large images
    optionsList->Add(NEW OptionInteger(Options::GDVLARGEIMAGE, "Show Large Images in Grid Deck View"));
    //prefetch
    if(WResourceManager::Instance()->IsThreaded())
        optionsList->Add(NEW OptionInteger(Options::CARDPREFETCHING, "Enable Prefetching"));
    optionsList->scaleItemsHeight(1.4f);
    optionsTabs->Add(optionsList);

    optionsList = NEW WGuiList("Game");
    optionsList->Add(NEW WGuiHeader("Interface Options"));
    optionsList->Add(NEW WDecoEnum(NEW OptionInteger(Options::SORTINGSETS, "Sort sets by", Constants::BY_DATE, 1,
                    Constants::BY_NAME, "", Constants::BY_SECTOR))); // Now sets can be sorted by sector(orderindex) or name or release date.
//    optionsList->Add(NEW WDecoEnum(NEW OptionInteger(Options::CLOSEDHAND, "Closed hand", 1, 1, 0)));
//    optionsList->Add(NEW WDecoEnum(NEW OptionInteger(Options::HANDDIRECTION, "Hand direction", 1, 1, 0)));
    optionsList->Add(NEW WDecoEnum(NEW OptionInteger(Options::MANADISPLAY, "Mana display", 3, 1, 0)));
    optionsList->Add(NEW OptionInteger(Options::REVERSETRIGGERS, "Reverse left and right triggers"));
    optionsList->Add(NEW OptionInteger(Options::DISABLECARDS, "Disable card images"));
    optionsList->Add(NEW OptionInteger(Options::TRANSITIONS, "Disable screen transitions"));
    optionsList->Add(NEW OptionInteger(Options::OSD, "Display InGame extra information"));
    optionsList->scaleItemsHeight(1.4f);
    optionsTabs->Add(optionsList);

    optionsList = NEW WGuiList("User");
    mUserTab = optionsList; // while this tab is active, its body IS the custom Profiles manager
    // The entire User tab body is a full-screen touch Profiles manager (painted in renderProfilesModal,
    // driven by updateProfilesModal). This placeholder header just keeps the WGui tab non-empty behind it.
    optionsList->Add(NEW WGuiHeader("Profiles"));
    optionsList->Add(NEW WDecoCheat(NEW OptionInteger(Options::CHEATMODE, "Enable Cheat Mode")));
    optionsList->Add(NEW WDecoCheat(NEW OptionInteger(Options::OPTIMIZE_HAND, "Optimize Starting Hand")));
    optionsList->Add(NEW WDecoCheat(NEW OptionInteger(Options::CHEATMODEAIDECK, "Unlock All Ai Decks")));

    optionsList->scaleItemsHeight(1.4f);
    optionsTabs->Add(optionsList);

    optionsList = NEW WGuiList("Advanced");
    optionsList->Add(NEW WGuiHeader("Advanced Options"));
    WDecoStyled * wAdv = NEW WDecoStyled(NEW WGuiHeader("The following options require a restart."));
    wAdv->mStyle = WDecoStyled::DS_STYLE_ALERT;
    optionsList->Add(wAdv);
    WDecoConfirm * cLang = NEW WDecoConfirm(this, NEW OptionLanguage("Language"));
    cLang->confirm = "Use this Language";
    optionsList->Add(cLang);
    WDecoEnum * oGra = NEW WDecoEnum(NEW OptionInteger(Options::MAX_GRADE, "Minimum Card Grade", Constants::GRADE_DANGEROUS, 1,
                    Constants::GRADE_BORDERLINE, "", Constants::GRADE_SUPPORTED));
    optionsList->Add(oGra);
    WDecoEnum * oASPhases = NEW WDecoEnum(NEW OptionInteger(Options::ASPHASES, "Phase Skip Automation", Constants::ASKIP_FULL, 1,
                    Constants::ASKIP_NONE, "", Constants::ASKIP_NONE));
    optionsList->Add(oASPhases);
    optionsList->scaleItemsHeight(1.4f);
    optionsTabs->Add(optionsList);

    WDecoEnum * oFirstPlayer = NEW WDecoEnum(NEW OptionInteger(Options::FIRSTPLAYER, "First Turn Player", Constants::WHO_R, 1,
                    Constants::WHO_P, "", Constants::WHO_P));
    optionsList->Add(oFirstPlayer);
    
    WDecoEnum * oKickerPay = NEW WDecoEnum(NEW OptionInteger(Options::KICKERPAYMENT, "Kicker Cost", Constants::KICKER_CHOICE, 1,
        Constants::KICKER_ALWAYS, "", Constants::KICKER_ALWAYS));
    optionsList->Add(oKickerPay);
#ifndef IOS
    optionsList = NEW WGuiKeyBinder("Key Bindings", this);
    optionsList->scaleItemsHeight(1.4f);
    optionsTabs->Add(optionsList);
#endif
    optionsList = NEW WGuiList("Credits");
    optionsList->failMsg = "";
    optionsList->scaleItemsHeight(1.4f);
    optionsTabs->Add(optionsList);
    mCreditsTab = optionsList; // remember it so credits render only on this tab

    optionsMenu = NEW SimpleMenu(JGE::GetInstance(), WResourceManager::Instance(), -102, this, Fonts::MAIN_FONT, 50, 170);
    optionsMenu->Add(GameStateOptionsConst::kBackToMainMenuID, "Back to Main Menu");
    optionsMenu->Add(GameStateOptionsConst::kSaveAndBackToMainMenuID, "Save And Exit");
    optionsMenu->Add(kCancelMenuID, "Cancel");

    optionsTabs->Entering(JGE_BTN_NONE);

    buildProfileRows(); // fill the User tab's Profiles manager (rows of profile + per-profile theme)

#if !defined (PSP)
    GameApp::playMusic("Track3.mp3"); // Added music for options.
#endif
}

void GameStateOptions::End()
{
    JRenderer::GetInstance()->EnableVSync(false);
    SAFE_DELETE(optionsTabs);
    SAFE_DELETE(optionsMenu);
    kBgFile = ""; //Reset the chosen background.
}

void GameStateOptions::Update(float dt)
{
    timer += dt * 10;

    if (options.keypadActive())
    {
        options.keypadUpdate(dt);

        if (newProfile != "")
        {
            newProfile = options.keypadFinish();
            if (newProfile != "")
            {
                options[Options::ACTIVE_PROFILE] = newProfile;
                options.reloadProfile();
                optionsTabs->Reload();
                buildProfileRows(); // refresh the Profiles manager list with the new profile
            }
            newProfile = "";
        }
    }
    else
        switch (mState)
        {
        default:
        case SAVE:
            switch (optionsTabs->needsConfirm())
            {
            case WGuiBase::CONFIRM_CANCEL:
                mState = SHOW_OPTIONS;
                break;
            case WGuiBase::CONFIRM_OK:
                optionsTabs->save();
                JSoundSystem::GetInstance()->SetSfxVolume(options[Options::SFXVOLUME].number);
                JSoundSystem::GetInstance()->SetMusicVolume(options[Options::MUSICVOLUME].number);
                mParent->DoTransition(TRANSITION_FADE, GAME_STATE_MENU);
                mState = SHOW_OPTIONS;
                GameStateMenu::genNbCardsStr();
                break;
            case WGuiBase::CONFIRM_NEED:
                optionsTabs->yieldFocus();
                break;
            }
            // Note : No break here : must continue to continue updating the menu elements.
        case SHOW_OPTIONS:
        {
            // On the User tab, the custom touch Profiles manager owns the body (it still lets the
            // tab bar switch tabs). Every other tab uses the normal WGui input path below.
            if (optionsTabs->Current() == mUserTab) { updateProfilesModal(dt); break; }
            JGE* j = JGE::GetInstance();
            JButton key = JGE_BTN_NONE;
            int x, y;
            if (grabber)
            {
                LocalKeySym sym;
                if (LOCAL_KEY_NONE != (sym = j->ReadLocalKey()))
                    grabber->KeyPressed(sym);
            }
            else
                while ((key = JGE::GetInstance()->ReadButton()) || JGE::GetInstance()->GetLeftClickCoordinates(x,y))
                {
                    // Reverse the vertical scroll direction to match the rest of the app (a swipe/
                    // drag up moves DOWN the settings list). Tabs navigate with LEFT/RIGHT, so they
                    // are unaffected.
                    if (key == JGE_BTN_UP) key = JGE_BTN_DOWN;
                    else if (key == JGE_BTN_DOWN) key = JGE_BTN_UP;
                    if (!optionsTabs->CheckUserInput(key) && key == JGE_BTN_MENU)
                        mState = SHOW_OPTIONS_MENU;
                }
            optionsTabs->Update(dt);
            break;
        }
        case SHOW_OPTIONS_MENU:
            optionsMenu->Update(dt);
            break;
        }
    if (mReload)
    {
        options.reloadProfile();
        Translator::EndInstance();
        Translator::GetInstance()->init();
        // Apply the now-active profile's per-profile theme/style: pick the active style, then reload
        // the theme resources (the visible part of a theme change).
        if (options.getStyleMan())
            options.getStyleMan()->determineActive(NULL, NULL);
        WResourceManager::Instance()->Refresh();
        optionsTabs->Reload();
        mReload = false;
    }
}

void GameStateOptions::Render()
{
    //Erase
    JRenderer::GetInstance()->ClearScreen(ARGB(0,0,0,0));
#if !defined (PSP)
    // Options uses the Trophy-Room backdrop (optionsbg.jpg, a copy of awardback.jpg) instead of the
    // busy random deck-editor wallpaper.
    JTexture * wpTex = WResourceManager::Instance()->RetrieveTexture("optionsbg.jpg");
    if (wpTex)
    {
        JQuadPtr wpQuad = WResourceManager::Instance()->RetrieveTempQuad("optionsbg.jpg");
        if (wpQuad.get())
            JRenderer::GetInstance()->RenderQuad(wpQuad.get(), 0, 0, 0, SCREEN_WIDTH_F / wpQuad->mWidth, SCREEN_HEIGHT_F / wpQuad->mHeight);
    }
#else
    JTexture * wpTex = WResourceManager::Instance()->RetrieveTexture("pspbgdeckeditor.jpg");
    if (wpTex)
    {
        JQuadPtr wpQuad = WResourceManager::Instance()->RetrieveTempQuad("pspbgdeckeditor.jpg");
        JRenderer::GetInstance()->RenderQuad(wpQuad.get(), 0, 0, 0, SCREEN_WIDTH_F / wpQuad->mWidth, SCREEN_HEIGHT_F / wpQuad->mHeight);
    }
#endif
    // Light scrim over the wallpaper: enough to keep labels legible, but sheer enough that the
    // wallpaper clearly shows through (the option rows/tabs draw their own solid backdrops on top).
    JRenderer::GetInstance()->FillRect(0, 0, SCREEN_WIDTH_F, SCREEN_HEIGHT_F, ARGB(115, 12, 14, 18));
    const char * const CreditsText[] = {
        "Wagic, The Homebrew?! by Wololo",
        "",
        "Updates, new cards, and more on the Wagic Discord",
        "Many thanks to the people who help this project",
        "",
        "",
        "Art:",
        "Ilya B, Julio, Jeck, J, Kaioshin, Lakeesha",
        "Check themeinfo.txt for the full credits of each theme!",
        "",
        "Dev Team:",
        "Abrasax, Almosthumane, Daddy32, DJardin, Dr.Solomat,",
        "J, Jeck, kevlahnota, Leungclj, linshier, Mootpoint,",
        "Mnguyen, Ph34rbot, Psyringe, Rolzad73, Salmelo, Superhiro,",
        "Vitty85, Wololo, Yeshua, Zethfox",
        "",
        "Music by Celestial Aeon Project, http://www.jamendo.com",
        "",
        "Deck Builders:",
        "Abrasax, AzureKnight, colarchon, Excessum, Hehotfarv,",
        "Jeremy, Jog1118, JonyAS, Lachaux, Link17, Muddobbers,",
        "Nakano, Niegen, Kaioshin, Psyringe, r1c47, Superhiro,",
        "Szei, Thanatos02, Vitty85, Whismer, Wololo",
        "",
        "Thanks also go to Dr.Watson, KF1, Luruz, Orine, Raphael,",
        "Sakya, Tacoghandi, Tyranid for their help.",
        "",
        "Thanks to everyone who contributes code/content on the forums!",
        "",
        "",
        "Source:",
        "http://code.google.com/p/wagic (2009-2013)",
        "https://github.com/WagicProject/wagic (2013- )",
        "",
        "Developed with the JGE++ Library",
        "SFX From www.soundsnap.com",
        "",
        "",
        "This work is not related to or endorsed by Wizards of the Coast, Inc",
        "",
        "Please support this project with donations at  Wagic Discord",
    };

    // The scrolling credits are the CONTENT of the Credits tab, so only draw them when that tab is
    // active (they used to scroll behind every settings tab).
    if (optionsTabs->Current() == mCreditsTab)
    {
        WFont * mFont = WResourceManager::Instance()->GetWFont(Fonts::MAGIC_FONT);
        mFont->SetColor(ARGB(255,200,200,200));
        mFont->SetScale(1.0f * SCALE);
        float startpos = SCREEN_HEIGHT_F - timer;
        float pos = startpos;
        int size = sizeof(CreditsText) / sizeof(CreditsText[0]);

        for (int i = 0; i < size; i++)
        {
            pos = startpos + 20 * SCALE_Y * i;
            if (pos > -20 && pos < SCREEN_HEIGHT + 20)
            {
                mFont->DrawString(CreditsText[i], SCREEN_WIDTH / 2, pos, JGETEXT_CENTER);
            }
        }
        if (pos < -20)
            timer = 0;
    }

    // Backing panel behind the settings content so every tab reads as the same dark card as the
    // (nice-looking) User tab, which draws its own panel below. The tab bar sits above PM_TOP and
    // stays visible; a little alpha lets the wallpaper hint through.
    if (optionsTabs->Current() != mUserTab)
    {
        const float PM_TOP = 32.0f;
        JRenderer::GetInstance()->FillRect(0, PM_TOP, SCREEN_WIDTH_F, SCREEN_HEIGHT_F - PM_TOP, ARGB(235, 12, 14, 18));
    }

    // Render the tabs + active settings list in the Trophy-Room dark palette.
    extern bool gWGuiDarkList;
    gWGuiDarkList = true;
    optionsTabs->Render();
    gWGuiDarkList = false;

    if (mState == SHOW_OPTIONS_MENU)
        optionsMenu->Render();

    if (optionsTabs->Current() == mUserTab)
        renderProfilesModal();

    if (options.keypadActive())
        options.keypadRender();
}

void GameStateOptions::ButtonPressed(int controllerId, int controlId)
{
    //Exit menu?
    if (controllerId == -102)
        switch (controlId)
        {
        case GameStateOptionsConst::kSaveAndBackToMainMenuID:
            mState = SAVE;
            break;
            //Set Audio volume
        case GameStateOptionsConst::kBackToMainMenuID:
            mParent->DoTransition(TRANSITION_FADE, GAME_STATE_MENU);
            break;
        case kCancelMenuID:
            mState = SHOW_OPTIONS;
            break;
        case GameStateOptionsConst::kNewProfileID:
            options.keypadStart("", &newProfile);
            options.keypadTitle("New Profile");
            break;
        case GameStateOptionsConst::kReloadID:
            mReload = true;
            break;
        case GameStateOptionsConst::kManageDataID:
#if defined (ANDROID)
            // Ask the Android layer to open the native import/export/download panel.
            JGE::GetInstance()->sendJNICommand("admin");
#endif
            break;
        }
    else
        optionsTabs->ButtonPressed(controllerId, controlId);
}
;

void GameStateOptions::GrabKeyboard(KeybGrabber* g)
{
    grabber = g;
}
void GameStateOptions::UngrabKeyboard(const KeybGrabber* g)
{
    if (g == grabber)
        grabber = NULL;
}

// ============================ Profiles manager (touch modal) ============================
// A self-contained, full-screen touch UI drawn over the Options screen. All coordinates are the
// virtual render space (0..SCREEN_WIDTH_F, 0..SCREEN_HEIGHT_F); taps arrive in the same space.

// A filled "pill" button with a centered label. Render + hit-test use identical rects, so the
// rect math lives next to each call site (not here) and this only paints.
static void pmPill(const char * label, float x, float y, float w, float h)
{
    JRenderer * r = JRenderer::GetInstance();
    r->FillRect(x, y, w, h, ARGB(255, 44, 60, 92));
    WFont * f = WResourceManager::Instance()->GetWFont(Fonts::OPTION_FONT);
    f->SetScale(SCALE);
    f->SetColor(ARGB(255, 240, 240, 245));
    f->DrawString(label, x + w / 2, y + (h - f->GetHeight()) / 2, JGETEXT_CENTER);
}

void GameStateOptions::buildProfileRows()
{
    mProfileRows.clear();

    // Enumerate profiles exactly as the profile selector does (scan profiles/ + a "Default" entry).
    OptionProfile * plist = NEW OptionProfile(mParent, this);
    std::vector<std::string> names = plist->selections;
    SAFE_DELETE(plist);

    std::string saved = options[Options::ACTIVE_PROFILE].str;
    for (size_t i = 0; i < names.size(); i++)
    {
        ProfileRow row;
        row.name = names[i];

        // Point ACTIVE_PROFILE at this profile so profileFile()/PlayerData read ITS files.
        options[Options::ACTIVE_PROFILE].str = names[i];

        std::string theme;
        int unlocked = 0;
        std::string contents;
        if (JFileSystem::GetInstance()->readIntoString(options.profileFile(PLAYER_SETTINGS), contents))
        {
            std::stringstream stream(contents);
            std::string s;
            while (std::getline(stream, s))
            {
                if (s.size() >= 6 && s.compare(0, 5, "Theme") == 0 && (s[5] == '=' || s[5] == ' '))
                    theme = s.substr(6);
                else if (s.substr(0, 9) == "unlocked_")
                    unlocked++;
            }
        }
        // Trim whitespace/CR from the parsed theme value.
        while (!theme.empty() && (theme[0] == ' ' || theme[0] == '=')) theme.erase(0, 1);
        while (!theme.empty() && (theme[theme.size() - 1] == '\r' || theme[theme.size() - 1] == '\n' || theme[theme.size() - 1] == ' '))
            theme.erase(theme.size() - 1);
        row.theme = theme.empty() ? std::string("MTG") : theme;

        PlayerData * pdata = NEW PlayerData(MTGCollection());
        char buf[256];
        sprintf(buf, "%i credits    %i cards    %i sets", pdata->credits, pdata->collection->totalCards(), unlocked);
        row.stats = buf;
        SAFE_DELETE(pdata);

        mProfileRows.push_back(row);
    }
    options[Options::ACTIVE_PROFILE].str = saved;
}

void GameStateOptions::renderProfileCard(const ProfileRow & row, float x, float y, float w, float h)
{
    JRenderer * r = JRenderer::GetInstance();
    WFont * big = WResourceManager::Instance()->GetWFont(Fonts::MAGIC_FONT);
    WFont * small = WResourceManager::Instance()->GetWFont(Fonts::OPTION_FONT);

    // Highlight the row of the profile that is currently the active user.
    bool active = (options[Options::ACTIVE_PROFILE].str == row.name);
    r->FillRect(x, y, w, h, active ? ARGB(255, 34, 46, 70) : ARGB(255, 26, 30, 40));
    if (active) r->FillRect(x, y, 5, h, ARGB(255, 92, 152, 236)); // active accent bar

    char buf[256];
    float tx = x + 8;

    // Avatar (left).
    if (row.name == "Default") sprintf(buf, "player/avatar.jpg");
    else sprintf(buf, "profiles/%s/avatar.jpg", row.name.c_str());
    JQuadPtr av = WResourceManager::Instance()->RetrieveTempQuad(buf, TEXTURE_SUB_EXACT);
    if (av && av->mHeight > 0)
    {
        float s = (h - 12) / av->mHeight;
        r->RenderQuad(av.get(), tx, y + 6, 0, s, s);
        tx += av->mWidth * s + 8;
    }

    // Name + stats (left).
    big->SetScale(SCALE);
    big->SetColor(ARGB(255, 245, 245, 250));
    big->DrawString(row.name.c_str(), tx, y + 6, JGETEXT_LEFT);
    small->SetScale(SCALE * 0.9f);
    small->SetColor(ARGB(255, 178, 184, 196));
    small->DrawString(row.stats.c_str(), tx, y + 8 + big->GetHeight(), JGETEXT_LEFT);

    // Theme preview image (right), with the theme name tucked above its left edge.
    sprintf(buf, "themes/%s/preview.png", row.theme.c_str());
    JQuadPtr pv = WResourceManager::Instance()->RetrieveTempQuad(buf, TEXTURE_SUB_EXACT);
    if (!pv || pv->mHeight <= 0)
        pv = WResourceManager::Instance()->RetrieveTempQuad("graphics/preview.png", TEXTURE_SUB_EXACT);
    if (pv && pv->mWidth > 0 && pv->mHeight > 0)
    {
        float pvH = h - 12;
        float s = pvH / pv->mHeight;
        float pvW = pv->mWidth * s;
        float maxW = w * 0.34f;
        if (pvW > maxW) { s = maxW / pv->mWidth; pvW = maxW; pvH = pv->mHeight * s; }
        float px = x + w - pvW - 8;
        float py = y + (h - pvH) / 2;
        r->RenderQuad(pv.get(), px, py, 0, s, s);
        small->SetScale(SCALE * 0.8f);
        small->SetColor(ARGB(255, 205, 210, 220));
        small->DrawString(row.theme.c_str(), px + pvW, py - small->GetHeight() - 1, JGETEXT_RIGHT);
    }
}

void GameStateOptions::renderProfilesModal()
{
    JRenderer * r = JRenderer::GetInstance();
    const float W = SCREEN_WIDTH_F, H = SCREEN_HEIGHT_F;
    const float PM_TOP = 32.f; // just below the tab bar

    // Opaque panel over the User tab body. The tab bar above PM_TOP stays visible + tappable.
    r->FillRect(0, PM_TOP, W, H - PM_TOP, ARGB(252, 12, 14, 18));

    if (mProfilesDetail < 0)
    {
        // ---- list view: one scrollable row per profile, then a "New Profile" row ----
        const float top = PM_TOP + 4, bottom = H - 8, rowH = 58;
        r->FillRect(10, top, W - 20, bottom - top, ARGB(255, 10, 12, 16));
        int n = (int) mProfileRows.size();
        for (int i = 0; i <= n; i++)
        {
            float ry = top + i * rowH - mProfilesScroll;
            if (ry + rowH - 2 < top || ry > bottom) continue;
            if (i < n)
                renderProfileCard(mProfileRows[i], 12, ry + 2, W - 24, rowH - 4);
            else
                pmPill("+  New Profile", 12, ry + 2 + (rowH - 4 - 30) / 2, W - 24, 30);
        }

        // Scroll indicator on the right edge when the list overflows the viewport (same colors as
        // the WGui lists' scrollbar).
        float contentH = (n + 1) * rowH;
        float viewH = bottom - top;
        if (contentH > viewH + 1)
        {
            float sbX = W - 10 - 4;
            r->FillRect(sbX, top, 3, viewH, ARGB(120, 40, 46, 56));       // track
            float thumbH = viewH * viewH / contentH;
            if (thumbH < 14) thumbH = 14;
            float denom = contentH - viewH;
            float thumbY = top + (denom > 0 ? (viewH - thumbH) * (mProfilesScroll / denom) : 0);
            r->FillRect(sbX, thumbY, 3, thumbH, ARGB(220, 180, 190, 205)); // thumb
        }
    }
    else if (mProfilesDetail < (int) mProfileRows.size())
    {
        // ---- detail view: the chosen profile + its options ----
        ProfileRow & row = mProfileRows[mProfilesDetail];
        renderProfileCard(row, 10, PM_TOP + 4, W - 20, 64);
        const float by = PM_TOP + 76; // below the 64px card + a small gap
        pmPill("Use This Profile", 10, by, W - 20, 30);
        pmPill("Select Theme  (tap to cycle)", 10, by + 36, W - 20, 30);
        pmPill("Back to List", 10, by + 72, W - 20, 30);
    }
}

void GameStateOptions::updateProfilesModal(float dt)
{
    JGE * j = JGE::GetInstance();
    const float W = SCREEN_WIDTH_F, H = SCREEN_HEIGHT_F;
    const float PM_TOP = 32.f;
    const float top = PM_TOP + 4, rowH = 58;

    JButton key = JGE_BTN_NONE;
    int x = -1, y = -1;
    while ((key = j->ReadButton()) || j->GetLeftClickCoordinates(x, y))
    {
        if (key == JGE_BTN_LEFT || key == JGE_BTN_RIGHT)
            optionsTabs->CheckUserInput(key);              // let the tab bar change tabs
        else if (key == JGE_BTN_UP)   { if (mProfilesDetail < 0) mProfilesScroll -= 40; }
        else if (key == JGE_BTN_DOWN) { if (mProfilesDetail < 0) mProfilesScroll += 40; }
        else if (key == JGE_BTN_MENU) mState = SHOW_OPTIONS_MENU;
        else if (key == JGE_BTN_NONE)                      // a tap is pending at (x, y)
        {
            if (y < PM_TOP)
            {
                optionsTabs->CheckUserInput(JGE_BTN_NONE);  // tapped the tab bar -> WGui switches tab
            }
            else if (mProfilesDetail < 0)
            {
                int idx = (int) ((y - top + mProfilesScroll) / rowH);
                int n = (int) mProfileRows.size();
                if (idx >= 0 && idx < n) mProfilesDetail = idx;                       // open a profile
                else if (idx == n) { options.keypadStart("", &newProfile); options.keypadTitle("New Profile"); }
            }
            else if (mProfilesDetail < (int) mProfileRows.size() && x >= 10 && x <= W - 10)
            {
                const float by = PM_TOP + 76;
                if (y >= by && y <= by + 30)                                          // Use This Profile
                {
                    options[Options::ACTIVE_PROFILE] = mProfileRows[mProfilesDetail].name;
                    mReload = true;
                    mProfilesDetail = -1;
                }
                else if (y >= by + 36 && y <= by + 66) cycleProfileTheme(mProfilesDetail); // Select Theme
                else if (y >= by + 72 && y <= by + 102) mProfilesDetail = -1;               // Back to List
            }
            j->ResetInput(); // clear the tap + its paired OK so nothing double-fires or hangs the loop
        }
        // JGE_BTN_OK (the paired press of a tap) and anything else: consumed by ReadButton, ignored.
    }

    if (mProfilesDetail < 0) // clamp the list scroll to its content
    {
        float contentH = ((int) mProfileRows.size() + 1) * rowH;
        float viewH = (H - 8) - top;
        float maxScroll = contentH - viewH;
        if (maxScroll < 0) maxScroll = 0;
        if (mProfilesScroll < 0) mProfilesScroll = 0;
        if (mProfilesScroll > maxScroll) mProfilesScroll = maxScroll;
    }

    optionsTabs->Update(dt); // keep the tab bar's own animations running
}

void GameStateOptions::cycleProfileTheme(int idx)
{
    if (idx < 0 || idx >= (int) mProfileRows.size()) return;

    // Themes available on disk (scan themes/ for preview.png), same source as the theme selector.
    OptionThemeStyle * ots = NEW OptionThemeStyle("Theme Style");
    OptionTheme * ot = NEW OptionTheme(ots);
    std::vector<std::string> themes = ot->selections;
    SAFE_DELETE(ot);
    SAFE_DELETE(ots);
    if (themes.empty()) return;

    size_t cur = 0;
    for (size_t t = 0; t < themes.size(); t++)
        if (themes[t] == mProfileRows[idx].theme) { cur = t; break; }
    std::string next = themes[(cur + 1) % themes.size()];

    // ACTIVE_THEME is per-profile, so to change THIS profile's theme we make it the active profile,
    // set + save its theme, then apply (this profile becomes the current user as a side effect).
    options[Options::ACTIVE_PROFILE] = mProfileRows[idx].name;
    options.reloadProfile();
    options[Options::ACTIVE_THEME] = next;
    options.save();
    if (options.getStyleMan())
        options.getStyleMan()->determineActive(NULL, NULL);
    WResourceManager::Instance()->Refresh();

    mProfileRows[idx].theme = next;
}
