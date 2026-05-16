$input v_color0, v_texcoord0, v_texcoord1

#include <bgfx_shader.sh>

SAMPLER2D(s_tex, 0);
SAMPLER2D(s_mask, 1);
uniform vec4 u_mask_params;
uniform vec4 u_wrap_params;
uniform vec4 u_tev_color0;
uniform vec4 u_tev_color1;
uniform vec4 u_triangle_tev_stages[4];

float gxWrapCoord(float coord, float mode)
{
    if (mode < 0.5) {
        return clamp(coord, 0.0, 1.0);
    }
    if (mode < 1.5) {
        return coord - floor(coord);
    }

    float mirrored = coord - floor(coord * 0.5) * 2.0;
    return mirrored <= 1.0 ? mirrored : 2.0 - mirrored;
}

vec2 gxWrapUv(vec2 uv, vec2 mode)
{
    return vec2(gxWrapCoord(uv.x, mode.x), gxWrapCoord(uv.y, mode.y));
}

vec2 projectedUv(vec3 texcoord)
{
    if (abs(texcoord.z) <= 0.000001) {
        return texcoord.xy;
    }

    return texcoord.xy / texcoord.z;
}

vec3 gxTevColorArg(float selector, vec3 previousColor, vec3 textureColor, vec3 rasterColor)
{
    if (selector < 0.5) {
        return previousColor;
    }
    if (selector < 2.5) {
        return previousColor;
    }
    if (selector < 7.5) {
        return u_tev_color1.rgb;
    }
    if (selector < 9.5) {
        return textureColor;
    }
    if (selector < 10.5) {
        return rasterColor;
    }
    if (selector < 11.5) {
        return rasterColor;
    }
    if (selector < 12.5) {
        return vec3(1.0, 1.0, 1.0);
    }
    if (selector < 13.5) {
        return vec3(0.5, 0.5, 0.5);
    }
    if (selector < 14.5) {
        return u_tev_color0.rgb;
    }
    return vec3(0.0, 0.0, 0.0);
}

float gxTevScale(float scale)
{
    if (scale < 0.5) {
        return 1.0;
    }
    if (scale < 1.5) {
        return 2.0;
    }
    if (scale < 2.5) {
        return 4.0;
    }
    return 0.5;
}

vec3 gxEvaluateTevColorStage(vec4 args, vec4 op, vec3 previousColor, vec3 textureColor, vec3 rasterColor)
{
    vec3 a = gxTevColorArg(args.x, previousColor, textureColor, rasterColor);
    vec3 b = gxTevColorArg(args.y, previousColor, textureColor, rasterColor);
    vec3 c = gxTevColorArg(args.z, previousColor, textureColor, rasterColor);
    vec3 d = gxTevColorArg(args.w, previousColor, textureColor, rasterColor);
    vec3 color = d + mix(a, b, c);
    if (op.y > 0.5 && op.y < 1.5) {
        color += vec3(0.5, 0.5, 0.5);
    } else if (op.y > 1.5) {
        color -= vec3(0.5, 0.5, 0.5);
    }
    color *= gxTevScale(op.z);
    if (op.w > 0.5) {
        color = clamp(color, 0.0, 1.0);
    }
    return color;
}

void main()
{
    vec4 texColor = texture2D(s_tex, gxWrapUv(projectedUv(v_texcoord0), u_wrap_params.xy));
    if (u_mask_params.w < -0.5) {
        vec4 secondaryColor = texture2D(s_mask, gxWrapUv(projectedUv(v_texcoord1), u_wrap_params.zw));
        float mode = -u_mask_params.w;
        if (mode < 1.5) {
            texColor.rgb *= secondaryColor.rgb;
        } else if (mode < 2.5) {
            texColor.rgb = min(texColor.rgb + secondaryColor.rgb, vec3(1.0, 1.0, 1.0));
        } else if (mode < 3.5) {
            texColor.rgb = 1.0 - ((1.0 - texColor.rgb) * (1.0 - secondaryColor.rgb));
        } else {
            vec3 prev = vec3(0.0, 0.0, 0.0);
            float combinedAlpha = texColor.a * secondaryColor.a * v_color0.a;
            if (u_mask_params.x > 0.5) {
                prev = gxEvaluateTevColorStage(u_triangle_tev_stages[0], u_triangle_tev_stages[1], prev, texColor.rgb, v_color0.rgb);
            }
            if (u_mask_params.x > 1.5) {
                prev = gxEvaluateTevColorStage(u_triangle_tev_stages[2], u_triangle_tev_stages[3], prev, secondaryColor.rgb, v_color0.rgb);
            }
            gl_FragColor = vec4(prev, combinedAlpha);
            return;
        }
        texColor.a = max(texColor.a, secondaryColor.a);
        gl_FragColor = texColor * v_color0;
        return;
    }

    if (u_mask_params.x > 0.5) {
        vec4 maskColor = texture2D(s_mask, gxWrapUv(projectedUv(v_texcoord1), u_wrap_params.zw));
        float maskAlpha = u_mask_params.z > 0.5
            ? maskColor.a
            : dot(maskColor.rgb, vec3(0.33333334, 0.33333334, 0.33333334));
        if (u_mask_params.y > 0.5) {
            maskAlpha = 1.0 - maskAlpha;
        }
        texColor.a *= maskAlpha;
    }
    if (u_mask_params.w > 1.5) {
        float intensity = dot(texColor.rgb, vec3(0.33333334, 0.33333334, 0.33333334));
        texColor.rgb = mix(vec3(1.0, 1.0, 1.0), v_color0.rgb, intensity);
        gl_FragColor = vec4(texColor.rgb, texColor.a * v_color0.a);
        return;
    }
    if (u_mask_params.w > 0.5) {
        float intensity = dot(texColor.rgb, vec3(0.33333334, 0.33333334, 0.33333334));
        float tevScale = u_mask_params.w > 1.1 ? 2.0 : 1.0;
        vec3 tevColor = mix(u_tev_color0.rgb, u_tev_color1.rgb, intensity);
        float tevAlpha = mix(u_tev_color0.a, u_tev_color1.a, texColor.a);
        if (u_mask_params.x > 0.5) {
            tevAlpha *= texColor.a;
        }
        gl_FragColor = vec4(min(tevColor * v_color0.rgb * tevScale, vec3(1.0, 1.0, 1.0)), tevAlpha * v_color0.a);
        return;
    }

    gl_FragColor = texColor * v_color0;
}
