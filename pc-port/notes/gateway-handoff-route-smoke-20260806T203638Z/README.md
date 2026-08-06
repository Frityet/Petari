# Gateway Handoff Route Smoke

Updated: 2026-08-06T20:36:38Z

## Outcome

`scripts/aurora_route_smoke.lua` now has an opt-in `gateway_handoff` scenario that drives the existing title and file-select route, advances all five picturebook stops with separated A-button pulses, and captures the resulting `HeavensDoorGalaxy` stage at frame 10350.

The default `title`, `file_select`, and `picturebook` scenario list and input scripts remain unchanged.

## Scenario Contract

- A pulses: `8000-8010`, `8400-8410`, `8800-8810`, `9200-9210`, and `9600-9610`.
- Scenario-only environment: `SMGPC_DEMO_ROUTE=heavensdoor_after_picturebook`.
- Capture frame: `10350`.
- Placement report summary:
  - `stage: HeavensDoorGalaxy`
  - `scenario: 1`
  - `total_objects: 242`
  - `intentionally_ignored_objects: 72`
  - `created_objects >= 164`
  - `blocked_objects <= 6`
- Exactly two placement records must match `status=created` and `object=RailCoin`; exactly two must also have `rail_info_attached=true`, proving both created RailCoins own attached paths.

Placement validation is generic: scenarios can assert exact, minimum, and maximum report summary fields plus counted object-field predicates with exact, minimum, and maximum counts. The created/blocked thresholds allow future source-close actor imports to improve support without breaking the route gate. The generated report path, expected contract, and parsed validation summary are included in each scenario manifest.

The stage placement report is emitted by the debug runtime, so this validation requires the normal debug `smg-pc` build when overriding `--pc-bin`.

## Real-disc Verification

Command:

```text
xmake aurora-route-smoke \
  --disc=/workspaces/pcport/RMGK01.wbfs \
  --work-dir=/workspaces/pcport/pc-port/.cache/aurora-route-smoke/gateway-handoff-20260806T203638Z \
  --timeout=300 \
  gateway_handoff
```

Result:

```text
aurora-route-smoke: gateway_handoff passed nonblack=0.9806 render_packets=553
aurora-route-smoke: passed
```

The app trace applied the `HeavensDoorGalaxy` scene change at frame 9724. At capture frame 10350, the trace contained 553 render packets and 1964 semantic events. The generated placement report observed 164 created, 6 blocked, and 72 ignored objects and passed every summary threshold and RailCoin assertion.

Artifacts:

- `.cache/aurora-route-smoke/gateway-handoff-20260806T203638Z/gateway_handoff/gateway_handoff-frame-10350.png`
- `.cache/aurora-route-smoke/gateway-handoff-20260806T203638Z/gateway_handoff/gateway_handoff-frame-10350.trace.sqlite`
- `.cache/aurora-route-smoke/gateway-handoff-20260806T203638Z/gateway_handoff/gateway_handoff-app.log`
- `.cache/aurora-route-smoke/gateway-handoff-20260806T203638Z/gateway_handoff/gateway_handoff-placement-report.md`
- `.cache/aurora-route-smoke/gateway-handoff-20260806T203638Z/gateway_handoff/manifest.json`

## Default-scenario Regression

The unchanged default invocation was rerun with `--no-build` against the same WBFS:

```text
title passed nonblack=1.0000 render_packets=22
file_select passed nonblack=0.9998 render_packets=27
picturebook passed nonblack=1.0000 render_packets=13
aurora-route-smoke: passed
```

No `Game/`, runtime, renderer, or visual-diff source was changed for this task.
