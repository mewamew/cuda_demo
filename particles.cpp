#include "particles.h"
#include <random>
#include <cmath>

std::vector<Particle> particles;

void InitializeParticles() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis_angle(-1.0, 1.0);
    std::uniform_real_distribution<> dis_speed(-2.0, 2.0);
    std::uniform_real_distribution<> dis_mass(MIN_MASS, MAX_MASS);

    const float spacing = PARTICLE_SIZE * 2;
    const float startX = (WINDOW_WIDTH - (COLS - 1) * spacing) / 2;
    const float startY = (WINDOW_HEIGHT - (ROWS - 1) * spacing) / 2;

    for (int i = 0; i < PARTICLE_COUNT; ++i) {
        int row = i / COLS;
        int col = i % COLS;
        
        Particle& p = particles[i];
        p.x = startX + col * spacing;
        p.y = startY + row * spacing;
        
        float angle = static_cast<float>(dis_angle(gen) * 2 * 3.14159);
        float speed = 2.0f + static_cast<float>(dis_speed(gen));
        p.vx = std::cos(angle) * speed;
        p.vy = std::sin(angle) * speed;
        p.mass = static_cast<float>(dis_mass(gen));
        p.color[0] = 1.0f;  // R (黄色)
        p.color[1] = 1.0f;  // G
        p.color[2] = 0.0f;  // B
        p.collision_time = 0.0f;  // 初始化碰撞时间为0
    }
}

float distance(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return std::sqrt(dx*dx + dy*dy);
}

void UpdateParticles() {
    float dt = 1.0f / 60.0f;  // 假设60FPS

    for (auto& p : particles) {
        // 更新碰撞时间
        if (p.collision_time > 0) {
            p.collision_time -= dt;
            if (p.collision_time <= 0) {
                // 恢复为黄色
                p.color[0] = 1.0f;
                p.color[1] = 1.0f;
                p.color[2] = 0.0f;
                p.collision_time = 0;
            }
        }

        // 更新位置
        p.x += p.vx;
        p.y += p.vy;
        
        // 边界检测和反弹
        if (p.x <= 0 || p.x >= WINDOW_WIDTH) {
            p.vx = -p.vx; 
            p.x = (p.x <= 0) ? 0 : WINDOW_WIDTH;
        }
        if (p.y <= 0 || p.y >= WINDOW_HEIGHT) {
            p.vy = -p.vy;
            p.y = (p.y <= 0) ? 0 : WINDOW_HEIGHT;
        }
    }

    // 碰撞检测
    for (int i = 0; i < PARTICLE_COUNT; ++i) {
        Particle& p1 = particles[i];
        
        for (int j = i + 1; j < PARTICLE_COUNT; ++j) {
            Particle& p2 = particles[j];
            
            float dist = distance(p1.x, p1.y, p2.x, p2.y);
            if (dist < PARTICLE_SIZE * 2) {
                // 计算碰撞后的速度
                float m1 = p1.mass;
                float m2 = p2.mass;
                float totalMass = m1 + m2;

                // 计算碰撞后的速度 (一维碰撞近似)
                float v1_new = ((m1 - m2) * p1.vx + 2 * m2 * p2.vx) / totalMass;
                float v2_new = ((m2 - m1) * p2.vx + 2 * m1 * p1.vx) / totalMass;
                float loss = 0.9999f;
                p1.vx = v1_new * loss;
                p2.vx = v2_new * loss;

                v1_new = ((m1 - m2) * p1.vy + 2 * m2 * p2.vy) / totalMass;
                v2_new = ((m2 - m1) * p2.vy + 2 * m1 * p1.vy) / totalMass;

                p1.vy = v1_new * loss;
                p2.vy = v2_new * loss;

                // 将碰撞的粒子变为红色
                p1.color[0] = 1.0f;  // R
                p1.color[1] = 0.0f;  // G
                p1.color[2] = 0.0f;  // B
                p1.collision_time = 0.1f;  // 设置碰撞时间为0.1秒

                p2.color[0] = 1.0f;  // R
                p2.color[1] = 0.0f;  // G
                p2.color[2] = 0.0f;  // B
                p2.collision_time = 0.1f;  // 设置碰撞时间为0.1秒
            }
        }
    }
}

const std::vector<Particle>& GetParticles() {
    return particles;
}

void ResizeParticles(int count) {
    particles.resize(count);
}