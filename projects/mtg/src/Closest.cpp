#ifndef _CLOSEST_H_
#define _CLOSEST_H_
#include "PrecompiledHeader.h"
#include "CardSelector.h"

template<typename T, typename Target>
static inline Target* closest(vector<Target*>& cards, Limitor* limitor, Target* ref)
{
    Target* card = ref;
    float curdist = 1000000.0f; // This is bigger than any possible distance
    for (typename vector<Target*>::iterator it = cards.begin(); it != cards.end(); ++it)
    {
        if (!T::test(ref, (*it)))
            continue;
        if ((*it)->actA < 32)
            continue;
        if ((NULL != limitor) && (!limitor->select(*it)))
            continue;
        if (ref)
        {
            float dist = ((*it)->x - ref->x) * ((*it)->x - ref->x) + ((*it)->y - ref->y) * ((*it)->y - ref->y);
            if (dist < curdist)
            {
                curdist = dist;
                card = *it;
            }
        }
        else
            card = *it;
    }
    return card;
}

template<typename T, typename Target>
static inline Target* closest(vector<Target*>& cards, Limitor* limitor, float x, float y)
{
    Target* card = NULL;
    float curdist = 1000000.0f; // This is bigger than any possible distance
    for (typename vector<Target*>::iterator it = cards.begin(); it != cards.end(); ++it)
    {
        if ((*it)->actA < 32)
            continue;
        if ((NULL != limitor) && (!limitor->select(*it)))
            continue;
        float dist = ((*it)->x - x) * ((*it)->x - x) + ((*it)->y - y) * ((*it)->y - y);
        if (dist < curdist)
        {
            curdist = dist;
            card = *it;
        }
    }
    return card;
}

// Touch-first hit-test: return the element whose rendered bounds actually contain
// the point (x, y), or NULL if the tap landed on empty space. When overlapping
// elements (e.g. fanned hand cards) both contain the point, the one whose center
// is nearest the tap wins, which matches the topmost/most-relevant card.
template<typename Target>
static inline Target* hitTest(vector<Target*>& cards, Limitor* limitor, float x, float y)
{
    Target* hit = NULL;
    float curdist = 1000000.0f; // This is bigger than any possible distance
    for (typename vector<Target*>::iterator it = cards.begin(); it != cards.end(); ++it)
    {
        if ((*it)->actA < 32)
            continue;
        if ((NULL != limitor) && (!limitor->select(*it)))
            continue;
        if (!(*it)->Contains(x, y))
            continue;
        float dist = ((*it)->actX - x) * ((*it)->actX - x) + ((*it)->actY - y) * ((*it)->actY - y);
        if (dist < curdist)
        {
            curdist = dist;
            hit = *it;
        }
    }
    return hit;
}

#endif
