// particle.h
#ifndef PARTICLE_H_
#define PARTICLE_H_

#define FLAG_ACTIVE (1u << 0)
#define FLAG_FROZEN (1u << 1)

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "raylib.h"

#define a_g 9.86
typedef struct {
    double x, y, z;     /* position */
    double vx, vy, vz;    /* velocity */
    double radius;
    double mass;
    double ax, ay, az;
    double prevX, prevY, PrevZ;   /* position before the last physics step, for render interpolation */
    unsigned flags;
} Particle;

Particle *particles_create(int n);
void      particles_destroy(Particle *p);
void      particles_init_random(Particle *p, int n, int width, int length, int height);
void      particles_step(Particle *p, int n, double dt);
void      particles_handle_walls(Particle *p, int n, int width, int length, int height);
void      render_frame(Particle *p, int n, int width, int length, int height);
void      particle_collision(Particle *p, int n, int width, int length, int height);

#endif