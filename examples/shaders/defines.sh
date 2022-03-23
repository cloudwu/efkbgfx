vec3 vec3_splat(vec3 v)
{
    return v;
}

vec2 vec2_splat(vec2 v)
{
    return v;
}

vec4 vec4_splat(vec4 v)
{
    return v;
}

#if !BGFX_SHADER_LANGUAGE_SPIRV
#define gl_InstanceIndex gl_InstanceID
#endif //BGFX_SHADER_LANGUAGE_SPIRV