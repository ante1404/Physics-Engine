#include "Particles2D.h"
#include "Vector.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "raylib.h"

Particle2D *particles2d_create(int n, int width, int height){

    srand(time(NULL));   // reseed rand() from the current time so each run gets fresh values, not just each recompile

    Particle2D *p = malloc(n * sizeof(Particle2D));
    if (p == NULL)
    {
        return NULL;
    }

    particles2d_init_random(p, n, width, height);

    return p;
}

void particles2d_destroy(Particle2D *p){
    free(p);
}

void render_frame_2d(Particle2D *p, int n, int width, int height){

    InitWindow(width, height, "Particles 2D");

    const double FIXED_DT = 1.0 / 60.0;   // physics always advances in this exact chunk size
    double accumulator = 0.0;

    for (int i = 0; i < n; i++) {          // seed prevX/prevY so frame 1 doesn't interpolate from garbage
        p[i].prevX = p[i].x;
        p[i].prevY = p[i].y;
        //p[i].Initial.x = (float)p[i].prevX;
        //p[i].Initial.y = (float)p[i].prevY;
    }
    
    while (!WindowShouldClose()) {        // runs every frame until you hit ESC or close the window

        
        double frameTime = GetFrameTime();
        if (frameTime > 0.25) frameTime = 0.25;   // clamp huge stalls so physics doesn't try to "catch up" forever
        accumulator += frameTime;

        while (accumulator >= FIXED_DT) {
            for (int i = 0; i < n; i++) {          // remember "before" so we can blend toward "after" when rendering
                p[i].prevX = p[i].x;
                p[i].prevY = p[i].y;     
            }
            particles2d_step(p, n, FIXED_DT);
            particles2d_handle_walls(p, n, width, height);
            inelastic_collision2d(p, n);
            accumulator -= FIXED_DT;
        }

        double alpha = accumulator / FIXED_DT;   // 0..1, how far past the last completed step we currently are

        BeginDrawing();
        ClearBackground(BLACK);

        DrawRectangleLines(0, 0, width, height, WHITE);   // 2D stand-in for the 3D version's wireframe box

        for (int i = 0; i < n; i++)
        {
            double renderX = p[i].prevX + (p[i].x - p[i].prevX) * alpha;
            double renderY = p[i].prevY + (p[i].y - p[i].prevY) * alpha;

            // Physics uses y=0 as the FLOOR (gravity is negative, pulling vy/y toward 0),
            // but raylib's 2D screen space has y=0 at the TOP, increasing downward. Flip
            // here so "falling" reads as falling toward the bottom of the window, not the top.
            float screenX = (float)renderX;
            float screenY = (float)height - (float)renderY;

            // Fake contact shadow: flat dark ellipse at the floor line, directly under the
            // particle, fading out the higher the particle currently is. This is the 2D
            // stand-in for the 3D version's depth-fog cue -- a flat circle can't self-shade,
            // but a shadow that shrinks/fades with height still reads as "distance from the ground."
            float shadowAlpha = 1.0f - ((float)renderY / ((float)height * 0.6f));
            if (shadowAlpha < 0.0f) shadowAlpha = 0.0f;
            if (shadowAlpha > 0.6f) shadowAlpha = 0.6f;
            if (shadowAlpha > 0.0f) {
                DrawEllipse((int)screenX, height - 2,
                            (int)((float)p[i].radius * 1.1f), (int)((float)p[i].radius * 0.35f),
                            Fade(BLACK, shadowAlpha));
            }
            Vector2 staPos = p[i].Initial;
            
            float dx = (float)screenX - staPos.x;
            float dy = (float)screenY - staPos.y;
            float distance = sqrtf(dx * dx + dy * dy);

            Vector2 endPos = staPos; // fallback if distance is 0
            if (distance > 0.0f) {
                endPos.x = (float)screenX - (dx / distance) * (float)p[i].radius;
                endPos.y = (float)screenY - (dy / distance) * (float)p[i].radius;
            }

            DrawLineEx(staPos, endPos, 2.0f, RED);

            float angle = atan2f(dy, dx) * (180.0f / PI);
            DrawPoly(endPos, 3, 5.0, angle, RED);
            DrawCircle((int)screenX, (int)screenY, (float)p[i].radius, p[i].color);

            // Small offset highlight simulating an overhead light source -- the 2D stand-in
            // for the 3D version's specular term. A flat circle has no real surface normal to
            // shade, so this is a deliberate, cheap fake: a brighter dot offset toward where
            // the light would be (up and slightly to the left), same as the 3D light's position.
            float highlightRadius = (float)p[i].radius * 0.35f;
            float highlightOffset = (float)p[i].radius * 0.35f;
            DrawCircle((int)(screenX - highlightOffset), (int)(screenY - highlightOffset),
                       highlightRadius, Fade(WHITE, 0.5f));
        }

        EndDrawing();
    }

    CloseWindow();
    
}

void particles2d_handle_walls(Particle2D *p, int n, int width, int height){

    for (int i = 0; i < n; i++)
    {
        if ((p[i].x - p[i].radius < 0 && p[i].vx < 0) ||
            (p[i].x + p[i].radius >= width && p[i].vx > 0))
        {
            p[i].vx = -(p[i].vx);
        }
        // clamp position back inside the box regardless of velocity direction --
        // at high speed, a single tick can overshoot the wall by more than the
        // ball's own radius, and the velocity flip alone never corrects that.
        if (p[i].x - p[i].radius < 0)         p[i].x = p[i].radius;
        if (p[i].x + p[i].radius > width)      p[i].x = width - p[i].radius;

        // The ball has sunk `depth` past the wall by the time we notice. Clamping y back
        // without touching speed would add m*|a|*depth of free potential energy every bounce.
        // Energy conservation (0.5*v^2 - a*y = const) gives the speed it has at the wall itself.
        if (p[i].y - p[i].radius <= 0 && p[i].vy < 0)
        {
            double depth = p[i].radius - p[i].y;
            double v2 = p[i].vy * p[i].vy + 2.0 * a_g * depth;
            p[i].vy = sqrt(v2 > 0.0 ? v2 : 0.0);
        }
        else if (p[i].y + p[i].radius >= height && p[i].vy > 0)
        {
            double depth = p[i].y + p[i].radius - height;
            double v2 = p[i].vy * p[i].vy - 2.0 * a_g * depth;
            p[i].vy = -sqrt(v2 > 0.0 ? v2 : 0.0);
        }
        if (p[i].y - p[i].radius < 0)          p[i].y = p[i].radius;
        if (p[i].y + p[i].radius > height)      p[i].y = height - p[i].radius;
    }
}

void particles2d_step(Particle2D *p, int n, double dt){
    
    for (int i = 0; i < n; i++)
    {
        if (p[i].flags & FLAG_FROZEN) continue;
        p[i].x  += p[i].vx * dt;
        p[i].y  += p[i].vy * dt + 0.5 * a_g * dt * dt;
        p[i].vy += (a_g * dt);
    }
    
}

void particles2d_init_random(Particle2D *p, int n, int width, int height){

    const double MASS_PER_RADIUS = 1.5;   // bigger balls are proportionally heavier

    for (int i = 0; i < n; i++)
    {
        p[i].flags  = FLAG_ACTIVE;
        p[i].ax     = 0;
        p[i].ay     = 0;
        p[i].radius = (rand() % 16 + 5);
        p[i].mass   = p[i].radius * MASS_PER_RADIUS;

        int minX = (int)p[i].radius;
        int maxX = width  - (int)p[i].radius;
        int minY = (int)p[i].radius;
        int maxY = height - (int)p[i].radius;

        p[i].x = rand() % (maxX - minX + 1) + minX;
        p[i].y = rand() % (maxY - minY + 1) + minY;
        
        p[i].Initial.x = (float)p[i].x;
        p[i].Initial.y = ((float)height - (float)p[i].y);

        printf("Particle %d: Initial Position = (%f, %f)\n", i, p[i].Initial.x, p[i].Initial.y);
  
        //p[i].vx = (rand() % 16 - 10)*100;
        //p[i].vy = (rand() % 16 - 10)*100; 

        p[i].vx = 00;
        p[i].vy = 00;


        p[i].velocity = sqrt(p[i].vx*p[i].vx + p[i].vy*p[i].vy);

        Color palette[] = { RED, ORANGE, YELLOW, GREEN, SKYBLUE, PURPLE };
        p[i].color = palette[rand() % 6];
    }
}

void particle2d_collision(Particle2D *p, int n, int width, int height){
    (void)width;    // unused -- kept only so this signature matches the 3D version's shape
    (void)height;

    double dx, dy, distance, nx, ny, d1, d2, dvx, dvy, d;

    for (int i = 0; i < n; i++)
    {
        for (int j = i + 1; j < n; j++)
        {
            dx = p[i].x - p[j].x;
            dy = p[i].y - p[j].y;
            distance = sqrt(dx*dx + dy*dy);

            if (distance <= (p[i].radius + p[j].radius))
            {
                nx = dx / distance;
                ny = dy / distance;

                dvx = p[i].vx - p[j].vx;
                dvy = p[i].vy - p[j].vy;

                d = dvx*nx + dvy*ny;

                if (d < 0)   // only resolve while they're still closing; skip if already separating
                {
                    d1 = 2.0 * p[j].mass / (p[i].mass + p[j].mass);
                    d2 = 2.0 * p[i].mass / (p[i].mass + p[j].mass);

                    p[i].vx = p[i].vx - d1 * d * nx;
                    p[i].vy = p[i].vy - d1 * d * ny;

                    p[j].vx = p[j].vx + d2 * d * nx;
                    p[j].vy = p[j].vy + d2 * d * ny;
                }
            }
        }
    }
}

double Restitution_coefficient(double v1_x, double v2_x, double v1_y, double v2_y){

    double v_0 = 0.3;
    double r_o = 0.95;

    double v_rel = sqrt(pow(v2_x - v1_x, 2) + pow(v2_y - v1_y, 2));
    if (0 < v_rel && v_rel <= v_0)
    {
        return (1-(1-r_o)*pow((v_rel/v_0),1.0/5.0));
    }
    else if (v_rel > v_0)
    {
        return (r_o*pow((v_rel/v_0),-1.0/4.0));
    }
    
    return 0;
}


void inelastic_collision2d(Particle2D *p, int n){

    double dx, dy, distance, nx, ny, dvx, dvy, d, tx, ty;

    double Px, Py;

    double r, Delta_KE;
    for (int i = 0; i < n; i++)
    {
        for (int j = i + 1; j < n; j++)
        {
            
            dx = p[i].x - p[j].x;
            dy = p[i].y - p[j].y;
            distance = sqrt(dx*dx + dy*dy);

            if (distance <= (p[i].radius + p[j].radius))
            {
                nx = dx / distance;
                ny = dy / distance;

                tx = -dy/distance;
                ty = dx/distance;

                // Positional correction: push the two balls apart along the normal by
                // however much they're currently overlapping, mass-weighted so the
                // heavier one moves less. This runs regardless of approach/separation
                // (unlike the velocity fix below) because penetration is a pure geometry
                // problem -- at high speed a single tick can drive them deep into each
                // other, and nothing here corrects position without this.
                double overlap = (p[i].radius + p[j].radius) - distance;
                if (overlap > 0)
                {
                    double totalMass = p[i].mass + p[j].mass;
                    double correction_i = overlap * (p[j].mass / totalMass);
                    double correction_j = overlap * (p[i].mass / totalMass);

                    p[i].x += nx * correction_i;
                    p[i].y += ny * correction_i;
                    p[j].x -= nx * correction_j;
                    p[j].y -= ny * correction_j;
                }

                dvx = p[i].vx - p[j].vx;
                dvy = p[i].vy - p[j].vy;

                d = dvx*nx + dvy*ny;

                double v_ii,v_ij;
                double vt_i, vt_j;   // each particle's OWN tangential speed (vx*tx + vy*ty) -- frictionless contact leaves this unchanged per-particle

                Px = p[i].mass*p[i].vx + p[j].mass*p[j].vx;
                Py = p[i].mass*p[i].vy + p[j].mass*p[j].vy;

                r = Restitution_coefficient(p[i].vx, p[j].vx, p[i].vy, p[j].vy);
                Delta_KE = 0.5 * (p[i].mass*p[j].mass/(p[i].mass+p[j].mass)) * d*d * (1 - r*r);

                v_ii = p[i].vx;
                v_ij = p[i].vy;

                vt_i = v_ii*tx + v_ij*ty;               // uses the pre-update snapshot, same reason v_ii/v_ij exist
                vt_j = p[j].vx*tx + p[j].vy*ty;         // p[j] hasn't been written yet at this point, so this is still the original velocity

                if (d < 0)   // only resolve while they're still closing; skip if already separating
                {
                    p[i].vx = (((Px*nx+Py*ny)/(p[i].mass + p[j].mass)) - sqrt(4*pow(p[i].mass, 2)*pow(p[j].mass, 2)*pow(d,2) - 8*p[i].mass*p[j].mass * (p[j].mass + p[i].mass) * Delta_KE)/(2*p[i].mass*(p[i].mass+p[j].mass)))*nx + vt_i*tx;
                    p[i].vy = (((Px*nx+Py*ny)/(p[i].mass + p[j].mass)) - sqrt(4*pow(p[i].mass, 2)*pow(p[j].mass, 2)*pow(d,2) - 8*p[i].mass*p[j].mass * (p[j].mass + p[i].mass) * Delta_KE)/(2*p[i].mass*(p[i].mass+p[j].mass)))*ny + vt_i*ty;

                    p[j].vx = (((Px*nx+Py*ny)/(p[i].mass + p[j].mass)) + sqrt(4*pow(p[i].mass, 2)*pow(p[j].mass, 2)*pow(d,2) - 8*p[i].mass*p[j].mass * (p[j].mass + p[i].mass) * Delta_KE)/(2*p[j].mass*(p[i].mass+p[j].mass)))*nx + vt_j*tx;
                    p[j].vy = (((Px*nx+Py*ny)/(p[i].mass + p[j].mass)) + sqrt(4*pow(p[i].mass, 2)*pow(p[j].mass, 2)*pow(d,2) - 8*p[i].mass*p[j].mass * (p[j].mass + p[i].mass) * Delta_KE)/(2*p[j].mass*(p[i].mass+p[j].mass)))*ny + vt_j*ty;
                }
            }
        }
    }
}