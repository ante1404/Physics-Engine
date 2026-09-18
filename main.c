#include "Particles3D.h"
#include "Particles2D.h"
#include <stdio.h>

int main(){

    Particle2D *p = NULL;
    /*p = particles_create3D(90);
    render_frame3D(p, 80, 1000, 1000, 1000); 
    particles_destroy3D(p);
    */
    p = particles2d_create(1,1000,1000);
    render_frame_2d(p, 1, 1000,1000);
    
    return 0;
}
