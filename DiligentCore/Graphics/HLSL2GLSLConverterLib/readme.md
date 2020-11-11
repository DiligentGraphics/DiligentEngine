
# HLSL2GLSLConverterLib

Implementation of HLSL to GLSL source converter

DirectX and OpenGL use different shading languages, which share a lot in common, but sometimes differ substantially. 
For cross-platform applications, maintaining two versions of each shader is time-consuming and error-prone. Diligent 
Engine uses HLSL2GLSL Converter that allows shader authored in HLSL to be converted into GLSL source.

# Details


## Requirements
The converter supports HLSL5.0, all shader types (vertex, geometry, pixel, domain, hull, and compute) as well as most 
of the language constructs. There are however few special requirements that must be met in order for the HLSL source 
to be successfully converted to GLSL:

* Inputs to a vertex shader is recommended to have `ATTRIBn` semantic, where n defines the location of the corresponding 
  GLSL input variable (`layout(location = n)`). For any other input semantic, the converter automatically assigns input location.
* Inputs of a subsequent shader stage must be declared in exact same order as outputs of the previous shader stage. 
  Return value of a function counts as its first output.

  The converter parses all input and output arguments (including structure members) in the order of declaration and automatically 
  assigns locations to every argument. To make sure that input and output locations match, the arguments must be declared in exact 
  same order. For the same reason, if an argument is not used by the shader, it still needs to be declared to preserve original ordering.

The code snippet below gives examples of supported shader declarations:

```
struct VSInput
{
    // It is recommended (though not required) to assign 
    // ATTRIBn semantics to inputs from input assembler
    in float3 f3PosWS : ATTRIB0;
    in float2 f2UV  : ATTRIB1;
    uint VertexID : SV_VertexID;
};

struct VSOutput
{
    float2 UV : TEX_COORDINATES;
    float3 Normal : NORMAL;
    float4 f4PosPS : SV_Position;
};

VSOutput VertexShader ( in VSInput VSIn,
                        in float3 f3UV  : ATTRIB2,
                        uint InstID : SV_InstanceID,
                        out float3 f3Tangent : TANGENT )
{
    VSOutput VSOut;
    // Body elided
    f3Tangent = ...
    return VSOut;
}

float4 PixelShader ( // Pixel shader inputs must be declared in exact same order 
                     // as outputs of the vertex shader
                     VSOutput PSIn,
                     in float3 f3Tangent : TaNgEnT // Semantics are case-insensitive
                     out float3 Color2 : SV_Target2 ) :  SV_Target
{
    // Body elided
}
```

* When tessellation is enabled in OpenGL, partitioning as well as output patch topology are defined by the 
tessellation evaluation shader (domain shader) rather than by the tessellation control shader (hull shader). 
As a result, the converter cannot generate GLSL code without using special hints. To indicate missing attributes, 
the following specially formatted comment should be added right on top of the domain shader entry function:

```
/* partitioning = {integer|fractional_even|fractional_odd}, outputtopology = {triangle_cw|triangle_ccw} */
```

For example, the following is a valid domain shader declaration:

```
[domain("quad")]
/* partitioning = fractional_even, outputtopology = triangle_cw */
DSOutput main( HS_CONSTANT_DATA_OUTPUT input, 
               float2 QuadUV : SV_DomainLocation, 
               OutputPatch<HSOutput, 2> QuadPatch)
{
    // Body elided
}
```

* Geometry, Domain and Hull shaders must be defined in separate files
* GLSL allows samplers to be declared as global variables or function arguments only. It does not allow local variables of sampler type.

## Textures and samplers

The following rules are used to convert HLSL texture declaration into GLSL sampler:

HLSL texture dimension defines GLSL sampler dimension:

* `Texture2D`     ->   `sampler2D`
* `TextureCube`   ->   `samplerCube`

HLSL texture component type defines GLSL sampler type. If no type is specified, `float4 ` is assumed:

* `Texture2D<float>`        ->   `sampler2D`
* `Texture3D<uint4>`        ->   `usampler3D`
* `Texture2DArray<int2>`    ->   `isampler2DArray`
* `Texture2D`               ->   `sampler2D`

To distinguish if sampler should be shadow or not, the converter tries to find `<Texture Name>_sampler` among samplers 
(global variables and function arguments). If the sampler type is comparison, the texture is converted to shadow sampler. 
If sampler state is either not comparison or not found, regular sampler is used. For example

```
Texture2D g_ShadowMap;
SamplerComparisonState g_ShadowMap_sampler;

Texture2D g_Tex2D;
SamplerState g_Tex2D_sampler;

Texture3D g_Tex3D;
```

is converted to

```
sampler2DShadow g_ShadowMap;
sampler2D g_Tex2D;
sampler3D g_Tex3D;
```

GLSL requires format to be specified for all images (rw textures) allowing writes. HLSL converter allows 
GLSL image format specification inside the special comment block:
```
RWTexture2D<float /* format=r32f */ > Tex2D;
```

## Important notes/known issues

* GLSL compiler does not handle `float3` (`vec3`) structure members correctly. It is strongly suggested avoid 
using this type in structure definitions
* At least NVidia GLSL compiler does not apply layout(row_major) to structure members. By default, all matrices 
in both HLSL and GLSL are column major
* GLSL compiler does not properly handle structs passed as function arguments!!!!

```
struct MyStruct
{
    matrix Matr; 
} 
void Func(in MyStruct S)
{ 
    ... 
    mul(f4PosWS, S.Matr); // This will not work!!! 
}
```

DO NOT pass structs to functions, use only built-in types!!!

* GLSL does not support most of the implicit type conversions. The following are some examples of the required modifications to HLSL code:
  - `float4 vec = 0;` ->  `float4 vec = float4(0.0, 0.0, 0.0, 0.0);`
  - `float x = 0;`    ->  `float x = 0.0;`
  - `uint x = 0;`     ->  `uint x = 0u;`
* GLES is immensely strict about type conversions. For instance, this code will produce compiler error: 
  `float4(0, 0, 0, 0)`. It must be written as `float4(0.0, 0.0, 0.0, 0.0)`
* GLSL does not support relational and boolean operations on vector types:

```
float2 p = float2(1.0, 2.0), q = float2(3.0, 4.0);
bool2 b = p < q; // Error
all(p < q); // Error
```
* To facilitate relational and Boolean operations on vector types, the following functions are predefined:
  - `Less`
  - `LessEqual`
  - `Greater`
  - `GreaterEqual`
  - `Equal`
  - `NotEqual`
  - `Not`
  - `And`
  - `Or`
  - `BoolToFloat`

* Examples:
  - `bool2 b = x < y;` -> `b = Less(x, y);`
  - `all(p>=q)` -> `all( GreaterEqual(p,q) )`

* When accessing elements of an HLSL matrix, the first index is always a row:
  `mat[row][column]`
  In GLSL, the first index is always a column:
  `mat[column][row]`
`MATRIX_ELEMENT(mat, row, col)`  macros is provided to facilitate matrix element retrieval

* The following functions do not have counterparts in GLSL and should be avoided:
  - `Texture2DArray.SampleCmpLevelZero()`
  - `TextureCube.SampleCmpLevelZero()`
  - `TextureCubeArray.SampleCmpLevelZero()`

## Limitations
Converter does not perform macros expansion, so usage of preprocessor directives is limited to 
text block that do not need to be converted. The following are some examples that are not supported.

Using macros in declarations of shader entry points:

```
VSOut TestVS  (
#ifdef SOME_MACRO
               in VSInput0 VSInput
#else
               in VSInput1 VSInput
#endif
               )
```

The following is not allowed as well:

```
#ifdef SOME_MACRO
VSOut TestVS  (in VSInput0 VSInput)
#else
VSOut TestVS  (in VSInput1 VSInput)
#endif
```

In cases like that it is necessary to create two separate shader entry points and give them 
distinctive names. Likewise, macros cannot be used in definitions of structures that are used 
to pass data between shader stages:

```
struct VSInput
{
    in float3 f3PosWS : ATTRIB0;
#ifdef SOME_MACRO
    in float2 f2UV  : ATTRIB1;
#else
    in float4 f4UV  : ATTRIB1;
#endif
    uint VertexID : SV_VertexID;
};
```

Similarly to shader entry points, in the scenario above, the two structures need to be defined with 
distinctive names. Shader macros are allowed in structures that are not used to pass data between shader stages.

Defining language keywords with macros is not allowed:
```
#define TEXTURE2D Texture2D
TEXTURE2D MacroTex2D;
```

Macros can be used within function bodies:

```
VSOut VSTes(...)
{
#ifdef SOME_MACRO
    // OK
#else
    // OK
#endif
}
```

# Features

Please visit [this page](http://diligentgraphics.com/diligent-engine/shader-converter/supported-features/) 
for the full list of supported language features.

# References

[HLSL to GLSL Source Converter](http://diligentgraphics.com/diligent-engine/shader-converter/)

# Release Notes

## 2.1

### New features

* Support for structured buffers
* HLSL->GLSL conversion is now a two-stage process:
  - Creating conversion stream
  - Creating GLSL source from the stream
* Geometry shader support
* Tessellation control and tessellation evaluation shader support
* Support for non-void shader functions
* Allowing structs as input parameters for shader functions

## 2.0

Reworked the API to follow D3D12 style

## 1.0

Initial release



**Copyright 2015-2018 Egor Yusov**

[diligentgraphics.com](http://diligentgraphics.com)
