# Independent original pane animation layers

The sixteenth original Gateway run passed stage archive/object table initialization and reached GameScene::initSequences. PauseMenu declares one root layer and two layers for its Stars pane; updateStarPane legitimately starts Star on Stars layer 1. The native runtime threw because it redundantly checked that index against the root's one-layer count.

Removed the seven root-player validation calls from named-pane animation operations and their debug read. LayoutPaneCtrl still checks every actual Game operation against its own declared pane layer count, and native pane storage still applies its own checked indexing. Root animation operations retain their original root count validation. No layer was fabricated, no count enlarged and no PauseMenu/Game source changed.

Exact stack: ../gateway-wakeup-demo-20260912/original-app-sixteenth-backtrace.log. Existing original LayoutPaneCtrl constructor and LayoutUtil pane dispatch establish the independent player arrays. No component test campaign; the next production run is the validation target.

Twenty-sixth production build passed. Seventeenth real-disc run passed GameScene::initSequences and reached StageDataHolder::initPlacementMario; the next crash is an absent Mario creator registration, with a valid actual Mario start iterator. Receipts and exact frame values are ../gateway-wakeup-demo-20260912/original-app-seventeenth-{run.json,backtrace.log,mario-creator.log}.
