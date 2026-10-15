"""notify-send replacement for tests.

notify.py [--app A] [--icon I] [--urgency 0|1|2] [--replace ID]
          [--action key=Label]... [--reply] [--image-rgb] [--timeout MS]
          [--wait] SUMMARY [BODY]

Prints the notification id. With --wait, prints ActionInvoked /
NotificationClosed / NotificationReplied signals for 8 seconds.
"""
import argparse

import dbus
from dbus.mainloop.glib import DBusGMainLoop

p = argparse.ArgumentParser()
p.add_argument("--app", default="test")
p.add_argument("--icon", default="")
p.add_argument("--urgency", type=int, default=1)
p.add_argument("--replace", type=int, default=0)
p.add_argument("--action", action="append", default=[])
p.add_argument("--reply", action="store_true")
p.add_argument("--image-rgb", action="store_true")
p.add_argument("--timeout", type=int, default=-1)
p.add_argument("--wait", action="store_true")
p.add_argument("summary")
p.add_argument("body", nargs="?", default="")
a = p.parse_args()

DBusGMainLoop(set_as_default=True)
bus = dbus.SessionBus()
n = dbus.Interface(bus.get_object("org.freedesktop.Notifications", "/org/freedesktop/Notifications"),
                   "org.freedesktop.Notifications")
actions = []
for x in a.action:
    key, label = x.split("=", 1)
    actions += [key, label]
if a.reply:
    actions += ["inline-reply", "Reply"]
hints = {"urgency": dbus.Byte(a.urgency)}
if a.image_rgb:
    w = h = 32
    data = bytes((x * 8) % 256 if (x // 4 + y // 4) % 2 else 40
                 for y in range(h) for x in range(w) for _ in range(3))
    hints["image-data"] = dbus.Struct((w, h, w * 3, False, 8, 3, dbus.ByteArray(data)),
                                      signature="iiibiiay")
nid = n.Notify(a.app, a.replace, a.icon, a.summary, a.body, actions, hints, a.timeout)
print(int(nid), flush=True)

if a.wait:
    from gi.repository import GLib
    loop = GLib.MainLoop()
    for sig in ("ActionInvoked", "NotificationClosed", "NotificationReplied"):
        bus.add_signal_receiver(lambda *args, s=sig: print(s, *map(str, args), flush=True),
                                signal_name=sig, dbus_interface="org.freedesktop.Notifications")
    GLib.timeout_add(8000, loop.quit)
    loop.run()
