"""Fake MPRIS player for tests: fakeplayer.py [SECONDS] (default 20).

Owns org.mpris.MediaPlayer2.fake, plays "Clair de Lune" at 1:35, and prints
PlayPause / Next / Previous calls.
"""
import sys

import dbus
import dbus.service
from dbus.mainloop.glib import DBusGMainLoop
from gi.repository import GLib

PLAYER = "org.mpris.MediaPlayer2.Player"
DBusGMainLoop(set_as_default=True)


class Player(dbus.service.Object):
    def __init__(self, bus):
        super().__init__(bus, "/org/mpris/MediaPlayer2")
        self.status = "Playing"

    def metadata(self):
        return dbus.Dictionary({
            "mpris:trackid": dbus.ObjectPath("/t/1"),
            "xesam:title": "Clair de Lune",
            "xesam:artist": dbus.Array(["Claude Debussy"], signature="s"),
            "mpris:length": dbus.Int64(312_000_000),
        }, signature="sv")

    def player(self):
        return {"PlaybackStatus": self.status, "Metadata": self.metadata(),
                "Position": dbus.Int64(95_000_000), "Rate": 1.0, "CanGoNext": True,
                "CanGoPrevious": True, "CanSeek": True, "CanControl": True,
                "CanPlay": True, "CanPause": True}

    def root(self):
        return {"Identity": "FakePlayer", "DesktopEntry": "fake"}

    @dbus.service.method("org.freedesktop.DBus.Properties", in_signature="ss", out_signature="v")
    def Get(self, interface, prop):
        return (self.player() if interface == PLAYER else self.root())[prop]

    @dbus.service.method("org.freedesktop.DBus.Properties", in_signature="s", out_signature="a{sv}")
    def GetAll(self, interface):
        return self.player() if interface == PLAYER else self.root()

    @dbus.service.signal("org.freedesktop.DBus.Properties", signature="sa{sv}as")
    def PropertiesChanged(self, interface, changed, invalidated):
        pass

    @dbus.service.method(PLAYER)
    def PlayPause(self):
        self.status = "Paused" if self.status == "Playing" else "Playing"
        print("PlayPause ->", self.status, flush=True)
        self.PropertiesChanged(PLAYER, {"PlaybackStatus": self.status}, [])

    @dbus.service.method(PLAYER)
    def Next(self):
        print("Next", flush=True)

    @dbus.service.method(PLAYER)
    def Previous(self):
        print("Previous", flush=True)


bus = dbus.SessionBus()
name = dbus.service.BusName("org.mpris.MediaPlayer2.fake", bus)
player = Player(bus)
loop = GLib.MainLoop()
GLib.timeout_add(int(sys.argv[1] if len(sys.argv) > 1 else 20) * 1000, loop.quit)
loop.run()
