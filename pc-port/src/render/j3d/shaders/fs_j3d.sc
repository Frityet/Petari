$input v_color0, v_texcoord0, v_texcoord1, v_texcoord2, v_texcoord3

#include <bgfx_shader.sh>

SAMPLER2D(s_j3d_tex0, 0);
SAMPLER2D(s_j3d_tex1, 1);
SAMPLER2D(s_j3d_tex2, 2);
SAMPLER2D(s_j3d_tex3, 3);
uniform vec4 u_j3d_params;
uniform vec4 u_j3d_wrap_params[2];
uniform vec4 u_j3d_tev_color0;
uniform vec4 u_j3d_tev_color1;
uniform vec4 u_j3d_tev_colors[4];
uniform vec4 u_j3d_k_colors[4];
uniform vec4 u_j3d_tev_stages[8];
uniform vec4 u_j3d_tev_alpha_stages[8];
uniform vec4 u_j3d_tev_color_dests;
uniform vec4 u_j3d_tev_alpha_dests;
uniform vec4 u_j3d_tev_texture_indices;
uniform vec4 u_j3d_tev_k_color_selectors;
uniform vec4 u_j3d_tev_k_alpha_selectors;
uniform vec4 u_j3d_tev_texture_swizzles[4];
uniform vec4 u_j3d_tev_raster_swizzles[4];
uniform vec4 u_j3d_alpha_compare;
uniform vec4 u_j3d_alpha_compare_extra;
uniform vec4 u_j3d_texture_sizes[4];
uniform vec4 u_j3d_ind_params;
uniform vec4 u_j3d_ind_orders[4];
uniform vec4 u_j3d_ind_matrices[6];
uniform vec4 u_j3d_tev_indirects[8];

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

vec4 stageTextureColor(float selector, vec4 tex0, vec4 tex1, vec4 tex2, vec4 tex3)
{
    if (selector < 0.5) {
        return tex0;
    }
    if (selector < 1.5) {
        return tex1;
    }
    if (selector < 2.5) {
        return tex2;
    }
    return tex3;
}

vec3 stageTextureCoord(float selector, vec3 texcoord0, vec3 texcoord1, vec3 texcoord2, vec3 texcoord3)
{
    if (selector < 0.5) {
        return texcoord0;
    }
    if (selector < 1.5) {
        return texcoord1;
    }
    if (selector < 2.5) {
        return texcoord2;
    }
    return texcoord3;
}

vec2 stageTextureWrapMode(float selector)
{
    if (selector < 0.5) {
        return u_j3d_wrap_params[0].xy;
    }
    if (selector < 1.5) {
        return u_j3d_wrap_params[0].zw;
    }
    if (selector < 2.5) {
        return u_j3d_wrap_params[1].xy;
    }
    return u_j3d_wrap_params[1].zw;
}

vec2 stageTextureSize(float selector)
{
    if (selector < 0.5) {
        return max(u_j3d_texture_sizes[0].xy, vec2(1.0, 1.0));
    }
    if (selector < 1.5) {
        return max(u_j3d_texture_sizes[1].xy, vec2(1.0, 1.0));
    }
    if (selector < 2.5) {
        return max(u_j3d_texture_sizes[2].xy, vec2(1.0, 1.0));
    }
    return max(u_j3d_texture_sizes[3].xy, vec2(1.0, 1.0));
}

vec4 sampleTextureBySelector(float selector, vec2 uv)
{
    vec2 wrappedUv = gxWrapUv(uv, stageTextureWrapMode(selector));
    if (selector < 0.5) {
        return texture2D(s_j3d_tex0, wrappedUv);
    }
    if (selector < 1.5) {
        return texture2D(s_j3d_tex1, wrappedUv);
    }
    if (selector < 2.5) {
        return texture2D(s_j3d_tex2, wrappedUv);
    }
    return texture2D(s_j3d_tex3, wrappedUv);
}

vec4 indirectOrder(float selector)
{
    if (selector < 0.5) {
        return u_j3d_ind_orders[0];
    }
    if (selector < 1.5) {
        return u_j3d_ind_orders[1];
    }
    if (selector < 2.5) {
        return u_j3d_ind_orders[2];
    }
    return u_j3d_ind_orders[3];
}

vec4 indirectMatrixRow(float matrixIndex, float row)
{
    if (matrixIndex < 0.5) {
        return row < 0.5 ? u_j3d_ind_matrices[0] : u_j3d_ind_matrices[1];
    }
    if (matrixIndex < 1.5) {
        return row < 0.5 ? u_j3d_ind_matrices[2] : u_j3d_ind_matrices[3];
    }
    return row < 0.5 ? u_j3d_ind_matrices[4] : u_j3d_ind_matrices[5];
}

float indirectBiasApplies(float bias, float component)
{
    if (component < 0.5) {
        return (bias == 1.0 || bias == 3.0 || bias == 5.0 || bias == 7.0) ? 1.0 : 0.0;
    }
    if (component < 1.5) {
        return (bias == 2.0 || bias == 3.0 || bias == 6.0 || bias == 7.0) ? 1.0 : 0.0;
    }
    return bias >= 4.0 ? 1.0 : 0.0;
}

vec3 decodeIndirectSample(vec4 sampleColor, vec4 ind0)
{
    vec3 coord = floor(clamp(sampleColor.abg, 0.0, 1.0) * 255.0 + 0.5);
    float biasValue = -128.0;
    if (ind0.y > 0.5 && ind0.y < 1.5) {
        coord = floor(coord / 8.0);
        biasValue = 1.0;
    } else if (ind0.y > 1.5 && ind0.y < 2.5) {
        coord = floor(coord / 16.0);
        biasValue = 1.0;
    } else if (ind0.y > 2.5) {
        coord = floor(coord / 32.0);
        biasValue = 1.0;
    }

    coord.x += indirectBiasApplies(ind0.z, 0.0) * biasValue;
    coord.y += indirectBiasApplies(ind0.z, 1.0) * biasValue;
    coord.z += indirectBiasApplies(ind0.z, 2.0) * biasValue;
    return coord;
}

float indirectWrapCoord(float coord, float textureExtent, float mode)
{
    if (mode < 0.5) {
        return coord;
    }
    if (mode > 5.5) {
        return 0.0;
    }

    float wrapSize = 256.0;
    if (mode > 1.5 && mode < 2.5) {
        wrapSize = 128.0;
    } else if (mode > 2.5 && mode < 3.5) {
        wrapSize = 64.0;
    } else if (mode > 3.5 && mode < 4.5) {
        wrapSize = 32.0;
    } else if (mode > 4.5) {
        wrapSize = 16.0;
    }
    return mod(coord * textureExtent, wrapSize) / textureExtent;
}

vec2 applyIndirectToUv(float selector, vec2 uv, vec4 ind0, vec4 ind1, vec3 texcoord0, vec3 texcoord1, vec3 texcoord2, vec3 texcoord3)
{
    if (ind1.w < 0.5 || ind0.x >= u_j3d_ind_params.x || ind0.w < 0.5) {
        return uv;
    }

    vec4 order = indirectOrder(ind0.x);
    if (order.w < 0.5) {
        return uv;
    }

    vec2 indUv = projectedUv(stageTextureCoord(order.x, texcoord0, texcoord1, texcoord2, texcoord3));
    indUv /= exp2(order.yz);
    vec3 indCoord = decodeIndirectSample(sampleTextureBySelector(order.x, indUv), ind0);

    vec2 textureSize = stageTextureSize(selector);
    vec2 wrappedUv = vec2(
        indirectWrapCoord(uv.x, textureSize.x, ind1.x),
        indirectWrapCoord(uv.y, textureSize.y, ind1.y));
    vec2 offset = vec2(0.0, 0.0);
    if (ind0.w > 0.5) {
        float matrixSelector = ind0.w;
        float matrixId = floor(matrixSelector / 4.0);
        float matrixIndex = matrixSelector - matrixId * 4.0 - 1.0;
        if (matrixIndex >= 0.0 && matrixId < 0.5) {
            vec4 row0 = indirectMatrixRow(matrixIndex, 0.0);
            vec4 row1 = indirectMatrixRow(matrixIndex, 1.0);
            vec2 offsetTexels = vec2(dot(row0.xyz, indCoord), dot(row1.xyz, indCoord)) * exp2(row0.w);
            offset = offsetTexels / textureSize;
        }
    }

    return wrappedUv + offset;
}

vec4 sampleStageTexture(float selector, vec4 ind0, vec4 ind1, vec3 texcoord0, vec3 texcoord1, vec3 texcoord2, vec3 texcoord3)
{
    vec2 uv = projectedUv(stageTextureCoord(selector, texcoord0, texcoord1, texcoord2, texcoord3));
    return sampleTextureBySelector(selector, applyIndirectToUv(selector, uv, ind0, ind1, texcoord0, texcoord1, texcoord2, texcoord3));
}

float gxSwizzleComponent(vec4 color, float selector)
{
    if (selector < 0.5) {
        return color.r;
    }
    if (selector < 1.5) {
        return color.g;
    }
    if (selector < 2.5) {
        return color.b;
    }
    return color.a;
}

vec4 gxSwizzleColor(vec4 color, vec4 swizzle)
{
    return vec4(
        gxSwizzleComponent(color, swizzle.x),
        gxSwizzleComponent(color, swizzle.y),
        gxSwizzleComponent(color, swizzle.z),
        gxSwizzleComponent(color, swizzle.w));
}

vec3 gxTevKonstColor(float selector)
{
    if (selector < 0.5) {
        return vec3(1.0, 1.0, 1.0);
    }
    if (selector < 1.5) {
        return vec3(0.875, 0.875, 0.875);
    }
    if (selector < 2.5) {
        return vec3(0.75, 0.75, 0.75);
    }
    if (selector < 3.5) {
        return vec3(0.625, 0.625, 0.625);
    }
    if (selector < 4.5) {
        return vec3(0.5, 0.5, 0.5);
    }
    if (selector < 5.5) {
        return vec3(0.375, 0.375, 0.375);
    }
    if (selector < 6.5) {
        return vec3(0.25, 0.25, 0.25);
    }
    if (selector < 7.5) {
        return vec3(0.125, 0.125, 0.125);
    }
    if (selector < 11.5) {
        return vec3(0.0, 0.0, 0.0);
    }
    if (selector < 12.5) {
        return u_j3d_k_colors[0].rgb;
    }
    if (selector < 13.5) {
        return u_j3d_k_colors[1].rgb;
    }
    if (selector < 14.5) {
        return u_j3d_k_colors[2].rgb;
    }
    if (selector < 15.5) {
        return u_j3d_k_colors[3].rgb;
    }
    if (selector < 16.5) {
        return u_j3d_k_colors[0].rrr;
    }
    if (selector < 17.5) {
        return u_j3d_k_colors[1].rrr;
    }
    if (selector < 18.5) {
        return u_j3d_k_colors[2].rrr;
    }
    if (selector < 19.5) {
        return u_j3d_k_colors[3].rrr;
    }
    if (selector < 20.5) {
        return u_j3d_k_colors[0].ggg;
    }
    if (selector < 21.5) {
        return u_j3d_k_colors[1].ggg;
    }
    if (selector < 22.5) {
        return u_j3d_k_colors[2].ggg;
    }
    if (selector < 23.5) {
        return u_j3d_k_colors[3].ggg;
    }
    if (selector < 24.5) {
        return u_j3d_k_colors[0].bbb;
    }
    if (selector < 25.5) {
        return u_j3d_k_colors[1].bbb;
    }
    if (selector < 26.5) {
        return u_j3d_k_colors[2].bbb;
    }
    if (selector < 27.5) {
        return u_j3d_k_colors[3].bbb;
    }
    if (selector < 28.5) {
        return u_j3d_k_colors[0].aaa;
    }
    if (selector < 29.5) {
        return u_j3d_k_colors[1].aaa;
    }
    if (selector < 30.5) {
        return u_j3d_k_colors[2].aaa;
    }
    return u_j3d_k_colors[3].aaa;
}

float gxTevKonstAlpha(float selector)
{
    if (selector < 0.5) {
        return 1.0;
    }
    if (selector < 1.5) {
        return 0.875;
    }
    if (selector < 2.5) {
        return 0.75;
    }
    if (selector < 3.5) {
        return 0.625;
    }
    if (selector < 4.5) {
        return 0.5;
    }
    if (selector < 5.5) {
        return 0.375;
    }
    if (selector < 6.5) {
        return 0.25;
    }
    if (selector < 7.5) {
        return 0.125;
    }
    if (selector < 15.5) {
        return 0.0;
    }
    if (selector < 16.5) {
        return u_j3d_k_colors[0].r;
    }
    if (selector < 17.5) {
        return u_j3d_k_colors[1].r;
    }
    if (selector < 18.5) {
        return u_j3d_k_colors[2].r;
    }
    if (selector < 19.5) {
        return u_j3d_k_colors[3].r;
    }
    if (selector < 20.5) {
        return u_j3d_k_colors[0].g;
    }
    if (selector < 21.5) {
        return u_j3d_k_colors[1].g;
    }
    if (selector < 22.5) {
        return u_j3d_k_colors[2].g;
    }
    if (selector < 23.5) {
        return u_j3d_k_colors[3].g;
    }
    if (selector < 24.5) {
        return u_j3d_k_colors[0].b;
    }
    if (selector < 25.5) {
        return u_j3d_k_colors[1].b;
    }
    if (selector < 26.5) {
        return u_j3d_k_colors[2].b;
    }
    if (selector < 27.5) {
        return u_j3d_k_colors[3].b;
    }
    if (selector < 28.5) {
        return u_j3d_k_colors[0].a;
    }
    if (selector < 29.5) {
        return u_j3d_k_colors[1].a;
    }
    if (selector < 30.5) {
        return u_j3d_k_colors[2].a;
    }
    return u_j3d_k_colors[3].a;
}

vec3 gxTevColorArg(float selector, vec4 previous, vec4 reg0, vec4 reg1, vec4 reg2, vec3 textureColor, float textureAlpha, vec3 rasterColor, float rasterAlpha, vec3 konstColor)
{
    if (selector < 0.5) {
        return previous.rgb;
    }
    if (selector < 1.5) {
        return previous.aaa;
    }
    if (selector < 2.5) {
        return reg0.rgb;
    }
    if (selector < 3.5) {
        return reg0.aaa;
    }
    if (selector < 4.5) {
        return reg1.rgb;
    }
    if (selector < 5.5) {
        return reg1.aaa;
    }
    if (selector < 6.5) {
        return reg2.rgb;
    }
    if (selector < 7.5) {
        return reg2.aaa;
    }
    if (selector < 8.5) {
        return textureColor;
    }
    if (selector < 9.5) {
        return vec3(textureAlpha, textureAlpha, textureAlpha);
    }
    if (selector < 10.5) {
        return rasterColor;
    }
    if (selector < 11.5) {
        return vec3(rasterAlpha, rasterAlpha, rasterAlpha);
    }
    if (selector < 12.5) {
        return vec3(1.0, 1.0, 1.0);
    }
    if (selector < 13.5) {
        return vec3(0.5, 0.5, 0.5);
    }
    if (selector < 14.5) {
        return konstColor;
    }
    return vec3(0.0, 0.0, 0.0);
}

float gxTevAlphaArg(float selector, vec4 previous, vec4 reg0, vec4 reg1, vec4 reg2, float textureAlpha, float rasterAlpha, float konstAlpha)
{
    if (selector < 0.5) {
        return previous.a;
    }
    if (selector < 1.5) {
        return reg0.a;
    }
    if (selector < 2.5) {
        return reg1.a;
    }
    if (selector < 3.5) {
        return reg2.a;
    }
    if (selector < 4.5) {
        return textureAlpha;
    }
    if (selector < 5.5) {
        return rasterAlpha;
    }
    if (selector < 6.5) {
        return konstAlpha;
    }
    return 0.0;
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

vec3 gxEvaluateTevColorStage(vec4 args, vec4 op, vec4 previous, vec4 reg0, vec4 reg1, vec4 reg2, vec4 textureColor, vec4 rasterColor, vec3 konstColor)
{
    vec3 a = gxTevColorArg(args.x, previous, reg0, reg1, reg2, textureColor.rgb, textureColor.a, rasterColor.rgb, rasterColor.a, konstColor);
    vec3 b = gxTevColorArg(args.y, previous, reg0, reg1, reg2, textureColor.rgb, textureColor.a, rasterColor.rgb, rasterColor.a, konstColor);
    vec3 c = gxTevColorArg(args.z, previous, reg0, reg1, reg2, textureColor.rgb, textureColor.a, rasterColor.rgb, rasterColor.a, konstColor);
    vec3 d = gxTevColorArg(args.w, previous, reg0, reg1, reg2, textureColor.rgb, textureColor.a, rasterColor.rgb, rasterColor.a, konstColor);
    vec3 color = op.x > 0.5 ? d - mix(a, b, c) : d + mix(a, b, c);
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

float gxEvaluateTevAlphaStage(vec4 args, vec4 op, vec4 previous, vec4 reg0, vec4 reg1, vec4 reg2, vec4 textureColor, vec4 rasterColor, float konstAlpha)
{
    float a = gxTevAlphaArg(args.x, previous, reg0, reg1, reg2, textureColor.a, rasterColor.a, konstAlpha);
    float b = gxTevAlphaArg(args.y, previous, reg0, reg1, reg2, textureColor.a, rasterColor.a, konstAlpha);
    float c = gxTevAlphaArg(args.z, previous, reg0, reg1, reg2, textureColor.a, rasterColor.a, konstAlpha);
    float d = gxTevAlphaArg(args.w, previous, reg0, reg1, reg2, textureColor.a, rasterColor.a, konstAlpha);
    float alpha = op.x > 0.5 ? d - mix(a, b, c) : d + mix(a, b, c);
    if (op.y > 0.5 && op.y < 1.5) {
        alpha += 0.5;
    } else if (op.y > 1.5) {
        alpha -= 0.5;
    }
    alpha *= gxTevScale(op.z);
    if (op.w > 0.5) {
        alpha = clamp(alpha, 0.0, 1.0);
    }
    return alpha;
}

bool gxAlphaCompareOne(float alpha, float compareMode, float reference)
{
    if (compareMode < 0.5) {
        return false;
    }
    if (compareMode < 1.5) {
        return alpha < reference;
    }
    if (compareMode < 2.5) {
        return abs(alpha - reference) < 0.5;
    }
    if (compareMode < 3.5) {
        return alpha <= reference;
    }
    if (compareMode < 4.5) {
        return alpha > reference;
    }
    if (compareMode < 5.5) {
        return abs(alpha - reference) >= 0.5;
    }
    if (compareMode < 6.5) {
        return alpha >= reference;
    }
    return true;
}

bool gxAlphaTest(float alpha)
{
    if (u_j3d_alpha_compare_extra.y < 0.5) {
        return true;
    }

    float alpha255 = floor(clamp(alpha, 0.0, 1.0) * 255.0 + 0.5);
    bool lhs = gxAlphaCompareOne(alpha255, u_j3d_alpha_compare.x, u_j3d_alpha_compare.y);
    bool rhs = gxAlphaCompareOne(alpha255, u_j3d_alpha_compare.w, u_j3d_alpha_compare_extra.x);
    if (u_j3d_alpha_compare.z < 0.5) {
        return lhs && rhs;
    }
    if (u_j3d_alpha_compare.z < 1.5) {
        return lhs || rhs;
    }
    if (u_j3d_alpha_compare.z < 2.5) {
        return lhs != rhs;
    }
    return lhs == rhs;
}

vec4 gxApplyAlphaTest(vec4 color)
{
    if (!gxAlphaTest(color.a)) {
        discard;
    }
    return color;
}

void gxStoreTevColor(float dest, vec3 color, inout vec4 previous, inout vec4 reg0, inout vec4 reg1, inout vec4 reg2)
{
    if (dest < 0.5) {
        previous.rgb = color;
    } else if (dest < 1.5) {
        reg0.rgb = color;
    } else if (dest < 2.5) {
        reg1.rgb = color;
    } else {
        reg2.rgb = color;
    }
}

void gxStoreTevAlpha(float dest, float alpha, inout vec4 previous, inout vec4 reg0, inout vec4 reg1, inout vec4 reg2)
{
    if (dest < 0.5) {
        previous.a = alpha;
    } else if (dest < 1.5) {
        reg0.a = alpha;
    } else if (dest < 2.5) {
        reg1.a = alpha;
    } else {
        reg2.a = alpha;
    }
}

vec3 gxTevRegisterColor(float dest, vec4 previous, vec4 reg0, vec4 reg1, vec4 reg2)
{
    if (dest < 0.5) {
        return previous.rgb;
    }
    if (dest < 1.5) {
        return reg0.rgb;
    }
    if (dest < 2.5) {
        return reg1.rgb;
    }
    return reg2.rgb;
}

float gxTevRegisterAlpha(float dest, vec4 previous, vec4 reg0, vec4 reg1, vec4 reg2)
{
    if (dest < 0.5) {
        return previous.a;
    }
    if (dest < 1.5) {
        return reg0.a;
    }
    if (dest < 2.5) {
        return reg1.a;
    }
    return reg2.a;
}

void main()
{
    if (u_j3d_params.z > 0.5) {
        gl_FragColor = gxApplyAlphaTest(v_color0);
        return;
    }

    vec4 tex0 = texture2D(s_j3d_tex0, gxWrapUv(projectedUv(v_texcoord0), u_j3d_wrap_params[0].xy));
    vec4 tex1 = texture2D(s_j3d_tex1, gxWrapUv(projectedUv(v_texcoord1), u_j3d_wrap_params[0].zw));
    vec4 tex2 = texture2D(s_j3d_tex2, gxWrapUv(projectedUv(v_texcoord2), u_j3d_wrap_params[1].xy));
    vec4 tex3 = texture2D(s_j3d_tex3, gxWrapUv(projectedUv(v_texcoord3), u_j3d_wrap_params[1].zw));
    if (u_j3d_params.y > 0.5) {
        gl_FragColor = gxApplyAlphaTest(stageTextureColor(u_j3d_params.y - 1.0, tex0, tex1, tex2, tex3));
        return;
    }
    vec4 texColor = tex0;
    if (u_j3d_params.w < -0.5) {
        vec4 secondaryColor = tex1;
        float mode = -u_j3d_params.w;
        if (mode < 1.5) {
            texColor.rgb *= secondaryColor.rgb;
        } else if (mode < 2.5) {
            texColor.rgb = min(texColor.rgb + secondaryColor.rgb, vec3(1.0, 1.0, 1.0));
        } else if (mode < 3.5) {
            texColor.rgb = 1.0 - ((1.0 - texColor.rgb) * (1.0 - secondaryColor.rgb));
        } else {
            vec4 previous = u_j3d_tev_colors[3];
            vec4 reg0 = u_j3d_tev_colors[0];
            vec4 reg1 = u_j3d_tev_colors[1];
            vec4 reg2 = u_j3d_tev_colors[2];
            float lastColorDest = 0.0;
            float lastAlphaDest = 0.0;
            if (u_j3d_params.x > 0.5) {
                vec4 stageTex = gxSwizzleColor(sampleStageTexture(u_j3d_tev_texture_indices.x, u_j3d_tev_indirects[0], u_j3d_tev_indirects[1], v_texcoord0, v_texcoord1, v_texcoord2, v_texcoord3), u_j3d_tev_texture_swizzles[0]);
                vec4 stageRas = gxSwizzleColor(v_color0, u_j3d_tev_raster_swizzles[0]);
                vec3 konstColor = gxTevKonstColor(u_j3d_tev_k_color_selectors.x);
                float konstAlpha = gxTevKonstAlpha(u_j3d_tev_k_alpha_selectors.x);
                vec3 colorResult = gxEvaluateTevColorStage(u_j3d_tev_stages[0], u_j3d_tev_stages[1], previous, reg0, reg1, reg2, stageTex, stageRas, konstColor);
                float alphaResult = gxEvaluateTevAlphaStage(u_j3d_tev_alpha_stages[0], u_j3d_tev_alpha_stages[1], previous, reg0, reg1, reg2, stageTex, stageRas, konstAlpha);
                lastColorDest = u_j3d_tev_color_dests.x;
                lastAlphaDest = u_j3d_tev_alpha_dests.x;
                gxStoreTevColor(lastColorDest, colorResult, previous, reg0, reg1, reg2);
                gxStoreTevAlpha(lastAlphaDest, alphaResult, previous, reg0, reg1, reg2);
            }
            if (u_j3d_params.x > 1.5) {
                vec4 stageTex = gxSwizzleColor(sampleStageTexture(u_j3d_tev_texture_indices.y, u_j3d_tev_indirects[2], u_j3d_tev_indirects[3], v_texcoord0, v_texcoord1, v_texcoord2, v_texcoord3), u_j3d_tev_texture_swizzles[1]);
                vec4 stageRas = gxSwizzleColor(v_color0, u_j3d_tev_raster_swizzles[1]);
                vec3 konstColor = gxTevKonstColor(u_j3d_tev_k_color_selectors.y);
                float konstAlpha = gxTevKonstAlpha(u_j3d_tev_k_alpha_selectors.y);
                vec3 colorResult = gxEvaluateTevColorStage(u_j3d_tev_stages[2], u_j3d_tev_stages[3], previous, reg0, reg1, reg2, stageTex, stageRas, konstColor);
                float alphaResult = gxEvaluateTevAlphaStage(u_j3d_tev_alpha_stages[2], u_j3d_tev_alpha_stages[3], previous, reg0, reg1, reg2, stageTex, stageRas, konstAlpha);
                lastColorDest = u_j3d_tev_color_dests.y;
                lastAlphaDest = u_j3d_tev_alpha_dests.y;
                gxStoreTevColor(lastColorDest, colorResult, previous, reg0, reg1, reg2);
                gxStoreTevAlpha(lastAlphaDest, alphaResult, previous, reg0, reg1, reg2);
            }
            if (u_j3d_params.x > 2.5) {
                vec4 stageTex = gxSwizzleColor(sampleStageTexture(u_j3d_tev_texture_indices.z, u_j3d_tev_indirects[4], u_j3d_tev_indirects[5], v_texcoord0, v_texcoord1, v_texcoord2, v_texcoord3), u_j3d_tev_texture_swizzles[2]);
                vec4 stageRas = gxSwizzleColor(v_color0, u_j3d_tev_raster_swizzles[2]);
                vec3 konstColor = gxTevKonstColor(u_j3d_tev_k_color_selectors.z);
                float konstAlpha = gxTevKonstAlpha(u_j3d_tev_k_alpha_selectors.z);
                vec3 colorResult = gxEvaluateTevColorStage(u_j3d_tev_stages[4], u_j3d_tev_stages[5], previous, reg0, reg1, reg2, stageTex, stageRas, konstColor);
                float alphaResult = gxEvaluateTevAlphaStage(u_j3d_tev_alpha_stages[4], u_j3d_tev_alpha_stages[5], previous, reg0, reg1, reg2, stageTex, stageRas, konstAlpha);
                lastColorDest = u_j3d_tev_color_dests.z;
                lastAlphaDest = u_j3d_tev_alpha_dests.z;
                gxStoreTevColor(lastColorDest, colorResult, previous, reg0, reg1, reg2);
                gxStoreTevAlpha(lastAlphaDest, alphaResult, previous, reg0, reg1, reg2);
            }
            if (u_j3d_params.x > 3.5) {
                vec4 stageTex = gxSwizzleColor(sampleStageTexture(u_j3d_tev_texture_indices.w, u_j3d_tev_indirects[6], u_j3d_tev_indirects[7], v_texcoord0, v_texcoord1, v_texcoord2, v_texcoord3), u_j3d_tev_texture_swizzles[3]);
                vec4 stageRas = gxSwizzleColor(v_color0, u_j3d_tev_raster_swizzles[3]);
                vec3 konstColor = gxTevKonstColor(u_j3d_tev_k_color_selectors.w);
                float konstAlpha = gxTevKonstAlpha(u_j3d_tev_k_alpha_selectors.w);
                vec3 colorResult = gxEvaluateTevColorStage(u_j3d_tev_stages[6], u_j3d_tev_stages[7], previous, reg0, reg1, reg2, stageTex, stageRas, konstColor);
                float alphaResult = gxEvaluateTevAlphaStage(u_j3d_tev_alpha_stages[6], u_j3d_tev_alpha_stages[7], previous, reg0, reg1, reg2, stageTex, stageRas, konstAlpha);
                lastColorDest = u_j3d_tev_color_dests.w;
                lastAlphaDest = u_j3d_tev_alpha_dests.w;
                gxStoreTevColor(lastColorDest, colorResult, previous, reg0, reg1, reg2);
                gxStoreTevAlpha(lastAlphaDest, alphaResult, previous, reg0, reg1, reg2);
            }
            if (lastColorDest > 0.5) {
                previous.rgb = gxTevRegisterColor(lastColorDest, previous, reg0, reg1, reg2);
            }
            if (lastAlphaDest > 0.5) {
                previous.a = gxTevRegisterAlpha(lastAlphaDest, previous, reg0, reg1, reg2);
            }
            gl_FragColor = gxApplyAlphaTest(previous);
            return;
        }
        texColor.a = max(texColor.a, secondaryColor.a);
    }

    gl_FragColor = gxApplyAlphaTest(texColor * v_color0);
}
