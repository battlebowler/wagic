#include "PrecompiledHeader.h"

#include "SimpleMenuItem.h"
#include "Translate.h"
#include "WResourceManager.h"


SimpleMenuItem::SimpleMenuItem(int id): SimpleButton(id)
{
}

SimpleMenuItem::SimpleMenuItem(SimpleMenu* _parent, int id, int fontId, string text, float x, float y, bool hasFocus, bool autoTranslate) :
    SimpleButton( _parent, id, fontId, text, x, y, hasFocus, autoTranslate)
{
    parent = (SimpleMenu *) _parent;
    mDescription = "";
    // Uniform rows: undo the base ctor's enlarged target for a focused item (see setFocus).
    mTargetScale = (getText().size() < 20) ? SCALE_LARGE_NORMAL : SCALE_NORMAL;
}

void SimpleMenuItem::Entering()
{
    checkUserClick();
    setFocus(true);
    if (getParent() != NULL)
    {
        SimpleMenu *menu = (SimpleMenu *) parent;
        menu->selectionTargetY = getY();
    }

}

void SimpleMenuItem::setFocus(bool value)
{
    SimpleButton::setFocus(value);
    // Uniform rows on touch popups: the focused row keeps the same size as the others (no resting
    // "selected" enlargement). Tapping is the only selection cue (a brief colour flash), so a row
    // being focused must not look different at rest.
    mTargetScale = (getText().size() < 20) ? SCALE_LARGE_NORMAL : SCALE_NORMAL;
}


/* Accessors */
string SimpleMenuItem::getDescription() const
{
    return mDescription;
}

void SimpleMenuItem::setDescription( const string& desc )
{
    mDescription = desc;
}


ostream& SimpleMenuItem::toString(ostream& out) const
{
    return out << "SimpleMenuItem ::: mHasFocus : " << hasFocus()
                << " ; parent : " << getParent()
                << " ; mText : " << getText()
                << " ; mScale : " << getScale()
                << " ; mTargetScale : " << getTargetScale()
                << " ; mX,mY : " << getX() << "," << getY();
}
