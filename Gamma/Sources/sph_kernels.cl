kernel void calcDensities(
    global const float4* restrict positions,
    global float* restrict densities,
    float restDensity,
    float smoothingRadius,
    float particleMass)
{
    const uint gid = get_global_id(0);
    if (gid >= NUM_PARTICLES)
        return;

    float density = 0.0f;
    const float4 p1 = positions[gid];

    // Naive n^2 test
    for (int i = 0; i < NUM_PARTICLES; i++) {
        const float4 p2 = positions[i];
        const float4 dir = p2 - p1;
        const float mOther = particleMass;
        const float r2 = dot(dir, dir);
        const float h2 = smoothingRadius * smoothingRadius;
        const float KPoly6 = (315.0f / (64.0f * 3.14159265f * pow(smoothingRadius, 9.0f)));
        const float W = KPoly6 * pow(max(0.0f, h2 - r2), 3.0f); // contributes if r2 < h2
        density += mOther * W;
    }

    densities[gid] = max(restDensity, density); // avoid negative pressure later
}


kernel void calcForces(
    global const float4* restrict positions,
    global const float4* restrict velocities,
    global const float* restrict densities,
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

    float4 force = (float4)(0.0f);
    const float4 p1 = positions[gid];
    const float mSelf = particleMass;
    const float densitySelf = densities[gid];
    const float PSelf = kPressure * (densitySelf - restDensity);

    // Naive n^2 test
    for (int i = 0; i < NUM_PARTICLES; i++) {
        if (i == gid) continue;
        const float4 p2 = positions[i];
        float4 dir = p2 - p1;
        const float r2 = dot(dir, dir);
        const float r = sqrt(r2);
        dir /= r; // normlaize
        const float mOther = particleMass;
        const float densityOther = densities[i];
        const float POther = kPressure * (densityOther - restDensity);

        if (r < smoothingRadius) {
            // Pressure force
            const float KSpiky = (-45.0f / (3.14159265f * pow(smoothingRadius, 6.0f)));
            const float gradW = KSpiky * pow(smoothingRadius - r, 2.0f);
            force += -1.0f * (mOther / mSelf) * (PSelf + POther) / (2.0f * densitySelf * densityOther) * gradW * dir;

            // Viscosity force
            const float r3 = r2 * r;
            const float h2 = smoothingRadius * smoothingRadius;
            const float h3 = smoothingRadius * h2;
            const float lapW = (-r3 / (2.0f * h3)) + (r2 / h2) + (smoothingRadius / (2.0f * r)) - 1.0f;
            force += kViscosity * (mOther / mSelf) * (1.0f / densityOther) * (velocities[i] - velocities[gid]) * lapW * dir;
        }
    }

    // External forces
    //force += (float4)(0.0f, -9.81f, 0.0f, 0.0f) * densitySelf; // density divided out in time integration step

    // Update
    forces[gid] = force;
}


kernel void integrate(
    global float4* restrict positions,
    global float4* restrict velocities,
    global const float4* restrict forces,
    global const float* restrict densities)
{
    const uint gid = get_global_id(0);
    if (gid >= NUM_PARTICLES)
        return;

    // Time step
    const float deltaT = 1.0f / 144.0f;

    // Acceleration (thanks Newton)
    const float4 dudt = forces[gid] / densities[gid];

    // Symplectic Euler integration scheme
    velocities[gid] += deltaT * (float4)(dudt.xyz, 0.0f);
    float4 pos = positions[gid] + deltaT * velocities[gid];

    // Boundaries
    const float side = 1.0f;
    pos = clamp(pos, side * (float4)(-1.0f), side * (float4)(1.0f));

    positions[gid] = pos;
}
