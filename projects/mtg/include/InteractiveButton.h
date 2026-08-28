//
//  InteractiveButton.h
//
//  Created by Michael Nguyen on 1/23/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef wagic_InteractiveButton_h
#define wagic_InteractiveButton_h

#include <string>
#include <vector>
#include <JLBFont.h>
#include <JGui.h>
#include "WResource_Fwd.h"
#include "SimpleButton.h"

using std::string;

#define SCALE_SELECTED      1.2f
#define SCALE_NORMAL        SCALE

const int kDismissButtonId       = 10000;
const int kToggleDeckActionId    = 10001;
const int kSellCardActionId      = 10002;
const int kSBActionId            = 10003;
const int kMenuButtonId          = 10004;
const int kFilterButtonId        = 10005;
const int kNextStatsButtonId     = 10006;
const int kPrevStatsButtonId     = 10007;
const int kCycleCardsButtonId    = 10008;
const int kShowCardListButtonId  = 10009;
const int kSwitchViewButton      = 10010;
const int kToggleUpButton        = 10011;
const int kToggleDownButton      = 10012;
const int kToggleLeftButton      = 10013;
const int kToggleRightButton     = 10014;

class InteractiveButton: public SimpleButton
{
private:
    JQuadPtr buttonImage;
    JButton mActionKey;
    
public:
    InteractiveButton(JGuiController* _parent, int id, int fontId, string text, float x, float y, JButton actionKey, bool hasFocus = false, bool autoTranslate = false);
    
    virtual void Entering();
    virtual bool ButtonPressed();
    virtual void setImage( const JQuadPtr imagePtr );
    virtual void checkUserClick();
    virtual void Render();
    virtual ostream& toString(ostream& out) const;

    // Right-align a bottom-bar row of buttons: the rightmost button's pill ends at rightEdge, each
    // preceding button sits 'gap' to its left. Widths are measured (MAIN_FONT @ 1.0, matching
    // Render) so spacing is uniform regardless of label length. Buttons in visual left-to-right
    // order; NULLs are skipped.
    static void layoutRowRight(const std::vector<InteractiveButton*>& btns, float rightEdge, float y, float gap);
};

#endif
