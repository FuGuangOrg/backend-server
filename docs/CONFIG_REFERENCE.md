# Configuration File Reference

## config.xml

Located at `./config.xml` (relative to server binary). Override path with `--config <path>`.

```xml
<config>
    <!-- Illumination -->
    <light_brightness>1000000</light_brightness>

    <!-- Motion control -->
    <move_speed>3000</move_speed>
    <move_step_x>10</move_step_x>
    <move_step_y>10</move_step_y>
    <position x="11920" y="3000"/>

    <!-- Inspection -->
    <fiber_end_count>8</fiber_end_count>
    <auto_detect>1</auto_detect>

    <!-- Storage -->
    <save_path>./saveimages</save_path>
</config>
```

### Field Descriptions

| Field | Type | Default | Description |
|---|---|---|---|
| `light_brightness` | int | 1000000 | Light source intensity (hardware unit, range depends on model) |
| `move_speed` | int | 3000 | Default axis movement speed (pulse/s or hardware unit) |
| `move_step_x` | int | 10 | X-axis step size per increment command (hardware unit) |
| `move_step_y` | int | 10 | Y-axis step size per increment command (hardware unit) |
| `position.x` | int | 11920 | Absolute X position of first inspection point (hardware unit) |
| `position.y` | int | 3000 | Absolute Y position of first inspection point (hardware unit) |
| `fiber_end_count` | int | 8 | Number of fiber end-faces in each image (for multi-fiber connectors) |
| `auto_detect` | int | 1 | 1 = auto-run detection on hardware trigger; 0 = manual trigger only |
| `save_path` | string | `./saveimages` | Root directory for saving focus images and result images |

---

## parameter.xml (algorithm parameters)

Algorithm parameters are loaded separately. Default path: `./parameter.xml`.

```xml
<parameter>
    <!-- Shape matching -->
    <score_thresh>0.5</score_thresh>
    <gradient_thresh>10.0</gradient_thresh>
    <pyramid_level>1</pyramid_level>
    <scale_min>0.98</scale_min>
    <scale_max>1.02</scale_max>
    <scale_step>0.01</scale_step>

    <!-- Defect detection -->
    <detect_type>0</detect_type>
    <mean_kernel_size>15</mean_kernel_size>
    <mean_image_offset>5</mean_image_offset>
    <min_area>0.0</min_area>
    <anomaly_dark>1</anomaly_dark>
    <anomaly_bright>1</anomaly_bright>

    <!-- Physical calibration -->
    <pixel_physical_size>1.0</pixel_physical_size>

    <!-- IEC 61300-3-35 zone definitions -->
    <detect_areas>
        <area name="A区" diameter_min="0" diameter_max="25" enabled="true">
            <anomaly_stat min="0" max="1" max_count="1"/>
            ...
        </area>
        <area name="B区" diameter_min="25" diameter_max="50" enabled="true"/>
        <area name="C区" diameter_min="50" diameter_max="75" enabled="true"/>
        <area name="D区" diameter_min="75" diameter_max="100" enabled="true"/>
    </detect_areas>

    <!-- Auto-focus -->
    <search_range>100</search_range>
    <move_speed>250</move_speed>
    <move_step>5</move_step>

    <!-- Auto-adjustment -->
    <adjustment_search_range>800</adjustment_search_range>
    <adjustment_move_speed>2000</adjustment_move_speed>
    <adjustment_move_step>50</adjustment_move_step>
</parameter>
```

### Algorithm Field Descriptions

| Field | Default | Description |
|---|---|---|
| `score_thresh` | 0.5 | Minimum shape match score to accept a detection |
| `gradient_thresh` | 10.0 | Minimum gradient magnitude for feature extraction |
| `pyramid_level` | 1 | Number of image pyramid levels for shape matching |
| `scale_min/max/step` | 0.98/1.02/0.01 | Scale variation range for shape model search |
| `detect_type` | 0 | 0=circular anomaly, 1=scratch/linear anomaly |
| `mean_kernel_size` | 15 | Kernel size (px) for local mean background estimation |
| `mean_image_offset` | 5 | Binarization threshold offset: defect if `|diff| > offset` |
| `min_area` | 0.0 | Minimum contour area (px²) to report as defect |
| `anomaly_dark` | 1 | Enable dark defect detection (1=on) |
| `anomaly_bright` | 1 | Enable bright defect detection (1=on) |
| `pixel_physical_size` | 1.0 | Physical size of one pixel in micrometers |
| `search_range` | 100 | Auto-focus Z-sweep range in motor units |
| `move_speed` (focus) | 250 | Z-axis speed during focus sweep |
| `move_step` (focus) | 5 | Z-axis step size during focus sweep |

---

## camera_configs.xml

Camera-specific parameters (exposure, gain, trigger mode). One `<camera>` block per physical camera.

```xml
<cameras>
    <camera id="cam_0">
        <exposure>10000</exposure>
        <gain>100</gain>
        <trigger_mode>hardware</trigger_mode>
        <width>1920</width>
        <height>1080</height>
    </camera>
</cameras>
```

---

## Notes

- All XML files are parsed by `pugixml` in the server layer. The algorithm library (`fuguang-algo`) never reads files directly.
- Config changes at runtime: send `set_server_config` or `set_algorithm_param` commands over TCP; changes take effect immediately and are persisted to XML on `save_config`.
- Paths in XML support both absolute (`C:\data\saveimages`) and relative paths (relative to the server binary directory).
