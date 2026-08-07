# Final-binary route regression

The final reviewed binary retained the required route:

| Checkpoint | Frame | Render packets | Non-black | Capture SHA-256 |
| --- | ---: | ---: | ---: | --- |
| Title | 90 | 22 | 1.0000 | `9b2f57887d8644b5ad8f714d9436a0be55038373dc05c27e126eac58a2320f23` |
| Six-slot file select | 1900 | 27 | 0.9998 | `f100e44af973418fa5140837b06f83b347b69d50cf035fa1ab015971756fa7c8` |
| Picturebook | 7600 | 390 | 1.0000 | `e145ba374696c0cf89cef835b32518ba3a18860f3ad9810642219003053f6196` |
| Gateway handoff | 10350 | 1063 | 1.0000 | `0b6523928d288cba6395969c126a430d11ee8e5adb92110ca689688a9737dbf3` |

Each SQLite trace validator required the `pc-port` emulator identity, exact
capture frame, frame/render/semantic records, semantic events, and at least one
render packet. Title additionally required `TitleLogo`, file select required
`FileNumber`, and picturebook required `PrologueDemo` plus `IconAButton`.
Gateway placement validation retained 242 objects: 168 created, 72
intentionally ignored, and two blocked. Its required two rail coins, three demo
rabbits, five-point rabbit rail, and middle-zone star-piece group were present.

The four PNGs are byte-identical to the already committed baseline captures in
`../demo-scene-definitions-20260807T010317Z/route-smoke/`, so this note records
their hashes instead of committing duplicate 1.2 MiB files. They were also
visually inspected: the title logo, all six file planets, localized picturebook
page, and Mario on the Gateway flower planetoid rendered correctly.

The first final-binary aggregate attempt reused one explicit X display for all
four scenarios. Its file-select replacement Xvfb raced the previous server's
teardown and failed to bind; the app log showed SDL's `No available video
device` before game logic began. Title, picturebook, and Gateway passed in that
run. File select then passed alone on fresh display `:307`, completing the set.
This is recorded in `verification.log`; it was not treated as a game failure.
