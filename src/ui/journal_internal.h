#ifndef JOURNAL_INTERNAL_H
#define JOURNAL_INTERNAL_H

#include "../render/ui_draw.h"

// What journal_view.cpp (the book, ribbons, page turns, input) and journal_pages.cpp (each section's page drawing)
// share. Only these two include it.
namespace journal_view {

constexpr ui::Rect LEFT_PAGE = {12, 12.5f, 66, 70};
constexpr ui::Rect RIGHT_PAGE = {78, 12.5f, 66, 70};

const ui::Rect& pageRect(int side);
ui::Rect answerRect(int side);
ui::Color darker(ui::Color c, float f);

} // namespace journal_view

#endif
