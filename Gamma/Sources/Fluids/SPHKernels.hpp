#pragma once

#include <clt.hpp>

#define STRINGIFY(...) #__VA_ARGS__


/*
SPH fluid simulation

Solves the incompressible Euler equations
(Navier-Stokes without explicit viscocity) (????)
using a non-iterative EOS solver.

Steps:
    * Find neighbors
    * Calculate pressure from estimated density
        - Compute presure force, self-advection force
        - Pressure force (EOS) handles incompressibility
            - Controlled by stifness parameter c, bigger param = less compressible, more unstable
            - Alternative to pressure Poisson equation (PPE)
    * Enforce boundary conditions
        - Compute boundary+external forces
    * Compute acceleration a=F/m, integrate velocity du = dt * a
    * Enforce incompressibility (pressure projection)
        - Alternative to state equation formulation (EOS)
        - Uses a pressure Poisson equation (PPE)
    * Integrate positions dx = dt * u

Kernels:
    Neighbors (hash grid)
    -> densities (Wspiky * mass)
    -> forces (pressure using equation of state (e.g. ideal gas law), viscocity)
    -> time integration (symplectic Euler => use updated v when updating x)
*/


namespace SPH {

    struct KernelData {
        cl::BufferGL clPositions;
        cl::BufferGL clVelocities;
        std::vector<cl::Memory> sharedMemory;
        cl_uint numParticles = 50 * 50U;
        cl_uint dims = 3; // simulation dimensionality
    };
    extern KernelData kernelData; // defined in cpp file

    class FindNeighborsKernel : public clt::Kernel {
    public:
        FindNeighborsKernel(void) : Kernel("", "update") {};
        void setArgs() override {
            setArg("leader", 0U);
        }
        CLT_KERNEL_IMPL(
        kernel void update(uint leader) {
            uint gid = get_global_id(0);
            if (gid == leader)
                printf("Updating neighbors\n");
        })
    };


    class CalcDensitiesKernel;
    class CalcForcesKernel;

    // Symplectic Euler integrator
    class TimeIntegrateKernel : public clt::Kernel {
    public:
        TimeIntegrateKernel(void) : Kernel("", "integrate") {};
        std::string getAdditionalBuildOptions(void) override {
            std::string args;
            args += " -DNUM_PARTICLES=" + std::to_string(kernelData.numParticles);
            args += " -DFLUID_REST_DENSITY=" + std::to_string(1000.0f); // water = kg / m^3
            return args;
        }
        void setArgs() override {
            setArg("positions", kernelData.clPositions);
            setArg("velocities", kernelData.clVelocities);
        }
        CLT_KERNEL_IMPL(
        kernel void integrate(global float4* positions, global float4* velocities) {
            uint gid = get_global_id(0);
            if (gid >= NUM_PARTICLES)
                return;

            float3 F = (float3)(0.0f);

            // External forces
            // TODO: test with gravity pointing towards origin
            const float3 g = (float3)(0.0f, -9.81f, 0.0f);
            F += g;

            // Acceleration (thanks Newton)
            const float density = FLUID_REST_DENSITY;
            const float V = 1e-6f; // m^3
            const float m = V * density;
            const float3 dudt = F / m * 0.0000001f;

            // Symplectic Euler integration scheme
            velocities[gid] += (float4)(dudt, 0.0f);
            positions[gid] += velocities[gid];
        })
    };
}
