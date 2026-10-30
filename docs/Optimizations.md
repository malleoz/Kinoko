# Performance Optimizations {#perfoptimization}

Since Kinoko does not aim to be a byte-matching decompilation, we can take creative liberty for the sake of improving performance. This page lists all performance optimizations that were added to Kinoko, ordered roughly in terms of largest to smallest improvement.

## KCL Prism Prefetch

`.kcl` files are comprised of multiple different sections. In brief, the following sections are relevant:
- The header (@ref Kinoko::Field::KColData::KColHeader "KColHeader")
- **Section 1:** Vertex position vectors
- **Section 2:** Prism normal vectors
- **Section 3:** Array of prisms (@ref Kinoko::Field::KColData::KCollisionPrism "KCollisionPrism")

The header contains offsets to each of the above sections within the file. Each individual prism stores the following:
- Index into section 1 for the first vertex's position
- Index into section 2 for the prism's face normal
- Index into section 2 for the prism's first edge normal
- Index into section 2 for the prism's second edge normal
- Index into section 2 for the prism's third edge normal

Since a prism maps to its vertices and normals with indices, accessing them requires pointer arithmetic overhead to seek throughout the file data, every time the game checks for collision against a KCL prism.

We can eliminate this overhead by pre-emptively fetching the vertex position and normals for all prisms, such that they can all exist in a struct that is contiguous in memory. With the prism data now existing in a `0x40` byte struct in memory, cache locality results in a substantial speedup. Though this results in slightly longer race initialization time, this is negligible given that KCL collision checks are the hottest path when profiling.

@see @ref Kinoko::Field::KColData "KColData" | @ref Kinoko::Field::KColData::KColHeader "KColHeader" | @ref Kinoko::Field::KColData::KCollisionPrism "KCollisionPrism" | @ref Kinoko::Field::KColData::KCollisionPrismGeometry "KCollisionPrismGeometry"


## TODO
- KCLCollisionPrismGeometry prefetch - https://github.com/vabold/Kinoko/pull/533
- Skip KartCamera https://github.com/vabold/Kinoko/pull/534
- KColData plane distance check first https://github.com/vabold/Kinoko/pull/546
- KColData early return if any distance is less than 0
- Skip Lakitu https://github.com/vabold/Kinoko/pull/543
- Skip Seagulls https://github.com/vabold/Kinoko/pull/542
- Remove multiplayer Hitbox::calc call https://github.com/vabold/Kinoko/pull/538
- Heap zero init https://github.com/vabold/Kinoko/pull/519
- AREA early return https://github.com/vabold/Kinoko/pull/545
- BoxColManager iterator loop: https://github.com/vabold/Kinoko/pull/547
- Faster checkpoint search tracking https://github.com/vabold/Kinoko/pull/544
- Skip timer https://github.com/vabold/Kinoko/pull/541
- Optimize firebar matrix https://github.com/vabold/Kinoko/pull/540
- Handlebar matrix and suspension down https://github.com/vabold/Kinoko/pull/539
- RailSmoothInterpolator::calcT early return https://github.com/vabold/Kinoko/pull/535
- Estimator step count https://github.com/vabold/Kinoko/pull/525
- KartSub::DT https://github.com/vabold/Kinoko/pull/524
- world inertia tensor https://github.com/vabold/Kinoko/pull/523