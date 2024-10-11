#include "particles.h"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <vector>
#include <random>
#include <cmath>
#include <cstdio>

// CUDA 设备变量
Particle* d_particles = nullptr;

// CUDA 错误检查宏
#define cudaCheckError(ans) { gpuAssert((ans), __FILE__, __LINE__); }
inline void gpuAssert(cudaError_t code, const char* file, int line, bool abort=true)
{
    if (code != cudaSuccess) 
    {
        fprintf(stderr,"GPUassert: %s %s %d\n", cudaGetErrorString(code), file, line);
        if (abort) exit(code);
    }
}

__device__ float distance(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return sqrtf(dx*dx + dy*dy);
}

__global__
void CollideParticlesKernel(Particle* d_particles, int count, float dt) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < count) {
        Particle p1 = d_particles[idx];
        
        // 更新碰撞时间
        if (p1.collision_time > 0) {
            p1.collision_time -= dt;
            if (p1.collision_time <= 0) {
                // 恢复为黄色
                p1.color[0] = 1.0f;
                p1.color[1] = 1.0f;
                p1.color[2] = 0.0f;
                p1.collision_time = 0;
            }
        }

        for (int j = idx + 1; j < count; j++) {
            Particle p2 = d_particles[j];
            
            float dist = distance(p1.x, p1.y, p2.x, p2.y);
            if (dist < d_PARTICLE_SIZE * 2) {
                // 碰撞处理代码...

                // 将碰撞的粒子变为红色
                p1.color[0] = 1.0f;  // R
                p1.color[1] = 0.0f;  // G
                p1.color[2] = 0.0f;  // B
                p1.collision_time = 0.1f;  // 设置碰撞时间为1秒

                p2.color[0] = 1.0f;  // R
                p2.color[1] = 0.0f;  // G
                p2.color[2] = 0.0f;  // B
                p2.collision_time = 0.1f;  // 设置碰撞时间为1秒

                d_particles[j] = p2;
            }
        }
        
        d_particles[idx] = p1;
    }
}

// CUDA 核函数，用于更新粒子位置
__global__
void UpdateParticlesKernel(Particle* d_particles, int count) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < count) {
        Particle p = d_particles[idx];
        
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
        
        d_particles[idx] = p;
    }
}

// 主机函数：初始化粒子
extern "C" void InitializeParticles(Particle* h_particles, int count) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis_angle(-1.0, 1.0);
    std::uniform_real_distribution<> dis_speed(-2.0, 2.0);
    std::uniform_real_distribution<> dis_mass(MIN_MASS, MAX_MASS);  // 质量范围从0.5到2.0

    const int rows = ROWS;
    const int cols = COLS;
    const float spacing = PARTICLE_SIZE * 2; // 空隙大小为粒子大小
    const float startX = (WINDOW_WIDTH - (cols - 1) * spacing) / 2; // 假设窗口宽度为800
    const float startY = (WINDOW_HEIGHT - (rows - 1) * spacing) / 2; // 假设窗口高度为600

    // 设置设备常量
    float h_PARTICLE_SIZE = 1.0f;
    cudaMemcpyToSymbol(d_PARTICLE_SIZE, &h_PARTICLE_SIZE, sizeof(float));


    for (int i = 0; i < count; ++i) {
        int row = i / cols;
        int col = i % cols;
        
        Particle p;
        p.x = startX + col * spacing;
        p.y = startY + row * spacing;
        
        float angle = static_cast<float>(dis_angle(gen) * 2 * 3.14159);
        float speed = 2.0f + static_cast<float>(dis_speed(gen));
        p.vx = std::cos(angle) * speed;
        p.vy = std::sin(angle) * speed;
        p.mass = static_cast<float>(dis_mass(gen));
        p.color[0] = 1.0f;  // R
        p.color[1] = 1.0f;  // G
        p.color[2] = 0.0f;  // B
        p.collision_time = 0.0f;  // 初始化碰撞时间为0
        h_particles[i] = p;
    }

    // 将数据复制到设备
    cudaCheckError(cudaMemcpy(d_particles, h_particles, count * sizeof(Particle), cudaMemcpyHostToDevice));
}

// 主机函数：更新粒子
extern "C" void UpdateParticles(int count, Particle* h_particles, float dt) {
    // 调用CUDA核函数
    int threadsPerBlock = 256;
    int blocksPerGrid = (count + threadsPerBlock - 1) / threadsPerBlock;
    
    UpdateParticlesKernel<<<blocksPerGrid, threadsPerBlock>>>(d_particles, count);
    CollideParticlesKernel<<<blocksPerGrid, threadsPerBlock>>>(d_particles, count, dt);
    
    // 将更新后的粒子数据复制回主机
    cudaMemcpy(h_particles, d_particles, count * sizeof(Particle), cudaMemcpyDeviceToHost);
}

// 主机函数：分配设备内存
extern "C" void AllocateDeviceMemory(int count) {
    cudaCheckError(cudaMalloc((void**)&d_particles, count * sizeof(Particle)));
}

// 主机函数：释放设备内存
extern "C" void CleanupCUDA() {
    if (d_particles != nullptr) {
        cudaFree(d_particles);
        d_particles = nullptr;
    }
}