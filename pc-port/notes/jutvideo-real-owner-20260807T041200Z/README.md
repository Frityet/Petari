# JUTVideo runtime ownership

`JUTVideo` no longer creates a private `WiiVideoService` or implicitly creates
its manager from `getManager()`.

`RuntimeContext` now creates the manager after the real runtime instance is
installed and destroys it during runtime teardown. Video operations require
that owner through `RuntimeContext::instance()`; outside the runtime,
`getManager()` reports absence with `nullptr`.

This retains the retail singleton-shaped JUT surface without using an
unowned process-global substitute.
