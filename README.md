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
5. **Steam games**: copy the launch options from the Setup tab into each VR game
   (right-click the game → Properties → Launch Options). They look like this:
   ```
   OXR_PARALLEL_VIEWS=0 XR_RUNTIME_JSON=/home/<you>/.local/share/flatpak/app/io.github.wivrn.wivrn/current/active/files/share/openxr/1/openxr_wivrn.json PRESSURE_VESSEL_FILESYSTEMS_RW=$XDG_RUNTIME_DIR/wivrn/comp_ipc %command%
   ```
6. Start games from the WiVRn app's game list in the headset, or from Steam on the PC.

Each time after that: **Start VR** → put the headset on → WiVRn → play → **Stop VR**.

## Troubleshooting

| Problem | Fix |
|---|---|
| Game says *OpenXR runtime not found* | The launch options are missing or wrong. Copy them again from the Setup tab. WiVRn's own suggestion (`PRESSURE_VESSEL_IMPORT_OPENXR_1_RUNTIMES=1`) wasn't enough with the Flatpak. |
| **Beat Saber pauses itself** and re-pauses as soon as you resume | Include `OXR_PARALLEL_VIEWS=0` in the launch options (already in the copied text). |
| Headset says *waiting for an app* but the game list is empty | WiVRn is a Flatpak and can't see Steam libraries on other drives. Use **Let WiVRn see them** on the Setup tab, or run `flatpak override --user --filesystem=/path/to/SteamLibrary:ro io.github.wivrn.wivrn`, then restart the WiVRn server. Restart Steam after installing new games. |
| No game sound in the headset | Turn on "Switch sound output to the headset", or press **Send audio to headset**. |
| PC doesn't appear in the headset list | Check the firewall line on the Setup tab and that Avahi is running. |
| Picture is blurry | Raise the bitrate in **Open WiVRn settings** (around 100 Mbit/s works well on 5 GHz) and the resolution scale in the headset app. Adjust the headset's IPD slider and fit too. |
| Stuttering | Use 5 GHz, keep the headset in line of sight of the PC, and don't use the hotspot card for internet at the same time. |
| SteamVR | Not used. WiVRn includes its own replacement ([xrizer](https://github.com/Supreeeme/xrizer)), so don't install or start SteamVR. |
| Desktop overlays (WayVR/WlxOverlay-S) on COSMIC with 2+ monitors | The pointer lands in the wrong place (cosmic-comp maps absolute pointer input onto one monitor). Not used by this app. |

Run `vr-control --check` in a terminal for the same checklist as the Setup tab.

## How it works

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
