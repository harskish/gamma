// Müller's poly6 kernel
inline float WPoly6(float r, float h) {
    const float K = (315.0f / (64.0f * 3.14159265f * pow(h, 9.0f)));
    return K * pow(max(0.0f, h*h - r*r), 3.0f); // contributes if r < h (assuming r,h > 0)
}

// Gradient of Müller's spiky kernel
inline float gradWSpiky(float r, float h) {
    const float K = -(45.0f / (3.14159265f * pow(h, 6.0f)));
    return K * pow(max(0.0f, h - r), 2.0f);
}

// Müller's viscosity kernel
inline float WViscosity(float r, float h) {
    if (r > h)
        return 0.0f;

    const float r2 = r * r;
    const float h2 = h * h;
    const float h3 = h2 * h;
    const float K = 15.0f / (2.0f * 3.14159265f * h3);
    return K * ((-(r * r2) / (2.0f * h3)) + (r2 / h2) + (h / (2.0f * r)) - 1.0f);
}

// Laplacian of Müller's viscosity kernel
inline float lapWViscosity(float r, float h) {
    const float K = (45.0f / (3.14159265f * pow(h, 6.0f)));
    return K * max(0.0f, h - r);
}

inline float4 pressureForce(float m1, float m2, float P1, float P2, float rho1, float rho2, float4 dir, float W) {
    //return -1.0f * (m2/m1) * ((P1+P2)) / (2.0f*rho1*rho2)) * W * dir;
    return -1.0f * (m2/m1) * (P1/(rho1*rho1) + P2/(rho2*rho2)) * W * dir;
}

inline float4 viscosityForce(float m1, float m2, float4 u1, float4 u2, float rho2, float4 dir, float K, float W) {
    return K * (m2 / m1) * (1.0f / rho2) * (u2 - u1) * W * dir;
}

inline uint GetFlatCellIndex(int3 cellIndex) {
    // Large primes
    const uint p1 = 73856093;
    const uint p2 = 19349663;
    const uint p3 = 83492791;
    uint n = ((((cellIndex.x * p2 ^ cellIndex.y * p1 ^ cellIndex.z * p3) << 6) + (cellIndex.x & 63) + 4 * (cellIndex.y & 63) + 16 * (cellIndex.z & 63)) & 0x7FFFFFFF);
    n %= CELL_COUNT;
    return n;
}

inline int3 posToCellIndex(float3 pos, float h) {
    float3 scaled = floor(pos / h);
    return convert_int3_sat_rte(scaled);
}

#define FOREACH_NEIGHBOR_NAIVE(pos, offsetList, particleIndexList, cellIndexList, BODY) \
{                                                                                       \
    for (uint nid = 0; nid < NUM_PARTICLES; nid++) {                                    \
        const uint i = particleIndexList[nid];                                          \
        BODY                                                                            \
    }                                                                                   \
}

#define FOREACH_NEIGHBOR_GRID(pos, offsetList, particleIndexList, cellIndexList, BODY)       \
{                                                                                            \
    int3 centerCell = posToCellIndex(pos.xyz, smoothingRadius);                              \
    uint flatCellIndex = GetFlatCellIndex(centerCell);                                       \
    for (int nid = 0; nid < 3 * 3 * 3; nid++) {                                              \
        int3 offset = (int3)(((nid / 1) % 3) - 1, ((nid / 3) % 3) - 1, ((nid / 9) % 3) - 1); \
        int3 offsetCell = centerCell + offset;                                               \
        uint flatNeighborId = GetFlatCellIndex(offsetCell);                                  \
        uint neighborIter = offsetList[flatNeighborId];                                      \
        while (neighborIter != CELL_EMPTY_MARKER && neighborIter < NUM_PARTICLES) {          \
            uint particleIdxOther = particleIndexList[neighborIter];                         \
            if (cellIndexList[neighborIter] != flatNeighborId) {                             \
                break;                                                                       \
            }                                                                                \
            const uint i = particleIdxOther;  /* TODO: use neighborIter instead */           \
            BODY                                                                             \
            neighborIter++;                                                                  \
        }                                                                                    \
    }                                                                                        \
}

// If numCells is small we have to check for duplicates
#define FOREACH_NEIGHBOR_GRID_SAFE(pos, offsetList, particleIndexList, cellIndexList, BODY)  \
{                                                                                            \
    int3 centerCell = posToCellIndex(pos.xyz, smoothingRadius);                              \
    uint flatCellIndex = GetFlatCellIndex(centerCell);                                       \
    uint numProcessedCells = 0;                                                              \
    uint processedCells[3 * 3 * 3];                                                          \
    for (int nid = 0; nid < 3 * 3 * 3; nid++) {                                              \
        int3 offset = (int3)(((nid / 1) % 3) - 1, ((nid / 3) % 3) - 1, ((nid / 9) % 3) - 1); \
        int3 offsetCell = centerCell + offset;                                               \
        uint flatNeighborId = GetFlatCellIndex(offsetCell);                                  \
        uint neighborIter = offsetList[flatNeighborId];                                      \
        bool shouldSkip = false;                                                             \
        for (uint j = 0; j < numProcessedCells; j++) {                                       \
            if (flatNeighborId == processedCells[j]) {                                       \
                shouldSkip = true;                                                           \
                break;                                                                       \
            }                                                                                \
        }                                                                                    \
        if (shouldSkip)                                                                      \
            continue;                                                                        \
        processedCells[numProcessedCells++] = flatNeighborId;                                \
        while (neighborIter != CELL_EMPTY_MARKER && neighborIter < NUM_PARTICLES) {          \
            uint particleIdxOther = particleIndexList[neighborIter];                         \
            if (cellIndexList[neighborIter] != flatNeighborId) {                             \
                break;                                                                       \
            }                                                                                \
            const uint i = particleIdxOther;                                                 \
            BODY                                                                             \
            neighborIter++;                                                                  \
        }                                                                                    \
    }                                                                                        \
}

#define CELL_EMPTY_MARKER (0xFFFFFFFF)
#define FLOAT4_NULLPTR ((float4*)0)


kernel void calcDensities(
    global const float4* restrict positions,
    global const uint* restrict particleIndices,
    global const uint* restrict cellIndices,
    global const uint* restrict offsets,
    global float* restrict densities,
    float restDensity,
    float smoothingRadius,
    float particleMass)
{
    const uint gid = get_global_id(0);
    if (gid >= NUM_PARTICLES)
        return;

    const uint pid = particleIndices[gid];

    float density = 0.0f;
    const float4 p1 = positions[pid];

    FOREACH_NEIGHBOR(p1, offsets, particleIndices, cellIndices, {
        const float4 p2 = positions[i];
        const float r = length(p1 - p2);
        const float mOther = particleMass;
        const float W = WPoly6(r, smoothingRadius);
        density += mOther * W;
    });

    // Remove attractive pressure forces, surface tension model can be added explicitly
    densities[pid] = max(density, restDensity);
}

// Pressure from Equation Of State
inline float calcPressure(const float k, const float p, const float p0) {
#if EOS == 0
    return k * (p - p0);
#elif EOS == 1
    return k * (pow(p/p0, 7.0f) - 1.0f);
#endif
}


kernel void calcForces(
    global const float4* restrict positions,
    global const float4* restrict velocities,
    global const float* restrict densities,
    global const uint* restrict particleIndices,
    global const uint* restrict cellIndices,
    global const uint* restrict offsets,
    global float4* restrict forces,
    float restDensity,
    float smoothingRadius,
    float kPressure,
    float kViscosity,
    float particleMass)
{
    const uint gid = get_global_id(0);
    if (gid >= NUM_PARTICLES)
        return;

    const uint pid = particleIndices[gid];

    float4 force = (float4)(0.0f);
    const float4 p1 = positions[pid];
    const float mSelf = particleMass;
    const float densitySelf = densities[pid];
    const float PSelf = calcPressure(kPressure, densitySelf, restDensity);


    FOREACH_NEIGHBOR(p1, offsets, particleIndices, cellIndices, {
        if (i != pid) {
            const float4 p2 = positions[i];
            float4 dir = p1 - p2;
            const float r2 = dot(dir, dir);
            const float r = max(1e-8f, sqrt(r2));
            dir /= r; // normalize

            const float mOther = particleMass;
            const float densityOther = densities[i];
            const float POther = calcPressure(kPressure, densityOther, restDensity);

            if (r < smoothingRadius) {
                const float gradW = gradWSpiky(r, smoothingRadius);
                force += pressureForce(mSelf, mOther, PSelf, POther, densitySelf, densityOther, dir, gradW);

                const float lapW = lapWViscosity(r, smoothingRadius);
                force += viscosityForce(mSelf, mOther, velocities[pid], velocities[i], densityOther, dir, kViscosity, lapW);
            }
        }
    });

    // External forces
    force += (float4)(0.0f, -9.81f * 2.0f, 0.0f, 0.0f) * densitySelf; // density divided out in time integration step

    // Update
    forces[pid] = force;
}


void dampReflect(int which, float barrier, float4* pos, float4* vel, float4* vhalf) {
    float* v = (float*)vel;
    float* vh = (float*)vhalf;
    float* x = (float*)pos;
    
    const float DAMP = 0.7f;

    if (v[which] == 0)
        return;
    
    // Scale back the distance traveled
    float tbounce = (x[which] - barrier) / v[which]; // time since collision
    *pos -= *vel * (1.0f - DAMP)*tbounce;
    
    // Reflect the position and velocity
    x[which] = 2.0f * barrier - x[which];
    v[which] *= -1.0f;
    vh[which] *= -1.0f;
    
    // Damp the velocities
    *vel *= DAMP;
    *vhalf *= DAMP;
}

void addBoundaryReflect(float4* pos, float4* vel, float4* vhalf, float boxSize, float particleSize) {
    const float XMIN = -boxSize, XMAX = boxSize;
    const float YMIN = -6.0f,    YMAX = 10.0f;
    const float ZMIN = -boxSize, ZMAX = boxSize;

    if (pos->x - particleSize < XMIN) dampReflect(0, XMIN + particleSize, pos, vel, vhalf); // breaks when leapfrogging
    if (pos->x + particleSize > XMAX) dampReflect(0, XMAX - particleSize, pos, vel, vhalf); // breaks when leapfrogging
    if (pos->y - particleSize < YMIN) dampReflect(1, YMIN + particleSize, pos, vel, vhalf);
    if (pos->y + particleSize > YMAX) dampReflect(1, YMAX - particleSize, pos, vel, vhalf);
    if (pos->z - particleSize < ZMIN) dampReflect(2, ZMIN + particleSize, pos, vel, vhalf);
    if (pos->z + particleSize > ZMAX) dampReflect(2, ZMAX - particleSize, pos, vel, vhalf);
}


// Hard boundaries => position clamped
void addHardBoundaries(float4* pos, float4* vel, float4* vhalf, float boxSize, float particleSize) {
    float elastic = 0.6f;
    if (pos->y + particleSize > 10.0f)
    {
        pos->y = 10.0f - particleSize;
        vel->y *= -elastic;
        vhalf->y *= -elastic;
    }
    if (pos->y - particleSize < -6.0f)
    {
        pos->y = -6.0f + particleSize;
        vel->y *= -elastic;
        vhalf->y *= -elastic;
    }
    if (pos->x + particleSize > boxSize)
    {
        pos->x = boxSize - particleSize;
        vel->x *= -elastic;
        vhalf->x *= -elastic;
    }
    if (pos->x - particleSize < -boxSize)
    {
        pos->x = -boxSize + particleSize;
        vel->x *= -elastic;
        vhalf->x *= -elastic;
    }
    if (pos->z + particleSize > boxSize)
    {
        pos->z = boxSize - particleSize;
        vel->z *= -elastic;
        vhalf->z *= -elastic;
    }
    if (pos->z - particleSize < -boxSize)
    {
        pos->z = -boxSize + particleSize;
        vel->z *= -elastic;
        vhalf->z *= -elastic;
    }
}

// Kernel for boundary forces (WCSPH, eq. 20)
float boundaryKernel(float dist, float h, float cs) {
    float q = dist / h;

    float K = 0;
    if (q < 2.0f/3.0f)
        K = 2.0f/3.0f;
    else if (q < 1.0f)
        K = 2.0f*q-(3.0f/2.0f)*q*q;
    else if (q < 2.0f)
        K = 0.5f*pow(2.0f-q, 2.0f);

    return 0.02f * cs * cs / dist * K;
}

float4 getBoundaryForces(float4 pos, float boxSize, float h, float cs) {
    const float ymin = -6.0;
    const float ymax = 10.0;
    
    float4 d;
    float len;
    float4 F = (float4)(0.0f);

    const float EPS = 1e-5f; // don't allow distance 0

    // Pos x
    d = (float4)(-1.0f, 0.0f, 0.0f, 0.0f);
    len = max(boxSize - pos.x, EPS);
    F += d * boundaryKernel(len, h, cs);
    
    // Neg x
    d = (float4)(1.0f, 0.0f, 0.0f, 0.0f);
    len = max(pos.x + boxSize, EPS);
    F += d * boundaryKernel(len, h, cs);

    // Pos y
    d = (float4)(0.0f, -1.0f, 0.0f, 0.0f);
    len = max(ymax - pos.y, EPS);
    F += d * boundaryKernel(len, h, cs);

    // Neg y
    d = (float4)(0.0f, 1.0f, 0.0f, 0.0f);
    len = max(pos.y - ymin, EPS);
    F += d * boundaryKernel(len, h, cs);

    // Pos z
    d = (float4)(0.0f, 0.0f, -1.0f, 0.0f);
    len = max(boxSize - pos.z, EPS);
    F += d * boundaryKernel(len, h, cs);
    
    // Neg z
    d = (float4)(0.0f, 0.0f, 1.0f, 0.0f);
    len = max(pos.z + boxSize, EPS);
    F += d * boundaryKernel(len, h, cs);

    // Assume cumulative boundary particles have 1000x the mass
    const float mBoundary = 1000.0f;
    float mRatio = mBoundary / (mBoundary + 1.0f);
    F *= mRatio;

    return F;
}


kernel void integrateLeapfrog(
    global float4* restrict positions,
    global float4* restrict velocities,
    global float4* restrict velocityHalf,
    global const float4* restrict forces,
    global const float* restrict densities,
    float particleSize,
    float boxSize,
    float h,
    float cs)
{
    const uint gid = get_global_id(0);
    if (gid >= NUM_PARTICLES)
        return;

    const float deltaT = 1.0f / 60.0f;
    float4 F = forces[gid];
    float4 pos = positions[gid];

    // Soft boundaries
    F += getBoundaryForces(pos, boxSize, h, cs);

    float4 a = (float4)(F.xyz / densities[gid], 0.0f);
    
#ifdef LEAPFROG_START
    // Special handling for first timestep, since we have no v_(-1/2)
    float4 vh = velocities[gid] + 0.5f * deltaT * a;
    float4 v = velocities[gid] + deltaT * a;
    if (gid == 0)
        printf("Leapfrog start\n");
#else
    // Compute approximate half step forward
    // Overwritten every iteration, no error is accumulated
    float4 vh = velocityHalf[gid] + deltaT * a;
    float4 v = velocityHalf[gid] + 0.5f * deltaT * a;
#endif
    pos += deltaT * vh;

    // Boundaries
    //addBoundaryReflect(&pos, &v, &vh, boxSize, particleSize); // buggy with leapfrog
    addHardBoundaries(&pos, &v, &vh, boxSize, particleSize);

    // Dampening
    vh *= 0.98f;
    v *= 0.98f;

    // TODO: Calculate optimal next timestep (local + global atomics)
    // https://github.com/rlguy/SPHFluidSim/blob/07548003daf6ebcc3020f6fed37e30981d9f7e81/src/sphfluidsimulation.cpp#L424

    velocityHalf[gid] = vh;
    velocities[gid] = v;
    positions[gid] = pos;
}


kernel void integrateSymplecticEuler(
    global float4* restrict positions,
    global float4* restrict velocities,
    global const float4* restrict forces,
    global const float* restrict densities,
    float particleSize,
    float boxSize,
    float h,
    float cs)
{
    const uint gid = get_global_id(0);
    if (gid >= NUM_PARTICLES)
        return;

    // ONE TO ONE MAPPING, PID UNNECESSARY

    // Time step
    const float deltaT = 1.0f / 60.0f;

    // Acceleration (thanks Newton)
    float4 dudt = forces[gid] / densities[gid];
    float4 pos = positions[gid];

    // Soft boundaries
    dudt += getBoundaryForces(pos, boxSize, h, cs);

    // Symplectic Euler integration scheme
    float4 vel = velocities[gid] + deltaT * (float4)(dudt.xyz, 0.0f);
    pos += deltaT * vel;

    // Boundaries
    float4 vhalf_dummy = (float4)(1.0f);
    addBoundaryReflect(&pos, &vel, &vhalf_dummy, boxSize, particleSize);

    // Dampening
    vel *= 0.98f;

    // TODO: Calculate optimal next timestep (local + global atomics)
    // https://github.com/rlguy/SPHFluidSim/blob/07548003daf6ebcc3020f6fed37e30981d9f7e81/src/sphfluidsimulation.cpp#L424

    positions[gid] = pos;
    velocities[gid] = vel;
}


/* HASH GRID */

kernel void calcGridIdx(
    global const uint* restrict particleIndices,
    global uint* restrict cellIndices,
    global const float4* restrict positions,
    const float smoothingRadius)
{
    const uint gid = get_global_id(0);
    if (gid >= NUM_PARTICLES)
        return;

    const uint pid = particleIndices[gid];

    if (pid >= NUM_PARTICLES) {
        printf("<%u> Invalid particle index: %u\n", gid, pid);
        return;
    }

    int3 cellIndex = posToCellIndex(positions[pid].xyz, smoothingRadius);
    uint flatCellIndex = GetFlatCellIndex(cellIndex);
    
    cellIndices[gid] = flatCellIndex; // save by gid!
}

// Offset list must be cleared every frame
kernel void clearOffsets(global uint* restrict offsets) {
    const uint gid = get_global_id(0);
    if (gid >= CELL_COUNT)
        return;

    // TODO: merge with calcGridIdx?

    // Uint max value, so that min works in offset kernel
    offsets[gid] = CELL_EMPTY_MARKER;
}

kernel void calcOffset(
    global const uint* restrict particleIndices,
    global const uint* restrict cellIndices,
    volatile global uint* restrict offsets)
{
    const uint gid = get_global_id(0);
    if (gid >= NUM_PARTICLES)
        return;

    uint flatCellIdx = cellIndices[gid];
    atomic_min(&offsets[flatCellIdx], gid); // indices into particle index buffer (not particle indices!)
}
