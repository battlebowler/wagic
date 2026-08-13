#include "PrecompiledHeader.h"
#include "DeckEditorMenu.h"
#include "DeckDataWrapper.h"
#include "DeckStats.h"
#include "JTypes.h"
#include "GameApp.h"
#include <iomanip>
#include "Translate.h"

DeckEditorMenu::DeckEditorMenu(int id, JGuiListener* listener, int fontId, const string& _title, DeckDataWrapper *_selectedDeck, StatsWrapper *stats) :
        DeckMenu(id, listener, fontId, _title), selectedDeck(_selectedDeck), stw(stats)
{
#if !defined (PSP)
    //Now it's possibile to randomly use up to 10 background images for deck editor selection (if random index is 0, it will be rendered the default "menubgdeckeditor.jpg" image).
    ostringstream bgFilename;
    char temp[4096];
    sprintf(temp, "menubgdeckeditor%i", std::rand() % 10);
    backgroundName.assign(temp);
    bgFilename << backgroundName << ".jpg";
    JQuadPtr background = WResourceManager::Instance()->RetrieveTempQuad(bgFilename.str(), TEXTURE_SUB_5551);
    if (!background.get()){
        backgroundName = "menubgdeckeditor"; //Fallback to default background image for deck editor selection.
    }
#else
    backgroundName = "menubgdeckeditor";
#endif

    mShowDetailsScreen = false;
    deckTitle = selectedDeck ? selectedDeck->parent->meta_name : "";

    // All positions as fractions of screen dimensions.
    // Original PSP values (for 480x272) in comments.
    mX = SCREEN_WIDTH_F * (123.0f / 480.0f);        // was 123
    mY = SCREEN_HEIGHT_F * (70.0f / 272.0f);         // was 70
    starsOffsetX = SCREEN_WIDTH_F * (50.0f / 480.0f); // was 50

    if(selectedDeck)
    {
#if defined PSP
        titleX = (SCREEN_WIDTH_F/2.f) + SCREEN_WIDTH_F * (10.0f / 480.0f);
#else
        titleX = (SCREEN_WIDTH_F/2.f);
#endif
        titleY = SCREEN_HEIGHT_F * (13.0f / 272.0f);  // was 13
    }
    else
    {
        titleX = SCREEN_WIDTH_F/6.5f;
        titleY = SCREEN_HEIGHT_F * (25.0f / 272.0f);  // was 25
    }
    titleWidth = SCREEN_WIDTH_F * (180.0f / 480.0f);  // was 180

    descX = SCREEN_WIDTH_F * (275.0f / 480.0f);       // was 275
    descY = SCREEN_HEIGHT_F * (80.0f / 272.0f);        // was 80
    descHeight = SCREEN_HEIGHT_F * (154.0f / 272.0f);   // was 154
    descWidth = SCREEN_WIDTH_F * (175.0f / 480.0f);    // was 175

    statsHeight = SCREEN_HEIGHT_F * (50.0f / 272.0f);   // was 50
    statsWidth = SCREEN_WIDTH_F * (185.0f / 480.0f);    // was 185
    statsX = SCREEN_WIDTH_F * (280.0f / 480.0f);        // was 280
    statsY = SCREEN_HEIGHT_F * (12.0f / 272.0f);         // was 12

    avatarX = SCREEN_WIDTH_F * (222.0f / 480.0f);       // was 222
    avatarY = SCREEN_HEIGHT_F * (8.0f / 272.0f);          // was 8

    float scrollerWidth = SCREEN_WIDTH_F * (80.0f / 480.0f);  // was 80
    float scrollerX = SCREEN_WIDTH_F * (40.0f / 480.0f);       // was 40
    float scrollerY = SCREEN_HEIGHT_F * (230.0f / 272.0f);     // was 230
    float scrollerH = SCREEN_HEIGHT_F * (100.0f / 272.0f);     // was 100
    SAFE_DELETE(mScroller); // need to delete the scroller init in the base class
    mScroller = NEW VerticalTextScroller(Fonts::MAIN_FONT, scrollerX, scrollerY, scrollerWidth, scrollerH);

}

void DeckEditorMenu::Render()
{
    JRenderer *r = JRenderer::GetInstance();
    r->FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ARGB(200,0,0,0));

    DeckMenu::Render();
    if (deckTitle.size() > 0)
    {
        float modt = (float)deckTitle.size()/2;
        WFont *mainFont = WResourceManager::Instance()->GetWFont(Fonts::OPTION_FONT);
        DWORD currentColor = mainFont->GetColor();
        mainFont->SetColor(ARGB(255,255,255,255));
#if defined PSP
        mainFont->DrawString(deckTitle.c_str(), (SCREEN_WIDTH_F / 2)-modt + SCREEN_WIDTH_F * (10.0f / 480.0f), (statsHeight / 2) + SCREEN_HEIGHT_F * (4.0f / 272.0f), JGETEXT_CENTER);
#else
        mainFont->DrawString(deckTitle.c_str(), (SCREEN_WIDTH_F / 2)-modt, (statsHeight / 2) + SCREEN_HEIGHT_F * (4.0f / 272.0f), JGETEXT_CENTER);
#endif
        mainFont->SetColor(currentColor);
    }

    if (stw && selectedDeck) drawDeckStatistics();

}

void DeckEditorMenu::drawDeckStatistics()
{
    ostringstream deckStatsString;

    deckStatsString
            << _("------- Deck Summary -----") << endl
            << _("Cards: ") << stw->cardCount << "      Sideboard: " << selectedDeck->parent->Sideboard.size() << endl
            << _("Creatures: ") << setw(2) << stw->countCreatures
            << _("  Enchantments: ") << stw->countEnchantments << endl
            << _("Instants: ") << setw(4) << stw->countInstants
            << _("   Sorceries:      ") << setw(2) << stw->countSorceries << endl
            << _("Lands: ")
            << _("A: ") << setw(2) << left  << stw->countLandsPerColor[ Constants::MTG_COLOR_ARTIFACT ] + stw->countBasicLandsPerColor[ Constants::MTG_COLOR_ARTIFACT ] << " "
            << _("G: ") << setw(2) << left  << stw->countLandsPerColor[ Constants::MTG_COLOR_GREEN ] + stw->countLandsPerColor[ Constants::MTG_COLOR_GREEN ] << " "
            << _("R: ") << setw(2) << left  << stw->countLandsPerColor[ Constants::MTG_COLOR_RED ] + stw->countBasicLandsPerColor[ Constants::MTG_COLOR_RED ] << " "
            << _("U: ") << setw(2) << left  << stw->countLandsPerColor[ Constants::MTG_COLOR_BLUE ] + stw->countBasicLandsPerColor[ Constants::MTG_COLOR_BLUE ] << " "
            << _("B: ") << setw(2) << left  << stw->countLandsPerColor[ Constants::MTG_COLOR_BLACK ] + stw->countBasicLandsPerColor[ Constants::MTG_COLOR_BLACK ] << " "
            << _("W: ") << setw(2) << left  << stw->countLandsPerColor[ Constants::MTG_COLOR_WHITE ] + stw->countBasicLandsPerColor[ Constants::MTG_COLOR_WHITE ] << endl
            << _("  --- Card color count ---  ") << endl
            << _("A: ") << setw(2) << left  << selectedDeck->getCount(Constants::MTG_COLOR_ARTIFACT) << " "
            << _("G: ") << setw(2) << left << selectedDeck->getCount(Constants::MTG_COLOR_GREEN) << " "
            << _("U: ") << setw(2) << left << selectedDeck->getCount(Constants::MTG_COLOR_BLUE) << " "
            << _("R: ") << setw(2) << left << selectedDeck->getCount(Constants::MTG_COLOR_RED) << " "
            << _("B: ") << setw(2) << left << selectedDeck->getCount(Constants::MTG_COLOR_BLACK) << " "
            << _("W: ") << setw(2) << left << selectedDeck->getCount(Constants::MTG_COLOR_WHITE) << endl

            << _(" --- Average Cost --- ") << endl
            << _("Creature: ") << setprecision(2) << stw->avgCreatureCost << endl
            << _("Mana: ") << setprecision(2) << stw->avgManaCost << "   "
            << _("Spell: ") << setprecision(2) << stw->avgSpellCost << endl;

    WFont *mainFont = WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT);
    mainFont->DrawString(deckStatsString.str().c_str(), descX, descY + SCREEN_HEIGHT_F * (25.0f / 272.0f));
}

DeckEditorMenu::~DeckEditorMenu()
{
    SAFE_DELETE( mScroller );
}