#include "PrecompiledHeader.h"

#include "SimplePad.h"
#include "JTypes.h"
#include "GameApp.h"
#include "Translate.h"

#define ALPHA_COLUMNS 8
#define ALPHA_ROWS 8

#define KPD_UP 0
#define KPD_DOWN 1
#define KPD_LEFT 2
#define KPD_RIGHT 3

// ---------------------------------------------------------------------------
// Layout constants — all as fractions of screen dimensions.
// Nothing here is in pixels. Change these to tune the keyboard appearance.
// ---------------------------------------------------------------------------

// The keyboard occupies this fraction of the screen
static const float KBD_W_FRAC  = 0.80f;   // 80% of screen width
static const float KBD_H_FRAC  = 0.62f;   // 62% of screen height

// Keyboard top-left corner as fraction of screen
static const float KBD_X_FRAC  = 0.10f;   // 10% from left  → centers 80% wide kbd
static const float KBD_Y_FRAC  = 0.20f;   // 6%  from top

// Internal layout fractions (of keyboard box dimensions)
// Rows: title(1) + bubble(1) + 5 key rows = 7 logical rows
// We give key rows more weight than title/bubble
static const float TITLE_H_FRAC  = 0.14f;  // title row
static const float BUBBLE_H_FRAC = 0.16f;  // input bubble row
static const float KEYS_H_FRAC   = 0.70f;  // remaining height for 5 key rows

// Column split: left letter block vs right special-key column
static const float LEFT_W_FRAC  = 0.72f;   // 72% of kbd width → 9 letter cols
static const float COL_GAP_FRAC = 0.04f;   // gap between blocks
static const float RIGHT_W_FRAC = 0.24f;   // right column (Del/Caps/Confirm/Cancel)

// Gap between keys as fraction of a single cell's dimension
static const float H_GAP_FRAC = 0.12f;     // fraction of cellW
static const float V_GAP_FRAC = 0.15f;     // fraction of cellH

// ---------------------------------------------------------------------------

SimpleKey::SimpleKey(string _ds, int _id)
{
    displayValue = _ds;
    id = _id;
    for (int x = 0; x < 4; x++)
        adjacency[x] = KPD_NOWHERE;
}

void SimplePad::linkKeys(int from, int to, int dir)
{
    if (keys[from] && keys[to])
    {
        keys[from]->adjacency[dir] = to;
        switch (dir)
        {
            case KPD_UP:
            case KPD_LEFT:
                dir++;
                break;
            default:
                dir--;
        }
        keys[to]->adjacency[dir] = from;
    }
}

SimplePad::SimplePad()
{
    nbitems = 0;
    bActive = false;
#ifdef IOS
    selected = KPD_OK;
#else
    selected = 0;
#endif
    priorKey = 0;
    cursor = 0;
    bShowCancel = false;
    bShowNumpad = false;
    bCapslock = true;
    char buf[2];
    buf[1] = '\0';
    SimpleKey * k;

    for (int x = 0; x < KPD_MAX; x++)
        keys[x] = NULL;

    for (int x = 'a'; x <= 'z'; x++)
    {
        buf[0] = x;
        k = Add(buf, x);
        int idx = x - 'a';

        if (idx > KPD_A) k->adjacency[KPD_LEFT] = idx - 1;
        if (idx < KPD_Z) k->adjacency[KPD_RIGHT] = idx + 1;
        if (idx > ALPHA_COLUMNS)
            k->adjacency[KPD_UP] = idx - 1 - ALPHA_COLUMNS;
        else
            k->adjacency[KPD_UP] = KPD_INPUT;
        if (idx < 25 - ALPHA_COLUMNS) k->adjacency[KPD_DOWN] = idx + 1 + ALPHA_COLUMNS;
    }

    Add(_("Spacebar"), KPD_SPACE);
    for (int x = 25 - ALPHA_COLUMNS; x < 26; x++)
        keys[x]->adjacency[KPD_DOWN] = KPD_SPACE;

    k = Add(_("Confirm"), KPD_OK);
    keys[KPD_Z]->adjacency[KPD_RIGHT] = KPD_OK;
    k->adjacency[KPD_UP] = KPD_CAPS;
    k->adjacency[KPD_LEFT] = KPD_Z;
    k->adjacency[KPD_DOWN] = KPD_CANCEL;

    k = Add(_("Cancel"), KPD_CANCEL);
    k->adjacency[KPD_UP] = KPD_OK;
    k->adjacency[KPD_LEFT] = KPD_SPACE;

    k = Add(_("Del"), KPD_DEL);
    keys[KPD_I]->adjacency[KPD_RIGHT] = KPD_DEL;
    k->adjacency[KPD_UP] = KPD_9;
    k->adjacency[KPD_DOWN] = KPD_CAPS;
    k->adjacency[KPD_LEFT] = KPD_I;

    k = Add(_("Caps"), KPD_CAPS);
    keys[KPD_R]->adjacency[KPD_RIGHT] = KPD_CAPS;
    keys[KPD_R]->adjacency[KPD_DOWN] = KPD_Z;
    k->adjacency[KPD_UP] = KPD_DEL;
    k->adjacency[KPD_DOWN] = KPD_OK;
    k->adjacency[KPD_LEFT] = KPD_R;

    for (int x = 0; x < 10; x++)
    {
        buf[0] = '0' + x;
        Add(buf, KPD_0 + x);
        if (x < 8) linkKeys(KPD_0 + x, KPD_A + x, KPD_DOWN);
        if (x > 0) linkKeys(KPD_0 + x, KPD_0 + x - 1, KPD_LEFT);
    }
    k = Add(_("."), KPD_DOT);
    linkKeys(KPD_DOT, KPD_9, KPD_LEFT);
    keys[KPD_DOT]->adjacency[KPD_DOWN] = KPD_DEL;

    keys[KPD_8]->adjacency[KPD_DOWN] = KPD_DEL;
    keys[KPD_9]->adjacency[KPD_DOWN] = KPD_DEL;

    keys[KPD_0]->adjacency[KPD_LEFT] = KPD_NOWHERE;
    keys[KPD_A]->adjacency[KPD_LEFT] = KPD_NOWHERE;
    keys[KPD_J]->adjacency[KPD_LEFT] = KPD_NOWHERE;
    keys[KPD_S]->adjacency[KPD_LEFT] = KPD_NOWHERE;
}

SimplePad::~SimplePad()
{
    for (int x = 0; x < KPD_MAX; x++)
        SAFE_DELETE(keys[x]);
}

SimpleKey * SimplePad::Add(string display, unsigned char id)
{
    if (nbitems >= KPD_MAX) return NULL;
    keys[nbitems++] = NEW SimpleKey(display, id);
    return keys[nbitems - 1];
}

void SimplePad::pressKey(unsigned char key)
{
    string input = "";
#ifdef IOS
    if (isalnum(key))
#else
    if (isalpha(key))
#endif
    {
        if (bCapslock) input += toupper(key);
        else           input += key;
        if (cursor < buffer.size()) cursor++;
        buffer.insert(cursor, input);
        if (bCapslock && buffer.size() == 1) bCapslock = !bCapslock;
    }
    else if (key >= KPD_0 && key <= KPD_9)
    {
        input += ('0' + key - KPD_0);
        if (cursor < buffer.size()) cursor++;
        buffer.insert(cursor, input);
    }
    else if (key == KPD_DOT)
    {
        if (cursor < buffer.size()) cursor++;
        buffer.insert(cursor, ".");
    }
    else if (key == KPD_SPACE)
    {
        if (cursor < buffer.size()) cursor++;
        buffer.insert(cursor, " ");
    }
    else if (key == KPD_CAPS)
        bCapslock = !bCapslock;
    else if (key == KPD_DEL)
    {
        if (!buffer.size()) return;
        if (cursor >= buffer.size())
        {
            cursor = buffer.size();
            buffer = buffer.substr(0, cursor - 1);
        }
        else
            buffer = buffer.substr(0, cursor) + buffer.substr(cursor + 1);
        cursor--;
    }
    else if (key == KPD_OK)     Finish();
    else if (key == KPD_CANCEL) CancelEdit();
}

void SimplePad::CancelEdit()
{
#ifdef IOS
    selected = KPD_CANCEL;
#endif
    bCanceled = true;
    Finish();
}

void SimplePad::MoveSelection(unsigned char moveto)
{
    if (!bShowNumpad && moveto >= KPD_0 && moveto <= KPD_9)
        moveto = KPD_INPUT;
    else if (!bShowCancel && moveto == KPD_CANCEL)
        moveto = KPD_SPACE;

    if (selected < KPD_MAX && selected >= 0) priorKey = selected;

    if (moveto < KPD_MAX)        selected = moveto;
    else if (moveto == KPD_INPUT) selected = KPD_INPUT;
}

void SimplePad::Update(float)
{
    JGE * mEngine = JGE::GetInstance();

    int x, y, n = selected;
    unsigned int minDistance = -1;
    if (mEngine->GetLeftClickCoordinates(x, y))
    {
        for (int i = 0; i < nbitems; i++)
        {
            unsigned int d = static_cast<unsigned int>(
                    (keys[i]->mY - (float)y) * (keys[i]->mY - (float)y) +
                    (keys[i]->mX - (float)x) * (keys[i]->mX - (float)x));
            if (d < minDistance) { minDistance = d; n = i; }
        }
        MoveSelection(n);
        JGE::GetInstance()->LeftClickedProcessed();
    }

    JButton key = mEngine->ReadButton();

    if (key == JGE_BTN_MENU)
    {
        if (selected != KPD_OK) selected = KPD_OK;
        else Finish();
    }
    else if (key == JGE_BTN_CTRL) bCapslock = !bCapslock;

    if (selected == KPD_SPACE)
    {
        if (bShowCancel && mEngine->GetButtonClick(JGE_BTN_RIGHT))
            selected = KPD_CANCEL;
        else if (key == JGE_BTN_LEFT || key == JGE_BTN_RIGHT ||
                 key == JGE_BTN_UP   || key == JGE_BTN_DOWN)
            selected = priorKey;
    }
    else if (selected == KPD_INPUT)
    {
        if (key == JGE_BTN_DOWN) selected = priorKey;
        if      (key == JGE_BTN_LEFT  && cursor > 0)             cursor--;
        else if (key == JGE_BTN_RIGHT && cursor < buffer.size())  cursor++;
    }
    else if (selected >= 0 && keys[selected])
    {
        if      (key == JGE_BTN_LEFT)  MoveSelection(keys[selected]->adjacency[KPD_LEFT]);
        else if (key == JGE_BTN_RIGHT) MoveSelection(keys[selected]->adjacency[KPD_RIGHT]);
        if      (key == JGE_BTN_DOWN)  MoveSelection(keys[selected]->adjacency[KPD_DOWN]);
        else if (key == JGE_BTN_UP)    MoveSelection(keys[selected]->adjacency[KPD_UP]);
    }

    if (selected >= 0 && selected < nbitems && keys[selected])
        if (key == JGE_BTN_OK) pressKey(keys[selected]->id);
    if (buffer.size() > 0 && key == JGE_BTN_SEC)
        buffer = buffer.substr(0, buffer.size() - 1);
    if (buffer.size() && key == JGE_BTN_PREV)
    {
        if (cursor > 0) cursor--;
    }
    else if (key == JGE_BTN_NEXT)
    {
        if (cursor < buffer.size()) cursor++;
        else { buffer += ' '; cursor = buffer.size(); }
    }

    mEngine->ResetInput();
}

void SimplePad::Start(string value, string * _dest)
{
    bActive   = true;
    bCanceled = false;
    buffer    = value;
    original  = buffer;
    dest      = _dest;
    cursor    = buffer.size();
    JGE::GetInstance()->ResetInput();
}

string SimplePad::Finish()
{
    bActive = false;
    JGE::GetInstance()->ResetInput();

    if (bCanceled)
    {
        dest = NULL;
        return original;
    }

    string whitespaces(" \t\f\v\n\r");
    size_t found = buffer.find_last_not_of(whitespaces);
    if (found != string::npos) buffer.erase(found + 1);
    else                       buffer = "";

    if (dest != NULL)
    {
        dest->clear();
        dest->insert(0, buffer);
        dest = NULL;
    }
    return buffer;
}

void SimplePad::Render()
{
    JRenderer * renderer = JRenderer::GetInstance();
    WFont     * mFont    = WResourceManager::Instance()->GetWFont(Fonts::MENU_FONT);

    // -----------------------------------------------------------------------
    // Step 1: Derive all dimensions from screen size using only the fractions
    // defined at the top of this file. Zero magic numbers below this block.
    // -----------------------------------------------------------------------
    const float SW = SCREEN_WIDTH_F;
    const float SH = SCREEN_HEIGHT_F;

    // Overall keyboard box
    const float kbdW = SW * KBD_W_FRAC;
    const float kbdH = SH * KBD_H_FRAC;
    const float kbdX = SW * KBD_X_FRAC;
    const float kbdY = SH * KBD_Y_FRAC;

    // Vertical split of keyboard box
    const float titleH  = kbdH * TITLE_H_FRAC;
    const float bubbleH = kbdH * BUBBLE_H_FRAC;
    const float keysH   = kbdH * KEYS_H_FRAC;

    // Horizontal split
    const float leftW   = kbdW * LEFT_W_FRAC;
    const float colGap  = kbdW * COL_GAP_FRAC;
    const float rightW  = kbdW * RIGHT_W_FRAC;

    // Individual letter cell size
    // 9 letter columns with H_GAP_FRAC gaps between them
    // leftW = 9*cellW + 8*hGap  where hGap = cellW*H_GAP_FRAC
    // leftW = cellW * (9 + 8*H_GAP_FRAC)
    const float cellW = leftW / (9.0f + 8.0f * H_GAP_FRAC);
    const float hGap  = cellW * H_GAP_FRAC;

    // 5 key rows with V_GAP_FRAC gaps between them
    // keysH = 5*cellH + 4*vGap  where vGap = cellH*V_GAP_FRAC
    // keysH = cellH * (5 + 4*V_GAP_FRAC)
    const float cellH = keysH / (5.0f + 4.0f * V_GAP_FRAC);
    const float vGap  = cellH * V_GAP_FRAC;

    // Scale font to fit within cell height
    // GetHeight() returns height at current scale, so compute needed scale
    float baseH = mFont->GetHeight() / mFont->GetScale();  // height at scale=1
    float fontScale = (cellH * 0.55f) / baseH;             // 55% of cell for text
    mFont->SetScale(fontScale);

    // -----------------------------------------------------------------------
    // Step 2: Anchor points
    // -----------------------------------------------------------------------
    const float titleX  = kbdX;
    const float titleY  = kbdY;
    const float bubbleX = kbdX;
    const float bubbleY = kbdY + titleH;
    const float keysX   = kbdX;                // left edge of letter block
    const float keysY = kbdY + kbdH * 0.38f;  // keys start 38% down the keyboard box
    const float rightX  = kbdX + leftW + colGap; // left edge of right column

    // Store mX/mY for touch detection baseline
    mX = keysX;
    mY = keysY;

    // -----------------------------------------------------------------------
    // Step 3: Draw background
    // -----------------------------------------------------------------------
    renderer->FillRoundRect(kbdX, kbdY, kbdW, kbdH,
                            cellW * 0.15f, ARGB(220, 0, 0, 0));

    // -----------------------------------------------------------------------
    // Step 4: Title
    // -----------------------------------------------------------------------
    if (title != "")
    {
        mFont->SetColor(ARGB(255, 255, 255, 255));
        float tW = mFont->GetStringWidth(_(title.c_str()).c_str());
        mFont->DrawString(_(title.c_str()), titleX + (kbdW - tW) / 2.0f,
                          titleY + (titleH - mFont->GetHeight()) / 2.0f);
    }

    // -----------------------------------------------------------------------
    // Step 5: Input bubble
    // -----------------------------------------------------------------------
    renderer->FillRoundRect(bubbleX + cellW * 0.1f, bubbleY + cellH * 0.1f,
                            kbdW - cellW * 0.2f, bubbleH - cellH * 0.2f,
                            cellW * 0.1f, ARGB(255, 255, 255, 255));

    // Cursor underline
    float fH = mFont->GetHeight();
    string bufSub = buffer.substr(0, cursor);
    float cursorX = bubbleX + cellW * 0.2f + mFont->GetStringWidth(bufSub.c_str());
    float cursorW = (cursor < buffer.size())
                    ? mFont->GetStringWidth(buffer.substr(cursor, 1).c_str())
                    : mFont->GetStringWidth("W");
    renderer->FillRect(cursorX, bubbleY + bubbleH - cellH * 0.15f,
                       cursorW, cellH * 0.08f, ARGB(180, 80, 80, 0));

    // Buffer text
    mFont->SetColor(ARGB(255, 0, 0, 0));
    mFont->DrawString(buffer.c_str(),
                      bubbleX + cellW * 0.2f,
                      bubbleY + (bubbleH - fH) / 2.0f);

    // -----------------------------------------------------------------------
    // Step 6: Draw all keys
    // -----------------------------------------------------------------------
#ifndef IOS
    for (int x = 0; x < nbitems; x++)
    {
        if (!keys[x]) continue;
        if (x == KPD_CANCEL && !bShowCancel)  continue;
        if (x >= KPD_0 && x <= KPD_DOT && !bShowNumpad) continue;

        // Compute this key's top-left pixel position and size
        float kx = 0, ky = 0, kw = cellW, kh = cellH;
        bool  isRightCol = false;

        if (x >= KPD_A && x <= KPD_Z)
        {
            int idx = x - KPD_A;
            int row = idx / 9;       // rows: 0=A-I, 1=J-R, 2=S-Z(partial)
            int col = idx % 9;

            // Row 2 only has 8 letters (S..Z = 8 keys, indices 18-25)
            kx = keysX + col * (cellW + hGap);
            ky = keysY + row * (cellH + vGap);
        }
        else if (x == KPD_SPACE)
        {
            // Spacebar spans full left block width, sits in row 3
            kx = keysX;
            ky = keysY + 3 * (cellH + vGap);
            kw = leftW;
        }
        else if (x == KPD_DEL)
        {
            kx = rightX; ky = keysY + 0 * (cellH + vGap);
            kw = rightW; isRightCol = true;
        }
        else if (x == KPD_CAPS)
        {
            kx = rightX; ky = keysY + 1 * (cellH + vGap);
            kw = rightW; isRightCol = true;
        }
        else if (x == KPD_OK)
        {
            kx = rightX; ky = keysY + 2 * (cellH + vGap);
            kw = rightW; isRightCol = true;
        }
        else if (x == KPD_CANCEL)
        {
            kx = rightX; ky = keysY + 3 * (cellH + vGap);
            kw = rightW; isRightCol = true;
        }
        else
            continue;

        // Key background
        PIXEL_TYPE bgCol = (x == selected)
                           ? ARGB(220, 120, 120, 120)
                           : ARGB(180,  50,  50,  50);
        renderer->FillRoundRect(kx, ky, kw, kh, cellW * 0.1f, bgCol);

        // Key label
        mFont->SetColor((x == selected)
                        ? ARGB(255, 255, 255, 255)
                        : ARGB(255, 255, 220,   0));

        string label = keys[x]->displayValue;
        if (!isRightCol && x != KPD_SPACE && x >= KPD_A && x <= KPD_Z)
        {
            char ch[2] = { (char)keys[x]->id, '\0' };
            if (bCapslock) ch[0] = toupper(ch[0]);
            label = string(ch);
        }

        float lW = mFont->GetStringWidth(label.c_str());
        float lH = mFont->GetHeight();
        mFont->DrawString(label.c_str(),
                          kx + (kw - lW) / 2.0f + lW * 0.35f,
                          ky + (kh - lH) / 2.0f + lH * 0.35f);

        // Touch hit center
        keys[x]->mX = kx + kw / 2.0f;
        keys[x]->mY = ky + kh / 2.0f;
    }
#endif

    // Restore font scale
    mFont->SetScale(SCALE);
}

unsigned int SimplePad::cursorPos()
{
    if (cursor > buffer.size()) return buffer.size();
    return cursor;
}