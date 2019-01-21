#pragma once

#include <clt.hpp>
#include <glm/glm.hpp>

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

#define FLOAT3(v) (cl_float3{ v.x, v.y, v.z })
#define FLOAT4(v) (cl_float4{ v.x, v.y, v.z, v.w })

namespace SPH {

    /*** KERNEL DATA AND PARAMETERS ***/

    struct KernelData {
        cl::BufferGL clPositions;
        cl::BufferGL clVelocities;
        cl::Buffer clForces;
        cl::Buffer clDensities;
        std::vector<cl::Memory> sharedMemory;
        cl_uint numParticles = (cl_uint)5000;
        cl_uint dims = 3; // simulation dimensionality
        glm::vec3 sunPosition = { 0.0f, 0.0f, 0.501f };
        cl_float sunMass = 1e3f;
        cl_float particleSize = 1.0f;
        cl_float particleMass = 0.1f;
        cl_float smoothingRadius = 1.0f; // make function of particle size?
        cl_float p0 = 1.0f; // rest density
        cl_float K = 250.0f; // pressure constant
        cl_float eps = 0.018f; // viscosity constant
    };
    extern KernelData kernelData; // defined in cpp file


    /*** KERNEL IMPLEMENTATIONS ***/

    inline std::string getAllKernelParams() {
        std::string args;
        args += " -DNUM_PARTICLES=" + std::to_string(kernelData.numParticles);
        return args;
    }

    // SPH density estimator
    class CalcDensitiesKernel : public clt::Kernel {
    public:
        CalcDensitiesKernel(void) : Kernel("Gamma/Sources/sph_kernels.cl", "calcDensities") {};
        std::string getAdditionalBuildOptions(void) override {
            return getAllKernelParams();
        }
        void setArgs() override {
            setArg("positions", kernelData.clPositions);
            setArg("densities", kernelData.clDensities);
            setArg("restDensity", kernelData.p0);
            setArg("smoothingRadius", kernelData.smoothingRadius);
            setArg("particleMass", kernelData.particleMass);
        }
    };

    class CalcForcesKernel : public clt::Kernel {
    public:
        CalcForcesKernel(void) : Kernel("Gamma/Sources/sph_kernels.cl", "calcForces") {};
        std::string getAdditionalBuildOptions(void) override {
            return getAllKernelParams();
        }
        void setArgs() override {
            setArg("positions", kernelData.clPositions);
            setArg("velocities", kernelData.clVelocities);
            setArg("densities", kernelData.clDensities);
            setArg("forces", kernelData.clForces);
            setArg("restDensity", kernelData.p0);
            setArg("smoothingRadius", kernelData.smoothingRadius);
            setArg("kPressure", kernelData.K);
            setArg("kViscosity", kernelData.eps);
            setArg("particleMass", kernelData.particleMass);
        }
    };

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

    // Symplectic Euler integrator
    class TimeIntegrateKernel : public clt::Kernel {
    public:
        TimeIntegrateKernel(void) : Kernel("Gamma/Sources/sph_kernels.cl", "integrate") {};
        std::string getAdditionalBuildOptions(void) override {
            return getAllKernelParams();
        }
        void setArgs() override {
            setArg("positions", kernelData.clPositions);
            setArg("velocities", kernelData.clVelocities);
            setArg("forces", kernelData.clForces);
            setArg("densities", kernelData.clDensities);
            setArg("particleSize", kernelData.particleSize);
        }
    };
}
