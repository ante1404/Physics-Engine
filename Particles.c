#include "Particles.h"



/*
int main(){
    int n = 80;

    Particle *p = particles_create(n);
    for (int i = 0; i < n; i++)
    {
        printf("%f, %f, %f, %f, %f, %f\n,", p[i].mass, p[i].radius, p[i].vx, p[i].vy, p[i].x, p[i].y);
    }
    
    printf("%.2f, %.2f\n", p->vx, p->vy);
    render_frame(p,n , 1000, 1000, 1000);
    return 0;
}
*/
Particle *particles_create(int n){

    Particle *p = malloc(n * sizeof(Particle));
    if (p == NULL)
    {
        return NULL;
    }
    
    particles_init_random(p, n, 0,0,0);

    return p;
}

void render_frame(Particle *p, int n, int width, int length, int height){

    InitWindow(width, height, "title");   // opens the window once
    SetTargetFPS(60);

    const double FIXED_DT = 1.0 / 60.0;   // physics always advances in this exact chunk size
    double accumulator = 0.0;

    for (int i = 0; i < n; i++) {          // seed prevX/prevY so frame 1 doesn't interpolate from garbage
        p[i].prevX = p[i].x;
        p[i].prevY = p[i].y;
    }

    while (!WindowShouldClose()) {        // runs every frame until you hit ESC or close the window
        double frameTime = GetFrameTime();
        if (frameTime > 0.25) frameTime = 0.25;   // clamp huge stalls (window drag, breakpoint, etc.) so physics doesn't try to "catch up" forever
        accumulator += frameTime;

        while (accumulator >= FIXED_DT) {
            for (int i = 0; i < n; i++) {          // remember "before" so we can blend toward "after" when rendering
                p[i].prevX = p[i].x;
                p[i].prevY = p[i].y;
            }
            particles_step(p, n, FIXED_DT);
            particles_handle_walls(p, n, width, length, height);
            particle_collision(p, n, width, length, height);
            accumulator -= FIXED_DT;
        }

        double alpha = accumulator / FIXED_DT;   // 0..1, how far past the last completed step we currently are

        BeginDrawing();                   // "start describing this frame"
            ClearBackground(BLACK);       // fills the whole frame with black — this is your background
            for (int i = 0; i < n; i++)
            {
               double renderX = p[i].prevX + (p[i].x - p[i].prevX) * alpha;
               double renderY = p[i].prevY + (p[i].y - p[i].prevY) * alpha;
               DrawCircle(renderX, renderY, p[i].radius, RED);
            }
            // draws one filled circle at (x, y) — this is your particle
        EndDrawing();                     // "done describing this frame" — raylib actually flips it to screen here
    }

CloseWindow();                        // cleanup once the loop exits


}

void particles_handle_walls(Particle *p, int n, int width, int length, int height){

    for (int i = 0; i < n; i++)
    {
        if ((p[i].x - p[i].radius < 0 && p[i].vx < 0) ||
            (p[i].x + p[i].radius >= width && p[i].vx > 0))
        {
            p[i].vx = -(p[i].vx);
        }
        if ((p[i].y - p[i].radius <= 0 && p[i].vy < 0) ||
            (p[i].y + p[i].radius >= height && p[i].vy > 0))
        {
            p[i].vy = -(p[i].vy);
        }
        if ((p[i].z - p[i].radius < 0 && p[i].vz < 0) || 
            (p[i].z + p[i].radius >= height && p[i].vz > 0))
        {
            p[i].vz = -(p[i].vz);
        }
        
    }

}

void particles_step(Particle *p, int n, double dt){
    for (int i = 0; i < n; i++)
    {
        if (p[i].flags == FLAG_FROZEN) { }
        else{
            p[i].vz += a_g*dt;
            p[i].x += p[i].vx * dt;     
            p[i].y += p[i].vy * dt;    
            p[i].z += p[i].vz * dt;  
        }
    }
}

void particles_init_random(Particle *p, int n, int width, int length, int height){

    const double MASS_PER_RADIUS = 1.5;   // bigger balls are proportionally heavier

    for (int i = 0; i < n; i++)
    {

        p[i].flags = FLAG_ACTIVE;
        p[i].ax = 0;
        p[i].ay = 0;
        p[i].az = a_g;
        p[i].radius = rand() %16 + 5;
        p[i].mass = p[i].radius * MASS_PER_RADIUS;
        int min = 0+p[i].radius;
        int max = 1000 - p[i].radius;
        p[i].vx = rand() % 16 - 10;
        p[i].vy = rand() % 16 - 10;
        p[i].vz = rand() % 16 - 10;
        p[i].vx = -40;
        p[i].vy = 100;
        p[i].vz = 40;
        p[i].y = rand() % (max - min + 1) + min;
        p[i].x = rand() % (max - min + 1) + min;
        p[i].z = rand() % (max - min + 1) + min;
    }
}

void particle_collision(Particle *p, int n, int width, int length, int height){

    double dx,dy,dz,distance, nx, ny, nz,d1, d2, d3,dvx, dvy, dvz, d;

    for (int i = 0; i < n; i++)
    {
        for (int j = i + 1; j < n; j++)
        {
            dx = p[i].x - p[j].x;
            dy = p[i].y - p[j].y;
            dz = p[i].z - p[j].z;
            distance = sqrt(dx*dx + dy*dy + dz*dz);
            if (distance <= (p[i].radius + p[j].radius))
            {
                nx = dx/distance;
                ny = dy/distance;
                nz = dz/distance;

                dvx = p[i].vx - p[j].vx;
                dvy = p[i].vy - p[j].vy;
                dvz = p[i].vz - p[j].vz;

                d = dvx*nx + dvz*nz + dvz*nz;

                if (d < 0)   // only resolve while they're still closing; skip if already separating
                {
                    printf("collision\n");

                    d1 = 2.0 * p[j].mass / (p[i].mass + p[j].mass);
                    d2 = 2.0 * p[i].mass / (p[i].mass + p[j].mass);

                    p[i].vx = p[i].vx - d1 * d * nx;
                    p[i].vy = p[i].vy - d1 * d * ny;
                    p[i].vz = p[i].vz - d1 * d * nz;

                    p[j].vx = p[j].vx + d2 * d * nx;
                    p[j].vy = p[j].vy + d2 * d * ny;
                    p[j].vz = p[j].vz + d2 * d * nz;
                }
            }
            
        }
        
    }
}