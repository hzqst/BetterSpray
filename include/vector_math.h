#ifndef VECTOR_MATH_H
#define VECTOR_MATH_H

#include <cmath>

typedef float vec3_t[3];

inline void VectorCopy(const vec3_t src, vec3_t dst) {
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
}

inline void VectorSet(vec3_t v, float x, float y, float z) {
    v[0] = x; v[1] = y; v[2] = z;
}

inline void VectorClear(vec3_t v) {
    v[0] = v[1] = v[2] = 0.0f;
}

inline void CrossProduct(const vec3_t a, const vec3_t b, vec3_t result) {
    result[0] = a[1] * b[2] - a[2] * b[1];
    result[1] = a[2] * b[0] - a[0] * b[2];
    result[2] = a[0] * b[1] - a[1] * b[0];
}

inline float DotProduct(const vec3_t a, const vec3_t b) {
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

inline float VectorLength(const vec3_t v) {
    return std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
}

inline float VectorNormalize(vec3_t v) {
    float len = VectorLength(v);
    if (len > 0.0001f) {
        v[0] /= len;
        v[1] /= len;
        v[2] /= len;
    }
    return len;
}

inline void VectorMA(const vec3_t start, float scale, const vec3_t direction, vec3_t result) {
    result[0] = start[0] + scale * direction[0];
    result[1] = start[1] + scale * direction[1];
    result[2] = start[2] + scale * direction[2];
}

inline void AngleVectors(const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up) {
    float pitch = angles[0] * (3.14159265f / 180.0f);
    float yaw = angles[1] * (3.14159265f / 180.0f);
    float roll = angles[2] * (3.14159265f / 180.0f);

    float sp = sinf(pitch), cp = cosf(pitch);
    float sy = sinf(yaw), cy = cosf(yaw);
    float sr = sinf(roll), cr = cosf(roll);

    if (forward) {
        forward[0] = cp * cy;
        forward[1] = cp * sy;
        forward[2] = -sp;
    }

    if (right) {
        right[0] = -1 * sr * sp * cy + -1 * cr * -sy;
        right[1] = -1 * sr * sp * sy + -1 * cr * cy;
        right[2] = -1 * sr * cp;
    }

    if (up) {
        up[0] = cr * sp * cy + -sr * -sy;
        up[1] = cr * sp * sy + -sr * cy;
        up[2] = cr * cp;
    }
}

#endif // VECTOR_MATH_H
