# Actual showcase application smoke

After native checkpoint 2e6a86270, built the runnable `smg-pc-showcase` target
on macOS arm64. `smg-pc-mario-gateway-walk-slice` is its static library,
not an executable. Build exit 0; smoke exit 0:

```
build/macosx/arm64/debug/smg-pc-showcase gateway --disc '../Super Mario Wii - Galaxy Adventure (Korea).rvz' --smoke --max-frames 360
```

The final app report passes 28 rendered frames, ordinary PlanetMap and real
animated Mario packet submission with an on-screen actor center, GPU draw
submission, probe gravity acceleration, and exact planet KCL contact.
This smoke is a rendering/lifecycle/probe check, not the complete bunny demo
or user-controlled jumping. Local logs: build-showcase.log and
showcase-smoke.log (not committed).
