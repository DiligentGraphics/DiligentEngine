
layout(location = 1) in vec2 ex_UV;
 
layout (location = 0) out vec4 out_Color0; 
layout (location = 1) out vec4 out_Color1; 

void main()
{
    out_Color0 = vec4( 0.5+0.5*cos(ex_UV.x*3.1415*6.0), 0.0, 0.0, 0.0 );
    out_Color1 = vec4( 0.0, 0.5+0.5*cos(ex_UV.y*3.1415*8.0), 0.0, 0.0 );
}
