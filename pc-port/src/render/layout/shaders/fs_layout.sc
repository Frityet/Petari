$input v_color0, v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_tex, 0);

void main()
{
    vec4 texColor = texture2D(s_tex, v_texcoord0);
    gl_FragColor = texColor * v_color0;
}
