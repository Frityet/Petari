$input v_color0, v_texcoord0

uniform sampler2D s_tex;

void main()
{
    vec4 texColor = texture2D(s_tex, v_texcoord0);
    gl_FragColor = texColor * v_color0;
}
