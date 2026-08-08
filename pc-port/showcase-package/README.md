# Super Mario Galaxy PC title showcase

This is packaging collateral for the current **title-only** Linux showcase. It
runs the exact retail `TitleSequenceProduct` and loads its exact retail assets
at runtime from a user-supplied retail disc image. It does not package gameplay,
stage selection, or any other showcase mode.

No disc image, extracted disc file, or game asset is included in this directory
or copied by the packaging script. You must provide your own legally obtained
disc image when you launch the showcase.

## Requirements

- Linux x86_64 with glibc 2.34 or newer
- An X11 display, either native X11 or Xwayland
- A Vulkan-capable GPU and Vulkan driver
- A readable retail disc image supported by Aurora

## Run the packaged showcase

Pass the disc-image path as the first argument:

```sh
./run-title-showcase.sh /path/to/your/disc-image
```

Alternatively, set `SMGPC_DISC_IMAGE`:

```sh
SMGPC_DISC_IMAGE=/path/to/your/disc-image ./run-title-showcase.sh
```

The first argument takes precedence when both are present. The launcher
resolves the package location independently of the current working directory
and executes:

```text
bin/smg-pc-showcase title --disc PATH
```

At the title prompt, hold keyboard **A+B** or **Enter+Backspace**.

## Create a package directory

From the `pc-port` directory, build the `smg-pc-showcase` target, then provide
the resulting executable and a new destination directory:

```sh
./showcase-package/package-showcase.sh \
  ./build/linux/x86_64/release/smg-pc-showcase \
  ./dist/smg-pc-title-showcase
```

The destination must not already exist. The script uses an explicit allowlist:
it copies only `bin/smg-pc-showcase`, `run-title-showcase.sh`, `README.md`, the
project `LICENSE`, and a generated `BUILD-INFO`. `BUILD-INFO` records the source
Git commit, the Aurora Git commit, each worktree state, and the packaged
executable's SHA-256 digest.

To publish an archive, archive only that generated destination directory. Do
not add a disc image or extracted assets to it.
