#ifndef PAGE_TURN_H
#define PAGE_TURN_H

// A book page turning over the spine, as a mesh bent round a cylinder (the journal's page turn).
//
// Page-local units: x from the spine (0) to the free edge (w), y from the bottom (0) to the top (h). The bottom free
// corner (w, 0) is pulled to `corner`; the page folds along the line halfway between the two, round a cylinder whose
// radius grows towards the middle of the turn, and lies flipped over beyond it. corner (w, 0) is the page lying flat,
// (-w, 0) the page turned over. The textures can reach `overhang` past the free edge (ribbons hanging out of the
// page): that strip bends with the page but is not part of it; its texels with alpha under 0.5 are not drawn.
struct PageCurl {
	float w = 0, h = 0;
	float cornerX = 0, cornerY = 0;
	float overhang = 0;
};

// Keeps the page on its spine: the bottom corner can be pulled no further than the page's width from the bottom of the
// spine, nor than its diagonal from the top.
void clampCorner(PageCurl& c);
// 0 flat .. 1 turned over, from how far the corner has moved.
[[nodiscard]] float curlProgress(const PageCurl& c);

#endif
