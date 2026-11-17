#!/usr/bin/env bash
# Removes VR Control (not WiVRn, not your hotspot connection).
set -eu
D="${XDG_DATA_HOME:-$HOME/.local/share}"
rm -f "$HOME/.local/bin/vr-control" "$D/applications/vr-control.desktop" \
      "$D/icons/hicolor/scalable/apps/vr-control.svg"
rm -rf "${XDG_CONFIG_HOME:-$HOME/.config}/vr-control"
rm -f "$HOME/.local/lib/vr-control/libnopause_layer.so"
rm -f "$D/openxr/1/api_layers/implicit.d/vr-control-nopause.json"
echo "VR Control removed."
echo "To also remove WiVRn:            flatpak uninstall $([ -n "$(flatpak info --user io.github.wivrn.wivrn 2>/dev/null)" ] && echo --user) io.github.wivrn.wivrn"
echo "To remove a hotspot connection:  nmcli con delete <name>   (list them: nmcli con show)"
