// ui.h - widget-uri simple (butoane, comutatoare, slidere) desenate cu raylib
#pragma once
#include <string>
#include <vector>

#include "raylib.h"

namespace ui {

struct Theme {
    Color bg = {13, 15, 20, 255}, panel = {20, 23, 30, 255}, card = {27, 31, 40, 255};
    Color cardHover = {34, 39, 50, 255}, border = {44, 50, 64, 255};
    Color text = {230, 233, 239, 255}, textDim = {139, 147, 167, 255};
    Color accent = {99, 102, 241, 255}, accentHover = {120, 123, 250, 255};
    Color success = {34, 197, 94, 255}, danger = {244, 63, 94, 255}, warning = {245, 158, 11, 255};
};
extern Theme T;

void LoadFonts();
void UnloadFonts();
void BeginFrame();  // la inceputul fiecarui cadru
bool Captured();    // un slider e tras cu mouse-ul

void Text(const std::string& s, float x, float y, float size, Color c, bool bold = false);
Vector2 Measure(const std::string& s, float size, bool bold = false);
std::vector<std::string> Wrap(const std::string& s, float w, float size, bool bold = false);
float TextWrapped(const std::string& s, float x, float y, float w, float size, Color c);  // -> inaltimea

void Card(Rectangle r);
void SectionTitle(const std::string& s, float x, float y);
bool Button(Rectangle r, const std::string& label, bool primary = false);
bool Toggle(Rectangle r, const std::string& label, bool& v);
bool Slider(Rectangle r, const std::string& label, float& v, float mn, float mx, bool enabled = true);
bool Segmented(Rectangle r, const std::vector<std::string>& items, int& sel);
void Tooltip(Vector2 at, const std::vector<std::string>& lines);

}  // namespace ui
