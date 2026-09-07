# Player compiler portability fixes

MarioFpView now uses static_cast<char*>(nullptr), because reinterpret_cast from nullptr_t is ill-formed in Clang. MarioStick selects the original u32 startPadVib overload explicitly; 0ul is a 64-bit unsigned long on the host and otherwise ambiguously matches the integer and pointer overloads. These are compiler/architecture fixes with no intended behavior change.

Fresh native LLVM 23 compilation succeeds for both full translation units. Fresh Wii baseline and modified compilation also succeeds; objdiff reports all 20 MarioFpView and 78 MarioStick compared symbols at 100% equivalence. The exact commands and concise symbol equivalence summary are retained. These files are not yet activated in the normal Game archive by this change; this is not a movement runtime claim.
