// geometry.h - primitive geometrice 2D folosite de planificator
#pragma once
#include <cmath>
#include <vector>

struct Vec2 {
    double x = 0, y = 0;
};
inline Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
inline Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
inline Vec2 operator*(Vec2 a, double s) { return {a.x * s, a.y * s}; }
inline double cross(Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; }
inline double dist(Vec2 a, Vec2 b) { return std::hypot(a.x - b.x, a.y - b.y); }

// Dreptunghi aliniat la axe: [x0,x1] x [y0,y1]
struct RectD {
    double x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    double w() const { return x1 - x0; }
    double h() const { return y1 - y0; }
    Vec2 center() const { return {(x0 + x1) * 0.5, (y0 + y1) * 0.5}; }
    bool contains(Vec2 p) const { return p.x >= x0 && p.x <= x1 && p.y >= y0 && p.y <= y1; }
};

using Polygon = std::vector<Vec2>;

Polygon makeRect(double x0, double y0, double x1, double y1);
Polygon makeRotatedSquare(Vec2 c, double side, double angleDeg);

bool pointInPolygon(Vec2 p, const Polygon& poly);
bool segmentsIntersect(Vec2 a, Vec2 b, Vec2 c, Vec2 d);
bool segmentCrossesRectInterior(Vec2 a, Vec2 b, const RectD& r);
bool rectInsidePolygon(const RectD& r, const Polygon& poly);
bool rectTouchesPolygon(const RectD& r, const Polygon& poly);
RectD boundingBox(const Polygon& poly);
double signedArea(const Polygon& poly);
bool isSimplePolygon(const Polygon& poly);
std::vector<int> triangulate(const Polygon& poly);  // ear clipping -> triplete de indici
Vec2 sharedBoundaryMid(const RectD& a, const RectD& b);  // mijlocul frontierei comune
