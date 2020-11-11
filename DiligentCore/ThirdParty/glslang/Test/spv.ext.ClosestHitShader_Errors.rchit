#version 460
#extension GL_EXT_ray_tracing : enable
hitAttributeEXT vec4 payload;
layout(binding = 0, set = 0) uniform accelerationStructureEXT accEXT;

void main()
{
    payload.x = 1.0f;                                       // ERROR, cannot write to hitattributeEXT in stage
    reportIntersectionEXT(1.0, 1U);                          // ERROR, unsupported builtin in stage 
    terminateRayEXT();
    ignoreIntersectionEXT();
    bool e1 = gl_IncomingRayFlagsEXT == gl_RayFlagsSkipAABBEXT;
}
