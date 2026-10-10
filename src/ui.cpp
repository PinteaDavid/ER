// ui.cpp
#include "ui.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace ui {

Theme T;
static Font gRegular, gBold;
static const void* gActive = nullptr;  // slider-ul tras in acest moment

// Font TTF cu diacritice romanesti (Arial pe macOS, DejaVu pe Linux)
static Font loadFirst(std::vector<const char*> paths) {
    std::vector<int> cps;
    for (int c = 32; c < 256; c++) cps.push_back(c);
    for (int c : {0x102, 0x103, 0x218, 0x219, 0x21A, 0x21B, 0x2013, 0x2014, 0x2192}) cps.push_back(c);
    for (const char* p : paths)
        if (FileExists(p)) {
            Font f = LoadFontEx(p, 40, cps.data(), (int)cps.size());
            GenTextureMipmaps(&f.texture);
            SetTextureFilter(f.texture, TEXTURE_FILTER_TRILINEAR);
            return f;
        }
    return GetFontDefault();
}

void LoadFonts() {
    gRegular = loadFirst({"/System/Library/Fonts/Supplemental/Arial.ttf",
                          "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"});
    gBold = loadFirst({"/System/Library/Fonts/Supplemental/Arial Bold.ttf",
                       "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"});
}

void UnloadFonts() {
    UnloadFont(gRegular);
    UnloadFont(gBold);
}

void Text(const std::string& s, float x, float y, float size, Color c, bool bold) {
    DrawTextEx(bold ? gBold : gRegular, s.c_str(), {std::round(x), std::round(y)}, size, 0, c);
}

Vector2 Measure(const std::string& s, float size, bool bold) {
    return MeasureTextEx(bold ? gBold : gRegular, s.c_str(), size, 0);
}

std::vector<std::string> Wrap(const std::string& s, float w, float size, bool bold) {
    std::vector<std::string> out;
    std::string line, word;
    for (size_t i = 0; i <= s.size(); i++) {
        char ch = i < s.size() ? s[i] : ' ';
        if (ch != ' ') {
            word += ch;
            continue;
        }
        std::string t = line.empty() ? word : line + " " + word;
        if (!line.empty() && Measure(t, size, bold).x > w) {
            out.push_back(line);
            line = word;
        } else {
            line = t;
        }
        word.clear();
    }
    if (!line.empty()) out.push_back(line);
    return out;
}

float TextWrapped(const std::string& s, float x, float y, float w, float size, Color c) {
    auto lines = Wrap(s, w, size);
    for (size_t i = 0; i < lines.size(); i++) Text(lines[i], x, y + i * (size + 3), size, c);
    return lines.size() * (size + 3);
}

// ------------------------------------------------------------------ widget-uri
static void RoundRect(Rectangle r, float rad, Color c) {
    DrawRectangleRounded(r, std::min(1.0f, 2 * rad / std::min(r.width, r.height)), 8, c);
}
static void RoundRectLines(Rectangle r, float rad, Color c) {
    DrawRectangleRoundedLinesEx(r, std::min(1.0f, 2 * rad / std::min(r.width, r.height)), 8, 1.0f, c);
}
static bool Hover(Rectangle r) { return !gActive && CheckCollisionPointRec(GetMousePosition(), r); }
static bool Clicked(Rectangle r) { return Hover(r) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT); }

void BeginFrame() {
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) gActive = nullptr;
}
bool Captured() { return gActive != nullptr; }

void Card(Rectangle r) {
    RoundRect(r, 10, T.card);
    RoundRectLines(r, 10, T.border);
}

void SectionTitle(const std::string& s, float x, float y) { Text(s, x, y, 12, T.textDim, true); }

bool Button(Rectangle r, const std::string& label, bool primary) {
    bool h = Hover(r);
    RoundRect(r, 8, primary ? (h ? T.accentHover : T.accent) : (h ? T.cardHover : T.card));
    if (!primary) RoundRectLines(r, 8, h ? T.accent : T.border);
    Vector2 m = Measure(label, 14, true);
    Text(label, r.x + (r.width - m.x) / 2, r.y + (r.height - m.y) / 2, 14, T.text, true);
    return Clicked(r);
}

bool Toggle(Rectangle r, const std::string& label, bool& v) {
    bool clicked = Clicked(r);
    if (clicked) v = !v;
    Text(label, r.x, r.y + (r.height - 14) / 2, 14, T.text);
    Rectangle tr = {r.x + r.width - 36, r.y + (r.height - 20) / 2, 36, 20};
    RoundRect(tr, 10, v ? T.accent : T.border);
    DrawCircleV({v ? tr.x + 26 : tr.x + 10, tr.y + 10}, 7, WHITE);
    return clicked;
}

bool Slider(Rectangle r, const std::string& label, float& v, float mn, float mx, bool enabled) {
    char buf[16];
    snprintf(buf, sizeof buf, "%.0f", v);
    Text(label, r.x, r.y, 14, enabled ? T.text : T.textDim);
    Text(buf, r.x + r.width - Measure(buf, 14, true).x, r.y, 14, enabled ? T.accentHover : T.textDim, true);

    Rectangle track = {r.x, r.y + 26, r.width, 6};
    if (enabled && Clicked({r.x - 4, r.y + 16, r.width + 8, 24})) gActive = &v;
    bool changed = false;
    if (gActive == &v) {
        float t = std::clamp((GetMousePosition().x - track.x) / track.width, 0.0f, 1.0f);
        float nv = std::round(mn + t * (mx - mn));
        changed = nv != v;
        v = nv;
    }
    float kx = track.x + track.width * (v - mn) / (mx - mn);
    RoundRect(track, 3, T.border);
    RoundRect({track.x, track.y, kx - track.x, track.height}, 3, enabled ? T.accent : T.border);
    DrawCircleV({kx, track.y + 3}, 8, enabled ? WHITE : T.textDim);
    DrawCircleV({kx, track.y + 3}, 4, enabled ? T.accent : T.border);
    return changed;
}

bool Segmented(Rectangle r, const std::vector<std::string>& items, int& sel) {
    Card(r);
    float w = (r.width - 6) / items.size();
    bool changed = false;
    for (int i = 0; i < (int)items.size(); i++) {
        Rectangle it = {r.x + 3 + i * w, r.y + 3, w, r.height - 6};
        if (i == sel) RoundRect(it, 7, T.accent);
        else if (Hover(it)) RoundRect(it, 7, T.cardHover);
        if (Clicked(it) && sel != i) sel = i, changed = true;
        Vector2 m = Measure(items[i], 13, true);
        Text(items[i], it.x + (it.width - m.x) / 2, it.y + (it.height - m.y) / 2, 13, i == sel ? WHITE : T.textDim, true);
    }
    return changed;
}

void Tooltip(Vector2 at, const std::vector<std::string>& lines) {
    float w = 0;
    for (auto& l : lines) w = std::max(w, Measure(l, 13).x);
    Rectangle r = {at.x + 16, at.y + 16, w + 20, lines.size() * 18.0f + 10};
    if (r.x + r.width > GetScreenWidth()) r.x = at.x - r.width - 10;
    if (r.y + r.height > GetScreenHeight()) r.y = at.y - r.height - 10;
    Card(r);
    for (size_t i = 0; i < lines.size(); i++)
        Text(lines[i], r.x + 10, r.y + 6 + i * 18.0f, 13, i == 0 ? T.text : T.textDim, i == 0);
}

}  // namespace ui
