#include "Particles3D.h"
#include "Particles2D.h"
#include <stdio.h>

int main(){

    Particle2D *p = particles2d_create(80, 1000, 1000);
    render_frame_2d(p, 80, 1000, 1000);

    /*
    double biggest = 0, temp;
    for (int i = 0; i < 10; i++) {
    printf("Particle %d:\n", i);
    printf("  Position:   x = %f, y = %f\n", p[i].x, p[i].y);
    printf("  Velocity:   vx = %f, vy = %f\n", p[i].vx, p[i].vy);
    printf("  Radius:     %f\n", p[i].radius);
    printf("  Mass:       %f\n", p[i].mass);
    printf("  Accel:      ax = %f, ay = %f\n", p[i].ax, p[i].ay);
    printf("  Prev Pos:   x = %f, y = %f\n", p[i].prevX, p[i].prevY);
    printf("  Flags:      %u\n", p[i].flags);
    printf("  Velocity:   %f\n", p[i].velocity);
    printf("\n");

    
    if (p[i].velocity > biggest)
    {
        biggest = p[i].velocity;
    }
}
    printf("%.3f\n", biggest);
    */
    return 0;
}
