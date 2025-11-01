#version 430

layout(local_size_x = 256) in;

struct Ant {
    vec2 pos;
    vec2 vel;
    int carryingFood;
    float pad;
};

layout(std430, binding = 0) buffer AntBuffer {
    Ant ants[];
};

layout(r32f, binding = 1) uniform image2D pheromone;

uniform float dt;
uniform float maxSpeed;
uniform int gridSize;

const float BOUNDS = 25.0;
const float DETECTION_RADIUS = 1.0;
const float PHEROMONE_DECAY = 0.999;
const float PHEROMONE_DEPOSIT = 0.01;
const float SENSE_ANGLE = 0.4;
const float SENSE_DIST = 0.5;
const vec2 homePos = vec2(0.0, 0.0);
const vec2 foodPos = vec2(10.0, 10.0);

float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

float samplePheromone(vec2 pos) {
    ivec2 grid = ivec2((pos + BOUNDS) / (2.0 * BOUNDS) * gridSize);
    if (grid.x < 0 || grid.x >= gridSize || grid.y < 0 || grid.y >= gridSize)
        return 0.0;
    return imageLoad(pheromone, grid).r;
}

void main() {
    uint i = gl_GlobalInvocationID.x;
    if (i >= ants.length()) return;

    Ant a = ants[i];
    float angle = atan(a.vel.y, a.vel.x);
    /*if (a.pos.x < -BOUNDS) { a.pos.x = -BOUNDS; a.vel.x *= -1; }
    if (a.pos.x > BOUNDS) { a.pos.x = BOUNDS; a.vel.x *= -1; }
    if (a.pos.y < -BOUNDS) { a.pos.y = -BOUNDS; a.vel.y *= -1; }
    if (a.pos.y > BOUNDS) { a.pos.y = BOUNDS; a.vel.y *= -1; }*/

    float dFood = length(a.pos - foodPos);
    float dHome = length(a.pos - homePos);

    if (a.carryingFood == 0 && dFood < DETECTION_RADIUS) {
        a.carryingFood = 1;
        angle += 3.14159;
    } 
    else if (a.carryingFood == 1 && dHome < DETECTION_RADIUS) {
        a.carryingFood = 0;
        angle += 3.14159;
    }

    vec2 forward = vec2(cos(angle), sin(angle));
    vec2 left = vec2(cos(angle + SENSE_ANGLE), sin(angle + SENSE_ANGLE));
    vec2 right = vec2(cos(angle - SENSE_ANGLE), sin(angle - SENSE_ANGLE));

    float ahead = samplePheromone(a.pos + forward * SENSE_DIST);
    float lft = samplePheromone(a.pos + left * SENSE_DIST);
    float rightward = samplePheromone(a.pos + right * SENSE_DIST);

    if (ahead > lft && ahead > rightward) {
        angle += (rand(a.pos) - 0.5) * 0.1;
    } else if (lft > rightward) {
        angle += 0.3;
    } else if (rightward > lft) {
        angle -= 0.3;
    } else {
        angle += (rand(a.pos) - 0.5) * 0.5;
    }

    a.vel = vec2(cos(angle), sin(angle)) * maxSpeed * 0.1;
    a.pos += a.vel * dt;

    ivec2 gridPos = ivec2((a.pos + BOUNDS) / (2.0 * BOUNDS) * gridSize);
    if (gridPos.x >= 0 && gridPos.x < gridSize && gridPos.y >= 0 && gridPos.y < gridSize)
    {
        float old = imageLoad(pheromone, gridPos).r;
        imageStore(pheromone, gridPos, vec4(clamp(old + PHEROMONE_DEPOSIT, 0.0, 1.0), 0.0, 0.0, 1.0));
    }

    if (a.pos.x < -BOUNDS) { a.pos.x = -BOUNDS; a.vel.x *= -1; }
    if (a.pos.x >  BOUNDS) { a.pos.x =  BOUNDS; a.vel.x *= -1; }
    if (a.pos.y < -BOUNDS) { a.pos.y = -BOUNDS; a.vel.y *= -1; }
    if (a.pos.y >  BOUNDS) { a.pos.y =  BOUNDS; a.vel.y *= -1; }

    ants[i] = a;

    //if (i < gridSize * gridSize) {
    //    ivec2 g = ivec2(i % gridSize, i / gridSize); //shit?
    //    //float p = imageLoad(pheromone, g).r * PHEROMONE_DECAY;
    //    //imageStore(pheromone, g, vec4(p, 0.0, 0.0, 1.0));
    //}
}
