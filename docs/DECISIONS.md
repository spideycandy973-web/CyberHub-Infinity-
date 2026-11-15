# Decisions

## M0

| # | Decision | Rationale |
|---|----------|-----------|
| 1 | Work only on main branch | User constraint. |
| 2 | GameActivity 2.0.2 | Stable with prefab support. |
| 3 | arm64-v8a only | Covers most active devices. |
| 4 | UNORM swapchain | Tonemap in M3 handles sRGB encode. |
| 5 | Pre-rotation | Correct landscape on portrait-native phones. |
| 6 | Fully offline | No INTERNET permission. |
| 7 | Validation layers not packaged yet | Planned for M1. |
| 8 | AndroidGlue.c compiled with -w | Suppress third-party warnings. |
| 9 | No VMA at M0 | Introduced in M1. |
| 10 | Per-image renderFinished semaphores | Avoids reuse bugs. |
| 11 | ALooper_pollOnce | pollAll is deprecated. |
| 12 | FIFO present mode | Always available. |
