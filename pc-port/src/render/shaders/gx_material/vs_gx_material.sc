$input a_position, a_texcoord0, a_texcoord1, a_texcoord2, a_texcoord3, a_color0
$output v_texcoord0, v_texcoord1, v_texcoord2, v_color0

#include "common.sh"

void main()
{
    gl_Position = vec4(a_position.xyz, a_texcoord3);
    v_texcoord0 = a_texcoord0;
    v_texcoord1 = a_texcoord1;
    v_texcoord2 = a_texcoord2;
    v_color0 = a_color0;
}
