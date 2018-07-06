
uniform sampler2D g_tex2DTest0;
uniform sampler2D g_tex2DTest1;
uniform sampler2D g_tex2DTest2;
 
layout(location = 1)in vec2 ex_UV;

layout(location = 0) out vec4 out_Color;

void main()
{
    vec2 UV = vec2( ex_UV.x, ex_UV.y );
#ifndef TARGET_API_VULKAN
    UV.y = 1.0 - UV.y;
#endif

    vec4 Col0 = texture(g_tex2DTest0, UV);
    vec4 Col1 = texture(g_tex2DTest1, UV);
    vec4 Col2 = texture(g_tex2DTest2, UV);
    out_Color = vec4(Col0.x, Col1.y, Col2.z, 0.0);
}
