#ifndef PARTICLES_H
#define PARTICLES_H

struct Particle {
    float x, y;
    float vx, vy;
    float mass;
    float color[3];  // 新增: RGB 颜色值
    float collision_time;  // 新增: 用于跟踪碰撞后的时间
};

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int ROWS = 20;
const int COLS = 20;
const int PARTICLE_COUNT = ROWS * COLS;
const float MAX_MASS = 5.0f;
const float MIN_MASS = 0.5f;

const float PARTICLE_SIZE = 1.0f;
#ifdef __CUDACC__
__constant__ float d_PARTICLE_SIZE = 1.0f;
#endif

#endif // PARTICLES_H