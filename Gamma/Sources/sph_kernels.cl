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
    // convert_int3_sat_rte
    return (int3)((int)scaled.x, (int)scaled.y, (int)scaled.z);
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


// Hard boundaries => position clamped
void addHardBoundaries(float4* pos, float4* vel, float boxSize, float particleSize) {
    float elastic = 0.6f;
    if (pos->y < -6.0f)
    {
        pos->y = -6.0f;
        vel->y *= -elastic;
    }
    if (pos->y > 10.0f)
    {
        pos->y = 10.0f;
        vel->y *= -elastic;
    }

    if (pos->x + particleSize > boxSize)
    {
        pos->x = boxSize - particleSize;
        vel->x *= -elastic;
    }
    if (pos->x - particleSize < -boxSize)
    {
        pos->x = -boxSize + particleSize;
        vel->x *= -elastic;
    }
    if (pos->z + particleSize > boxSize)
    {
        pos->z = boxSize - particleSize;
        vel->z *= -elastic;
    }
    if (pos->z - particleSize < -boxSize)
    {
        pos->z = -boxSize + particleSize;
        vel->z *= -elastic;
    }
}


kernel void integrate(
    global float4* restrict positions,
    global float4* restrict velocities,
    global const float4* restrict forces,
    global const float* restrict densities,
    float particleSize,
    float boxSize)
{
    const uint gid = get_global_id(0);
    if (gid >= NUM_PARTICLES)
        return;

    // ONE TO ONE MAPPING, PID UNNECESSARY

    // Time step
    const float deltaT = 1.0f / 60.0f;

    // Acceleration (thanks Newton)
    const float4 dudt = forces[gid] / densities[gid];

    // Symplectic Euler integration scheme
    float4 vel = velocities[gid] + deltaT * (float4)(dudt.xyz, 0.0f);
    float4 pos = positions[gid] + deltaT * vel;

    // Boundaries
    addHardBoundaries(&pos, &vel, boxSize, particleSize);

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
