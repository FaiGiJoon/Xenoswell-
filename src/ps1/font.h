#ifndef PS1_FONT_H
#define PS1_FONT_H

extern const char FONT_CHARS[];
extern const unsigned char FONT_COLS[][5];
/* Glyph index of c (lower case folds to upper), or -1 when there is nothing to draw. */
int font_index(char c);

#endif
