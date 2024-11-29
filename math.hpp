#pragma once
#include <cmath>

struct vec2 {
  double x;
  double y;
};

vec2 vec2_add(const vec2 &a, const vec2 &b) {
  return vec2{a.x + b.x, a.y + b.y};
}

vec2 vec2_sub(const vec2 &a, const vec2 &b) {
  return vec2{a.x - b.x, a.y - b.y};
}

vec2 vec2_mul(const vec2 &a, double scalar) {
  return vec2{a.x * scalar, a.y * scalar};
}

double vec2_dot(const vec2 &a, const vec2 &b) { return a.x * b.x + a.y * b.y; }

double vec2_cross(const vec2 &a, const vec2 &b) {
  return a.x * b.y - b.x * a.y;
}

void vec2_normalize(vec2 &a) {
  double length = sqrt(a.x * a.x + a.y * a.y);
  if (length > 0) {
    a.x /= length;
    a.y /= length;
  }
}

double vec2_length(const vec2 &a) { return sqrt(a.x * a.x + a.y * a.y); }

double vec2_length_squared(const vec2 &a) { return a.x * a.x + a.y * a.y; }
