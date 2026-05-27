#ifndef TEXT_UTIL_H
#define TEXT_UTIL_H
#include <nds/ndstypes.h>

constexpr int TEXT_MAX_LINES = 64;

// Word-wrap text into lines fitting maxPixelWidth. Returns line count.
int textWrap(const char* src, int srcLen, int maxPixelWidth, const char** lines, int* lineLen);

// Render multi-line wrapped text on top screen. Returns total lines.
int textRenderWrapped(int x, int y, const char* text, int textLen, int maxWidth, u16 color, int scrollOffset, int maxVisibleLines);

// Same for sub screen.
int textRenderWrappedSub(int x, int y, const char* text, int textLen, int maxWidth, u16 color, int scrollOffset, int maxVisibleLines);

#endif
