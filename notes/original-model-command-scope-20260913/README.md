# Original model-loading scheduling

The twenty-fifth original app run reached TicoBaby's still-pending FileLoader archive request. The exact original `ModelManager::init` legitimately called `FileHolderFileEntry::waitReadDone`, but the old native `ModelManagerOwner` wrapper disabled scheduling around the entire method. The SDK correctly rejected that blocking sleep.

Removed that obsolete whole-init command scope and its whole-J3DSys snapshot/restore. Current serialized guest execution and the now-imported original model creation code supersede the standalone assumptions recorded in the September 3 model-manager notes. The original `MR::newJ3DModel` mutexes and `ProhibitSchedulerAndInterrupts` command-generation region remain unchanged, as do the narrow ResourceHolder/J3D loader scopes and every SDK scheduler check. Exception-only recovery still releases mutex acquisitions abandoned by a native exception.

The actual command generators clear their GD pointer at the end (`J3DDisplayListObj::endDL`, `J3DShape::makeVcdVatCmd`, `J3DModelData::indexToPtr`), and loader scopes retain their existing GD restoration. No new resource prefetch or duplicate original initialization logic was introduced.

Evidence: `../gateway-wakeup-demo-20260912/original-app-twenty-fifth-backtrace.log`. The thirty-fifth production build passed. The twenty-sixth real run passed the pending resource wait, completed `SceneFunction::startActorPlacement`, and reached `GameScene::init` line 151: post-placement event setup. It then stopped because an authored Power Star had no creator registration in the native factory. This establishes completed actor-placement execution for the supported set, not complete stage initialization, rendering or gameplay. No component tests were added or run.
