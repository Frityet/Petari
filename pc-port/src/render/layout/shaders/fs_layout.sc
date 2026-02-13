$input v_color0, v_texcoord0, v_texcoord1

#include <bgfx_shader.sh>

SAMPLER2D(s_tex, 0);
SAMPLER2D(s_mask, 1);
uniform vec4 u_mask_params;

void main()
{
    vec4 texColor = texture2D(s_tex, v_texcoord0);
    if (u_mask_params.x > 0.5) {
        vec4 maskColor = texture2D(s_mask, v_texcoord1);
        float maskAlpha = u_mask_params.z > 0.5
            ? maskColor.a
            : dot(maskColor.rgb, vec3(0.33333334, 0.33333334, 0.33333334));
        if (u_mask_params.y > 0.5) {
            maskAlpha = 1.0 - maskAlpha;
        }
        texColor.a *= maskAlpha;
    }

    gl_FragColor = texColor * v_color0;
}
