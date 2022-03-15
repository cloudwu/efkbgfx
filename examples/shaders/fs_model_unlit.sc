$input v_color, v_texcoord0, v_posP
#include <bgfx_shader.sh>
#include <shaderlib.sh>

uniform	vec4 fLightDirection;
uniform	vec4 fLightColor;
uniform	vec4 fLightAmbient;

uniform	vec4 fFlipbookParameter; // x:enable, y:interpolationType

uniform	vec4 fUVDistortionParameter; // x:intensity, y:blendIntensity, zw:uvInversed

uniform	vec4 fBlendTextureParameter; // x:blendType

uniform	vec4 fCameraFrontDirection;

uniform	vec4 fFalloffParameter; // x:enable, y:colorblendtype, z:pow
uniform	vec4 fFalloffBeginColor;
uniform	vec4 fFalloffEndColor;

uniform	vec4 fEmissiveScaling; // x:emissiveScaling

uniform	vec4 fEdgeColor;
uniform	vec4 fEdgeParameter; // x:threshold, y:colorScaling

	// which is used for only softparticle
uniform	vec4 softParticleParam;
uniform	vec4 reconstructionParam1;
uniform	vec4 reconstructionParam2;
uniform	vec4 mUVInversedBack;
uniform	vec4 miscFlags;

SAMPLER2D(s_sampler_colorTex, 0);

void main()
{
    //TODO: set _colorTex as sRGB texture in bgfx
	bool convertColorSpace = miscFlags.x != 0.0f;

	//vec4 Output = ConvertFromSRGBTexture(_colorTex.Sample(sampler_colorTex, Input.UV), convertColorSpace) * Input.Color;
    vec4 texcolor = texture2D(s_sampler_colorTex, v_texcoord0);
    if (convertColorSpace){
        texcolor = toLinear(texcolor);
    }
    vec4 Output = texcolor * v_color;
	Output.rgb *= fEmissiveScaling.x;

	if (Output.a == 0.0)
		discard;

    if (convertColorSpace)
        gl_FragColor = toGamma(Output);
    else
        gl_FragColor = Output;
}
