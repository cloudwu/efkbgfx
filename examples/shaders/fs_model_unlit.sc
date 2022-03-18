$input v_color0, v_texcoord0, v_posP
#include <bgfx_shader.sh>
#include <shaderlib.sh>

uniform	vec4 u_EmmisiveParam; // x:emissiveScaling
#define fEmissiveScaling u_EmmisiveParam.x
uniform vec4 u_MiscFlags;
SAMPLER2D(s_sampler_colorTex, 0);

void main()
{
    //TODO: set s_sampler_colorTex as sRGB texture in bgfx and make the frame buffer with sRGB flags
	bool convertColorSpace = u_MiscFlags.x != 0.0f;

	//vec4 Output = ConvertFromSRGBTexture(_colorTex.Sample(sampler_colorTex, Input.UV), convertColorSpace) * Input.Color;
    vec4 texcolor = texture2D(s_sampler_colorTex, v_texcoord0);
    if (convertColorSpace){
        texcolor = toLinear(texcolor);
    }
    vec4 Output = texcolor * v_color0;
	Output.rgb *= fEmissiveScaling;

	if (Output.a == 0.0)
		discard;

    if (convertColorSpace)
        gl_FragColor = toGamma(Output);
    else
        gl_FragColor = Output;
}
