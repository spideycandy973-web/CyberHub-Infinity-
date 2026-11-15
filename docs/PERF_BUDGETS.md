# Performance Budgets

| Tier | FPS | Frame Budget |
|------|-----|-------------|
| Low | 30 | 33.3 ms |
| Mid | 60 | 16.6 ms |
| High | 60-120 | 16.6-8.3 ms |

## Mid tier target (M3+)

| Stage | Budget |
|-------|--------|
| Culling + draw setup | 2 ms |
| Shadow (CSM) | 3 ms |
| Forward/GBuffer | 4 ms |
| Lighting + toon | 2 ms |
| Post (bloom, TAA, tonemap) | 3 ms |
| UI | 1 ms |
| CPU logic | 2 ms |
| Headroom | 0.6 ms |
