# Hardware Setup Guide

## Supported Hardware

| Category | Supported Models | Interface |
|---|---|---|
| Industrial camera | Hikvision MVS (USB3/GigE), Daheng DVP2 | USB 3.0 / GigE |
| Motion controller | Serial port (custom protocol) | RS-232/RS-485 |
| PLC motion controller | Modbus TCP | Ethernet |
| Light source | Serial port | RS-232 |

---

## Camera Setup

### Hikvision MVS

1. Install Hikvision MVS SDK (tested with v3.4.x)
2. Connect camera via USB 3.0 or GigE
3. Open MVS client to verify camera is visible
4. Note the camera's serial number (used as `camera_id` in config)

### Daheng DVP2

1. Install Daheng Galaxy SDK
2. Connect camera via USB 3.0
3. Verify with Galaxy Viewer

### GigE Cameras

Assign a static IP to the camera in the same subnet as the server PC's NIC:
- Camera IP: `192.168.1.100`
- Server NIC IP: `192.168.1.1`
- Disable Windows firewall for the GigE adapter or add exception for `fuguang-server.exe`

---

## Motion Control Setup

### Serial Port Mode

Hardware: custom motion controller board communicating over RS-232/485.

```xml
<motion_control>
    <type>port</type>
    <port_name>COM3</port_name>
    <baud_rate>115200</baud_rate>
</motion_control>
```

Verify in Device Manager that the COM port is recognized. Use a terminal (PuTTY, SSCOM) to confirm the controller responds to commands.

### PLC Mode (Modbus TCP)

Hardware: Mitsubishi/Siemens PLC with Ethernet module.

```xml
<motion_control>
    <type>plc</type>
    <ip_address>192.168.1.200</ip_address>
    <port>502</port>
    <port_name>COM4</port_name>  <!-- For light source serial -->
    <baud_rate>9600</baud_rate>
</motion_control>
```

Verify Modbus connectivity: `modpoll -m tcp -a 1 -r 1 -c 1 192.168.1.200`

#### Modbus Register Map

| Register | ID | Description |
|---|---|---|
| HD304 | 304 | X-axis move distance |
| HD314 | 314 | Y-axis move distance |
| HD310 | 310 | X-axis speed |
| HD320 | 320 | Y-axis speed |
| HD1000 | 1000 | X-axis target position |
| HD1010 | 1010 | Y-axis target position |
| M610 | 610 | Execute X relative move |
| M611 | 611 | Execute Y relative move |
| M1000 | 1000 | Execute X absolute move |
| M1001 | 1001 | Execute Y absolute move |
| SM1000 | 1000 | X-axis moving status |
| SM1020 | 1020 | Y-axis moving status |
| HSD0 | 0 | X-axis current position |
| HSD4 | 4 | Y-axis current position |

---

## System Wiring Checklist

- [ ] Camera USB/GigE cable connected and strain-relieved
- [ ] Motion controller power on, home position set
- [ ] Serial cable (DB9) connected between PC COM port and controller
- [ ] Light source power on, serial cable connected
- [ ] All axes homed before first run (`reset_axis` command)
- [ ] `config.xml` position values verified against physical inspection positions

---

## First-Time Calibration

1. Start server: `fuguang-server.exe --config config.xml`
2. Connect UI: open fuguang-ui, File → Connect → `127.0.0.1:5555`
3. Open camera (Camera → Open)
4. Move to inspection position (Motion → Move To Position)
5. Adjust exposure/gain until fiber end-face is clearly visible
6. Run `start_process` once with `auto_detect=0` to verify motion path
7. Adjust `position.x/y` in config to center fiber connector under camera
8. Set `pixel_physical_size` by imaging a calibration target with known feature size
9. Save config (File → Save Config)
