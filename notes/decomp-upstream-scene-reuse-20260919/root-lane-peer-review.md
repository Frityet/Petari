# Read-only utility and library merge review

Reviewed `resolve-root.py` as a decision record; did not execute it. Compared the current sources with pre-merge HEAD and upstream. A conservative definition-name scan covered the 36 listed root-lane files, followed by manual review of flagged omissions and the requested template/header boundaries. This source review is not a compilation or exhaustive semantic-equivalence proof.

One actionable omission was found and reported: Functor.hpp had lost the local `Functor_Inline(void (*)())` and single-argument member overload. The first remains required by recovered `ParticleDrawExecutor.cpp:20`. Parent restored both; a subsequent read confirms them alongside the corrected const single-argument FunctorV1M return type. No production edits were made by this reviewer.

The focused preservation checks pass:

* ObjUtil's five recovered bodies (`getCsvDataBool`, `getCsvDataVec`, `getCsvDataColor`, `findNamePosOnGround`, matrix `tryFindLinkNamePos`) are present as definitions, with body tokens equal to HEAD after whitespace/comment normalization. The second vector-output overload remains separately defined. They are not left as upstream's forward declarations.
* TPartition3 has both two-vector and three-vector `set` definitions once. The three-vector operation keeps the same `cross(third-second, second-first)` order, normalizes it, and projects the first point; the two-vector operation copies the normal and computes its point dot. Out-of-class template placement is the reason the quick scanner initially flagged them absent.
* Array's constructor/destructor and push_back bodies are retained out of class. `Vector::insert` has one implementation, preserving shift-before-write and count increment. Rect has one `SetHeight` implementation and its independent MoveTo/Normalize methods.
* CharWriter retains cursor X/Y getters/setters, cursor motion, both SetCursor and SetScale overloads, font/alpha/color accessors and the upstream assertion-bearing helpers. There is one SetCursorX and one SetCursorY definition. This differs from the stale initial mutation script comment because parent already reconciled the duplication.
* BothDirList keeps the recovered bool constructor and upstream default constructor. JointController keeps both name-based and index-based creator overloads. FurBank was relocated from FurCtrl.hpp to FurMulti.hpp with its constructor and check/regist declarations present, not deleted. Layout text-operation execute methods are retained in LayoutUtil.hpp while the recursive traversal stays in the source.

No additional concrete lost public function or duplicated definition was found in this bounded pass. `root-lane-peer-review.json` records the exact checked hashes and ObjUtil body comparisons. The parent's canonical compilation is still the authoritative next check for template instantiation and complete dependency closure.
