#include "Particles2D.h"
#include "Vector.h"
#include <stdio.h>  

Vector2 Position_vector2D(Particle2D *p){

    Vector2 d;

    d.y = (float)p->y;
    d.x = (float)p->x;
    

    return d;

}