# Web

| File | Content |
| --- | --- |
| `CandleBridge.*` | Snapshot and requests between the loop task and the web worker task (spinlock). |
| `routes.candle.*` | `GET` / `POST /api/candle`, reads the snapshot and forwards requests to the bridge. |

The web UI itself lives in `data/web/index.html` and is uploaded with `uploadfs`.
