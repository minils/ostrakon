# Icons

The icons of the status screens are the ones PebbleOS shows in its own
dialogs, from [PebbleOS](https://github.com/coredevices/PebbleOS)
(`resources/*/base/images`), licensed under the Apache License 2.0 (see
`LICENSE` in this folder):

* `Pebble_*_Watch_disconnected`: the phone is not connected
* `Pebble_*_Check_internet_connection`: the phone cannot reach the server
* `Pebble_*_Generic_warning`: other errors

They were converted from SVG to Pebble draw commands (`.pdc`) unchanged,
with `tools/generate_pdcs/pdc_gen.py` of PebbleOS. The 50x50 icons are
for the smaller watches, the 80x80 ones for emery and gabbro.
