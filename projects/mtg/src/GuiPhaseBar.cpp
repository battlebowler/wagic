#include "PrecompiledHeader.h"

#include "GameApp.h"
#include "GuiPhaseBar.h"
#include "GameObserver.h"
#include "Translate.h"
#include "CardSelector.h"
/*
 static int colors[] =
 {
     ARGB(255, 255, 255, 255),
     ARGB(255, 255, 000, 000),
     ARGB(255, 000, 255, 000),
     ARGB(255, 000, 000, 255),
     ARGB(255, 255, 255, 000),
     ARGB(255, 255, 000, 255),
     ARGB(255, 000, 255, 255),
     ARGB(255, 000, 000, 000),
     ARGB(255, 255, 255, 255),
     ARGB(255, 255, 255, 255),
     ARGB(255, 255, 255, 255),
     ARGB(255, 255, 255, 255)
 };
 */

const float GuiPhaseBar::zoom_big = (float)(1.5 * 1.25);
const float GuiPhaseBar::zoom_small = 1.5;
const float GuiPhaseBar::step = M_PI/6.0f;

namespace
{
    //width and height of the phase symbol textures
    const float kWidth = 28;
    const float kHeight = kWidth;
    const unsigned kPhases = NB_MTG_PHASES - 2; //there are two phases we do not show
}

void GuiPhaseBar::DrawGlyph(JQuad *inQuad, int phaseId, float x, float y, float scale, float z)
{
    inQuad->SetTextureRect(phaseId * (kWidth + 1), z, kWidth, kHeight);
    JRenderer::GetInstance()->RenderQuad(inQuad, x, y - scale * kWidth/2, 0.0f, scale, scale);
}

GuiPhaseBar::GuiPhaseBar(DuelLayers* duelLayers) :
    GuiLayer(duelLayers->getObserver()), PlayGuiObject(80, 0, 106, 0, false),
    displayedPhaseId(0), angle(0.0f), zoomFactor(zoom_small), angleEasing(angle),
    zoomFactorEasing(zoomFactor), mpDuelLayers(duelLayers)
{
    if(duelLayers->getObserver()->getResourceManager())
    {
        JQuadPtr quad = WResourceManager::Instance()->GetQuad("phasebar");
        if (quad.get() != NULL)
        {
            quad->mHeight = kHeight;
            quad->mWidth = kWidth;
        }
        else
            GameApp::systemError = "Error loading phasebar texture : " __FILE__;
    }

    mpDuelLayers->getCardSelector()->Add(this);
}

GuiPhaseBar::~GuiPhaseBar()
{
}

void GuiPhaseBar::Update(float dt)
{
    angleEasing.update(dt);

    if(angle <= -step)
    {
        displayedPhaseId = (displayedPhaseId + 1) % kPhases;
        angleEasing.translate(step);
    }

    zoomFactorEasing.update(dt);
}

void GuiPhaseBar::Entering()
{
    mHasFocus = true;
    zoomFactorEasing.start(zoom_big, 0.3f);
}

bool GuiPhaseBar::Leaving(JButton)
{
    mHasFocus = false;
    zoomFactorEasing.start(zoom_small, 0.6f);
    return true;
}

void GuiPhaseBar::Render()
{
    JQuadPtr quad = WResourceManager::Instance()->GetQuad("phasebar");
#if defined (PSP)
    JQuadPtr phaseinfo = WResourceManager::Instance()->RetrieveTempQuad("pspfakebar.png"); //new fakebar graphics
#else
    JQuadPtr phaseinfo = WResourceManager::Instance()->RetrieveTempQuad("fakebar.png"); //new fakebar graphics
#endif
    //uncomment to draw a hideous line across hires screens.
    // JRenderer::GetInstance()->DrawLine(0, CENTER, SCREEN_WIDTH, CENTER, ARGB(255, 255, 255, 255));

    // Phase "wheel" (the arc of phase glyphs on the left edge) is hidden — the phase
    // name text below is enough, and tapping the top-right phase text advances the phase.
    (void) quad;

    //print phase name
    WFont * font = WResourceManager::Instance()->GetWFont(Fonts::MAIN_FONT);
    string currentP = _("your turn");
    string interrupt = "";
    if (observer->currentPlayer == mpDuelLayers->getRenderedPlayerOpponent())
    {
        currentP = _("opponent's turn");
    }
    font->SetColor(ARGB(255, 255, 255, 255));
    if (observer->currentlyActing() && observer->currentlyActing()->isAI())
    {
        font->SetColor(ARGB(255, 128, 128, 128));
    }
    if (observer->currentlyActing() != observer->currentPlayer)
    {
        if (observer->currentPlayer == mpDuelLayers->getRenderedPlayer())
        {
            interrupt = _(" - ") + _("opponent plays");
        }
        else
        {
            interrupt = _(" - ") + _("you play");
        }
    }

    char buf[200];
    //running this string through translate returns gibberish even though we defined the variables in the lang.txt
    //the conversion from phase bar phases to mtg phases is x%kPhases + 1
    //todo: just to this when the displayedPhaseId updates
    //string phaseNameToTranslate = observer->phaseRing->phaseName(displayedPhaseId%kPhases + 1);
    string phaseNameToTranslate = observer->phaseRing->phaseName(observer->phaseRing->getCurrentPhase()->id);
    phaseNameToTranslate = _(phaseNameToTranslate);
    sprintf(buf, _("(%s%s) %s").c_str(), currentP.c_str(), interrupt.c_str(),phaseNameToTranslate.c_str());
#if !defined (PSP)
    // Phase banner: a drawn rounded rect sized to the text (top-right), replacing the fakebar.png
    // texture. It auto-fits the phase string as it changes, and extends past the right edge so only
    // the LEFT corners read as rounded (matching the old pill that was flush to the screen edge).
    (void) phaseinfo;
    {
        float tw = font->GetStringWidth(buf);
        float fh = font->GetHeight();
        const float rad = 5.0f;
        const float ry = 2.0f;                            // nudge down so the top stays on-screen
        float bx = SCREEN_WIDTH_F - 15.0f - tw - 9.0f;   // left edge, padded off the text
        float visW = (SCREEN_WIDTH_F + 8.0f) - bx;        // run past the right edge
        float visH = fh + 2.0f;                           // thin: just wraps the text
        JRenderer * rr = JRenderer::GetInstance();
        rr->FillRoundRect(bx, ry, visW - 2.0f * rad, visH - 2.0f * rad, rad, ARGB(235, 16, 16, 20));
        rr->DrawRoundRect(bx, ry, visW - 2.0f * rad, visH - 2.0f * rad, rad, ARGB(255, 176, 148, 84));
    }
#endif
    font->DrawString(buf, SCREEN_WIDTH - 15, 2, JGETEXT_RIGHT);
}

int GuiPhaseBar::receiveEventMinus(WEvent *e)
{
    WEventPhaseChange *event = dynamic_cast<WEventPhaseChange*> (e);
    if (event)
    {
        //convert the mtg phase to the phases of the phase wheel
        //the mapping is
        //0  -> none
        //1..12 -> 0..11
        //13 -> none
        int targetPhase = event->to->id;

        if(targetPhase != 0 && targetPhase != 13)
        {
            targetPhase -= 1;

            int phasesToAnimate = (targetPhase - displayedPhaseId + kPhases) % kPhases;
            angleEasing.start(float(phasesToAnimate * (- step)), 0.3f * float(sqrt(float(phasesToAnimate))));
        }
    }
    return 1;
}

ostream& GuiPhaseBar::toString(ostream& out) const
{
    return out << "GuiPhaseBar";
}
