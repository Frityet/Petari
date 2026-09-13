# Actor model instance heap ownership

The eighteenth actual Gateway run reaches MarioActor::initDrawAndModel, then fails its 0x50-byte ModelManager allocation. Native ModelManagerOwner selected the ResourceHolderService domain, which is the process root in the original process; that root has already reserved its actual child heaps.

The actor boundary now retains the exact current original heap before entering host metadata allocation. ModelManagerOwner uses that retained instance domain while separately retaining model/animation archive resources. Removed the invalid requirement that instance and resource heap identities match. Sound ownership follows the model instance domain, or the original current heap for actors without models. Native metadata remains host-allocated; Game ModelManager and its original child graph still use the selected Game heap. No heap sizes or Game methods were changed.

Production failure stack: ../gateway-wakeup-demo-20260912/original-app-eighteenth-run.log. No component test campaign; next production run is the validation target.

Twenty-eighth production build passed. Nineteenth real-disc run passed ModelManager allocation, progressed through Mario model initialization into TornadoMario::init, then exposed the SDK paired-single zero-normalization assertion discrepancy.

The twentieth production run reached MarioAnimator and exposed the same obsolete instance/resource domain identity requirement in MarioAnimatorLifetime. Removed that equality; the existing owner already retains the actual model domain and archive resources separately. Its actual ModelManager identity and resource service checks remain.

Thirtieth full production build passed. Twenty-first real-disc run completed Mario initialization and entered ordinary PlacementInfoOrdered actor construction. The next original exception is the existing unconditional tryCreateMirrorActor blocker while constructing a Coin, followed by a separately identified shadow rollback double-retirement panic. Exact first-exception stack: ../gateway-wakeup-demo-20260912/original-app-twenty-first-exception.log.
