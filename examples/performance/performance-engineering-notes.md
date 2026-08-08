# Performance Engineering Notes

This document summarizes selected performance work from the private research implementation. It intentionally omits research-specific formulas and algorithm internals.

## Context

The project runs inside Autodesk Maya, so numerical work that is acceptable in an offline script can become a usability problem when it executes on every drag update or animation frame. The implementation therefore separates work into:

- **Bind-time** work that can be performed once
- **Endpoint-cache** work that is recomputed only when a keyframe endpoint changes
- **Per-frame** work that must stay lightweight enough for interactive playback

## Selected bottlenecks and fixes

### 1. Scalar per-vertex evaluation

The original hot path evaluated complex basis sums in nested scalar loops and updated Maya geometry one vertex at a time.

**Change:** reorganized the data so dense matrix-vector products can be executed with Eigen / MKL-backed kernels, while keeping cached basis matrices available across frames.

**Why it matters:** for meshes with tens of thousands of vertices, memory access patterns and vectorized linear algebra dominate the cost of an otherwise simple mathematical expression.

### 2. Repeated MATLAB Engine transfers

Several solver paths moved large matrices between MATLAB and C++ more often than necessary. A dimension query could also cause an additional transfer before the actual data fetch.

**Change:** added single-fetch matrix access paths that retrieve a variable once, inspect its dimensions from the returned array, and copy directly into native buffers.

**Why it matters:** the MATLAB Engine is an out-of-process boundary, so transfer volume and call count matter independently of numerical FLOPs.

### 3. Recomputing basis data in hot loops

Some boundary and derivative basis values were rebuilt during repeated checks.

**Change:** cache reusable basis rows at bind time and consume those cached matrices during later validation and solve steps.

### 4. Repeated numerical factorization

Some reconstruction paths solved multiple right-hand sides using the same system matrix.

**Change:** factor once and reuse the factorization for multiple right-hand sides where mathematically valid.

### 5. Parallel bind-time work

Independent topology/boundary calculations were parallelized with OpenMP where they had no shared-write hazards.

One implementation detail was avoiding `std::vector<bool>` for parallel scratch storage because its packed-bit representation can introduce write races between logically distinct elements. A byte-addressable scratch buffer was used instead and packed later.

## Verification approach

Performance changes were not accepted solely because they compiled or appeared faster. They were checked against reference outputs using seeded numerical inputs and relative-error comparisons. Changes to solver kernels were also regression-tested to ensure that optimization did not alter the numerical result beyond floating-point tolerance.

## Engineering takeaway

The main lesson from this work was that interactive numerical software requires attention to more than algorithmic complexity. Data layout, API-call granularity, process boundaries, caching, and factorization reuse can dominate real runtime even when the mathematics itself is unchanged.
