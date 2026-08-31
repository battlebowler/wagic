/*
  Shared UI design tokens + primitives for the touch UI.

  Screens pull colours, shape, and panel drawing from here so the whole app reads as one system:
  dark translucent "glass" panels over the photographic backgrounds, a warm red accent (matching
  the pill buttons), crisp DRAWN shapes (no bitmap frame textures), and finger-friendly targets.

  Colours are ARGB. Sizes are in the engine's virtual-screen units (the same units the menus
  already position with), so they scale with the display.
*/
#ifndef _UITHEME_H_
#define _UITHEME_H_

#include <JTypes.h>
#include <JRenderer.h>

namespace UITheme
{
    // --- Palette --------------------------------------------------------------------------------
    const PIXEL_TYPE kPanelFill   = ARGB(224,  16,  18,  24);  // modal / panel backing (dark glass)
    const PIXEL_TYPE kPanelBorder = ARGB(205, 150, 160, 182);  // crisp light edge
    const PIXEL_TYPE kTitleFill   = ARGB(255,  30,  34,  44);  // title bar, a touch lighter
    const PIXEL_TYPE kSelection   = ARGB(210,  52,  70, 104);  // selected / focused row
    const PIXEL_TYPE kAccent      = ARGB(255, 150,  28,  28);  // warm red (matches the pill buttons)
    const PIXEL_TYPE kTextPrimary = ARGB(255, 240, 240, 245);
    const PIXEL_TYPE kTextMuted   = ARGB(255, 150, 152, 162);

    // --- Shape ----------------------------------------------------------------------------------
    const float kRadius       = 5.0f;   // panel / button corner radius
    const float kTouchRowMin  = 22.0f;  // minimum comfortable row height for a finger target

    // --- Layout ---------------------------------------------------------------------------------
    // Bottom-of-screen button row. This is the Y of the TOP of the pill (both the deck-setup rects
    // and InteractiveButton::Render draw the pill with its top at getY()), as a fraction of screen
    // height. Shared so every screen's bottom buttons (Back / Select Deck / Menu / New Cards / ...)
    // sit on one line, clear of the panel above and the screen edge below.
    //   Use as:  y = SCREEN_HEIGHT_F * UITheme::kBottomButtonRowYFrac;
    const float kBottomButtonRowYFrac = 246.0f / 272.0f;

    // Draw a filled, subtly bordered panel whose VISUAL bounds are exactly [x, x+w] x [y, y+h].
    // (FillRoundRect / DrawRoundRect expand by the radius on each side, so inset w/h by 2*radius to
    // land on the requested bounds.)
    inline void drawPanel(JRenderer * r, float x, float y, float w, float h,
                          PIXEL_TYPE fill = kPanelFill, PIXEL_TYPE border = kPanelBorder)
    {
        float rad = kRadius;
        if (w < 2.0f * rad || h < 2.0f * rad) rad = 0.0f;
        r->FillRoundRect(x, y, w - 2.0f * rad, h - 2.0f * rad, rad, fill);
        r->DrawRoundRect(x, y, w - 2.0f * rad, h - 2.0f * rad, rad, border);
    }
}

#endif
