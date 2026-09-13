# Actor-owned shadow drawer registration

The twenty-first real-disc original run completed Mario initialization and started placed Coin construction. A missing mirror helper threw during Coin initialization. SceneObjHolder rollback then deleted the registered ShadowDrawer before deleting its actor; ShadowControllerOwnership subsequently deleted the same drawer again and correctly triggered the allocation provenance guard.

ShadowControllerOwnership now claims the registered drawer at adoption through the existing general NameObj ownership API. Scene construction continues to observe it for original callbacks, while rollback and normal scene ownership exclude it from independent deletion. The actor owner remains responsible for the actual drawer and controller. This covers every supported drawer kind; no allocator guard, exception, or stage behavior is suppressed.

Validation is the next actual stage-load run together with the mirror owner fix. No new test suite was added. Failure evidence: ../gateway-wakeup-demo-20260912/original-app-twenty-first-run.log.

Thirty-first production build passed. Twenty-second actual stage run passed Coin creation and reached the SphereAir model resource load. That next failure is the resource layer rereading an already mounted archive while the model command scope has disabled scheduling. This run establishes construction progress; it does not exercise rollback again or establish completed stage loading.
