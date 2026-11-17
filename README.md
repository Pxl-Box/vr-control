# VR Control

Wireless PC VR on Linux for the Meta Quest (Quest 2, 3, 3S, Pro), with no Meta PC software
and no SteamVR. One window does the whole thing:

- **Start VR / Stop VR**: turns on a private 5 GHz Wi-Fi hotspot from your PC and starts the
  [WiVRn](https://github.com/WiVRn/WiVRn) streaming server.
- **Setup tab**: checks everything you need, with ✔ / ✘ marks and one-click fixes, links or
  commands for anything missing. It can install WiVRn and create the hotspot for you.
- **Pairing code**: shows the code to type on the headset the first time it connects.
- **Live status**: headset connected, how long, game running, refresh rate, bitrate,
  hotspot devices and Wi-Fi traffic, GPU and video-encoder load (NVIDIA).
- **Games in the headset**: fills WiVRn's in-headset game list with your installed Steam VR
  games and adds the WiVRn launch options to every one of them, so you never type them in.
- **Streaming presets**: Latency / Balanced / Quality switch the video codec and bit depth
  WiVRn uses on the PC.
- **Headset audio**: optionally switches sound output and/or mic to the headset while it's
  connected, and back afterwards.

Built and tested on Pop!\_OS 24.04 (COSMIC), NVIDIA RTX 3080 Ti, Quest 2, WiVRn 26.9.
It should work on any distro with NetworkManager and Flatpak.

## Install

```bash
git clone https://github.com/Pxl-Box/vr-control.git
cd vr-control
./install.sh
```

The installer checks the requirements, installs WiVRn from Flathub if you say yes, checks
your firewall, adds **VR Control** to your app menu and prints a setup checklist. Anything that
needs `sudo` is shown and only run if you agree.

Prefer an AI assistant to do it? Paste [LLM-SETUP.md](LLM-SETUP.md) into one that can run
terminal commands (for example Claude Code).

Remove: `./uninstall.sh`

## Requirements

| Need | Why | Checked by the app |
|---|---|---|
| Linux PC with a GPU that can encode video (NVIDIA, AMD or Intel) | WiVRn encodes the stream | ✔ |
| Python 3 + GTK 3 (`python3-gi`, `gir1.2-gtk-3.0`) | runs VR Control | installer |
| Flatpak + Flathub | installs WiVRn | ✔ (one-click fix) |
| WiVRn (`io.github.wivrn.wivrn`) | the streaming server | ✔ (one-click install) |
| NetworkManager + a Wi-Fi card that supports hotspot (AP) mode, 5 GHz ideally | private Wi-Fi for the headset | ✔ (one-click hotspot) |
| Avahi | headset finds the PC automatically | ✔ |
| Firewall ports 9757 tcp/udp, 5353 udp open | WiVRn connection + discovery | ✔ (shows the command) |
| PipeWire/PulseAudio `pactl` | headset audio switching | ✔ |
| Steam (native, not Flatpak, is easiest) | Steam games | ✔ |
| **WiVRn app on the headset**, same version as the PC | the headset side | link + instructions |

**No hotspot-capable Wi-Fi card?** Use your router instead: plug the PC into the router with a
cable and put the headset on the router's 5 GHz Wi-Fi. Skip the hotspot and just turn on the
WiVRn server.

## First-time setup

1. Run `./install.sh`, then open **VR Control**. Fix anything marked ✘ on the **Setup** tab.
2. **Headset**: install the **WiVRn** app on the Quest. It's on the Meta Horizon Store
   (search "WiVRn"), or you can sideload the APK from
   [WiVRn releases](https://github.com/WiVRn/WiVRn/releases). The headset app version must
   match the PC version shown on the Setup tab.
3. Press **Start VR**. On the headset, join the hotspot Wi-Fi. The Control tab shows its
   network name and password.
4. Open WiVRn on the headset and pick your PC. The first time, press **Start pairing** in
   VR Control and type the 6-digit code into the headset.
5. **Steam games**: on the Setup tab, press **Set for all VR games** if it's marked ✘.
   VR Control writes these launch options into every installed Steam VR game, closing and
   reopening Steam to save them (a backup `localconfig.vdf.before-vr-control` is kept):
   ```
   OXR_PARALLEL_VIEWS=0 XR_RUNTIME_JSON=/home/<you>/.local/share/flatpak/app/io.github.wivrn.wivrn/current/active/files/share/openxr/1/openxr_wivrn.json PRESSURE_VESSEL_FILESYSTEMS_RW=$XDG_RUNTIME_DIR/wivrn/comp_ipc %command%
   ```
   Any options you already had (like `gamemoderun %command%` or game arguments) are kept.
   **Start VR** also does this automatically whenever Steam is closed. For non-Steam apps,
   copy the line from the Setup tab.
6. Start games from the WiVRn app's game list in the headset, or from Steam on the PC.

Each time after that: **Start VR** → put the headset on → WiVRn → play → **Stop VR**.

## Troubleshooting

| Problem | Fix |
|---|---|
| Game says *OpenXR runtime not found* | The launch options are missing or wrong. Press **Set for all VR games** on the Setup tab. WiVRn's own suggestion (`PRESSURE_VESSEL_IMPORT_OPENXR_1_RUNTIMES=1`) wasn't enough with the Flatpak. |
| **Beat Saber pauses itself** and re-pauses as soon as you resume | Include `OXR_PARALLEL_VIEWS=0` in the launch options (already in the copied text). |
| Headset says *waiting for an app* but the game list is empty | WiVRn 26.9 reads the list from `steamapps.vrmanifest`, which current Steam no longer writes; VR Control writes it for you (Setup tab → "Headset game list"). Reconnect the headset after installing games. Also, WiVRn is a Flatpak and can't see Steam libraries on other drives. Use **Let WiVRn see them** on the Setup tab, or run `flatpak override --user --filesystem=/path/to/SteamLibrary:ro io.github.wivrn.wivrn`, then restart the WiVRn server. Restart Steam after installing new games. |
| No game sound in the headset | Turn on "Switch sound output to the headset", or press **Send audio to headset**. |
| PC doesn't appear in the headset list | Check the firewall line on the Setup tab and that Avahi is running. |
| Picture is blurry | Raise the bitrate in **Open WiVRn settings** (around 100 Mbit/s works well on 5 GHz) and the resolution scale in the headset app. Adjust the headset's IPD slider and fit too. |
| Stuttering | Use 5 GHz, keep the headset in line of sight of the PC, and don't use the hotspot card for internet at the same time. |
| SteamVR | Not used. WiVRn includes its own replacement ([xrizer](https://github.com/Supreeeme/xrizer)), so don't install or start SteamVR. |
| Desktop overlays (WayVR/WlxOverlay-S) on COSMIC with 2+ monitors | The pointer lands in the wrong place (cosmic-comp maps absolute pointer input onto one monitor). Not used by this app. |

Run `vr-control --check` in a terminal for the same checklist as the Setup tab.

## Streaming presets and latency

The preset card changes what the PC controls in WiVRn 26.9: the video codec and bit depth.
It applies the next time the headset connects.

| Preset | Codec on the PC | For |
|---|---|---|
| Latency | H.264, 8-bit (quickest to encode/decode) | Beat Saber and other fast games |
| Balanced (default) | WiVRn default: H.265, 10-bit | most games |
| Quality | H.265, 10-bit, always | slow games, sightseeing |

Refresh rate, resolution and bitrate can only be changed in the **headset app**
(WiVRn → Settings). For lower latency set:

| Setting | Value | Why |
|---|---|---|
| Refresh rate | **Automatic** | WiVRn then picks the highest rate the game sustains (120 Hz on a decent GPU). A new frame every 8 ms instead of 11–14 ms is the biggest win. |
| Resolution scale | 1.0 (default 1.2) | Less to encode, send and decode per frame. |
| Bitrate | 60–80 Mbit/s with Latency, 50 otherwise | H.264 needs a bit more for the same look. |

Also: use 5 GHz, keep the headset in line of sight of the PC, and keep other devices off the
hotspot. WiVRn's in-game menu (click **both thumbsticks**) → **Stats** shows encode, network
and decode times, so you can compare settings.

## No-pause layer (Beat Saber)

Beat Saber pauses whenever the headset takes focus (holding the Meta button to recenter, the
system menu, the guardian) or its proximity sensor says the headset came off, and afterwards it
can keep re-pausing every time you resume. `install.sh` adds a tiny OpenXR API layer
(`nopause-layer/`) that hides those two signals from the game, so it never pauses for them.
The pause button still works.

- It only acts on games listed in `VRCONTROL_NOPAUSE_APPS` (default `Beat Saber`; `*` = all).
  Other games pass straight through.
- Turn it off for a game with the launch option `DISABLE_VRCONTROL_NOPAUSE=1`.
- What it hid is logged to `~/.local/state/vr-control/nopause-layer.log`.
- `nopause-layer/test_layer.c` checks its behaviour without a headset
  (`gcc test_layer.c -ldl && ./a.out`).

## How it works

- Headset game list: VR games are read from Steam's `appcache/appinfo.vdf` (`openvrsupport`)
  and written to `<steam>/config/steamapps.vrmanifest`, the file WiVRn 26.9 reads.
- Launch options: written to `userdata/<id>/config/localconfig.vdf` (`LaunchOptions`) only
  while Steam is closed, since Steam overwrites that file when it exits.

- Hotspot: an ordinary NetworkManager access-point connection
  (`nmcli con up/down <name>`), so it also shows up in your normal network settings.
- WiVRn server: `flatpak run --command=wivrn-server io.github.wivrn.wivrn`, controlled and
  monitored over WiVRn's D-Bus API (`io.github.wivrn.Server`).
- Audio: `pactl set-default-sink/source` plus moving running streams to `wivrn.sink` /
  `wivrn.source`. Your previous devices are saved in `~/.config/vr-control/prev-audio.json`.
- Settings: `~/.config/vr-control/settings.json`.

## Credits

All the streaming is done by [WiVRn](https://github.com/WiVRn/WiVRn); this app is a front-end
for it. Not affiliated with Meta or Valve.

## License

MIT
