#include "PrecompiledHeader.h"
#include "DeckEditorMenu.h"
#include "DeckDataWrapper.h"
#include "DeckStats.h"
#include "JTypes.h"
#include "GameApp.h"
#include <iomanip>
#include "Translate.h"
#include "UITheme.h"

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

    // Both variants use DeckMenu's clean list-only layout (narrow left list + right info panel,
    // left-aligned rows, title centered over the list). The selection variant (no selected deck)
    // uses DeckMenu's generic info panel to preview the focused deck; the deck-builder variant (a
    // deck IS selected) suppresses it and draws the edited deck's avatar + "Deck: <name>" + stats
    // itself (see Render). So we keep DeckMenu's list-only positions and don't override them here.
    mListOnlyLayout = true;
    mDrawInfoPanel = (selectedDeck == NULL);

    // Deck-stats text position for the deck-builder variant: inside the right info panel, below the
    // avatar (the list, title and panel frames come from DeckMenu's list-only positions).
    descX = SCREEN_WIDTH_F * (236.0f / 480.0f);
    descY = SCREEN_HEIGHT_F * (45.0f / 272.0f);

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

    DeckMenu::Render(); // frames + left-aligned list + centered title (generic info panel shown for
                        // the selection variant, suppressed for the deck-builder variant below)

    // Deck-builder variant: draw the edited deck's avatar + "Deck: <name>" + stats INSIDE the right
    // info panel (its frame is drawn by DeckMenu's list-only layout), matching the other screens.
    if (stw && selectedDeck)
    {
        const float ipX = SCREEN_WIDTH_F * (224.0f / 480.0f);
        const float ipY = SCREEN_HEIGHT_F * (4.0f / 272.0f);
        const float avW = SCREEN_WIDTH_F * (37.0f / 480.0f);
        const float avH = SCREEN_HEIGHT_F * (50.0f / 272.0f);
        const float avX = ipX + SCREEN_WIDTH_F * (12.0f / 480.0f);
        const float avY = ipY + SCREEN_HEIGHT_F * (12.0f / 272.0f);

        // Use the deck's own chosen avatar (#AVATAR:) when set, else the default portrait. Read
        // straight from the live MTGDeck so the "Set Avatar" picker's choice shows immediately in
        // the editor (no save/reopen needed).
        string avatarName = (selectedDeck->parent && selectedDeck->parent->meta_avatar.size())
                            ? selectedDeck->parent->meta_avatar : "avatar.jpg";
        JQuadPtr quad = WResourceManager::Instance()->RetrieveTempQuad(avatarName, TEXTURE_SUB_AVATAR);
        if (quad.get())
        {
            // Uniform scale keeps the avatar's native aspect (no 16:9 horizontal stretch).
            float as = (avW / quad->mWidth < avH / quad->mHeight) ? avW / quad->mWidth : avH / quad->mHeight;
            r->RenderQuad(quad.get(), avX, avY, 0, as, as);
            r->DrawRect(avX, avY, quad->mWidth * as, quad->mHeight * as, ARGB(200, 3, 3, 3));
        }

        WFont * hf = WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT);
        hf->SetColor(ARGB(255, 240, 240, 245));
        hf->DrawString((_("Deck: ") + deckTitle).c_str(), avX + avW + SCREEN_WIDTH_F * (8.0f / 480.0f), avY + SCREEN_HEIGHT_F * (6.0f / 272.0f));

        drawDeckStatistics();
    }
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