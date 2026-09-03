#!/usr/bin/env python3
"""Reproduce the bounded source checks for the 2026-09-03 Aurora merge."""
import hashlib
import json
import subprocess
from pathlib import Path

NOTE_DIR = Path(__file__).resolve().parent
ROOT = NOTE_DIR.parents[1] / 'aurora'
BASELINE = 'deab54e'
UPSTREAM = 'f1189541e5d8b97fdf61946377853488d504d9df'
ANCESTOR = '1d10fa1bc502910a6336fdac32f31cd0ac39710d'

def git(*args):
    return subprocess.check_output(['git', '-C', str(ROOT), *args])

assert git('merge-base', BASELINE, UPSTREAM).decode().strip() == ANCESTOR
assert len(git('rev-list', BASELINE + '..' + UPSTREAM).splitlines()) == 21
assert not git('diff', '--name-only', '--diff-filter=U').strip()
subprocess.run(['git', '-C', str(ROOT), 'diff', '--cached', '--check'], check=True)

preserved = {}
for path in ('lib/dolphin/mtx/mtx.c', 'include/dolphin/mtx.h',
             'include/dolphin/ppc_math.h', 'include/functional.hpp'):
    data = (ROOT / path).read_bytes()
    assert data == git('show', BASELINE + ':' + path), path
    preserved[path] = hashlib.sha256(data).hexdigest()

assert (ROOT / 'lib/dolphin/mtx/vec.c').read_bytes() == git('show', UPSTREAM + ':lib/dolphin/mtx/vec.c')
assert not (ROOT / 'lib/gfx/common.cpp').exists()
assert not (ROOT / 'lib/gfx/common.hpp').exists()

# The module migration must retain these local surface declarations/boundaries.
required = {
    'lib/gfx/types.hpp': ['struct CopyFilter', 'coefficients{0, 64, 0}'],
    'lib/gfx/frame_packet.hpp': ['PipelineRef pipeline', 'taggedDepthSnapshot', 'captureLegacyDepthSnapshot'],
    'lib/gfx/recording.cpp': ['.pipeline = data.pipeline', 'struct CopyUniformBlock',
                             'request_depth_snapshot', 'invalidate_surface_resources'],
    'lib/gfx/encoding.cpp': ['wait_for_pipeline(command.data.draw.pipeline)',
                            'encode_tagged_snapshot', 'needsScaling || needsFiltering'],
    'lib/gx/command_processor.cpp': ['require_draw_array_spans', 'checked_array_span_end',
                                   'GX_AURORA_REQUEST_TAGGED_DEPTH_SNAPSHOT', 'GX_AURORA_LOAD_COPY_FILTER'],
    'lib/gx/gx.cpp': ['viewport.znear != g_gxState.logicalViewport.znear',
                      'viewport.zfar != g_gxState.logicalViewport.zfar'],
    'lib/gx/regs.cpp': ['ox - 342.0f', '(oz - sz) / 16777216.0f', 'array.requiredSize = 0'],
    'lib/gx/shader.cpp': ['depth_range: vec2f', 'depth_range_pad: vec2u', 'enable clip_distances;'],
}
for path, snippets in required.items():
    source = (ROOT / path).read_text()
    for snippet in snippets:
        assert snippet in source, (path, snippet)

result = {
    'baseline': git('rev-parse', BASELINE).decode().strip(),
    'upstream': UPSTREAM,
    'common_ancestor': ANCESTOR,
    'incoming_commits': 21,
    'preserved_sha256': preserved,
    'upstream_vec_c_sha256': hashlib.sha256((ROOT / 'lib/dolphin/mtx/vec.c').read_bytes()).hexdigest(),
    'source_invariants': 'passed',
    'scope': 'Source retention and merge structure only; native build and runtime recorded separately.',
}
print(json.dumps(result, indent=2))
