# Performance Optimizations {#perfoptimization}

Since Kinoko does not aim to be a byte-matching decompilation, we can take creative liberty for the sake of improving performance. This page lists all performance optimizations that were added to Kinoko, ordered roughly in terms of largest to smallest improvement.

## 


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