#ifndef PAGE_CURL_H
#define PAGE_CURL_H

#include "page_turn.h"

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
