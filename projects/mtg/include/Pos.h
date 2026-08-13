#ifndef _POS_H_
#define _POS_H_

#include "JGE.h"

struct Pos
{
    float actX, actY, actZ, actT, actA;
    float x, y, zoom, t, alpha;
    float width, height;
    PIXEL_TYPE mask;
    Pos(float, float, float, float, float);
    virtual ~Pos(){};
    virtual void Update(float dt);
    void UpdateNow();
    virtual void Render();
    void Render(JQuad*);

    // Touch hit-testing: returns true if the point (px, py), expressed in game
    // coordinates, lies within this element's rendered bounds. Elements are drawn
    // centered on (actX, actY) at scale actZ, so the footprint is
    // width*actZ by height*actZ around that center.
    virtual bool Contains(float px, float py) const;
};

#endif // _POS_H_
