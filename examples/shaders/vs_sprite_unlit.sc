$input a_position, a_color0, a_texcoord0
$output v_color0, v_texcoord0, v_posP

// linear centroid float4 Color : COLOR;
// linear centroid float2 UV : TEXCOORD0;


#include <bgfx_shader.sh>

uniform mat4 u_Camera;
uniform mat4 u_CameraProj;
uniform vec4 u_UVInversed;

void main()
{
	v_posP = mul(u_CameraProj, vec4(a_position, 1.0));
    gl_Position = v_posP;

    v_texcoord0 = vec2(a_texcoord0.x,
        u_UVInversed.x + u_UVInversed.y * a_texcoord0.y);

    v_color0 = a_color0;
}
