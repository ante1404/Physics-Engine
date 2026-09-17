#include "Particles3D.h"
#include "Particles2D.h"
#include <stdio.h>

int main(){

    Particle3D *p = particles_create3D(90);
    render_frame3D(p, 80, 1000, 1000, 1000); 
    particles_destroy3D(p);

    for (int i = 0; i < 80; i++)
    {
        printf("test\n");
    }
    return 0;
}
