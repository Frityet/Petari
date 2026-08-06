# Reproducible RMGK02 full decomp build

Updated: 2026-08-06T19:22:45Z

## Outcome

The root decompilation can again be configured, incrementally compiled, fully linked, and hash-verified against the extracted Korean Rev 1 executable:

```text
python3 configure.py --version RMGK02 --non-matching --verbose
ninja
[276/277] LINK build/RMGK02/main.elf
[277/277] DOL build/RMGK02/main.dol

sha1sum build/RMGK02/main.dol
54b71431af0d509097bfdef4ec28617afc487e89  build/RMGK02/main.dol
```

The generated hash exactly matches both `orig/RMGK02/sys/main.dol` and `config/RMGK02/build.sha1`. A follow-up `ninja` reports `no work to do`.

## Configuration restoration

Upstream commit `577f4af0b` removed Korean Rev 1 support even though this workspace's complete extracted data and authoritative assembly/object evidence are RMGK02. The former configuration was restored with current generated-output conventions:

- `configure.py` accepts `RMGK01` and `RMGK02` and sets `VERSION=0/1` respectively.
- `config/RMGK02/config.yml` uses the known Rev 1 main-DOL hash while sharing the existing Korean symbols/splits.
- `config/RMGK02/build.sha1` verifies the generated DOL rather than the original input.
- The top-level README again documents both revisions.

No disc image or extracted `orig/` content is tracked.

## Header repairs exposed by the clean build

Regeneration rebuilt hundreds of units and found three independent stale-header problems. Each was repaired narrowly and verified against RMGK02 before continuing:

1. `CameraGeneralParam` was missing the signed low/high halfword accessors used by `CamTranslatorSpiral`; the repaired unit is 100% exact.
2. `FileSelectModel.hpp` repeated the same four nerve declarations; removing the duplicate block leaves its unit 100% exact.
3. `StageDataHolder.hpp` repeated two identical method declarations; the affected methods and formerly blocked `SceneDataInitializer` are 100% exact.

Detailed assembly, compiler, and objdiff evidence is in:

- `camera-general-param-build-repair-20260806T191544Z`
- `file-select-model-build-repair-20260806T191755Z`
- `stage-data-holder-build-repair-20260806T192009Z`

These repairs change no PC game code and make the root decomp workflow required by `AGENT_DECOMP_GUIDE.md` reproducible for subsequent Gateway work.

