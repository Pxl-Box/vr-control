#!/usr/bin/env bash
# VR Control installer: installs the app for the current user, and optionally WiVRn.
# Usage: ./install.sh            (interactive)
#        ./install.sh --yes      (answer yes to everything that doesn't need sudo)
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN="$HOME/.local/bin"
APPS="${XDG_DATA_HOME:-$HOME/.local/share}/applications"
ICONS="${XDG_DATA_HOME:-$HOME/.local/share}/icons/hicolor/scalable/apps"
WIVRN=io.github.wivrn.wivrn
YES=0; [[ "${1:-}" == "--yes" ]] && YES=1

g=$'\e[32m'; r=$'\e[31m'; y=$'\e[33m'; b=$'\e[1m'; n=$'\e[0m'
ok()   { echo "  ${g}✔${n} $*"; }
bad()  { echo "  ${r}✘${n} $*"; }
warn() { echo "  ${y}!${n} $*"; }
ask()  { [[ $YES == 1 ]] && return 0; read -r -p "  $* [Y/n] " a; [[ -z "$a" || "$a" =~ ^[Yy] ]]; }

family() {
  local ids; ids="$( . /etc/os-release 2>/dev/null; echo "${ID:-} ${ID_LIKE:-}")"
  case "$ids" in
    *arch*) echo arch ;; *fedora*|*rhel*) echo fedora ;; *suse*) echo suse ;;
    *debian*|*ubuntu*) echo debian ;; *) echo unknown ;;
  esac
}
FAM="$(family)"

pkg_install() {  # pkg_install <debian pkgs> <fedora pkgs> <arch pkgs> <suse pkgs>
  local cmd
  case "$FAM" in
    debian) cmd="sudo apt install -y $1" ;; fedora) cmd="sudo dnf install -y $2" ;;
    arch) cmd="sudo pacman -S --needed $3" ;; suse) cmd="sudo zypper install -y $4" ;;
    *) bad "Unknown distro – install these yourself: $1"; return 1 ;;
  esac
  echo "    needs admin rights: ${b}$cmd${n}"
  if [[ $YES == 0 ]] && ask "Run it now?"; then $cmd; else return 1; fi
}

echo "${b}VR Control installer${n}"
echo
echo "${b}1. Requirements${n}"
if python3 -c 'import gi; gi.require_version("Gtk","3.0"); from gi.repository import Gtk' 2>/dev/null; then
  ok "Python 3 + GTK 3"
else
  bad "Python 3 GTK bindings missing"
  pkg_install "python3-gi gir1.2-gtk-3.0" "python3-gobject gtk3" "python-gobject gtk3" \
              "python3-gobject typelib-1_0-Gtk-3_0" || { bad "VR Control can't run without them."; exit 1; }
fi
command -v nmcli >/dev/null && ok "NetworkManager (nmcli)" || {
  bad "NetworkManager missing (needed for the hotspot)"
  pkg_install network-manager NetworkManager networkmanager NetworkManager || true; }
command -v pactl >/dev/null && ok "pactl (audio switching)" || {
  warn "pactl missing (only needed for automatic headset audio)"
  pkg_install "pipewire-pulse pulseaudio-utils" "pipewire-pulseaudio pulseaudio-utils" \
              "pipewire-pulse libpulse" "pipewire-pulseaudio pulseaudio-utils" || true; }
if command -v flatpak >/dev/null; then ok "Flatpak"; else
  bad "Flatpak missing (needed for WiVRn)"
  pkg_install flatpak flatpak flatpak flatpak || true
fi

echo
echo "${b}2. WiVRn streaming server${n}"
if command -v flatpak >/dev/null; then
  if flatpak info "$WIVRN" >/dev/null 2>&1; then
    ok "WiVRn $(flatpak info "$WIVRN" | awk -F': *' '/Version/{print $2; exit}') already installed"
  elif ask "Install WiVRn from Flathub (for this user, ~300 MB)?"; then
    flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
    flatpak install -y --user flathub "$WIVRN"
    ok "WiVRn installed"
  else
    warn "Skipped – you can install it later from the app's Setup tab"
  fi
else
  warn "Skipped (no Flatpak)"
fi

echo
echo "${b}3. Firewall${n}"
if grep -qs '^ENABLED=yes' /etc/ufw/ufw.conf; then
  WIFI="$(nmcli -t -f DEVICE,TYPE dev 2>/dev/null | awk -F: '$2=="wifi" && $1 !~ /^p2p/ {print $1; exit}')"
  cmd="sudo ufw allow 9757/tcp && sudo ufw allow 9757/udp && sudo ufw allow 5353/udp${WIFI:+ && sudo ufw allow in on $WIFI}"
  warn "ufw is on – WiVRn needs ports 9757 tcp/udp and 5353 udp: ${b}$cmd${n}"
  if [[ $YES == 0 ]] && ask "Run it now?"; then bash -c "$cmd"; fi
elif systemctl is-active --quiet firewalld 2>/dev/null; then
  cmd="sudo firewall-cmd --permanent --add-port=9757/tcp --add-port=9757/udp --add-service=mdns && sudo firewall-cmd --reload"
  warn "firewalld is on – WiVRn needs ports opened: ${b}$cmd${n}"
  if [[ $YES == 0 ]] && ask "Run it now?"; then bash -c "$cmd"; fi
else
  ok "No active firewall"
fi

echo
echo "${b}4. Installing VR Control${n}"
mkdir -p "$BIN" "$APPS" "$ICONS"
install -m 755 "$HERE/vr-control" "$BIN/vr-control"
install -m 644 "$HERE/vr-control.svg" "$ICONS/vr-control.svg"
cat > "$APPS/vr-control.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=VR Control
Comment=Stream PC VR to a Meta Quest: hotspot, WiVRn server, pairing code and live stats
Exec=$BIN/vr-control
Icon=vr-control
Terminal=false
Categories=Game;Network;
Keywords=vr;quest;oculus;wivrn;hotspot;
StartupWMClass=io.github.vr_control.VRControl
EOF
command -v update-desktop-database >/dev/null && update-desktop-database "$APPS" 2>/dev/null || true
command -v gtk-update-icon-cache >/dev/null && gtk-update-icon-cache -f -t "${ICONS%/scalable/apps}" 2>/dev/null || true
ok "Installed to $BIN/vr-control (menu entry: VR Control)"

echo
echo "${b}Setup check${n}"
"$BIN/vr-control" --check || true
echo
echo "Done. Open ${b}VR Control${n} from your app menu – its Setup tab fixes anything still marked ✘."
if [[ $YES == 0 ]] && ask "Open it now?"; then (setsid "$BIN/vr-control" >/dev/null 2>&1 &); fi
