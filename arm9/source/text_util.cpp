#include "text_util.h"
#include "render.h"
#include <cstring>

static char sWrapBuf[4096];
static const char* sLines[TEXT_MAX_LINES];
static int sLineLens[TEXT_MAX_LINES];

int textWrap(const char* src, int srcLen, int maxPixelWidth, const char** lines, int* lineLen) {
    int maxChars = maxPixelWidth / 6;   // 6px per character (CHAR_W + CHAR_GAP)
    if (maxChars < 1) maxChars = 1;

    // Clamp srcLen to buffer size minus one (for NUL terminator)
    int copyLen = srcLen < 4095 ? srcLen : 4095;
    memcpy(sWrapBuf, src, copyLen);
    sWrapBuf[copyLen] = '\0';

    int lineCount = 0;
    int pos = 0;

    while (pos < copyLen && lineCount < TEXT_MAX_LINES) {
        // Start of this line
        lines[lineCount] = &sWrapBuf[pos];

        // Remaining characters
        int remaining = copyLen - pos;
        if (remaining <= maxChars) {
            // Last line -- everything fits
            lineLen[lineCount] = remaining;
            lineCount++;
            break;
        }

        // Find the last space within maxChars
        int breakAt = maxChars;
        while (breakAt > 0 && sWrapBuf[pos + breakAt] != ' ') breakAt--;

        if (breakAt == 0) {
            // No space found -- force break at maxChars
            breakAt = maxChars;
        }

        lineLen[lineCount] = breakAt;
        lineCount++;
        pos += breakAt;

        // Skip the space at the break point
        if (pos < copyLen && sWrapBuf[pos] == ' ') pos++;
    }

    return lineCount;
}

// Shared implementation for both screens
static int textRenderWrappedImpl(int x, int y, const char* text, int textLen,
                                  int maxWidth, u16 color,
                                  int scrollOffset, int maxVisibleLines,
                                  bool useSub) {
    const char* lines[TEXT_MAX_LINES];
    int lens[TEXT_MAX_LINES];
    int total = textWrap(text, textLen, maxWidth, lines, lens);

    static char lineBuf[256];  // temporary buffer for null-terminated line
    int lineH = 10;            // 7px glyph + 3px spacing
    int drawn = 0;

    for (int i = scrollOffset; i < total && drawn < maxVisibleLines; i++) {
        // Copy line into buffer and null-terminate
        int len = lens[i] < 255 ? lens[i] : 255;
        memcpy(lineBuf, lines[i], len);
        lineBuf[len] = '\0';

        if (useSub) {
            renderTextSub(x, y + drawn * lineH, lineBuf, color);
        } else {
            renderText(x, y + drawn * lineH, lineBuf, color);
        }
        drawn++;
    }

    return total;  // caller uses this for scroll bounds
}

int textRenderWrapped(int x, int y, const char* text, int textLen, int maxWidth, u16 color, int scrollOffset, int maxVisibleLines) {
    return textRenderWrappedImpl(x, y, text, textLen, maxWidth, color, scrollOffset, maxVisibleLines, false);
}

int textRenderWrappedSub(int x, int y, const char* text, int textLen, int maxWidth, u16 color, int scrollOffset, int maxVisibleLines) {
    return textRenderWrappedImpl(x, y, text, textLen, maxWidth, color, scrollOffset, maxVisibleLines, true);
}
