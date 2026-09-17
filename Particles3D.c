#include "Particles3D.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "raylib.h"

Particle3D *particles_create3D(int n){

    srand(time(NULL));   // reseed rand() from the current time so each run gets fresh values, not just each recompile

    Particle3D *p = malloc(n * sizeof(Particle3D));
    if (p == NULL)
    {
        return NULL;
    }

    particles_init_random3D(p, n, 0,0,0);

    return p;
}

void render_frame3D(Particle3D *p, int n, int width, int length, int height){

    Camera3D camera = { 0 };
    camera.position   = (Vector3){ 1000.0f, 1000.0f, 1000.0f }; // where the "eye" sits in 3D space
    camera.target     = (Vector3){ 500.0f, 500.0f, 500.0f };     // point the camera looks toward
    camera.up         = (Vector3){ 0.0f, 0.0f, 1.0f };     // which world direction is "up"
    camera.fovy       = 45.0f;                              // field of view, degrees
    camera.projection = CAMERA_PERSPECTIVE;

    InitWindow(width, height, "title");

    // ---- lighting/material setup (embedded GLSL, no external asset files needed) ----

    const char *vsCode =
        "#version 330\n"
        "in vec3 vertexPosition;\n"
        "in vec2 vertexTexCoord;\n"
        "in vec3 vertexNormal;\n"
        "uniform mat4 mvp;\n"
        "uniform mat4 matModel;\n"
        "uniform mat4 matNormal;\n"
        "out vec3 fragPosition;\n"
        "out vec2 fragTexCoord;\n"
        "out vec3 fragNormal;\n"
        "void main()\n"
        "{\n"
        "    fragPosition = vec3(matModel*vec4(vertexPosition, 1.0));\n"
        "    fragTexCoord = vertexTexCoord;\n"
        "    fragNormal = normalize(vec3(matNormal*vec4(vertexNormal, 1.0)));\n"
        "    gl_Position = mvp*vec4(vertexPosition, 1.0);\n"
        "}\n";

    const char *fsCode =
        "#version 330\n"
        "in vec3 fragPosition;\n"
        "in vec2 fragTexCoord;\n"
        "in vec3 fragNormal;\n"
        "uniform sampler2D texture0;\n"
        "uniform vec4 colDiffuse;\n"
        "uniform vec3 lightPos;\n"
        "uniform vec3 viewPos;\n"
        "uniform vec4 ambientColor;\n"
        "out vec4 finalColor;\n"
        "void main()\n"
        "{\n"
        "    vec4 texelColor = texture(texture0, fragTexCoord);\n"
        "    vec3 normal = normalize(fragNormal);\n"
        "    vec3 lightDir = normalize(lightPos - fragPosition);\n"
        "    float diff = max(dot(normal, lightDir), 0.0);\n"
        "    vec3 viewDir = normalize(viewPos - fragPosition);\n"
        "    vec3 reflectDir = reflect(-lightDir, normal);\n"
        "    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);\n"
        "    vec3 diffuse  = diff*colDiffuse.rgb;\n"
        "    vec3 specular = spec*vec3(1.0);\n"
        "    vec3 ambient  = ambientColor.rgb;\n"
        "    vec3 litColor = (ambient + diffuse + specular)*texelColor.rgb;\n"
        "    float dist = length(viewPos - fragPosition);\n"
        "    float fogFactor = clamp(dist/2500.0, 0.0, 0.6);\n"
        "    litColor = mix(litColor, vec3(0.0), fogFactor);\n"
        "    finalColor = vec4(litColor, texelColor.a*colDiffuse.a);\n"
        "}\n";

    Shader lightShader = LoadShaderFromMemory(vsCode, fsCode);

    lightShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(lightShader, "matModel");
    lightShader.locs[SHADER_LOC_VECTOR_VIEW]  = GetShaderLocation(lightShader, "viewPos");

    int lightPosLoc     = GetShaderLocation(lightShader, "lightPos");
    int ambientColorLoc = GetShaderLocation(lightShader, "ambientColor");

    Vector3 lightPos     = (Vector3){ width * 0.5f, length * 0.5f, height * 1.5f };  // above the box
    Vector4 ambientColor = (Vector4){ 0.15f, 0.15f, 0.18f, 1.0f };                    // dim fill light so shadowed sides aren't pure black
    SetShaderValue(lightShader, lightPosLoc, &lightPos, SHADER_UNIFORM_VEC3);
    SetShaderValue(lightShader, ambientColorLoc, &ambientColor, SHADER_UNIFORM_VEC4);

    // procedural checker texture -- generated in memory, no external image file needed
    Image checkerImg = GenImageChecked(64, 64, 8, 8, (Color){230,230,230,255}, (Color){60,60,60,255});
    Texture2D checkerTex = LoadTextureFromImage(checkerImg);
    UnloadImage(checkerImg);
    GenTextureMipmaps(&checkerTex);
    SetTextureFilter(checkerTex, TEXTURE_FILTER_TRILINEAR);

    // one unit sphere mesh, reused for every particle (DrawModel scales it by radius per-call)
    Mesh sphereMesh   = GenMeshSphere(1.0f, 16, 16);
    Model sphereModel = LoadModelFromMesh(sphereMesh);
    sphereModel.materials[0].shader = lightShader;
    sphereModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = checkerTex;

    const double FIXED_DT = 1.0 / 60.0;   // physics always advances in this exact chunk size
    double accumulator = 0.0;

    for (int i = 0; i < n; i++) {          // seed prevX/prevY so frame 1 doesn't interpolate from garbage
        p[i].prevX = p[i].x;
        p[i].prevY = p[i].y;
        p[i].prevZ = p[i].z;
    }

    // Manual orbit camera: drag with left mouse button to rotate, scroll to zoom.
    // We don't use raylib's UpdateCamera(CAMERA_FREE)/DisableCursor combo because it
    // rotates from mere mouse movement (no click needed) and its cursor-lock behavior
    // is unreliable over WSLg's forwarded display. This version only reacts while the
    // button is actually held, using frame-to-frame mouse delta, so it needs no cursor lock at all.
    double cameraYaw      = 45.0;    // degrees, rotation around the vertical (Z) axis
    double cameraPitch    = 30.0;    // degrees, tilt up/down
    double cameraDistance = 1200.0;  // distance from the target point
    Vector3 orbitTarget   = (Vector3){ width / 2.0f, length / 2.0f, height / 2.0f };  // center of the box

    while (!WindowShouldClose()) {        // runs every frame until you hit ESC or close the window

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 mouseDelta = GetMouseDelta();      // pixels moved since last frame, only meaningful while dragging
            cameraYaw   -= mouseDelta.x * 0.3;
            cameraPitch += mouseDelta.y * 0.3;
            if (cameraPitch > 89.0)  cameraPitch = 89.0;   // clamp so it can't flip past straight up/down
            if (cameraPitch < -89.0) cameraPitch = -89.0;
        }

        cameraDistance -= GetMouseWheelMove() * 40.0;      // scroll wheel zooms in/out
        if (cameraDistance < 50.0) cameraDistance = 50.0;  // clamp so you can't zoom past the target

        double yawRad   = cameraYaw   * DEG2RAD;
        double pitchRad = cameraPitch * DEG2RAD;

        camera.target   = orbitTarget;
        camera.position = (Vector3){
            orbitTarget.x + cameraDistance * cos(pitchRad) * cos(yawRad),
            orbitTarget.y + cameraDistance * cos(pitchRad) * sin(yawRad),
            orbitTarget.z + cameraDistance * sin(pitchRad)
        };

        // the shader's specular highlight depends on where the eye is, so this needs updating every frame
        float cameraPosArr[3] = { camera.position.x, camera.position.y, camera.position.z };
        SetShaderValue(lightShader, lightShader.locs[SHADER_LOC_VECTOR_VIEW], cameraPosArr, SHADER_UNIFORM_VEC3);

        double frameTime = GetFrameTime();
        if (frameTime > 0.25) frameTime = 0.25;   // clamp huge stalls (window drag, breakpoint, etc.) so physics doesn't try to "catch up" forever
        accumulator += frameTime;

        while (accumulator >= FIXED_DT) {
            for (int i = 0; i < n; i++) {          // remember "before" so we can blend toward "after" when rendering
                p[i].prevX = p[i].x;
                p[i].prevY = p[i].y;
                p[i].prevZ = p[i].z;
            }
            particles_step3D(p, n, FIXED_DT);
            particles_handle_walls3D(p, n, width, length, height);
            inelastic_collision3d(p, n);   // MERGE NOTE: switched from particle_collision3D per the Downloads copy -- confirm this is the intended active model
            accumulator -= FIXED_DT;
        }

        double alpha = accumulator / FIXED_DT;   // 0..1, how far past the last completed step we currently are

        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode3D(camera);
        DrawCubeWires((Vector3){ width/2.0f, length/2.0f, height/2.0f }, width, length, height, WHITE);
            for (int i = 0; i < n; i++)
            {
               double renderX = p[i].prevX + (p[i].x - p[i].prevX) * alpha;
               double renderY = p[i].prevY + (p[i].y - p[i].prevY) * alpha;
               double renderZ = p[i].prevZ + (p[i].z - p[i].prevZ) * alpha;
               DrawModel(sphereModel, (Vector3){ renderX, renderY, renderZ }, p[i].radius, p[i].color);
            }
        EndMode3D();                     // "done describing this frame" — raylib actually flips it to screen here
        EndDrawing();
    }

    // Confirmed against the actual installed raylib source (rmodels.c): UnloadModel does NOT
    // free a material's shader or its textures -- it only frees the maps array, on the
    // assumption that a shader/texture might be shared across multiple models. So all three
    // calls below are required; there is no double-free risk between them.
    UnloadModel(sphereModel);
    UnloadShader(lightShader);
    UnloadTexture(checkerTex);

    CloseWindow();                        // cleanup once the loop exits
}

void particles_handle_walls3D(Particle3D *p, int n, int width, int length, int height){

    for (int i = 0; i < n; i++)
    {
        if ((p[i].x - p[i].radius < 0 && p[i].vx < 0) ||
            (p[i].x + p[i].radius >= width && p[i].vx > 0))
        {
            p[i].vx = -(p[i].vx);
        }

        if (p[i].x - p[i].radius < 0)         p[i].x = p[i].radius;
        if (p[i].x + p[i].radius > width)      p[i].x = width - p[i].radius;

        if ((p[i].y - p[i].radius <= 0 && p[i].vy < 0) ||
            (p[i].y + p[i].radius >= length && p[i].vy > 0))
        {
            p[i].vy = -(p[i].vy);
        }

        if (p[i].y - p[i].radius < 0)         p[i].y = p[i].radius;
        if (p[i].y + p[i].radius > length)      p[i].y = length - p[i].radius;

        if ((p[i].z - p[i].radius < 0 && p[i].vz < 0) ||
            (p[i].z + p[i].radius >= height && p[i].vz > 0))
        {
            p[i].vz = -(p[i].vz);
        }

        if (p[i].z - p[i].radius < 0)         p[i].z = p[i].radius;
        if (p[i].z + p[i].radius > height)      p[i].z = height - p[i].radius;

    }

}

void particles_step3D(Particle3D *p, int n, double dt){
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

void particles_init_random3D(Particle3D *p, int n, int width, int length, int height){

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
        p[i].vx = (rand() % 16 - 10)*100;
        p[i].vy = (rand() % 16 - 10)*100;
        p[i].vz = (rand() % 16 - 10)*100;
        p[i].y = rand() % (max - min + 1) + min;
        p[i].x = rand() % (max - min + 1) + min;
        p[i].z = rand() % (max - min + 1) + min;
        // in particles_init_random, per particle:
        Color palette[] = { RED, ORANGE, YELLOW, GREEN, SKYBLUE, PURPLE };
        p[i].color = palette[rand() % 6];
    }
}

void particle_collision3D(Particle3D *p, int n, int width, int length, int height){

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

                d = dvx*nx + dvz*nz + dvy*ny;

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

double Restitution_coefficient3D(double v1_x, double v2_x, double v1_y, double v2_y, double v1_z, double v2_z){

    double v_0 = 0.3;
    double r_o = 0.95;

    double v_rel = sqrt(pow(v2_x - v1_x, 2) + pow(v2_y -  v1_y, 2) + pow(v2_z - v1_z, 2));
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

void inelastic_collision3d(Particle3D *p, int n){

    double dx, dy, dz, distance, nx, ny, nz, dvx, dvy, dvz, d;

    double Px, Py, Pz;

    double r, Delta_KE;
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
                nx = dx / distance;
                ny = dy / distance;
                nz = dz / distance;

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
                    p[i].z += nz * correction_i;
                    p[j].x -= nx * correction_j;
                    p[j].y -= ny * correction_j;
                    p[j].z -= nz * correction_j;
                }

                dvx = p[i].vx - p[j].vx;
                dvy = p[i].vy - p[j].vy;
                dvz = p[i].vz - p[j].vz;

                d = dvx*nx + dvy*ny + dvz*nz;

                // 3D has no single perpendicular direction the way 2D does -- the tangent
                // plane is itself 2D, so there is no unique (tx,ty,tz) to project onto.
                // Instead keep each particle's tangential motion as a whole vector,
                // v_t = v - (v.n)n. Frictionless contact leaves that vector untouched,
                // which is exactly what vt_i*tx / vt_i*ty accomplishes in the 2D version.
                double v_in = p[i].vx*nx + p[i].vy*ny + p[i].vz*nz;   // pre-update snapshot, same reason the 2D version snapshots v_ii/v_ij
                double v_jn = p[j].vx*nx + p[j].vy*ny + p[j].vz*nz;   // p[j] has not been written yet here, so this is still the original velocity

                double vt_ix = p[i].vx - v_in*nx;
                double vt_iy = p[i].vy - v_in*ny;
                double vt_iz = p[i].vz - v_in*nz;

                double vt_jx = p[j].vx - v_jn*nx;
                double vt_jy = p[j].vy - v_jn*ny;
                double vt_jz = p[j].vz - v_jn*nz;

                Px = p[i].mass*p[i].vx + p[j].mass*p[j].vx;
                Py = p[i].mass*p[i].vy + p[j].mass*p[j].vy;
                Pz = p[i].mass*p[i].vz + p[j].mass*p[j].vz;

                r = Restitution_coefficient3D(p[i].vx, p[j].vx, p[i].vy, p[j].vy, p[i].vz, p[j].vz);
                Delta_KE = 0.5 * (p[i].mass*p[j].mass/(p[i].mass+p[j].mass)) * d*d * (1 - r*r);

                if (d < 0)   // only resolve while they're still closing; skip if already separating
                {
                    // hoisted out of the six assignments below -- the 2D version repeats this
                    // inline, but in 3D that would be six copies of the same expression.
                    double Pn   = Px*nx + Py*ny + Pz*nz;
                    double root = sqrt(4*pow(p[i].mass, 2)*pow(p[j].mass, 2)*pow(d,2) - 8*p[i].mass*p[j].mass * (p[j].mass + p[i].mass) * Delta_KE);

                    double vn_i = (Pn/(p[i].mass + p[j].mass)) - root/(2*p[i].mass*(p[i].mass+p[j].mass));
                    double vn_j = (Pn/(p[i].mass + p[j].mass)) + root/(2*p[j].mass*(p[i].mass+p[j].mass));

                    p[i].vx = vn_i*nx + vt_ix;
                    p[i].vy = vn_i*ny + vt_iy;
                    p[i].vz = vn_i*nz + vt_iz;

                    p[j].vx = vn_j*nx + vt_jx;
                    p[j].vy = vn_j*ny + vt_jy;
                    p[j].vz = vn_j*nz + vt_jz;
                }
            }
        }
    }
}

void particles_destroy3D(Particle3D *p){


    free(p);

}
