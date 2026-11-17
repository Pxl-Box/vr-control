# Set up wireless Quest VR on Linux with an AI assistant

Copy everything inside the box below into an AI assistant that can run commands on your
computer, for example Claude Code (`claude` in a terminal, started inside this folder).
It will check your system, install what's missing and walk you through the headset side.
It asks before anything that needs your password.

````text
You are setting up wireless PC VR streaming to a Meta Quest headset on this Linux PC, using
WiVRn and the "VR Control" app in the current folder (a git clone of the vr-control repo).
Work step by step, run checks yourself, and explain each result in plain language.

Rules:
- Ask me before running anything with sudo, and show me the exact command first.
- Never ask for, type or store my password; I'll type it into sudo prompts myself.
- Don't install SteamVR or Meta software. WiVRn replaces both.
- Don't change settings unrelated to this setup.

Steps:
1. Identify the distro (/etc/os-release), GPU (lspci | grep -iE 'vga|3d'), whether
   NetworkManager (nmcli) and Flatpak are installed, and the Wi-Fi device
   (nmcli -t -f DEVICE,TYPE dev). For the Wi-Fi device, check hotspot support with
   nmcli -f WIFI-PROPERTIES dev show <dev> (AP and 5GHZ should be "yes").
2. Make sure Python 3 + GTK 3 bindings are installed (Debian/Ubuntu: python3-gi
   gir1.2-gtk-3.0; Fedora: python3-gobject gtk3; Arch: python-gobject gtk3).
3. Run ./install.sh --yes from this folder. It installs VR Control for the current user and
   installs WiVRn from Flathub (flatpak install --user flathub io.github.wivrn.wivrn).
   --yes never runs sudo; tell me about any sudo commands it printed and offer to run them.
4. Run ~/.local/bin/vr-control --check and fix every line marked ✘:
   - Flathub missing: flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
   - WiVRn missing: flatpak install -y --user flathub io.github.wivrn.wivrn
   - No hotspot connection (and the Wi-Fi card supports AP mode): ask me for a network name
     and a password (8+ characters), then create it:
     nmcli con add type wifi ifname <dev> con-name <name> autoconnect no ssid <name>
       802-11-wireless.mode ap 802-11-wireless.band a ipv4.method shared
       wifi-sec.key-mgmt wpa-psk wifi-sec.proto rsn wifi-sec.pairwise ccmp
       wifi-sec.group ccmp wifi-sec.psk <password> 802-11-wireless.powersave 2
     (use band bg if 5 GHz isn't supported). Save the name in
     ~/.config/vr-control/settings.json as {"hotspot": "<name>"} (keep any other keys).
     If the card can't do AP mode, explain the router alternative: PC wired to the router,
     headset on its 5 GHz Wi-Fi, no hotspot needed.
   - Firewall: if ufw is enabled (/etc/ufw/ufw.conf has ENABLED=yes), offer:
     sudo ufw allow 9757/tcp && sudo ufw allow 9757/udp && sudo ufw allow 5353/udp && sudo ufw allow in on <dev>
     If firewalld is active, offer:
     sudo firewall-cmd --permanent --add-port=9757/tcp --add-port=9757/udp --add-service=mdns && sudo firewall-cmd --reload
   - Avahi not running: offer to install/enable avahi-daemon.
   Run --check again until nothing is marked ✘.
5. Steam: if Steam is installed natively, show me the "Steam launch options" line from
   --check and tell me to paste it into each VR game (right-click the game → Properties →
   Launch Options). If Steam is the Flatpak version, tell me that native Steam is simpler and
   point me to https://github.com/WiVRn/WiVRn for the extra Flatpak permissions.
6. Tell me how to install the WiVRn app on the headset: the Meta Horizon Store (search
   "WiVRn"), or sideload the APK from https://github.com/WiVRn/WiVRn/releases. Its version
   must match the PC's WiVRn version (shown by --check).
7. Finish with the steps for each play session: open VR Control → Start VR → on the headset
   join the hotspot Wi-Fi (name and password are on the Control tab) → open WiVRn → pick
   this PC → the first time, press "Start pairing" in VR Control and type the code into the
   headset → play → Stop VR.
````
