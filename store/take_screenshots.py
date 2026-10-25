# Takes the screenshots in the emulator, with the demo data of
# store/demo_server.js:
#   store/screenshots/<platform>-<n>-<name>.png   the store screenshots
#   site/img/watch/                               the homepage's watch: every
#       screen of the app it can reach, and watch.js with what each button
#       leads to
# Run it with the Python of the pebble tool, which has libpebble2, after
# `pebble build`:
#   ~/.local/share/uv/tools/pebble-tool/bin/python store/take_screenshots.py
#       [--store basalt chalk emery] [--site]
# Without options it takes everything. It wipes the emulators it uses.
import argparse
import dbm.dumb
import hashlib
import io
import json
import os
import shutil
import subprocess
import sys
import tempfile
import threading
import time

from libpebble2.communication import PebbleConnection
from libpebble2.communication.transports.qemu.protocol import QemuButton, QemuPacket
from libpebble2.communication.transports.websocket import MessageTargetPhone, WebsocketTransport
from libpebble2.communication.transports.websocket.protocol import WebSocketRelayQemu
from libpebble2.services.screenshot import Screenshot
from PIL import Image
from pebble_tool.commands.screenshot import ScreenshotCommand
from pebble_tool.sdk import get_sdk_persist_dir

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
UUID = json.load(open(os.path.join(ROOT, 'package.json')))['pebble']['uuid']
PORT = 5230
SETTINGS = {'url': 'http://localhost:%d' % PORT, 'token': 'demo', 'tags': '#pebble',
            'todoTag': '#todo', 'keywords': None, 'vibrate': False, 'language': 'en'}

# the rows of the main menu that open something without dictation
MAIN_ROWS = {1: 'list', 2: 'list', 3: 'tags', 5: 'drafts'}


def pebble(*args):
    subprocess.run(['pebble'] + list(args), cwd=ROOT, check=True,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def kill_emulators():
    subprocess.run(['pebble', 'kill'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    # it keeps their process ids, which may belong to other processes later
    try:
        os.remove(os.path.join(tempfile.gettempdir(), 'pb-emulator.json'))
    except OSError:
        pass


class Watch(object):
    """An emulator with the app installed, connected for screenshots and
    button presses."""

    def __init__(self, platform):
        self.platform = platform
        kill_emulators()
        # a fresh emulator, so the launcher shows only this app
        persist = get_sdk_persist_dir(platform)
        shutil.rmtree(persist)
        os.makedirs(os.path.join(persist, 'localstorage'))
        store = dbm.dumb.open(os.path.join(persist, 'localstorage', UUID), 'c')
        store['ostrakon_settings'] = json.dumps(SETTINGS)
        store.close()
        # the demo data starts over for every watch
        self.server = subprocess.Popen(['node', os.path.join(ROOT, 'store', 'demo_server.js'), str(PORT)],
                                       stdout=subprocess.DEVNULL)
        pebble('install', '--emulator', platform)
        info = json.load(open(os.path.join(tempfile.gettempdir(), 'pb-emulator.json')))
        port = list(info[platform].values())[0]['pypkjs']['port']
        self.pebble = PebbleConnection(WebsocketTransport('ws://localhost:%d/' % port))
        self.pebble.connect()
        self.pebble.run_async()
        # ScreenshotCommand corrects the colors like `pebble screenshot`
        self.correction = ScreenshotCommand.__new__(ScreenshotCommand)
        self.correction.pebble = self.pebble
        self.last = None

    def close(self):
        # the connection's reader thread fails once the emulator is gone
        threading.excepthook = lambda args: None
        self.server.terminate()
        kill_emulators()
        threading.excepthook = threading.__excepthook__

    def grab(self):
        rows = Screenshot(self.pebble).grab_image()
        rows = self.correction._correct_colours(rows)
        rows = self.correction._roundify(rows)
        height, width = len(rows), len(rows[0]) // 4
        data = b''.join(bytes(row) for row in rows)
        return Image.frombytes('RGBA', (width, height), data)

    def screen(self, settle=0.4):
        """The screen once it stops changing, e.g. after an animation or
        while loading."""
        time.sleep(settle)
        image = self.grab()
        for _ in range(40):
            time.sleep(0.25)
            again = self.grab()
            if again.tobytes() == image.tobytes():
                break
            image = again
        self.last = image
        return image

    def press(self, button, settle=0.4):
        state = {'back': QemuButton.Button.Back, 'up': QemuButton.Button.Up,
                 'select': QemuButton.Button.Select, 'down': QemuButton.Button.Down}[button]
        for value in (state, 0):
            data = QemuButton(state=value)
            packet = QemuPacket(data=data)
            packet.serialise()
            self.pebble.transport.send_packet(
                WebSocketRelayQemu(protocol=packet.protocol, data=data.serialise()),
                target=MessageTargetPhone())
            time.sleep(0.08)
        return self.screen(settle)


def save(image, path):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    buffer = io.BytesIO()
    image.convert('RGB').save(buffer, 'PNG', optimize=True)
    with open(path, 'wb') as f:
        f.write(buffer.getvalue())


def take_store(platform):
    """The seven store screenshots, in the order of store/description.md."""
    out = os.path.join(ROOT, 'store', 'screenshots')
    watch = Watch(platform)
    try:
        watch.screen(1)
        shots = []
        watch.press('down')
        shots.append(('recent', watch.press('select', 1.5)))
        shots.append(('memo', watch.press('select', 1.5)))
        watch.press('select', 1)                     # actions, at Tasks
        shots.append(('tasks', watch.press('select', 1.5)))
        watch.press('back', 1)                       # the memo
        watch.press('select', 1)                     # actions
        watch.press('down')                          # Comments
        shots.append(('comments', watch.press('select', 1.5)))
        watch.press('down')
        shots.append(('comment', watch.press('select', 1.5)))
        for _ in range(4):                           # back to the main menu
            watch.press('back', 1)
        watch.press('down')
        watch.press('down')
        shots.append(('tags', watch.press('select', 1.5)))
        # leaving the app updates its glance; the launcher shows it
        watch.press('back', 1)
        watch.press('back', 1.5)
        shots.append(('glance', watch.press('select', 1.5)))
        for n, (name, image) in enumerate(shots, 1):
            save(image, os.path.join(out, '%s-%d-%s.png' % (platform, n, name)))
    finally:
        watch.close()


class Crawler(object):
    """Walks through the app with the buttons and notes every screen and
    where up, down and select lead from it. Back is left to the homepage,
    which keeps the screens it came from."""

    def __init__(self, watch):
        self.watch = watch
        self.nodes = {}     # id -> {'i': image id, 'u': id, 'd': id, 's': id}
        self.images = {}    # image id -> image
        self.windows = {}   # id of a window's first screen -> its kind

    def node(self, image, context=''):
        """The id of a screen. Screens that look the same are one, except
        where they belong to different memos: the actions and tasks of a
        memo, whose images do not show which memo they are of."""
        image_id = hashlib.sha1(image.tobytes()).hexdigest()[:10]
        key = hashlib.sha1((image_id + context).encode()).hexdigest()[:10]
        if key not in self.nodes:
            self.nodes[key] = {'i': image_id}
            self.images[image_id] = image
        return key

    def selectable(self, kind, count):
        """The rows whose select opens something to look at, and the kind of
        window it opens. Nothing that dictates, changes or deletes."""
        if kind == 'main':
            return dict(MAIN_ROWS)
        if kind == 'list':
            return {row: 'memo' for row in range(count)}
        if kind == 'tags':
            return {row: 'list' for row in range(count)}
        if kind == 'comments':
            return {row: 'memo' for row in range(count)}
        if kind == 'actions':
            # Tasks comes first; a memo with comments has 7 actions, the
            # second being Comments
            rows = {0: 'tasks'}
            if count == 7:
                rows[1] = 'comments'
            return rows
        return {}

    def window(self, kind, context=''):
        """Explores the window on screen, of the given kind, and comes back
        to its first screen. Returns the id of that screen."""
        first = self.node(self.watch.last, context)
        if first in self.windows:
            return first  # the same window was seen elsewhere
        self.windows[first] = kind
        print('  %s %s' % (kind, first), flush=True)

        # every position up and down, until down changes nothing
        positions = [first]
        while True:
            key = self.node(self.watch.press('down'), context)
            self.nodes[positions[-1]]['d'] = key
            if key in positions:
                break
            self.nodes[key]['u'] = positions[-1]
            positions.append(key)
        self.nodes[positions[0]]['u'] = positions[0]
        for _ in range(len(positions) - 1):
            self.watch.press('up')

        rows = self.selectable(kind, len(positions))
        if kind == 'memo':
            # select opens the actions wherever the memo is scrolled to
            self.watch.press('select', 1)
            actions = self.window('actions', first)
            self.watch.press('back', 1)
            for key in positions:
                self.nodes[key]['s'] = actions
            return first
        reopen = kind == 'actions'  # an action menu closes when chosen
        for row, child_kind in sorted(rows.items()):
            for _ in range(row):
                self.watch.press('down')
            self.watch.press('select', 1.5)
            # what a memo's actions lead to belongs to that memo, too
            child_context = context if kind in ('actions', 'tasks') else ''
            self.nodes[positions[row]]['s'] = self.window(child_kind, child_context)
            self.watch.press('back', 1)
            if reopen:
                self.watch.press('select', 1)
            else:
                for _ in range(row):
                    self.watch.press('up')
        return first


def take_site():
    out = os.path.join(ROOT, 'site', 'img', 'watch')
    watch = Watch('emery')
    try:
        watch.screen(1)
        crawler = Crawler(watch)
        start = crawler.window('main')
        # leaving the app shows its glance in the launcher
        watch.press('back', 1.5)
        launcher = crawler.node(watch.press('select', 1.5))
        crawler.nodes[launcher]['s'] = start
    finally:
        watch.close()

    if os.path.isdir(out):
        shutil.rmtree(out)
    for key, image in crawler.images.items():
        save(image, os.path.join(out, key + '.png'))
    # a script rather than JSON, so the page also works opened as a file
    with open(os.path.join(out, 'watch.js'), 'w') as f:
        f.write('// Made by store/take_screenshots.py\nwindow.OSTRAKON_WATCH = ')
        json.dump({'start': start, 'launcher': launcher, 'screens': crawler.nodes}, f,
                  separators=(',', ':'), sort_keys=True)
        f.write(';\n')
    print('%d screens' % len(crawler.nodes))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--store', nargs='*', metavar='PLATFORM')
    parser.add_argument('--site', action='store_true')
    args = parser.parse_args()
    everything = args.store is None and not args.site
    platforms = args.store or (['basalt', 'chalk', 'emery'] if everything else [])
    for platform in platforms:
        print('store screenshots on %s' % platform, flush=True)
        take_store(platform)
    if args.site or everything:
        print('screens of the homepage watch', flush=True)
        take_site()


if __name__ == '__main__':
    main()
