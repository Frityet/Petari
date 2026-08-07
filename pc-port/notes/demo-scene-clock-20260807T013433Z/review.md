# Independent source-fidelity review

The final shared tree was reviewed read-only against the root decompiled demo
sources. The reviewer approved it with no remaining high- or medium-priority
findings.

Three generic fidelity gaps found during review were fixed and regression
tested before approval:

1. Ordinary registered starts now infer Mario-puppetable ownership from the
   actor-selected executor's parsed Player-row count. The explicit Mario
   overload still forces puppetable ownership.
2. `MR::endDemo` treats owner and name as informational and always terminates
   the current demo, matching `DemoDirector::endDemo`.
3. Void time-keep APIs retain the existing no-registry programmable fallback;
   try APIs continue to reject safely without a registry.

The reviewer separately confirmed main/SubPart update order, pause retention,
suspend and natural-end boundaries, the one-frame final overshoot, main-before-
SubPart and duplicate-first lookup, and first-membership actor queries. The only
deliberate differences are the documented safe null/unknown sentinels and a
zero-total rate of `0.0`.
