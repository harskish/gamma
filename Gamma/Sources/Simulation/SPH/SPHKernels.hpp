#pragma once

#include <clt.hpp>
#include <glm/glm.hpp>
#include <vexcl/vector.hpp>

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

    /*** KERNEL DATA AND PARAMETERS ***/

    struct KernelData {
        cl::BufferGL clPositions;
        cl::BufferGL clVelocities;
        cl::Buffer clForces;
        cl::Buffer clDensities;
        std::vector<cl::Memory> sharedMemory;
        cl_uint numParticles = (cl_uint)50000;
        cl_uint dims = 3; // simulation dimensionality
        glm::vec3 sunPosition = { 0.0f, 10.0f, 10.0f };
        cl_float sunMass = 1e3f;
        cl_float particleSize = 0.5f;
        cl_float particleMass = 0.3f;
        cl_float smoothingRadius = 0.7f; // make function of particle size?
        cl_float p0 = 4.0f; // rest density
        cl_float K = 220.0f; // pressure constant
        cl_float eps = 0.25f; // viscosity constant
        cl_float boxSize = 8.0f;
        cl_float dropSize = 8.0f;
        cl_int EOS = 0; // 0 => k(p - p0), 1 => k((p/p0)^7 - 1)

        cl::Buffer particleIndices;
        cl::Buffer cellIndices;
        cl::Buffer offsetList;
        vex::vector<cl_uint> vexParticleIndices;
        vex::vector<cl_uint> vexCellIndices;
        cl_uint numCells = 128 * 128 * 64;

        std::string foreachFunString = "FOREACH_NEIGHBOR_GRID";
    };

    extern KernelData kernelData; // defined in cpp file


    /*** KERNEL IMPLEMENTATIONS ***/

    inline std::string getAllKernelParams() {
        std::string args;
        args += " -DNUM_PARTICLES=" + std::to_string(kernelData.numParticles);
        args += " -DCELL_COUNT=" + std::to_string(kernelData.numCells);
        args += " -DEOS=" + std::to_string(kernelData.EOS);
        args += " -DFOREACH_NEIGHBOR=" + kernelData.foreachFunString;
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
            setArg("particleIndices", kernelData.particleIndices);
            setArg("cellIndices", kernelData.cellIndices);
            setArg("offsets", kernelData.offsetList);
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
            setArg("particleIndices", kernelData.particleIndices);
            setArg("cellIndices", kernelData.cellIndices);
            setArg("offsets", kernelData.offsetList);
            setArg("forces", kernelData.clForces);
            setArg("restDensity", kernelData.p0);
            setArg("smoothingRadius", kernelData.smoothingRadius);
            setArg("kPressure", kernelData.K);
            setArg("kViscosity", kernelData.eps);
            setArg("particleMass", kernelData.particleMass);
        }
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
            setArg("boxSize", kernelData.boxSize);
        }
    };


    /* Hash grid */

    // 1. Get 1D hashed cell index for each particle
    class CalcCellIndexKernel : public clt::Kernel {
    public:
        CalcCellIndexKernel(void) : Kernel("Gamma/Sources/sph_kernels.cl", "calcGridIdx") {};
        std::string getAdditionalBuildOptions(void) override {
            return getAllKernelParams();
        }
        void setArgs() override {
            setArg("particleIndices", kernelData.particleIndices);
            setArg("cellIndices", kernelData.cellIndices);
            setArg("positions", kernelData.clPositions);
            setArg("smoothingRadius", kernelData.smoothingRadius);
        }
    };

    // 2. Sort particle indices by cellid

    // 3. Clear offset list (every frame)
    class ClearOffsetKernel : public clt::Kernel {
    public:
        ClearOffsetKernel(void) : Kernel("Gamma/Sources/sph_kernels.cl", "clearOffsets") {};
        std::string getAdditionalBuildOptions(void) override {
            return getAllKernelParams();
        }
        void setArgs() override {
            setArg("offsets", kernelData.offsetList);
        }
    };

    // 4. Create new offset list from sorted particles
    class CalcOffsetKernel : public clt::Kernel {
    public:
        CalcOffsetKernel(void) : Kernel("Gamma/Sources/sph_kernels.cl", "calcOffset") {};
        std::string getAdditionalBuildOptions(void) override {
            return getAllKernelParams();
        }
        void setArgs() override {
            setArg("particleIndices", kernelData.particleIndices);
            setArg("cellIndices", kernelData.cellIndices);
            setArg("offsets", kernelData.offsetList);
        }
    };

}
