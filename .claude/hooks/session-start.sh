#!/bin/bash
# Sets up a Claude Code cloud session for Ostrakon: the Pebble tool, the
# Pebble SDK with its emulator, and the PebbleOS sources to look things up
set -euo pipefail

if [ "${CLAUDE_CODE_REMOTE:-}" != "true" ]; then
  exit 0
fi

export PATH="$HOME/.local/bin:$PATH"
echo "export PATH=\"\$HOME/.local/bin:\$PATH\"" >> "${CLAUDE_ENV_FILE:-/dev/null}"

# Pebble tool and SDK, as in .github/workflows/build.yml
if ! command -v pebble >/dev/null; then
  uv tool install pebble-tool --python 3.13
fi
if ! pebble sdk list 2>/dev/null | grep -q '(active)'; then
  # answers the license and analytics prompts; yes ends with SIGPIPE
  { yes || true; } | pebble sdk install latest
fi

# pypkjs, the phone side of the emulator, listens on IPv6 by default,
# which the container does not have
for file in "$HOME"/.local/share/uv/tools/pebble-tool/lib/python3.*/site-packages/pypkjs/runner/websocket.py; do
  [ -f "$file" ] && sed -i 's/pywsgi.WSGIServer(("", self.port)/pywsgi.WSGIServer(("127.0.0.1", self.port)/' "$file"
done

# PebbleOS sources, only to read; the SDK has everything to build
if [ ! -d "$HOME/pebbleos/.git" ]; then
  git clone --depth 1 --quiet https://github.com/coredevices/PebbleOS.git "$HOME/pebbleos" ||
    echo "Could not clone PebbleOS, continuing without it" >&2
fi
