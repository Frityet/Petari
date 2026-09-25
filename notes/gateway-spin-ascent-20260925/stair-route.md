# Controller-only tower stair route

`stair-route.json` supplies the requested `points` plus `reach_distance: 85`. It is a geometry-derived candidate, **not a tested ascent**. No production edits, builds, tests, actor-position writes or input actions were performed in this task.

The first point is the known grounded endpoint `[14916.831,-9164.090,8241.563]` from the last route. In the current tower frame it is `(-925.9,1900.2,91.8)`, angle174.3°. This is already the first tread, KCL prism236; the post-demo point `[15344,-10002,8664]` is lower/outside at local `(-1480.8,1295.4,715.6)`. The approach point uses the parent's previously observed direct approach, not a newly verified path across that lower section.

The following 26 points use means of paired coplanar KCL triangle centres, staying inside each tread. Travel clockwise in local X/Z (decreasing atan2(z,x)), around the tower rather than straight toward Rosalina. Lower treads lie around radius1020; upper treads broaden to around1130. The old ten-point route cut large curved sections and skipped individual height changes. The revised sequence follows them at roughly12° intervals.

Important rises, with zero-based point indices: **6** (+76.7), **14** (+75.6), **16** (+93.6 bump), **20** (+148.2), **23** (+200), **26** (+100 through the parapet opening). Point17 descends the isolated bump. Use an ordinary A jump if grounded and stalled against these risers; release A between attempts. A flat tangent input cannot climb the200-unit vertical face. Existing stall-jump logic is appropriate, but this document does not assert its timing succeeds here.

Point26 is the actual low parapet opening, paired prisms271/272 at local Y2500 near angle-114°. Its local radial interval is about805–884, contiguous with the preceding Y2400 outer tread. Points27/28 move inward across the top floor toward Rosalina. Clear steering if the real proximity-triggered tutorial/demo begins; continuing to drive toward a fixed position during the demo is unnecessary.

The JSON transforms local geometry using the **current** actor1108 base matrix at frame17990, not the slightly different historical matrix stored in steps-geometry.json. That transform was checked against current ground prism236 by its shared local vertex identities. Source evidence: `notes/demo-system-verification-20260919/physics/steps-geometry.json`, current `route-actors.jsonl`, and historical `final2-tower-block-ui.png` (visually shows the riser/curving stairs). The historical screenshot is not proof of current rendered or collision state.
