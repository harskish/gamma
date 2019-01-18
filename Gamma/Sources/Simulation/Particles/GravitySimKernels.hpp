#pragma once

#include <clt.hpp>
#include <glm/glm.hpp>

/*
Simple particle system with glowing particles and an attractor
No particle interaction (...yet?)
*/

#define FLOAT3(v) (cl_float3{ v.x, v.y, v.z })
#define FLOAT4(v) (cl_float4{ v.x, v.y, v.z, v.w })

namespace Gravity {

    /*** KERNEL DATA AND PARAMETERS ***/

    struct KernelData {
        cl::BufferGL clPositions;
        cl::BufferGL clVelocities;
        std::vector<cl::Memory> sharedMemory;
        cl_uint numParticles = (cl_uint)5e5;
        cl_uint dims = 3; // simulation dimensionality
        glm::vec3 sunPosition = { 0.0f, 0.55f, 0.0f };
        cl_float sunMass = 5e5f;
        cl_float particleSize = 5.0f;
    };
    extern KernelData kernelData; // defined in cpp file


    /*** KERNEL IMPLEMENTATIONS ***/

    // Symplectic Euler integrator
    class TimeIntegrateKernel : public clt::Kernel {
    public:
        TimeIntegrateKernel(void) : Kernel("", "integrate") {};
        std::string getAdditionalBuildOptions(void) override {
            std::string args;
            args += " -DNUM_PARTICLES=" + std::to_string(kernelData.numParticles);
            return args;
        }
        void setArgs() override {
            setArg("positions", kernelData.clPositions);
            setArg("velocities", kernelData.clVelocities);
            setArg("orig", FLOAT3(kernelData.sunPosition));
            setArg("sunMass", kernelData.sunMass);
        }
        CLT_KERNEL_IMPL(
        kernel void integrate(global float4* restrict positions, global float4* restrict velocities, float3 orig, float sunMass) {
            uint gid = get_global_id(0);
            if (gid >= NUM_PARTICLES)
                return;

            // Cumulative force
            float3 F = (float3)(0.0f);
            const float m = 1e-4f;
            
            // Gravity towards origin
            const float G = 6.674e-11f;
            const float m2 = sunMass;
            const float dist = max(1e-6f, length(positions[gid].xyz - orig)); // simulate radius
            const float3 gdir = normalize(orig - positions[gid].xyz);
            const float3 g = gdir * G * m * m2 / (dist * dist);
            F += g;

            // Acceleration (thanks Newton)
            const float3 dudt = F / m;

            // Symplectic Euler integration scheme
            velocities[gid] += (float4)(dudt, 0.0f);
            positions[gid] += velocities[gid];
        })
    };
}
