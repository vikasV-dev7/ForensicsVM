# Engineering Rules

These rules are mandatory. Code that violates these rules will be rejected.

1. **No logic dumped into `main.cpp`.** `main.cpp` only serves as an entry point.
2. **No monolithic manager class.** Ensure Single Responsibility Principle (SRP).
3. **No direct QEMU dependencies in domain-level code.** All hypervisor interactions must go through the virtualization abstraction layer.
4. **Interfaces belong at architectural boundaries.** Use dependency inversion to maintain layer independence.
5. **Infrastructure implementations must remain replaceable.** Infrastructure code (like InMemory or QEMU adapters) MUST reside in `src/infrastructure/` and NEVER in `src/core/application` or domain layers.
6. **Evidence must never be modified accidentally.** Assume read-only by default.
7. **Destructive operations require explicit intent.** 
8. **Tests are required for core behavior.** 
9. **Every major architectural decision gets an ADR.**
10. **Build must remain warning-clean.** 
11. **Do not add dependencies without architectural justification.** 
12. **Do not implement functionality merely because it is listed in the future roadmap.** 
13. **Do not claim a feature is implemented unless it is actually implemented and tested.**
14. **No generic DI container unless future complexity justifies it.** Use explicit constructor injection.
15. **Composition belongs in the Composition Root.** (`ApplicationBootstrap` instantiates concrete types; everything else depends on abstractions).
16. **Do not create speculative abstractions without current use.**
