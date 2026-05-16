$input v_texcoord0, v_texcoord1, v_texcoord2, v_color0

#include "common.sh"

SAMPLER2D(s_gx_texture0, 0);
SAMPLER2D(s_gx_texture1, 1);
SAMPLER2D(s_gx_texture2, 2);

uniform vec4 u_gx_material_params;
uniform vec4 u_gx_tev_color_in0;
uniform vec4 u_gx_tev_color_in1;
uniform vec4 u_gx_tev_color_in2;
uniform vec4 u_gx_tev_alpha_in0;
uniform vec4 u_gx_tev_alpha_in1;
uniform vec4 u_gx_tev_alpha_in2;
uniform vec4 u_gx_tev_color_op0;
uniform vec4 u_gx_tev_color_op1;
uniform vec4 u_gx_tev_color_op2;
uniform vec4 u_gx_tev_alpha_op0;
uniform vec4 u_gx_tev_alpha_op1;
uniform vec4 u_gx_tev_alpha_op2;
uniform vec4 u_gx_tev_out0;
uniform vec4 u_gx_tev_out1;
uniform vec4 u_gx_tev_out2;
uniform vec4 u_gx_tev_konst0;
uniform vec4 u_gx_tev_konst1;
uniform vec4 u_gx_tev_konst2;
uniform vec4 u_gx_tev_reg0;
uniform vec4 u_gx_tev_reg1;
uniform vec4 u_gx_tev_reg2;
uniform vec4 u_gx_tev_reg3;
uniform vec4 u_gx_alpha_compare0;
uniform vec4 u_gx_alpha_compare1;

vec2 project_tex_coord(vec3 coord)
{
    if (abs(coord.z) > 0.00001) {
        return coord.xy / coord.z;
    }
    return coord.xy;
}

float tev_bias(float bias)
{
    if (bias < 0.5) {
        return 0.0;
    }
    if (bias < 1.5) {
        return 128.0;
    }
    if (bias < 2.5) {
        return -128.0;
    }
    return 0.0;
}

float tev_lshift(float scale)
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
    return 1.0;
}

float tev_rshift(float scale)
{
    return scale > 2.5 ? 2.0 : 1.0;
}

vec3 read_color_arg(float arg, vec4 reg0, vec4 reg1, vec4 reg2, vec4 reg3, vec4 texture_color, vec4 raster, vec4 konst)
{
    if (arg < 0.5) {
        return reg0.rgb;
    }
    if (arg < 1.5) {
        return vec3_splat(reg0.a);
    }
    if (arg < 2.5) {
        return reg1.rgb;
    }
    if (arg < 3.5) {
        return vec3_splat(reg1.a);
    }
    if (arg < 4.5) {
        return reg2.rgb;
    }
    if (arg < 5.5) {
        return vec3_splat(reg2.a);
    }
    if (arg < 6.5) {
        return reg3.rgb;
    }
    if (arg < 7.5) {
        return vec3_splat(reg3.a);
    }
    if (arg < 8.5) {
        return texture_color.rgb;
    }
    if (arg < 9.5) {
        return vec3_splat(texture_color.a);
    }
    if (arg < 10.5) {
        return raster.rgb;
    }
    if (arg < 11.5) {
        return vec3_splat(raster.a);
    }
    if (arg < 12.5) {
        return vec3_splat(1.0);
    }
    if (arg < 13.5) {
        return vec3_splat(128.0 / 255.0);
    }
    if (arg < 14.5) {
        return konst.rgb;
    }
    return vec3_splat(0.0);
}

float read_alpha_arg(float arg, vec4 reg0, vec4 reg1, vec4 reg2, vec4 reg3, vec4 texture_color, vec4 raster, vec4 konst)
{
    if (arg < 0.5) {
        return reg0.a;
    }
    if (arg < 1.5) {
        return reg1.a;
    }
    if (arg < 2.5) {
        return reg2.a;
    }
    if (arg < 3.5) {
        return reg3.a;
    }
    if (arg < 4.5) {
        return texture_color.a;
    }
    if (arg < 5.5) {
        return raster.a;
    }
    if (arg < 6.5) {
        return konst.a;
    }
    return 0.0;
}

vec3 tev_regular_color(vec4 operation, vec3 a, vec3 b, vec3 c, vec3 d)
{
    float lshift = tev_lshift(operation.z);
    float rshift = tev_rshift(operation.z);
    vec3 ai = floor(a * 255.0 + 0.5);
    vec3 bi = floor(b * 255.0 + 0.5);
    vec3 ci = floor(c * 255.0 + 0.5);
    vec3 di = floor(d * 255.0 + 0.5);
    vec3 c256 = ci + floor(ci / 128.0);
    vec3 temp = ai * (256.0 - c256) + bi * c256;
    temp *= lshift;
    if (operation.z < 2.5) {
        temp += operation.x > 0.5 ? 127.0 : 128.0;
    }
    temp = floor(temp / 256.0);
    if (operation.x > 0.5) {
        temp = -temp;
    }
    vec3 result = ((di + tev_bias(operation.y)) * lshift + temp) / rshift;
    if (operation.w > 0.5) {
        result = clamp(result, 0.0, 255.0);
    } else {
        result = clamp(result, -1024.0, 1023.0);
    }
    return result / 255.0;
}

float tev_regular_alpha(vec4 operation, float a, float b, float c, float d)
{
    float lshift = tev_lshift(operation.z);
    float rshift = tev_rshift(operation.z);
    float ai = floor(a * 255.0 + 0.5);
    float bi = floor(b * 255.0 + 0.5);
    float ci = floor(c * 255.0 + 0.5);
    float di = floor(d * 255.0 + 0.5);
    float c256 = ci + floor(ci / 128.0);
    float temp = ai * (256.0 - c256) + bi * c256;
    temp *= lshift;
    if (operation.z < 2.5) {
        temp += operation.x > 0.5 ? 127.0 : 128.0;
    }
    temp = floor(temp / 256.0);
    if (operation.x > 0.5) {
        temp = -temp;
    }
    float result = ((di + tev_bias(operation.y)) * lshift + temp) / rshift;
    if (operation.w > 0.5) {
        result = clamp(result, 0.0, 255.0);
    } else {
        result = clamp(result, -1024.0, 1023.0);
    }
    return result / 255.0;
}

void write_color_register(inout vec4 reg0, inout vec4 reg1, inout vec4 reg2, inout vec4 reg3, float register_index, vec3 color)
{
    if (register_index < 0.5) {
        reg0.rgb = color;
    } else if (register_index < 1.5) {
        reg1.rgb = color;
    } else if (register_index < 2.5) {
        reg2.rgb = color;
    } else {
        reg3.rgb = color;
    }
}

void write_alpha_register(inout vec4 reg0, inout vec4 reg1, inout vec4 reg2, inout vec4 reg3, float register_index, float alpha)
{
    if (register_index < 0.5) {
        reg0.a = alpha;
    } else if (register_index < 1.5) {
        reg1.a = alpha;
    } else if (register_index < 2.5) {
        reg2.a = alpha;
    } else {
        reg3.a = alpha;
    }
}

vec3 read_color_register(float index, vec4 reg0, vec4 reg1, vec4 reg2, vec4 reg3)
{
    if (index < 0.5) {
        return reg0.rgb;
    }
    if (index < 1.5) {
        return reg1.rgb;
    }
    if (index < 2.5) {
        return reg2.rgb;
    }
    return reg3.rgb;
}

float read_alpha_register(float index, vec4 reg0, vec4 reg1, vec4 reg2, vec4 reg3)
{
    if (index < 0.5) {
        return reg0.a;
    }
    if (index < 1.5) {
        return reg1.a;
    }
    if (index < 2.5) {
        return reg2.a;
    }
    return reg3.a;
}

void run_tev_stage(vec4 texture_color, vec4 color_in, vec4 alpha_in, vec4 color_operation, vec4 alpha_operation, vec4 outputs, vec4 konst,
                   inout vec4 reg0, inout vec4 reg1, inout vec4 reg2, inout vec4 reg3, vec4 raster)
{
    vec3 color_a = read_color_arg(color_in.x, reg0, reg1, reg2, reg3, texture_color, raster, konst);
    vec3 color_b = read_color_arg(color_in.y, reg0, reg1, reg2, reg3, texture_color, raster, konst);
    vec3 color_c = read_color_arg(color_in.z, reg0, reg1, reg2, reg3, texture_color, raster, konst);
    vec3 color_d = read_color_arg(color_in.w, reg0, reg1, reg2, reg3, texture_color, raster, konst);
    vec3 color = tev_regular_color(color_operation, color_a, color_b, color_c, color_d);
    write_color_register(reg0, reg1, reg2, reg3, outputs.x, color);

    float alpha_a = read_alpha_arg(alpha_in.x, reg0, reg1, reg2, reg3, texture_color, raster, konst);
    float alpha_b = read_alpha_arg(alpha_in.y, reg0, reg1, reg2, reg3, texture_color, raster, konst);
    float alpha_c = read_alpha_arg(alpha_in.z, reg0, reg1, reg2, reg3, texture_color, raster, konst);
    float alpha_d = read_alpha_arg(alpha_in.w, reg0, reg1, reg2, reg3, texture_color, raster, konst);
    float alpha = tev_regular_alpha(alpha_operation, alpha_a, alpha_b, alpha_c, alpha_d);
    write_alpha_register(reg0, reg1, reg2, reg3, outputs.y, alpha);
}

vec4 texture_for_stage(float texture_stage, vec4 texture0, vec4 texture1, vec4 texture2)
{
    if (texture_stage < 0.5) {
        return texture0;
    }
    if (texture_stage < 1.5) {
        return texture1;
    }
    return texture2;
}

bool compare_alpha(float alpha, float compare, float reference)
{
    if (compare < 0.5) {
        return false;
    }
    if (compare < 1.5) {
        return alpha < reference;
    }
    if (compare < 2.5) {
        return abs(alpha - reference) < 0.5 / 255.0;
    }
    if (compare < 3.5) {
        return alpha <= reference;
    }
    if (compare < 4.5) {
        return alpha > reference;
    }
    if (compare < 5.5) {
        return abs(alpha - reference) >= 0.5 / 255.0;
    }
    if (compare < 6.5) {
        return alpha >= reference;
    }
    return true;
}

bool passes_alpha_compare(float alpha)
{
    if (u_gx_alpha_compare1.y < 0.5) {
        return true;
    }
    bool lhs = compare_alpha(alpha, u_gx_alpha_compare0.x, u_gx_alpha_compare0.y);
    bool rhs = compare_alpha(alpha, u_gx_alpha_compare0.w, u_gx_alpha_compare1.x);
    if (u_gx_alpha_compare0.z < 0.5) {
        return lhs && rhs;
    }
    if (u_gx_alpha_compare0.z < 1.5) {
        return lhs || rhs;
    }
    if (u_gx_alpha_compare0.z < 2.5) {
        return lhs != rhs;
    }
    return lhs == rhs;
}

void main()
{
    vec4 raster = v_color0;
    vec4 texture0 = texture2D(s_gx_texture0, project_tex_coord(v_texcoord0));
    vec4 texture1 = texture2D(s_gx_texture1, project_tex_coord(v_texcoord1));
    vec4 texture2 = texture2D(s_gx_texture2, project_tex_coord(v_texcoord2));

    vec4 reg0 = u_gx_tev_reg0;
    vec4 reg1 = u_gx_tev_reg1;
    vec4 reg2 = u_gx_tev_reg2;
    vec4 reg3 = u_gx_tev_reg3;
    vec4 final_outputs = u_gx_tev_out0;

    run_tev_stage(texture_for_stage(u_gx_tev_out0.z, texture0, texture1, texture2), u_gx_tev_color_in0, u_gx_tev_alpha_in0, u_gx_tev_color_op0, u_gx_tev_alpha_op0, u_gx_tev_out0, u_gx_tev_konst0,
                  reg0, reg1, reg2, reg3, raster);
    if (u_gx_material_params.x > 1.5) {
        final_outputs = u_gx_tev_out1;
        run_tev_stage(texture_for_stage(u_gx_tev_out1.z, texture0, texture1, texture2), u_gx_tev_color_in1, u_gx_tev_alpha_in1, u_gx_tev_color_op1, u_gx_tev_alpha_op1, u_gx_tev_out1, u_gx_tev_konst1,
                      reg0, reg1, reg2, reg3, raster);
    }
    if (u_gx_material_params.x > 2.5) {
        final_outputs = u_gx_tev_out2;
        run_tev_stage(texture_for_stage(u_gx_tev_out2.z, texture0, texture1, texture2), u_gx_tev_color_in2, u_gx_tev_alpha_in2, u_gx_tev_color_op2, u_gx_tev_alpha_op2, u_gx_tev_out2, u_gx_tev_konst2,
                      reg0, reg1, reg2, reg3, raster);
    }

    vec4 result = vec4(read_color_register(final_outputs.x, reg0, reg1, reg2, reg3), read_alpha_register(final_outputs.y, reg0, reg1, reg2, reg3));
    if (!passes_alpha_compare(result.a)) {
        discard;
    }
    gl_FragColor = clamp(result, 0.0, 1.0);
}
