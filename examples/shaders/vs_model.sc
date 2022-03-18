// struct VS_Input
// {
// 	float3 Pos : POSITION0;
// 	float3 Normal : NORMAL0;
// 	float3 Binormal : NORMAL1;
// 	float3 Tangent : NORMAL2;
// 	vec2 UV : TEXCOORD0;
// 	vec4 Color : NORMAL3;
// #if defined(ENABLE_DIVISOR)
// 	float Index		: BLENDINDICES0;
// #elif !defined(DISABLE_INSTANCE)
// 	uint Index	: SV_InstanceID;
// #endif
// };
// #if defined(ENABLE_DISTORTION)

// struct VS_Output
// {
// 	vec4 PosVS : SV_POSITION;
// 	linear centroid vec4 Color : COLOR0;
// 	linear centroid vec2 UV : TEXCOORD0;
// 	vec4 ProjBinormal : TEXCOORD1;
// 	vec4 ProjTangent : TEXCOORD2;
// 	vec4 PosP : TEXCOORD3;
// };

// #else

// struct VS_Output
// {
// 	vec4 PosVS : SV_POSITION;
// 	linear centroid vec4 Color : COLOR;
// 	linear centroid vec2 UV : TEXCOORD0;

// #ifdef ENABLE_LIGHTING
// 	float3 WorldN : TEXCOORD1;
// 	float3 WorldB : TEXCOORD2;
// 	float3 WorldT : TEXCOORD3;
// #endif

// 	vec4 PosP : TEXCOORD4;
// };

// #endif

#ifdef ENABLE_DIVISOR
#define BLEND_INDEX a_indices
#else //!ENABLE_DIVISOR
#define BLEND_INDEX
#endif //ENABLE_DIVISOR

#if defined(ENABLE_DISTORTION) || defined(ENABLE_LIGHTING)
#	define OUTPUT_TANGENT 	 v_tangent
#	define OUTPUT_BITANGENT  v_bitangent
#	ifdef ENABLE_LIGHTING
#		define OUTPUT_NORMAL v_normal
#	endif //ENABLE_LIGHTING
#else //!(defined(ENABLE_DISTORTION) || defined(ENABLE_LIGHTING))
#	define OUTPUT_NORMAL 
#	define OUTPUT_TANGENT 
#	define OUTPUT_BITANGENT 
#endif //(defined(ENABLE_DISTORTION) || defined(ENABLE_LIGHTING))

$input a_position a_normal a_bitangent a_tangent a_texcoord0 a_color0
$output v_color0 v_texcoord0 v_posP OUTPUT_TANGENT OUTPUT_BITANGENT OUTPUT_NORMAL

#include <bgfx_shader.sh>

uniform mat4 u_CameraProj;
uniform vec4 u_UVInversed;

#ifdef DISABLE_INSTANCE
uniform mat4 u_Model;
uniform vec4 u_ModelUV;
uniform vec4 u_ModelColor;
#else //!DISABLE_INSTANCE
uniform mat4 u_Model[__INST__];
uniform vec4 u_ModelUV[__INST__];
uniform vec4 u_ModelColor[__INST__];
#endif //DISABLE_INSTANCE

void main()
{
	uint instIdx = gl_InstanceID;
	mat4 model = u_Model[instIdx];
	vec4 uv = u_ModelUV[instIdx];
	v_color0 = u_ModelColor[instIdx] * a_color0;

	vec4 worldPos = mul(model, vec4(a_position, 1.0));
	v_posP = mul(u_CameraProj, worldPos);
	gl_Position = v_posP;

	v_texcoord0 = uv.xy + uv.zw * a_texcoord0;
	v_texcoord0.y = u_UVInversed.x + u_UVInversed.y * v_texcoord0.y;

#if defined(ENABLE_LIGHTING) || defined(ENABLE_DISTORTION)
	v_bitangent	= normalize(mul(model, vec4(a_bitangent, 0.0)));
	v_tangent	= normalize(mul(model, vec4(a_tangent, 0.0)));

#if defined(ENABLE_LIGHTING)
	v_normal = normalize(mul(model, vec4(a_normal, 0.0)));
#elif defined(ENABLE_DISTORTION)
	v_bitangent = mul(u_CameraProj, worldPos + v_bitangent);
	v_tangent = mul(u_CameraProj, worldPos + v_tangent);
#endif

#endif //defined(ENABLE_LIGHTING) || defined(ENABLE_DISTORTION)
}
