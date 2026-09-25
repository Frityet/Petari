# Upstream-preferred decomp merge, 2026-09-25

Merged all 325 incoming upstream commits through `cae17c10e9a8c59a019e81da7454736f71a84595` into the fork's `pcp-decomp` branch. Published merge: `1a126cb5da311fedff662f53fb31c5aeaf851408`; verified `origin/pcp-decomp` resolves to it and upstream is its ancestor.

## Resolution policy

The user explicitly requested all upstream changes, preferring upstream. Started with `git merge --no-ff --no-commit -X theirs upstream/master`, which left no unresolved Git conflicts. Compilation then exposed overlapping local/upstream recoveries that Git had combined textually: duplicate definitions, mixed variable/member names, malformed method bodies and obsolete explicit destructor declarations. Chose complete upstream implementations for these overlaps and retained nonoverlapping recovered providers where still required. The four repair notes document the decisions. No native `src/Game` counterpart was changed by this merge.

## Validation

`python3 configure.py --no-progress`, then `ninja -j6 -k 0 all_source build/RMGK01/main.dol` passes. All 2,186 configured source targets (1,606 Game units) are built and up to date, and the original Wii toolchain links `main.elf` and emits `main.dol`. A final repeat reports no work to do. `validation.json` records the exact validated Git tree, output hashes and all failed/successful build attempts. This is compilation/link evidence, not exact retail matching or gameplay validation.

Upstream whitespace issues are retained with exact-line provenance in `whitespace-provenance.json`. The preexisting untracked `decomp/NPCUtil.d` is byte-identical. Parent staged edits and unrelated working changes were preserved; this parent checkpoint changes only the decomp gitlink and this evidence directory.

## Next goal

The user expanded the runtime goal from Rosalina spawning to Mario collecting the Grand Star and finishing Gateway Galaxy. This merge supplies newer original source for that work. Existing historical runs reached Rosalina, but no current Grand Star completion is claimed. Native imports require coherent headers, providers and resource/lifetime validation.
