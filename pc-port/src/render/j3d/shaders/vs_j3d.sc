$input a_position, a_color0, a_texcoord0, a_texcoord1, a_texcoord2, a_texcoord3
$output v_color0, v_texcoord0, v_texcoord1, v_texcoord2, v_texcoord3

#include <bgfx_shader.sh>

void main()
{
    gl_Position = mul(u_modelViewProj, a_position);
    v_color0 = a_color0;
    v_texcoord0 = a_texcoord0;
    v_texcoord1 = a_texcoord1;
    v_texcoord2 = a_texcoord2;
    v_texcoord3 = a_texcoord3;
}
