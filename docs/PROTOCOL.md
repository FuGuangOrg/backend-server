# TCP Communication Protocol

## Transport Layer

All communication between `fuguang-server` and `fuguang-ui` uses a simple framing protocol over TCP:

```
 0       1       2       3       4                     4+N
 ┌───────────────────────┬──────────────────────────────┐
 │  Length (4 bytes BE)  │   Payload (UTF-8 JSON, N bytes) │
 └───────────────────────┴──────────────────────────────┘
```

- **Length prefix:** 4 bytes, big-endian unsigned integer, value = byte length of payload
- **Payload:** UTF-8 encoded JSON object
- **Port:** 5555 (commands + small JSON results)

### Image Delivery (port 5556, remote mode only)

When the client connects from a non-localhost IP, images are streamed on a separate TCP connection (port 5556) to avoid blocking the command channel:

```
 0       3       4+H                 4+H+B
 ┌──────────────┬────────────────────┬──────────────────┐
 │ Length (4 BE)│  JSON header (H B) │ JPEG bytes (B B) │
 └──────────────┴────────────────────┴──────────────────┘
```

JSON header example:
```json
{
  "frame_id": 12345,
  "type": "trigger",
  "width": 1920,
  "height": 1080,
  "jpeg_quality": 85
}
```

`type` values: `"trigger"` (raw capture), `"annotated"` (with detection overlays).

On same-machine connections (`127.0.0.1`), images use `QSharedMemory` keys `trigger_image` and `detect_image` for zero-copy delivery.

---

## Request/Response Format

Every request from the client includes a `request_id` field. The server echoes it in every response so the client can match async replies.

### Base Request

```json
{
  "request_id": "uuid-string",
  "command": "<command_name>",
  ... command-specific fields ...
}
```

### Base Response

```json
{
  "request_id": "uuid-string",
  "command": "<command_name>",
  "success": true,
  "task_finished": true,
  "error_code": 0,
  "error_message": "",
  ... result-specific fields ...
}
```

`task_finished = false` means the server will send more responses for the same `request_id` (multi-stage commands like `start_process`).

---

## Command Reference

### `open_camera`

Open and initialize the camera.

**Request:**
```json
{
  "request_id": "...",
  "command": "open_camera",
  "camera_id": "cam_0"
}
```

**Response:**
```json
{
  "request_id": "...",
  "command": "open_camera",
  "success": true,
  "task_finished": true,
  "camera_params": {
    "width": 1920,
    "height": 1080,
    "exposure": 10000,
    "gain": 100
  }
}
```

---

### `close_camera`

```json
{ "request_id": "...", "command": "close_camera", "camera_id": "cam_0" }
```

---

### `set_camera_param`

```json
{
  "request_id": "...",
  "command": "set_camera_param",
  "camera_id": "cam_0",
  "param_name": "exposure",
  "param_value": 15000
}
```

---

### `start_stream`

Start continuous image capture. Server sends one image response per frame until `stop_stream` is called.

```json
{ "request_id": "...", "command": "start_stream", "camera_id": "cam_0" }
```

Each frame response:
```json
{
  "request_id": "...",
  "command": "start_stream",
  "task_finished": false,
  "frame_id": 42,
  "shared_memory_key": "trigger_image"
}
```

---

### `stop_stream`

```json
{ "request_id": "...", "command": "stop_stream" }
```

---

### `trigger`

Single-shot capture.

```json
{ "request_id": "...", "command": "trigger" }
```

Response:
```json
{
  "request_id": "...",
  "command": "trigger",
  "success": true,
  "task_finished": true,
  "frame_id": 43,
  "shared_memory_key": "trigger_image"
}
```

---

### `start_process`

Main inspection workflow: move to position → auto-focus → detect defects. The server sends multiple intermediate responses during execution.

**Request:**
```json
{ "request_id": "...", "command": "start_process" }
```

**Intermediate response (per camera position):**
```json
{
  "request_id": "...",
  "command": "start_process",
  "task_finished": false,
  "stage": "moving",
  "position_index": 0,
  "pos_x": 11920,
  "pos_y": 3000
}
```

**Intermediate response (after auto-focus):**
```json
{
  "request_id": "...",
  "command": "start_process",
  "task_finished": false,
  "stage": "focused",
  "position_index": 0,
  "shared_memory_key": "detect_image"
}
```

**Intermediate response (after detection, one per fiber end-face):**
```json
{
  "request_id": "...",
  "command": "start_process",
  "task_finished": false,
  "stage": "detected",
  "fiber_index": 0,
  "pass": true,
  "detect_boxes": [
    {
      "zone": "A",
      "boxes": [
        { "score": 0.85, "x0": 10, "y0": 20, "x1": 30, "y1": 40 }
      ]
    }
  ]
}
```

**Final response:**
```json
{
  "request_id": "...",
  "command": "start_process",
  "task_finished": true,
  "success": true
}
```

---

### `stop_process`

Interrupt an in-progress `start_process`.

```json
{ "request_id": "...", "command": "stop_process" }
```

---

### `move`

Direct axis movement command.

```json
{
  "request_id": "...",
  "command": "move",
  "axis": "x",
  "mode": "distance",
  "value": 1000,
  "speed": 3000
}
```

`mode`: `"distance"` | `"position"`

---

### `reset_axis`

Home the specified axis.

```json
{ "request_id": "...", "command": "reset_axis", "axis": "x" }
```

---

### `get_position`

```json
{ "request_id": "...", "command": "get_position" }
```

Response:
```json
{
  "request_id": "...",
  "command": "get_position",
  "success": true,
  "task_finished": true,
  "x": 11920,
  "y": 3000,
  "z": 0
}
```

---

### `set_light`

```json
{
  "request_id": "...",
  "command": "set_light",
  "frequency": 1000,
  "duty_cycle": 80
}
```

---

### `get_server_config`

```json
{ "request_id": "...", "command": "get_server_config" }
```

Response includes the full `st_config_data` serialized to JSON.

---

### `set_server_config`

```json
{
  "request_id": "...",
  "command": "set_server_config",
  "config": { ... }
}
```

---

### `enum_devices`

Enumerate available cameras.

```json
{ "request_id": "...", "command": "enum_devices" }
```

Response:
```json
{
  "request_id": "...",
  "command": "enum_devices",
  "success": true,
  "task_finished": true,
  "devices": [
    { "camera_id": "cam_0", "model": "MVS-CA050-10UC", "serial": "00D5..." }
  ]
}
```

---

## Error Codes

| Code | Meaning |
|---|---|
| 0 | Success |
| 1 | Unknown command |
| 2 | Camera not open |
| 3 | Motion control not initialized |
| 4 | Process already running |
| 5 | Hardware communication timeout |
| 6 | Config file not found |
| 7 | Algorithm initialization failed |
| 99 | Internal server error |
