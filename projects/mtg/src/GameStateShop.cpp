/*
 The shop is where the player can buy cards, decks...
 */
#include "PrecompiledHeader.h"

#include <JRenderer.h>
#include "GameStateShop.h"
#include "GameStateMenu.h"
#include "GameApp.h"
#include "MTGDeck.h"
#include "MTGPack.h"
#include "Translate.h"
#include "TestSuiteAI.h"

#include <hge/hgedistort.h>
#include "WFont.h"

float GameStateShop::_x1[] = { 79, 19, 27, 103, 154, 187, 102, 144, 198, 133, 183 };
float GameStateShop::_y1[] = { 150, 194, 222, 167, 164, 156, 195, 190, 175, 220, 220 };

float GameStateShop::_x2[] = { 103, 48, 74, 135, 183, 215, 138, 181, 231, 171, 225 };
float GameStateShop::_y2[] = { 155, 179, 218, 165, 166, 155, 195, 186, 177, 225, 216 };

float GameStateShop::_x3[] = { 48, 61, 9, 96, 139, 190, 81, 146, 187, 97, 191 };
float GameStateShop::_y3[] = { 164, 205, 257, 184, 180, 170, 219, 212, 195, 251, 252 };

float GameStateShop::_x4[] = { 76, 90, 65, 131, 171, 221, 123, 187, 225, 141, 237 };
float GameStateShop::_y4[] = { 169, 188, 250, 182, 182, 168, 220, 208, 198, 259, 245 };

namespace
{
#ifndef TOUCH_ENABLED
    float kPspIconScaleFactor = 0.5f;
#endif // TOUCH_ENABLED
    std::string kOtherCardsString(": Other cards");
    std::string kCreditsString("Credits: ");

    // Layout for the flat vertical shop list (booster packs on top, cards below), which
    // replaced the old "tabletop" perspective layout. Heights are fixed (SCREEN_HEIGHT is
    // constant); the list width is derived from SCREEN_WIDTH_F at runtime.
    const float kShopListX    = 8.0f;
    const float kShopListTop  = 20.0f;
    const float kShopHeaderH  = 14.0f;
    const float kShopRowH     = 17.0f;

    // Width of the list column (left side); the card preview sits to the right of it.
    inline float shopListW() { return SCREEN_WIDTH_F * 0.42f; }

    // Top-left y of the row for a given shop slot, accounting for the two section headers
    // ("Booster Packs" then "Cards").
    inline float shopRowY(int slot)
    {
        float y = kShopListTop + kShopHeaderH; // below the "Booster Packs" header
        if (slot < BOOSTER_SLOTS)
            return y + kShopRowH * slot;
        return y + kShopRowH * BOOSTER_SLOTS + kShopHeaderH + kShopRowH * (slot - BOOSTER_SLOTS);
    }

    // Which shop slot (if any) a point falls on. Returns -1 if the point is off the list.
    inline int shopSlotAtPoint(float x, float y, float listW)
    {
        if (x < kShopListX || x > kShopListX + listW)
            return -1;
        for (int i = 0; i < SHOP_SLOTS; i++)
        {
            float ry = shopRowY(i);
            if (y >= ry && y < ry + kShopRowH)
                return i;
        }
        return -1;
    }
}


BoosterDisplay::BoosterDisplay(int id, GameObserver* game, int x, int y, JGuiListener * listener, TargetChooser * tc,
                int nb_displayed_items) :
    CardDisplay(id, game, x, y, listener, tc, nb_displayed_items)
{
}

bool BoosterDisplay::CheckUserInput(JButton key)
{
    if (JGE_BTN_UP == key || JGE_BTN_DOWN == key || JGE_BTN_PRI == key)
        return false;

    return CardDisplay::CheckUserInput(key);

}

GameStateShop::GameStateShop(GameApp* parent) :
    GameState(parent, "shop")
{
    menu = NULL;
    boosterDisplay = NULL;
    taskList = NULL;
    srcCards = NULL;
    shopMenu = NULL;
    bigDisplay = NULL;
    myCollection = NULL;
    packlist = NULL;
    pricelist = NULL;
    playerdata = NULL;
    booster = NULL;
    lightAlpha = 0;
    filterMenu = NULL;
    alphaChange = 0;
    mRefreshCount = 0;
    mLastRefreshDay = 0;
    mStockLoaded = false;
    for (int i = 0; i < BOOSTER_SLOTS; i++) mBoosterArt[i] = 0;
    for (int i = 0; i < SHOP_ITEMS; i++)
    {
        mPrices[i] = 0;
        mCounts[i] = 0;
        mFoilSingle[i] = false;
    }
    mTouched = false;

    kOtherCardsString = _(kOtherCardsString);
    kCreditsString = _(kCreditsString);
    
    cycleCardsButton = NEW InteractiveButton(NULL, kCycleCardsButtonId, Fonts::MAIN_FONT, "New Cards", SCREEN_WIDTH_F - 110, SCREEN_HEIGHT_F - 20, JGE_BTN_PRI);
    
    // "Show List" button removed: the flat item list is always visible now.
    showCardListButton = NULL;
    shopMenuButton = NEW InteractiveButton(NULL, kMenuButtonId, Fonts::MAIN_FONT, "Menu", SCREEN_WIDTH_F - 45, SCREEN_HEIGHT_F - 20, JGE_BTN_MENU);
    // Task-board Back button, styled identically to New Cards/Menu, placed to their left.
    taskBackButton = NEW InteractiveButton(NULL, kMenuButtonId, Fonts::MAIN_FONT, "Back", SCREEN_WIDTH_F - 150, SCREEN_HEIGHT_F - 20, JGE_BTN_SEC);
    disablePurchase = false;
    clearInput = false;
}

GameStateShop::~GameStateShop()
{
    SAFE_DELETE( cycleCardsButton );
    SAFE_DELETE( showCardListButton );
    SAFE_DELETE( shopMenuButton );
    SAFE_DELETE( taskBackButton );
    End();
    // The persistent shop stock kept across visits (see Start/End) is freed on shutdown.
    SAFE_DELETE( srcCards );
    SAFE_DELETE( packlist );
}

void GameStateShop::Create()
{
}

void GameStateShop::Start()
{
    menu = NULL;
    bListCards = false;
    mTouched = false;
    mStage = STAGE_FADE_IN;
    needLoad = true;
    booster = NULL;
    // Keep the same shop stock across visits: only build (and shuffle) srcCards the first
    // time. Recreating it every visit reshuffled the singles = a free refresh on re-entry.
    if (!srcCards)
    {
        srcCards = NEW WSrcUnlockedCards(0);
        srcCards->setElapsed(15);
        srcCards->addFilter(NEW WCFilterNOT(NEW WCFilterRarity("T")));
        srcCards->addFilter(NEW WCFilterNOT(NEW WCFilterSet(MTGSets::INTERNAL_SET)));
    }

    shopMenu = NEW WGuiMenu(JGE_BTN_DOWN, JGE_BTN_UP, true, &bigSync);
    MTGAllCards * ac = MTGCollection();
    playerdata = NEW PlayerData(ac);
    myCollection = NEW DeckDataWrapper(playerdata->collection);
    pricelist = NEW PriceList("settings/prices.dat", ac);
    for (int i = 0; i < SHOP_SLOTS; i++)
    {
        WGuiCardDistort * dist;
        if (i < BOOSTER_SLOTS)
            dist = NEW WGuiCardDistort(NULL, true);
        else
        {
            dist = NEW WGuiCardDistort(srcCards, true);
            dist->mOffset.setOffset(i - BOOSTER_SLOTS);
        }
        dist->xy = WDistort(_x1[i], _y1[i], _x2[i], _y2[i], _x3[i], _y3[i], _x4[i], _y4[i]);
        shopMenu->Add(NEW WGuiButton(dist, -102, i, this));
    }
    shopMenu->Entering(JGE_BTN_NONE);

    if (!bigDisplay)
    {
        bigDisplay = NEW WGuiCardImage(srcCards);
        bigDisplay->mOffset.Hook(&bigSync);
        bigDisplay->mOffset.setOffset(-BOOSTER_SLOTS);
        // Preview sits to the right of the list (~72% across) so it no longer overlaps
        // the item list on the left.
        bigDisplay->setX(SCREEN_WIDTH_F * 0.72f);
        bigDisplay->setY(135);
        // Full-size card art nearly fills the preview column and dwarfs the booster packs;
        // trim it a touch so cards and packs read at a similar size.
        bigDisplay->setScale(0.85f);
    }

    for (int i = 0; i < 8; ++i)
    {
        std::ostringstream stream;
        stream << "iconspsp" << i;
        pspIcons[i] = WResourceManager::Instance()->RetrieveQuad("iconspsp.png", (float) i * 32, 0, 32, 32, stream.str(), RETRIEVE_MANAGE);
        pspIcons[i]->SetHotSpot(16, 16);
    }

#if !defined (PSP)
    GameApp::playMusic("Track2.mp3"); // Added music for shop.
#endif

    JRenderer::GetInstance()->EnableVSync(true);

    taskList = NULL;
    // packlist is kept across visits too: booster slots (mBooster) hold pointers into it, so
    // it must outlive the stock we're now persisting.
    if (!packlist)
    {
        packlist = NEW MTGPacks();
        packlist->loadAll();
    }
    // Roll the shop stock (singles + boosters) once, then keep it; on re-entry just refresh
    // the owned-copy counts. This is what stops leaving/returning from being a free refresh.
    if (!mStockLoaded)
    {
        load();
        mStockLoaded = true;
    }
    else
        updateCounts();
}

string GameStateShop::descPurchase(int controlId, bool tiny)
{
    char buffer[4096];
    string name;
    if (controlId < BOOSTER_SLOTS)
    {
        name = mBooster[controlId].getName();
    }
    else
    {
        MTGCard * c = srcCards->getCard(controlId - BOOSTER_SLOTS);
        if (!c)
            return "";
        name = _(c->data->getName());
        if (mFoilSingle[controlId]) name = "[Foil] " + name;
    }
    if (mInventory[controlId] <= 0)
    {
        if (tiny)
            sprintf(buffer, _("SOLD OUT").c_str(), name.c_str());
        else
            sprintf(buffer, _("%s : SOLD OUT").c_str(), name.c_str());
        return buffer;
    }

    if (tiny)
    {
        if (controlId < BOOSTER_SLOTS || mCounts[controlId] == 0)
            return name;

        sprintf(buffer, _("%s (%i)").c_str(), name.c_str(), mCounts[controlId]);
        return buffer;
    }
    switch (options[Options::ECON_DIFFICULTY].number)
    {
    case Constants::ECON_HARD:
    case Constants::ECON_NORMAL:
        if (mCounts[controlId] < 1)
            sprintf(buffer, _("%s").c_str(), name.c_str());
        else
            sprintf(buffer, _("%s (%i)").c_str(), name.c_str(), mCounts[controlId]);
        break;
    default:
        if (mCounts[controlId] < 1)
            sprintf(buffer, _("%s : %i credits").c_str(), name.c_str(), mPrices[controlId]);
        else
            sprintf(buffer, _("%s (%i) : %i credits").c_str(), name.c_str(), mCounts[controlId], mPrices[controlId]);
        break;
    }
    return buffer;
}
void GameStateShop::beginPurchase(int controlId)
{
    WFont * mFont = WResourceManager::Instance()->GetWFont(Fonts::MENU_FONT);
    mFont->SetScale(SCALE_NORMAL);
    SAFE_DELETE(menu);
    if (mInventory[controlId] <= 0)
    {
        menu = NEW SimpleMenu(JGE::GetInstance(), WResourceManager::Instance(), -145, this, Fonts::MENU_FONT, SCREEN_WIDTH - 300, SCREEN_HEIGHT / 2, _("Sold Out").c_str());
        menu->Add(-1, "Ok");
    }
    else if (playerdata->credits - mPrices[controlId] < 0)
    {
        menu = NEW SimpleMenu(JGE::GetInstance(), WResourceManager::Instance(), -145, this, Fonts::MENU_FONT, SCREEN_WIDTH - 300, SCREEN_HEIGHT / 2, _("Not enough credits").c_str());
        menu->Add(-1, "Ok");
        if (options[Options::CHEATMODE].number)
        {
            menu->Add(-2, _("Steal it").c_str());
        }
    }
    else
    {
        char buf[512];
        if (controlId < BOOSTER_SLOTS)
            sprintf(buf, _("Purchase Booster: %i credits").c_str(), mPrices[controlId]);
        else
            sprintf(buf, _("Purchase Card: %i credits").c_str(), mPrices[controlId]);
        menu = NEW SimpleMenu(JGE::GetInstance(), WResourceManager::Instance(), -145, this, Fonts::MENU_FONT, SCREEN_WIDTH - 300, SCREEN_HEIGHT / 2, buf);

        menu->Add(controlId, "Yes");
        menu->Add(-1, "No");
    }
}
void GameStateShop::cancelCard(int controlId)
{
    //Update prices
    MTGCard * c = srcCards->getCard(controlId - BOOSTER_SLOTS);
    if (!c || !c->data || playerdata->credits - mPrices[controlId] < 0)
        return; //We only care about their opinion if they /can/ buy it.

    int price = mPrices[controlId];
    int rnd;
    switch (options[Options::ECON_DIFFICULTY].number)
    {
    case Constants::ECON_HARD:
        rnd = rand() % 10;
        break;
    case Constants::ECON_EASY:
        rnd = rand() % 50;
        break;
    default:
        rnd = rand() % 25;
        break;
    }
    price = price - (rnd * price) / 100;
    if (price < pricelist->getPrice(c)) //filters have a tendancy to increase the price instead of lowering it!
        pricelist->setPrice(c, price);
    //Prices do not immediately go down when you ignore something.
    return;
}
void GameStateShop::cancelBooster(int)
{
    return; //TODO FIXME Tie boosters into pricelist.
}

// Foils command a big premium over the same card non-foil.
static const int kFoilPriceMult = 6;

// Older sets cost more: scale the price up as the set's release year recedes. Sets from
// ~2022 on stay at 1x; the oldest sets (Alpha/Beta, 1993) top out around 5x. A year of 0
// (unknown, or a mixed/custom pack with no single set) gets no age premium.
static float agePriceMultiplier(int year)
{
    if (year <= 0) return 1.0f;
    float mult = 1.0f + 0.12f * (float) (2022 - year);
    if (mult < 1.0f) mult = 1.0f;
    if (mult > 5.0f) mult = 5.0f;
    return mult;
}

void GameStateShop::purchaseCard(int controlId)
{
    MTGCard * c = srcCards->getCard(controlId - BOOSTER_SLOTS);
    if (!c || !c->data || playerdata->credits - mPrices[controlId] < 0 || (c && c->getRarity() == Constants::RARITY_T))//cant buy tokens....
        return;
    myCollection->Add(c);
    if (mFoilSingle[controlId] && playerdata && playerdata->collection)
    {
        // Record the foil copy in the collection; save()'s Rebuild keeps foilCount.
        playerdata->collection->addFoil(c->getId(), 1);
        mFoilSingle[controlId] = false; // the foil copy has been sold
    }
    int price = mPrices[controlId];
    playerdata->credits -= price;
    GameApp::mycredits = playerdata->credits;
    //Update prices. The demand bump applies to the BASE pricelist price only — the age and
    //foil premiums are display multipliers and must NOT be baked into the saved base price
    //(otherwise buying a foil/old-set card would permanently inflate that card everywhere).
    int rnd;
    switch (options[Options::ECON_DIFFICULTY].number)
    {
    case Constants::ECON_HARD:
        rnd = rand() % 50;
        break;
    case Constants::ECON_EASY:
        rnd = rand() % 10;
        break;
    default:
        rnd = rand() % 25;
        break;
    }
    int base = pricelist->getPurchasePrice(c->getMTGId());
    base = base + (rnd * base) / 100;
    pricelist->setPrice(c->getMTGId(), base);
    //Re-apply the age premium for display (the foil copy, if any, was already consumed above).
    MTGSetInfo * psi = setlist.getInfo(c->setId);
    mPrices[controlId] = (int) (pricelist->getPurchasePrice(c->getMTGId()) * agePriceMultiplier(psi ? psi->year : 0)); //Prices go up immediately.
    mInventory[controlId]--;
    updateCounts();
    mTouched = true;
    menu->Close();
}
void GameStateShop::purchaseBooster(int controlId)
{
    if (playerdata->credits - mPrices[controlId] < 0)
        return;
    playerdata->credits -= mPrices[controlId];
    GameApp::mycredits = playerdata->credits;
    mInventory[controlId]--;
    SAFE_DELETE(booster);
    deleteDisplay();
    booster = NEW MTGDeck(MTGCollection());
    boosterDisplay = NEW BoosterDisplay(12, NULL, SCREEN_WIDTH - 255, SCREEN_HEIGHT-65, this, NULL, 7);
    mBooster[controlId].addToDeck(booster, srcCards);

    string sort = mBooster[controlId].getSort();
    DeckDataWrapper * ddw = NEW DeckDataWrapper(booster);
    if (sort == "alpha")
        ddw->Sort(WSrcCards::SORT_ALPHA);
    else if (sort == "collector")
        ddw->Sort(WSrcCards::SORT_COLLECTOR);
    else
        ddw->Sort(WSrcCards::SORT_RARITY);

    vector<MTGCardInstance*> thisPack; // instances opened in THIS pack (for the foil roll)
    for (int x = 0; x < ddw->Size(); x++)
    {
        MTGCard * c = ddw->getCard(x);
        for (int copies = 0; copies < ddw->count(c); ++copies)
        {
            MTGCardInstance * ci = NEW MTGCardInstance(c, NULL);
            boosterDisplay->AddCard(ci);
            subBooster.push_back(ci);
            thisPack.push_back(ci);
        }
    }
    SAFE_DELETE(ddw);

    myCollection->loadMatches(booster);

    // Foil pull: only sets from the foil era can yield a foil, and older foil-era sets are
    // rarer (matching e.g. 7ED's notoriously tough foils). Pre-foil sets never yield foils.
    // When pulled, one random card in the pack becomes foil (shown shiny) and is recorded
    // in the collection.
    {
        const int kFoilFromYear = 1999;      // foils debuted ~Urza's Legacy / early 7ED era
        int yr = mBooster[controlId].getSetYear();
        // Foil-era packs yield a foil ~15% of the time; older foil-era sets (pre-2004) are
        // rarer at ~7%, matching e.g. 7ED's notoriously tough foils.
        int foilChance = 15;
        if (yr && yr < 2004) foilChance = 7;
        if (!thisPack.empty() && yr >= kFoilFromYear && (rand() % 100) < foilChance)
        {
            int idx = rand() % (int) thisPack.size();
            MTGCardInstance * fc = thisPack[idx];
            if (fc && fc->getId())
            {
                fc->foil = true;
                if (playerdata && playerdata->collection)
                    playerdata->collection->addFoil(fc->getId(), 1);
                if (boosterDisplay)
                    boosterDisplay->setCurrentCard(idx); // open the reveal on the foil card
            }
        }
    }

    mTouched = true;
    save(true);
    menu->Close();
}

int GameStateShop::purchasePrice(int offset)
{
    MTGCard * c = NULL;
    if (!pricelist || !srcCards || (c = srcCards->getCard(offset)) == NULL)
        return 0;
    float price = (float) pricelist->getPurchasePrice(c->getMTGId());
    int filteradd = srcCards->Size(true);
    filteradd = ((filteradd - srcCards->Size()) / filteradd);

    switch (options[Options::ECON_DIFFICULTY].number)
    {
    case Constants::ECON_EASY:
        filteradd /= 2;
        break;
    case Constants::ECON_HARD:
        filteradd *= 2;
        break;
    default:
        break;
    }
    return (int) (price + price * (filteradd * srcCards->filterFee()));
}
void GameStateShop::updateCounts()
{
    for (int i = BOOSTER_SLOTS; i < SHOP_ITEMS; i++)
    {
        MTGCard * c = srcCards->getCard(i - BOOSTER_SLOTS);
        if (!c)
            mCounts[i] = 0;
        else
            mCounts[i] = myCollection->countByName(c);
    }
}
int GameStateShop::computeRefreshCost()
{
    // Cool-down: the escalation drops by 1 for each in-game day (duel) elapsed since the
    // last refresh/charge, so the cost recovers over time instead of climbing forever.
    int passed = TaskList::sDaysElapsed - mLastRefreshDay;
    if (passed > 0)
    {
        mRefreshCount -= passed;
        if (mRefreshCount < 0) mRefreshCount = 0;
        mLastRefreshDay = TaskList::sDaysElapsed;
    }
    // Grows with each refresh AND with current wealth, so it can't be power-spammed.
    return 250 * (mRefreshCount + 1) + (playerdata ? playerdata->credits / 8 : 0);
}
// Booster art can now ship as numbered variants: booster_<SET>_1, booster_<SET>_2, ...
// Count how many exist (stopping at the first gap) and pick one at random, so each shop
// slot shows a stable, randomly-chosen pack art. Returns the chosen variant number, or 0 if
// the set has no numbered variants (caller then falls back to booster_<SET> / generic art).
static int chooseBoosterArtVariant(const string& setId)
{
    if (setId.empty()) return 0;
    const int kMaxVariants = 16;
    int count = 0;
    char buf[256];
    for (int k = 1; k <= kMaxVariants; ++k)
    {
        sprintf(buf, "booster_%s_%d.png", setId.c_str(), k);
        JQuadPtr q = WResourceManager::Instance()->RetrieveTempQuad(buf);
        bool exists = (q.get() && q->mHeight > 0);
        if (!exists)
        {
            sprintf(buf, "booster_%s_%d.jpg", setId.c_str(), k);
            q = WResourceManager::Instance()->RetrieveTempQuad(buf);
            exists = (q.get() && q->mHeight > 0);
        }
        if (!exists) break; // variants are numbered contiguously from 1; stop at the first gap
        count = k;
    }
    return (count <= 0) ? 0 : (1 + rand() % count);
}

void GameStateShop::load()
{
    for (int i = 0; i < BOOSTER_SLOTS; i++)
    {
        mBooster[i].randomize(packlist);
        mInventory[i] = 1 + rand() % mBooster[i].maxInventory();
        mPrices[i] = (int) (pricelist->getOtherPrice(mBooster[i].basePrice()) * agePriceMultiplier(mBooster[i].getSetYear()));
        mBoosterArt[i] = chooseBoosterArtVariant(mBooster[i].getSetId());
    }
    for (int i = BOOSTER_SLOTS; i < SHOP_ITEMS; i++)
    {
        MTGCard * c = NULL;
        if ((c = srcCards->getCard(i - BOOSTER_SLOTS)) == NULL)
        {
            mPrices[i] = 0;
            mCounts[i] = 0;
            mInventory[i] = 0;
            mFoilSingle[i] = false;
            continue;
        }
        // Foils in singles: rarer than in packs. Only foil-era sets, ~4% per restock.
        mFoilSingle[i] = false;
        MTGSetInfo * si = setlist.getInfo(c->setId);
        int yr = si ? si->year : 0;
        if (yr >= 1999 && (rand() % 100) < 4)
            mFoilSingle[i] = true;
        // Older sets cost more; foils cost a lot more (both stack on the base price).
        mPrices[i] = (int) (purchasePrice(i - BOOSTER_SLOTS) * agePriceMultiplier(yr));
        if (mFoilSingle[i]) mPrices[i] *= kFoilPriceMult;
        mCounts[i] = myCollection->countByName(c);
        switch (c->getRarity())
        {
        case Constants::RARITY_C:
            mInventory[i] = 2 + rand() % 8;
            break;
        case Constants::RARITY_L:
            mInventory[i] = 100;
            break;
        default: //We're using some non-coded rarities (S) in cards.dat.
        case Constants::RARITY_U:
            mInventory[i] = 1 + rand() % 5;
            break;
        case Constants::RARITY_R:
            mInventory[i] = 1 + rand() % 2;
            break;
        }

    }
}
void GameStateShop::save(bool force)
{
    if (mTouched || force)
    {
        if (myCollection)
            myCollection->Rebuild(playerdata->collection);
        if (playerdata)
            playerdata->save();
        if (pricelist)
            pricelist->save();
    }
    mTouched = false;
}
void GameStateShop::End()
{
    save();
    JRenderer::GetInstance()->EnableVSync(false);

    SAFE_DELETE(shopMenu);
    SAFE_DELETE(bigDisplay);
    // srcCards and packlist are intentionally NOT freed here — they hold the persistent shop
    // stock (see Start()). They're released in the destructor instead.
    SAFE_DELETE(playerdata);
    SAFE_DELETE(pricelist);
    SAFE_DELETE(myCollection);
    SAFE_DELETE(booster);
    SAFE_DELETE(filterMenu);
    deleteDisplay();

    SAFE_DELETE(menu);
    SAFE_DELETE(taskList);
}

void GameStateShop::Destroy()
{
}
void GameStateShop::beginFilters()
{
    if (!filterMenu)
    {
        filterMenu = NEW WGuiFilters("Ask about...", srcCards);
        filterMenu->setY(2);
        filterMenu->setHeight(SCREEN_HEIGHT - 2);
    }
    mStage = STAGE_ASK_ABOUT;
    filterMenu->Entering(JGE_BTN_NONE);
}
void GameStateShop::Update(float dt)
{
    if (menu && menu->isClosed())
        SAFE_DELETE(menu);
    srcCards->Update(dt);
    alphaChange = 25 - static_cast<int>((float) (rand() - 1) / (RAND_MAX) * 50.0f);
    lightAlpha += alphaChange;
    if (lightAlpha < 0)
        lightAlpha = 0;
    if (lightAlpha > 50)
        lightAlpha = 50;

    JButton btn;
    switch (mStage)
    {
    case STAGE_SHOP_PURCHASE:
        if (menu)
            menu->Update(dt);
        beginPurchase(mBuying);
        mStage = STAGE_SHOP_SHOP;
        break;
    case STAGE_SHOP_MENU:
        if (menu)
            menu->Update(dt);
        else
        {
            menu = NEW SimpleMenu(JGE::GetInstance(), WResourceManager::Instance(), 11, this, Fonts::MENU_FONT, SCREEN_WIDTH / 2 - 100, 20);
            menu->Add(22, _("Ask about...").c_str());
            menu->Add(14, _("Check task board").c_str());
            if (options[Options::CHEATMODE].number)
                menu->Add(-2, _("Steal 2,000 credits").c_str());
            menu->Add(12, _("Save And Exit").c_str());
            menu->Add(kCancelMenuID, _("Cancel").c_str());
        }
        break;
    case STAGE_SHOP_TASKS:
        if (menu && !menu->isClosed())
        {
            menu->Update(dt);
            return;
        }
        if (taskList)
        {
            // On-screen Back button (same widget style as New Cards/Menu): tap it to leave.
            if (taskBackButton && taskBackButton->ButtonPressed())
            {
                taskList->End();
                return;
            }
            btn = mEngine->ReadButton();
            taskList->Update(dt);
            if (taskList->getState() != TaskList::TASKS_INACTIVE)
            {
                if (btn == JGE_BTN_SEC || btn == JGE_BTN_CANCEL || btn == JGE_BTN_PREV)
                {
                    taskList->End();
                    return;
                }
                else if (taskList->getState() == TaskList::TASKS_ACTIVE && btn == JGE_BTN_MENU)
                {
                    if (!menu)
                    {
                        menu = NEW SimpleMenu(JGE::GetInstance(), WResourceManager::Instance(), 11, this, Fonts::MENU_FONT, SCREEN_WIDTH / 2 - 100, 20);
                        menu->Add(15, "Return to shop");
                        menu->Add(12, "Save And Exit");
                        menu->Add(kCancelMenuID, "Cancel");
                    }
                }
            }
            else
                mStage = STAGE_SHOP_SHOP;
        }

#ifdef TESTSUITE
        if ((mEngine->GetButtonClick(JGE_BTN_PRI)) && (taskList))
        {
            taskList->passOneDay();
            if (taskList->getTaskCount() < 6)
            {
                taskList->addRandomTask();
                taskList->addRandomTask();
            }
            taskList->save();
        }
#endif
        break;
    case STAGE_ASK_ABOUT:
        btn = mEngine->ReadButton();
        if (menu && !menu->isClosed())
        {
            menu->CheckUserInput(btn);
            menu->Update(dt);
            return;
        }
        if (filterMenu)
        {
            if (btn == JGE_BTN_CTRL)
            {
                needLoad = filterMenu->Finish();
                filterMenu->Update(dt);
                return;
            }
            if (filterMenu->isFinished())
            {
                if (needLoad)
                {
                    srcCards->addFilter(NEW WCFilterNOT(NEW WCFilterRarity("T")));
                    srcCards->addFilter(NEW WCFilterNOT(NEW WCFilterSet(MTGSets::INTERNAL_SET)));
                    if (!srcCards->Size())
                    {
                        srcCards->clearFilters(); //Repetition of check at end of filterMenu->Finish(), for the token removal
                        srcCards->addFilter(NEW WCFilterNOT(NEW WCFilterRarity("T")));
                        srcCards->addFilter(NEW WCFilterNOT(NEW WCFilterSet(MTGSets::INTERNAL_SET)));
                    }
                    load();
                }
                mStage = STAGE_SHOP_SHOP;
            }
            else
            {
                filterMenu->CheckUserInput(btn);
                filterMenu->Update(dt);
            }
            return;
        }
        break;
    case STAGE_SHOP_SHOP:
        btn = mEngine->ReadButton();
        if (menu && !menu->isClosed())
        {
            menu->CheckUserInput(btn);
            menu->Update(dt);
            return;
        }
        if (btn == JGE_BTN_MENU)
        {
            if (boosterDisplay)
            {
                deleteDisplay();
                return;
            }
            mStage = STAGE_SHOP_MENU;
            return;
        }
        else if (btn == JGE_BTN_CTRL)
            beginFilters();
        else if (btn == JGE_BTN_NEXT)
        {
            mStage = STAGE_SHOP_TASKS;
            if (!taskList)
                taskList = NEW TaskList();
            taskList->Start();
        }
        else if (boosterDisplay)
        {
            if (btn == JGE_BTN_SEC)
                deleteDisplay();
            else
            {
                // Finger-anchored browsing: while dragging across the reveal, inspect the
                // thumbnail directly under the finger instead of stepping like a slider.
                // Drains the swipe's directional keys so they don't also step the selection.
                int dx = -1, dy = -1;
                if (mEngine->GetDragCoordinates(dx, dy))
                {
                    int idx = boosterDisplay->thumbAtPoint((float)dx, (float)dy);
                    if (idx >= 0) boosterDisplay->setCurrentCard(idx);
                    mEngine->DragProcessed();
                    while (mEngine->ReadButton()) {} // drop the swipe's directional keys
                    boosterDisplay->Update(dt);
                    return;
                }
                boosterDisplay->CheckUserInput(btn);
                boosterDisplay->Update(dt);
            }
            return;
        }
        else if (btn == JGE_BTN_PRI) // "New Cards": confirm, then reshuffle for an escalating fee.
        {
            int refreshCost = computeRefreshCost();
            SAFE_DELETE(menu);
            if (!playerdata || playerdata->credits < refreshCost)
            {
                menu = NEW SimpleMenu(JGE::GetInstance(), WResourceManager::Instance(), -145, this, Fonts::MENU_FONT,
                                      SCREEN_WIDTH - 300, SCREEN_HEIGHT / 2, _("Not enough credits to refresh").c_str());
                menu->Add(-1, "Ok");
                clearInput = true;
                return;
            }
            // Confirm the paid refresh with the standard floating menu (like buy/save prompts).
            char buf[128];
            sprintf(buf, _("Refresh cards for %i credits?").c_str(), refreshCost);
            menu = NEW SimpleMenu(JGE::GetInstance(), WResourceManager::Instance(), -146, this, Fonts::MENU_FONT,
                                  SCREEN_WIDTH - 300, SCREEN_HEIGHT / 2, buf);
            menu->Add(1, "Yes");
            menu->Add(-1, "No");
            clearInput = true;
            return;
        }
        else if (btn == JGE_BTN_CANCEL)
            options[Options::DISABLECARDS].number = !options[Options::DISABLECARDS].number;
        else if (btn == JGE_BTN_SEC)
        {
            bListCards = !bListCards;
            disablePurchase = false;
            clearInput = true;
            return;
        }
        else if (shopMenu)
        {
            // Finger-anchored browsing: while dragging, select the row under the finger and
            // preview it (no purchase), like the open hand — instead of the swipe stepping
            // the selection like a slider. Drains the swipe's nav keys so it doesn't also
            // step. A drag never commits a tap (LeftClicked only fires on a no-move up).
            {
                int dx = -1, dy = -1;
                if (mEngine->GetDragCoordinates(dx, dy))
                {
                    int slot = shopSlotAtPoint((float)dx, (float)dy, shopListW());
                    if (slot >= 0 && shopMenu->getSelected() != slot)
                    {
                        shopMenu->setSelected(slot);
                        bigSync.setOffset(slot);
                        srcCards->Touch();
                    }
                    mEngine->DragProcessed();
                    while (mEngine->ReadButton()) {} // drop the swipe's directional keys
                    if (shopMenu) shopMenu->Update(dt);
                    return;
                }
            }

            // Bottom toolbar buttons (New Cards / Show List / Menu) get first crack at a tap.
#if defined (IOS) || defined (ANDROID)
            if ((cycleCardsButton->ButtonPressed() || shopMenuButton->ButtonPressed()))
#else
            if ( (btn == JGE_BTN_OK) && (cycleCardsButton->ButtonPressed() || shopMenuButton->ButtonPressed()))
#endif
            {
                disablePurchase = true;
                return;
            }

            // Touch: any tap that reaches here is on the play area. If it lands on a shop
            // list row, select that item and begin its purchase; otherwise swallow it so
            // it can't reach the (hidden) card widgets underneath.
            {
                int cx = -1, cy = -1;
                if (mEngine->GetLeftClickCoordinates(cx, cy))
                {
                    int slot = shopSlotAtPoint((float)cx, (float)cy, shopListW());
                    mEngine->LeftClickedProcessed();
                    if (slot >= 0)
                    {
                        shopMenu->setSelected(slot);
                        // Sync the preview + purchase target to the tapped slot. bigSync
                        // isn't hooked upstream, so setting its offset is equivalent to
                        // the menu's (protected) syncMove().
                        bigSync.setOffset(slot);
                        ButtonPressed(-102, slot);
                    }
                    return;
                }
            }

#if defined (IOS) || defined (ANDROID)
            if (clearInput && (btn == JGE_BTN_OK))
            {
                clearInput = false;
                disablePurchase = false;
                return;
            }
            else
#endif
            // Swipe up/down (directional keys) moves the selection to browse the list and
            // updates the preview, without buying. Lists no longer wrap, so a swipe stops
            // at the ends instead of scrolling forever. Tapping a row still buys it.
            if (btn == JGE_BTN_UP || btn == JGE_BTN_DOWN || btn == JGE_BTN_LEFT || btn == JGE_BTN_RIGHT)
            {
                if (shopMenu->CheckUserInput(btn))
                    srcCards->Touch();
            }
        }
        if (shopMenu)
            shopMenu->Update(dt);

        break;
    case STAGE_FADE_IN:
        mParent->DoAnimation(TRANSITION_FADE_IN);
        mStage = STAGE_SHOP_SHOP;
        break;
    }
}

void GameStateShop::deleteDisplay()
{
    vector<MTGCardInstance*>::iterator i;
    for (i = subBooster.begin(); i != subBooster.end(); i++)
    {
        if (!*i)
            continue;
        delete *i;
    }
    subBooster.clear();
    SAFE_DELETE(boosterDisplay);
}

void GameStateShop::enableButtons()
{
    cycleCardsButton->setIsSelectionValid(true);
    shopMenuButton->setIsSelectionValid(true);
}

void GameStateShop::renderButtons()
{
    cycleCardsButton->Render();
    shopMenuButton->Render();
}

void GameStateShop::Render()
{
    //Erase
    WFont * mFont = WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT);
    JRenderer * r = JRenderer::GetInstance();
    r->ClearScreen(ARGB(0,0,0,0));
    if (mStage == STAGE_FADE_IN)
        return;

#if defined (PSP)
    JQuadPtr mBg = WResourceManager::Instance()->RetrieveTempQuad("pspshop.jpg", TEXTURE_SUB_5551);
    if (mBg.get())
        r->RenderQuad(mBg.get(), 0, 0, 0, SCREEN_WIDTH_F / mBg->mWidth, SCREEN_HEIGHT_F / mBg->mHeight);

    JQuadPtr quad = WResourceManager::Instance()->RetrieveTempQuad("pspshop_light.jpg", TEXTURE_SUB_5551);
#else
    JQuadPtr mBg = WResourceManager::Instance()->RetrieveTempQuad("shop.jpg", TEXTURE_SUB_5551);
    if (mBg.get())
        r->RenderQuad(mBg.get(), 0, 0, 0, SCREEN_WIDTH_F / mBg->mWidth, SCREEN_HEIGHT_F / mBg->mHeight);

    JQuadPtr quad = WResourceManager::Instance()->RetrieveTempQuad("shop_light.jpg", TEXTURE_SUB_5551);
#endif

    if (quad.get())
    {
        r->EnableTextureFilter(false);
        r->SetTexBlend(BLEND_SRC_ALPHA, BLEND_ONE);
        quad->SetColor(ARGB(lightAlpha,255,255,255));
        quad->SetHotSpot(0,quad->mHeight);
        r->RenderQuad(quad.get(), 0, SCREEN_HEIGHT, 0, 255.f / quad->mWidth, 272.f / quad->mHeight);
        r->SetTexBlend(BLEND_SRC_ALPHA, BLEND_ONE_MINUS_SRC_ALPHA);
        r->EnableTextureFilter(true);
    }

    // Flat vertical shop: a clean list of everything for sale (booster packs, then
    // cards), with the selected item previewed on the right. Replaces the old tabletop
    // (perspective-distorted) card layout.
    if (filterMenu && !filterMenu->isFinished())
    {
        filterMenu->Render();
    }
    else if (boosterDisplay)
    {
        boosterDisplay->Render(true); // viewing a just-opened booster pack
    }
    else
    {
        int sel = shopMenu ? shopMenu->getSelected() : 0;

        bool isBoosterSlot = (shopMenu && sel >= 0 && sel < BOOSTER_SLOTS);

        // Preview of the selected item on the right. Skipped for booster slots — those get
        // the dedicated booster image below, so bigDisplay must not also draw a card back
        // (that produced the "card back inside a card back" double image).
        if (bigDisplay && !isBoosterSlot)
        {
            if (bigDisplay->mOffset.getPos() >= 0)
                bigDisplay->setSource(srcCards);
            else
                bigDisplay->setSource(NULL);
            bigDisplay->setFoil(sel >= 0 && sel < SHOP_ITEMS ? mFoilSingle[sel] : false);
            bigDisplay->Render();
        }

        // Booster slots: show a unique, per-set booster pack image in the preview area.
        // Drop art named "booster_<SETID>.png" (or .jpg) into the theme (e.g.
        // booster_XLN.png). PNG is tried first so transparent pack art works. If the
        // set-specific image is missing it falls back to a generic "booster.png/.jpg", and
        // if that's absent too, to the generic card back ("back.png/.jpg"). The active
        // custom theme folder is searched before the base graphics/sets folders.
        if (isBoosterSlot)
        {
            string setId = mBooster[sel].getSetId();
            JQuadPtr bq;
            const char * candidates[8];
            int n = 0;
            char cv0[256], cv1[256], c0[256], c1[256];
            // Prefer the randomly-chosen numbered art variant (booster_<SET>_<N>) for this
            // slot; fall back to the old un-numbered booster_<SET>, then generic art.
            if (setId.size() && sel >= 0 && sel < BOOSTER_SLOTS && mBoosterArt[sel] >= 1)
            {
                sprintf(cv0, "booster_%s_%d.png", setId.c_str(), mBoosterArt[sel]); candidates[n++] = cv0;
                sprintf(cv1, "booster_%s_%d.jpg", setId.c_str(), mBoosterArt[sel]); candidates[n++] = cv1;
            }
            if (setId.size())
            {
                sprintf(c0, "booster_%s.png", setId.c_str()); candidates[n++] = c0;
                sprintf(c1, "booster_%s.jpg", setId.c_str()); candidates[n++] = c1;
            }
            candidates[n++] = "booster.png";
            candidates[n++] = "booster.jpg";
            candidates[n++] = "back.png";
            candidates[n++] = "back.jpg"; // generic card back default
            // Keep trying until we get a VALID (non-empty) quad, so a missing or broken
            // candidate doesn't stop the chain before the card-back fallback.
            for (int ci = 0; ci < n && !(bq.get() && bq->mHeight > 0); ++ci)
                bq = WResourceManager::Instance()->RetrieveTempQuad(candidates[ci]);
            if (bq.get() && bq->mHeight > 0)
            {
                // Bigger pack art (was 0.6), centred on the same point as the card preview
                // (0.72W, y=135) so cards and packs sit at a matched size and position.
                float targetH = SCREEN_HEIGHT_F * 0.75f;
                float scale = targetH / bq->mHeight;
                bq->SetHotSpot(bq->mWidth / 2.0f, bq->mHeight / 2.0f);
                r->RenderQuad(bq.get(), SCREEN_WIDTH_F * 0.72f, 135.0f, 0, scale, scale);
            }
        }

        float listW = shopListW();

        // Section headers.
        mFont->SetScale(0.9f);
        mFont->SetColor(ARGB(255, 235, 205, 120));
        mFont->DrawString(_("Booster Packs").c_str(), kShopListX + 4.0f, kShopListTop + 1.0f);
        mFont->DrawString(_("Cards").c_str(), kShopListX + 4.0f,
                          kShopListTop + kShopHeaderH + kShopRowH * BOOSTER_SLOTS + 1.0f);

        // Rows (booster packs + cards).
        for (int i = 0; i < SHOP_SLOTS; i++)
        {
            float y = shopRowY(i);
            bool seld = (i == sel);
            r->FillRect(kShopListX, y, listW, kShopRowH - 2.0f,
                        seld ? ARGB(215, 70, 55, 25) : ARGB(150, 18, 18, 18));
            if (seld)
                r->DrawRect(kShopListX, y, listW, kShopRowH - 2.0f, ARGB(230, 225, 190, 90));
            mFont->SetColor(seld ? ARGB(255, 255, 240, 150) : ARGB(255, 225, 225, 225));
            mFont->DrawString(descPurchase(i, false).c_str(), kShopListX + 6.0f, y + 3.0f);
        }
        mFont->SetScale(1.0f);
    }

    //Render the info bar
    r->FillRect(0, SCREEN_HEIGHT - 17, SCREEN_WIDTH, 17, ARGB(128,0,0,0));
    std::ostringstream stream;
    stream << kCreditsString << playerdata->credits;
    mFont->SetColor(ARGB(255,255,255,255));
    mFont->DrawString(stream.str(), 5, SCREEN_HEIGHT - 14);

#ifndef TOUCH_ENABLED
    float len = 4 + mFont->GetStringWidth(kOtherCardsString.c_str());
    r->RenderQuad(pspIcons[6].get(), SCREEN_WIDTH - len - 0.5 - 10, SCREEN_HEIGHT - 8, 0, kPspIconScaleFactor, kPspIconScaleFactor);
    mFont->DrawString(kOtherCardsString, SCREEN_WIDTH - len, SCREEN_HEIGHT - 14);
#else
    enableButtons();
#endif
    
    mFont->SetColor(ARGB(255,255,255,0));
    mFont->DrawString(descPurchase(bigSync.getPos()).c_str(), SCREEN_WIDTH / 2, SCREEN_HEIGHT - 14, JGETEXT_CENTER);
    mFont->SetColor(ARGB(255,255,255,255));

    if (mStage == STAGE_SHOP_TASKS && taskList)
    {
        taskList->Render();
        // On-screen Back button (same style as New Cards/Menu, to their left).
        if (taskBackButton)
            taskBackButton->Render();
    }
    if (menu)
        menu->Render();
    
    if ((!filterMenu || (filterMenu && filterMenu->isFinished()))&&!boosterDisplay)
        renderButtons();
}

void GameStateShop::ButtonPressed(int controllerId, int controlId)
{
    int sel = bigSync.getOffset();
    switch (controllerId)
    {
    case -102: //Buying something...
        mStage = STAGE_SHOP_PURCHASE;
        if (menu)
            menu->Close();
        mBuying = controlId;
        return;
    case -145:
        if (controlId == -1)
        { //Nope, don't buy.
            if (sel < BOOSTER_SLOTS)
                cancelBooster(sel);
            else
                cancelCard(sel);
            menu->Close();
            mStage = STAGE_SHOP_SHOP;
            return;
        }
        if (sel > -1 && sel < SHOP_ITEMS)
        {
            if (controlId == -2)
            {
                playerdata->credits += mPrices[sel]; //We stole it.
                GameApp::mycredits = playerdata->credits;
            }
            if (sel < BOOSTER_SLOTS) //Clicked a booster.
                purchaseBooster(sel);
            else
                purchaseCard(sel);

            //Check if we just scored an award...
            if (myCollection && myCollection->totalPrice() > 10000)
            {
                GameOptionAward * goa = dynamic_cast<GameOptionAward *> (&options[Options::AWARD_COLLECTOR]);
                if (goa) goa->giveAward();
            }
        }
        mStage = STAGE_SHOP_SHOP;
        return;
    case -146: // "New Cards" refresh confirmation
        if (menu)
            menu->Close();
        if (controlId == 1 && playerdata)
        {
            int refreshCost = computeRefreshCost();
            if (playerdata->credits >= refreshCost)
            {
                playerdata->credits -= refreshCost;
                GameApp::mycredits = playerdata->credits;
                mRefreshCount++;
                mLastRefreshDay = TaskList::sDaysElapsed; // start this refresh's cooldown today
                srcCards->Shuffle();
                load();
                disablePurchase = false;
                mTouched = true;
                save(true);
            }
        }
        return;
    }
    //Basic Menu.
    switch (controlId)
    {
    case 12:
        if (taskList)
            taskList->save();
        mStage = STAGE_SHOP_SHOP;
        mParent->DoTransition(TRANSITION_FADE, GAME_STATE_MENU);
        save();
        GameStateMenu::genNbCardsStr();
        break;
    case 14:
        mStage = STAGE_SHOP_TASKS;
        if (!taskList)
            taskList = NEW TaskList();
        taskList->Start();
        break;
    case 15:
        if (taskList)
            taskList->End();
        break;
    case 22:
        beginFilters();
        break;
    case -2:
        {
        playerdata->credits += 2000;
        GameApp::mycredits = playerdata->credits;
        }
    default:
        mStage = STAGE_SHOP_SHOP;
    }
    menu->Close();
}

void GameStateShop::OnScroll(int inXVelocity, int)
{
    // we ignore magnitude since there isn't any scrolling in the shop
    if (abs(inXVelocity) > 200)
    {
        bool flickRight = (inXVelocity >= 0);
        if (flickRight)
            mEngine->HoldKey_NoRepeat(JGE_BTN_PRI);
    }
}

//ShopBooster
ShopBooster::ShopBooster()
{
    pack = NULL;
    mainSet = NULL;
    altSet = NULL;
}
string ShopBooster::getSort()
{
    if (pack)
        return pack->getSort();
    return "";
}
;
string ShopBooster::getSetId() const
{
    if (mainSet)
        return mainSet->id;
    return "";
}

int ShopBooster::getSetYear() const
{
    return mainSet ? mainSet->year : 0;
}

string ShopBooster::getName()
{
    char buffer[512];
    if (!mainSet && pack)
        return pack->getName();
    if (altSet == mainSet)
        altSet = NULL;
    if (altSet)
        sprintf(buffer, _("%s & %s (15 Cards)").c_str(), mainSet->id.c_str(), altSet->id.c_str());
    else if (mainSet)
        sprintf(buffer, _("%s Booster (15 Cards)").c_str(), mainSet->id.c_str());
    return buffer;
}

void ShopBooster::randomize(MTGPacks * packlist)
{
    mainSet = NULL;
    altSet = NULL;
    pack = NULL;
    if (!setlist.size())
        return;
    if (packlist && setlist.size() > 10)
    { //FIXME make these an unlockable item.
        int rnd = rand() % 100;
        if (rnd <= Constants::CHANCE_CUSTOM_PACK)
        {
            randomCustom(packlist);
            return;
        }
    }
    randomStandard();
}
int ShopBooster::basePrice()
{
    if (pack)
        return pack->getPrice();
    else if (altSet)
        return Constants::PRICE_MIXED_BOOSTER;
    return Constants::PRICE_BOOSTER;
}
void ShopBooster::randomCustom(MTGPacks * packlist)
{
    pack = packlist->randomPack();
    if (pack && !pack->isUnlocked())
        pack = NULL;
    if (!pack)
        randomStandard();
}
void ShopBooster::randomStandard()
{
    // A shop booster slot always offers a SINGLE set (per user request) — never a mixed
    // "X & Y" booster. Pick one random set and use its pack (or the default pack scoped to
    // that set if it has none).
    mainSet = setlist.randomSet(-1);
    altSet = NULL;
    pack = mainSet ? mainSet->mPack : NULL;
}
int ShopBooster::maxInventory()
{
    if (altSet || pack)
        return 2;
    return 5;
}
void ShopBooster::addToDeck(MTGDeck * d, WSrcCards *)
{
    if (!pack)
    { //A combination booster.
        MTGPack * mP = MTGPacks::getDefault();
        if (!altSet && mainSet->mPack)
            mP = mainSet->mPack;
        char buf[512];
        if (!altSet)
            sprintf(buf, "set:%s;", mainSet->id.c_str());
        else
            sprintf(buf, "set:%s;|set:%s;", mainSet->id.c_str(), altSet->id.c_str());
        mP->pool = buf;
        mP->assemblePack(d); //Use the primary packfile. assemblePack deletes pool.
    }
    else
        pack->assemblePack(d);
}

#ifdef TESTSUITE
bool ShopBooster::unitTest()
{
    //this tests the default random pack creation.
    MTGDeck * d = NEW MTGDeck(MTGCollection());
    char result[1024];

    randomStandard();
    MTGPack * mP = MTGPacks::getDefault();
    if(!altSet && mainSet->mPack) mP = mainSet->mPack;
    char buf[512];
    if(!altSet) sprintf(buf,"set:%s;",mainSet->id.c_str());
    else sprintf(buf,"set:%s;|set:%s;",mainSet->id.c_str(),altSet->id.c_str());
    mP->pool = buf;
    mP->assemblePack(d); //Use the primary packfile. assemblePack deletes pool.
    DeckDataWrapper* ddw = NEW DeckDataWrapper(d);
    bool res = true;

    int u = 0, r = 0, s = 0;
    int card = 0;
    for(int i=0;i<ddw->Size(true);i++)
    {
        MTGCard * c = ddw->getCard(i);
        if(!c) break;
        if(c->getRarity() == Constants::RARITY_R || c->getRarity() == Constants::RARITY_M)
        r+=ddw->count(c);
        else if(c->getRarity() == Constants::RARITY_U)
        u+=ddw->count(c);
        else if(c->getRarity() == Constants::RARITY_S)
        s+=ddw->count(c);
        card++;
    }
    int count = ddw->getCount();
    SAFE_DELETE(ddw);
    SAFE_DELETE(d);
    //Note: When there are special cards, the count IS going to be messed up, so do not perform the check for Rare and Uncommon in that case
    //also see http://code.google.com/p/wagic/issues/detail?id=644
    if(!s && (r != 1 || u != 3) ) 
    {
        sprintf(result, "<span class=\"error\">==Unexpected rarity count==</span><br />");
        TestSuite::Log(result);
        res = false;
    }
    if(count < 14)
    {
        sprintf(result, "<span class=\"error\">==Unexpected card count==</span><br />");
        TestSuite::Log(result);
        res = false;
    }
    sprintf(result, "<span class=\"success\">==Test Successful !==</span><br />");
    TestSuite::Log(result);
    SAFE_DELETE(ddw);
    SAFE_DELETE(d);
    return res;
}
#endif
