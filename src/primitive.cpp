#include "primitive.h"

Vec3::Vec3(const Vec3 &other) {
    x = other.x;
    y = other.y;
    z = other.z;
}

void Vec3::operator=(const Vec3 &other) {
    x = other.x;
    y = other.y;
    z = other.z;
}

bool Vec3::operator==(const Vec3 &other) { return x == other.x && y == other.y && z == other.z; }

// Invert the vector
Vec3 Vec3::operator-() const { return Vec3(-x, -y, -z); }

// Element-wise multiplication
Vec3 Vec3::operator*(const Vec3 &v) const { return Vec3(x * v.x, y * v.y, z * v.z); }

// Element-wise division
Vec3 Vec3::operator/(const Vec3 &v) const { return Vec3(x / v.x, y / v.y, z / v.z); }

// Element-wise addition
Vec3 Vec3::operator+(const Vec3 &v) const { return Vec3(x + v.x, y + v.y, z + v.z); }

// Vector subtraction
Vec3 Vec3::operator-(const Vec3 &v) const { return Vec3(x - v.x, y - v.y, z - v.z); }

// This does the dot product of two vectors.
float Vec3::operator^(const Vec3 &v) const { return x * v.x + y * v.y + z * v.z; }

// Vector cross-product
Vec3 Vec3::operator%(const Vec3 &v) const {
    Vec3 v_out;
    v_out.x = y * v.z - z * v.y;
    v_out.y = z * v.x - x * v.z;
    v_out.z = x * v.y - y * v.x;
    return v_out;
}

float Vec3::sum() const { return x + y + z; }

float Vec3::magnitude() const { return sqrt(x * x + y * y + z * z); }

// Normalize
Vec3 Vec3::normalize() const {
    float mag = magnitude();
    return Vec3(x / mag, y / mag, z / mag);
}
void rotate2(float *x, float *y, double radians) {
    // CAREFUL: LEFT-HANDED ROTATIONS!
    float c = cos(-radians);
    float s = sin(-radians);
    float n_x = c * *x - s * *y;
    float n_y = s * *x + c * *y;
    *x = n_x;
    *y = n_y;
}

Vec3 Vec3::rotate(int axis, float radians_cw) const {
    Vec3 vec = *this;
    switch (axis) {
    case 0:
        rotate2(&vec.y, &vec.z, radians_cw);
        break;
    case 1:
        rotate2(&vec.x, &vec.z, radians_cw);
        break;
    case 2:
        rotate2(&vec.x, &vec.y, radians_cw);
        break;
    default:
        exit(-127);
    }
    return vec;
}

Vec3 Vec3::rotate(const Vec3 &rpy) const {
    Vec3 vec = rotate(0, rpy.x);
    vec = vec.rotate(1, rpy.y);
    vec = vec.rotate(2, rpy.z);
    return vec;
}

unsigned char Vec3::compare(const Vec3& other) const{
    unsigned char bitcode = 0;
    bitcode |= x > other.x;
    bitcode |= (y > other.y) << 1;
    bitcode |= (z > other.z) << 2;
    return bitcode;
}

/************************ Externally defined vector operators ******************/
Vec3 const operator*(const Vec3 &v, float s) { return Vec3(v.x * s, v.y * s, v.z * s); }

Vec3 const operator*(float s, const Vec3 &v) { return v * s; }

Vec3 const operator/(const Vec3 &v, float s) { return Vec3(v.x / s, v.y / s, v.z / s); }

Vec3 const operator/(float s, const Vec3 &v) { return Vec3(s / v.x, s / v.y, s / v.z); }