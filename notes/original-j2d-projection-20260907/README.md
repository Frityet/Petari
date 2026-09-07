# Original J2D projection boundary

Imported original J2DGrafContext and J2DOrthoGraph outside Game, recovered their missing overload/type functions in decomp, and recovered J2DOrthoGraphSimple constructor plus original MR projection wrapper. Native TColor preserves the original RGBA packed word across host byte order.

The explicit J2D graph CPU fixture builds, links and passes using actual Aurora GX state: viewport, scissor, virtual placement, asymmetric clip bounds, orthographic coefficients and packed color channels. Wii depth range is [-1,0]; initial test expectations used the wrong depth translation sign and were corrected against original C_MTXOrtho.

The Simple graph needs actual JUTVideo / RuntimeContext. A first combined fixture reached that boundary and correctly failed for an absent runtime owner. It is now a separate OriginalJ2DProjectionOwnerTests fixture requiring the real disc and renderer; that fixture is source ready, not yet linked or run. There is no full-camera or gameplay claim.

Three complete reference TUs compile with Wii; reference-proof.json records all comparisons. The recovered MR wrapper and Simple setPort are100%. Simple constructor is61.973682% (early integer-to-float conversions and TBox helper inlining); this is a functional recovery with identical dimensions, calls and bounds, not a high-fuzzy claim. Existing GrafContext constructor52.017242% also differs in helper inlining. Original OrthoGraph constructors, setOrtho, setPort, setLookat are100%; recovered TBox assignment and four-float virtual place also100%.
