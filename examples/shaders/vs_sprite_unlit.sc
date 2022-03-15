$input a_position, a_color0, a_texcoord0
$output v_color, v_texcoord0, v_posP

// linear centroid float4 Color : COLOR;
// linear centroid float2 UV : TEXCOORD0;


#include <bgfx_shader.sh>

// replace by u_view, u_viewProj
// float4x4 mCamera;
// float4x4 mCameraProj;
uniform vec4 mUVInversed;

void main()
{
	v_posP = mul(u_viewProj, vec4(a_position, 1.0));
    gl_Position = v_posP;

    v_texcoord0 = vec2(a_texcoord0.x,
        mUVInversed.x + mUVInversed.y * a_texcoord0.y);

    v_color = a_color0;
}
