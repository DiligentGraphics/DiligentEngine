#ifndef _SHADER_UTILITIES_FXH_
#define _SHADER_UTILITIES_FXH_

// Transforms camera-space Z to normalized device z coordinate
float CameraZToNormalizedDeviceZ(in float CameraZ, in float4x4 mProj)
{
    // In Direct3D and Vulkan, normalized device z range is [0, +1]
    // In OpengGL, normalized device z range is [-1, +1] (unless GL_ARB_clip_control extension is used to correct this nonsense).
    return MATRIX_ELEMENT(mProj,2,2) + MATRIX_ELEMENT(mProj,3,2) / CameraZ;
}

// Transforms the normal from tangent space to world space using the
// position and UV derivatives.
float3 TransformTangentSpaceNormalGrad(in float3 dPos_dx,     // Position dx derivative
                                       in float3 dPos_dy,     // Position dy derivative
                                       in float2 dUV_dx,      // Normal map UV coordinates dx derivative
                                       in float2 dUV_dy,      // Normal map UV coordinates dy derivative
                                       in float3 MacroNormal, // Macro normal, must be normalized
                                       in float3 TSNormal     // Tangent-space normal
                                       )

{
	float3 n = MacroNormal;

    float3 t = (dUV_dy.y * dPos_dx - dUV_dx.y * dPos_dy) / (dUV_dx.x * dUV_dy.y - dUV_dy.x * dUV_dx.y);
    t = normalize(t - n * dot(n, t));

    float3 b = normalize(cross(t, n));

    float3x3 tbn = MatrixFromRows(t, b, n);

    return normalize(mul(TSNormal, tbn));
}

// Transforms the normal from tangent space to world space, without using the
// explicit tangent frame.
float3 TransformTangentSpaceNormal(in float3 Position,    // Vertex position in world space
                                   in float3 MacroNormal, // Macro normal, must be normalized
                                   in float3 TSNormal,    // Tangent-space normal
                                   in float2 NormalMapUV  // Normal map uv coordinates
                                   )
{
    float3 dPos_dx = ddx(Position);
    float3 dPos_dy = ddy(Position);

    float2 dUV_dx = ddx(NormalMapUV);
    float2 dUV_dy = ddy(NormalMapUV);

    return TransformTangentSpaceNormalGrad(dPos_dx, dPos_dy, dUV_dx, dUV_dy, MacroNormal, TSNormal);
}

#endif //_SHADER_UTILITIES_FXH_
