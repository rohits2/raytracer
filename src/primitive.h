#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include <cmath>

const float EPS = 1e-5;
const float PI = 3.1415926;

struct Vec3;

Vec3 const operator*(const Vec3 &v, float s);
Vec3 const operator*(float s, const Vec3 &v);
Vec3 const operator/(const Vec3 &v, float s);
Vec3 const operator/(float s, const Vec3 &v);

struct Vec3 {
  public:
    float x, y, z;

    Vec3(const Vec3 &other);
    Vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z){};

    // No vector operators
    Vec3 operator-() const;

    // Single-vector operators
    void operator=(const Vec3 &other);
    bool operator==(const Vec3 &other);

    // Two-vector operations.
    Vec3 operator*(const Vec3 &v) const;
    Vec3 operator/(const Vec3 &v) const;
    Vec3 operator+(const Vec3 &v) const;
    Vec3 operator-(const Vec3 &v) const;
    Vec3 operator%(const Vec3 &v) const;
    float operator^(const Vec3 &v) const;

    // Helper functions
    Vec3 normalize() const;
    float magnitude() const;
    float sum() const;
    float dot(const Vec3 &v) const;
    Vec3 rotate(int axis, float radians_cw) const;
    Vec3 rotate(const Vec3 &rpy) const;
    Vec3 unrotate(const Vec3 &rpy) const;
    unsigned char compare(const Vec3& other) const;
};

#endif
