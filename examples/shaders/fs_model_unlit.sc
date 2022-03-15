$input v_posVS, v_color, v_texcoord0, v_posP
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

SAMPLER2D(_colorTex, 0);

float SoftParticle(float backgroundZ, float meshZ, vec4 softparticleParam, vec4 reconstruct1, vec4 reconstruct2)
{
	float distanceFar = softparticleParam.x;
	float distanceNear = softparticleParam.y;
	float distanceNearOffset = softparticleParam.z;

	float2 rescale = reconstruct1.xy;
	vec4 params = reconstruct2;

	float2 zs = float2(backgroundZ * rescale.x + rescale.y, meshZ);

	float2 depth = (zs * params.w - params.y) / (params.x - zs * params.z);
	float dir = sign(depth.x);
	depth *= dir;
	float alphaFar = (depth.x - depth.y) / distanceFar;
	float alphaNear = (depth.y - distanceNearOffset) / distanceNear;
	return min(max(min(alphaFar, alphaNear), 0.0), 1.0);
}

// #include "SoftParticle_PS.fx"
// #include "Linear_sRGB.fx"

void main()
{
    //TODO: set _colorTex as sRGB texture in bgfx
	bool convertColorSpace = miscFlags.x != 0.0f;

	//vec4 Output = ConvertFromSRGBTexture(_colorTex.Sample(sampler_colorTex, Input.UV), convertColorSpace) * Input.Color;
    vec4 texcolor = texture2D(_colorTex, v_texcoord0);
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
