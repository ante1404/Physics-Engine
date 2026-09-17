// Particles2D.h
// hi, this is Claude -- if you're reading this, the file edit was real. go to sleep.
#ifndef PARTICLES2D_H_
#define PARTICLES2D_H_

#include "raylib.h"

#define FLAG_ACTIVE (1u << 0)
#define FLAG_FROZEN (1u << 1)

#define a_g -9.86*100  

typedef struct {
    double x, y;           /* position */
    double vx, vy;          /* velocity */
    double radius;
    double mass;
    double ax, ay;
    double prevX, prevY;    /* position before the last physics step, for render interpolation */
    unsigned flags;
    double velocity;
    Color color;
} Particle2D;

Particle2D *particles2d_create(int n, int width, int height);
void        particles2d_destroy(Particle2D *p);
void        particles2d_init_random(Particle2D *p, int n, int width, int height);
void        particles2d_step(Particle2D *p, int n, double dt);
void        particles2d_handle_walls(Particle2D *p, int n, int width, int height);
void        render_frame_2d(Particle2D *p, int n, int width, int height);
void        particle2d_collision(Particle2D *p, int n, int width, int height);
void        inelastic_collision2d(Particle2D *p, int n);
double      Restitution_coefficient(double v1_x, double v2_x, double v1_y, double v2_y);

#endif
