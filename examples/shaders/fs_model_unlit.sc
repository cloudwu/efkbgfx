$input v_Color,v_UV,v_PosP

#include <bgfx_shader.sh>

uniform vec4 u_fLightDirection;
uniform vec4 u_fLightColor;
uniform vec4 u_fLightAmbient;
uniform vec4 u_fFlipbookParameter;
uniform vec4 u_fUVDistortionParameter;
uniform vec4 u_fBlendTextureParameter;
uniform vec4 u_fCameraFrontDirection;
uniform vec4 u_fFalloffParameter;
uniform vec4 u_fFalloffBeginColor;
uniform vec4 u_fFalloffEndColor;
uniform vec4 u_fEmissiveScaling;
uniform vec4 u_fEdgeColor;
uniform vec4 u_fEdgeParameter;
uniform vec4 u_softParticleParam;
uniform vec4 u_reconstructionParam1;
uniform vec4 u_reconstructionParam2;
uniform vec4 u_mUVInversedBack;
uniform vec4 u_miscFlags;
SAMPLER2D (s_colorTex,0);
SAMPLER2D (s_depthTex,1);

struct PS_Input
{
    vec4 PosVS;
    vec4 Color;
    vec2 UV;
    vec4 PosP;
};

vec3 PositivePow(vec3 base, vec3 power)
{
    return pow(max(abs(base), vec3_splat(1.1920928955078125e-07)), power);
}

vec3 LinearToSRGB(vec3 c)
{
    vec3 param = c;
    vec3 param_1 = vec3_splat(0.4166666567325592041015625);
    return max((PositivePow(param, param_1) * 1.05499994754791259765625) - vec3_splat(0.054999999701976776123046875), vec3_splat(0.0));
}

vec4 LinearToSRGB(vec4 c)
{
    vec3 param = c.xyz;
    return vec4(LinearToSRGB(param), c.w);
}

vec4 ConvertFromSRGBTexture(vec4 c, bool isValid)
{
    if (!isValid)
    {
        return c;
    }
    vec4 param = c;
    return LinearToSRGB(param);
}

float SoftParticle(float backgroundZ, float meshZ, vec4 softparticleParam, vec4 reconstruct1, vec4 reconstruct2)
{
    float distanceFar = softparticleParam.x;
    float distanceNear = softparticleParam.y;
    float distanceNearOffset = softparticleParam.z;
    vec2 rescale = reconstruct1.xy;
    vec4 params = reconstruct2;
    vec2 zs = vec2((backgroundZ * rescale.x) + rescale.y, meshZ);
    vec2 depth = ((zs * params.w) - vec2_splat(params.y)) / (vec2_splat(params.x) - (zs * params.z));
    float dir = sign(depth.x);
    depth *= dir;
    float alphaFar = (depth.x - depth.y) / distanceFar;
    float alphaNear = (depth.y - distanceNearOffset) / distanceNear;
    return min(max(min(alphaFar, alphaNear), 0.0), 1.0);
}

vec3 SRGBToLinear(vec3 c)
{
    return min(c, c * ((c * ((c * 0.305306017398834228515625) + vec3_splat(0.6821711063385009765625))) + vec3_splat(0.01252287812530994415283203125)));
}

vec4 SRGBToLinear(vec4 c)
{
    vec3 param = c.xyz;
    return vec4(SRGBToLinear(param), c.w);
}

vec4 ConvertToScreen(vec4 c, bool isValid)
{
    if (!isValid)
    {
        return c;
    }
    vec4 param = c;
    return SRGBToLinear(param);
}

vec4 _main(PS_Input Input)
{
    bool convertColorSpace = !(u_miscFlags.x == 0.0);
    vec4 param = texture2D(s_colorTex, Input.UV);
    bool param_1 = convertColorSpace;
    vec4 Output = ConvertFromSRGBTexture(param, param_1) * Input.Color;
    vec3 _258 = Output.xyz * u_fEmissiveScaling.x;
    Output = vec4(_258.x, _258.y, _258.z, Output.w);
    vec4 screenPos = Input.PosP / vec4_splat(Input.PosP.w);
    vec2 screenUV = (screenPos.xy + vec2_splat(1.0)) / vec2_splat(2.0);
    screenUV.y = 1.0 - screenUV.y;
    screenUV.y = u_mUVInversedBack.x + (u_mUVInversedBack.y * screenUV.y);
    if (!(u_softParticleParam.w == 0.0))
    {
        float backgroundZ = texture2D(s_depthTex, screenUV).x;
        float param_2 = backgroundZ;
        float param_3 = screenPos.z;
        vec4 param_4 = u_softParticleParam;
        vec4 param_5 = u_reconstructionParam1;
        vec4 param_6 = u_reconstructionParam2;
        Output.w *= SoftParticle(param_2, param_3, param_4, param_5, param_6);
    }
    if (Output.w == 0.0)
    {
        discard;
    }
    vec4 param_7 = Output;
    bool param_8 = convertColorSpace;
    return ConvertToScreen(param_7, param_8);
}

void main()
{
    PS_Input Input;
    Input.PosVS = gl_FragCoord;
    Input.Color = v_Color;
    Input.UV = v_UV;
    Input.PosP = v_PosP;
    vec4 _359 = _main(Input);
    gl_FragColor = _359;
}
