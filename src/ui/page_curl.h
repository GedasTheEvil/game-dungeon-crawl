#ifndef PAGE_CURL_H
#define PAGE_CURL_H

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

// Where the turning page lies on the canvas: the spine's foot, which way it turns, the textures of its two sides.
struct PageCurlPlacement {
	float spineX = 0, bottomY = 0;
	bool mirror = false; // a left page, turning to the right
	int front = 0; // the side seen while flat: u 0..1 left to right as laid out (the overhang included), v 0..1 bottom
				   // to top
	int back = 0;  // the side that comes into view, laid out as the page on the other side of the spine
	float eyeX = 0, eyeY = 0; // the lifted part grows a little towards the viewer, away from this point
};

// The page and its shadow on the page under it. Draw the spread under it first; uses the depth buffer (clears it)
// and leaves texturing off.
void drawPageCurl(const PageCurl& c, const PageCurlPlacement& at);

#endif
