// particle.h
#ifndef PARTICLE_H_
#define PARTICLE_H_

#define FLAG_ACTIVE (1u << 0)
#define FLAG_FROZEN (1u << 1)

#include "raylib.h"

#define a_g -9.86*100 
typedef struct {
    double x, y, z;     /* position */
    double vx, vy, vz;    /* velocity */
    double radius;
    double mass;
    double ax, ay, az;
    double prevX, prevY, prevZ;   /* position before the last physics step, for render interpolation */
    unsigned flags;
    Color color;
} Particle3D;

Particle3D *particles_create3D(int n);
void      particles_destroy3D(Particle3D *p);
void      particles_init_random3D(Particle3D *p, int n, int width, int length, int height);
void      particles_step3D(Particle3D *p, int n, double dt);
void      particles_handle_walls3D(Particle3D *p, int n, int width, int length, int height);
void      render_frame3D(Particle3D *p, int n, int width, int length, int height);
void      particle_collision3D(Particle3D *p, int n, int width, int length, int height);

#endif