"""Fake tray items for tests: faketray.py [SECONDS] (default 60).

Registers two StatusNotifierItems with the watcher, the way apps do:
  - "Fake Chat": icon from pixels (IconPixmap) registered by object path
    (like Electron), a dbusmenu with a checkbox, radio items, a separator, a
    submenu and a disabled entry. Goes NeedsAttention after 5 s.
  - "Fake Sync": a themed icon, registered by bus name, no menu; Activate
    fails, so a left click falls back to ContextMenu.
Prints every Activate / ContextMenu / Scroll / menu Event.
"""
import os
import sys

import dbus
import dbus.service
from dbus.mainloop.glib import DBusGMainLoop
from gi.repository import GLib

ITEM = "org.kde.StatusNotifierItem"
MENU = "com.canonical.dbusmenu"
PROPS = "org.freedesktop.DBus.Properties"
DBusGMainLoop(set_as_default=True)


def pixmap(size, argb):
    data = bytes([(argb >> 24) & 0xff, (argb >> 16) & 0xff, (argb >> 8) & 0xff, argb & 0xff]) * (size * size)
    return dbus.Struct((dbus.Int32(size), dbus.Int32(size), dbus.ByteArray(data)), signature="iiay")


class Item(dbus.service.Object):
    def __init__(self, bus, path, props):
        super().__init__(bus, path)
        self.props = props

    @dbus.service.method(PROPS, in_signature="ss", out_signature="v")
    def Get(self, interface, prop):
        return self.props[prop]

    @dbus.service.method(PROPS, in_signature="s", out_signature="a{sv}")
    def GetAll(self, interface):
        return dbus.Dictionary(self.props, signature="sv")

    @dbus.service.method(ITEM, in_signature="ii")
    def Activate(self, x, y):
        if self.props["Id"] == "fake-sync":
            raise dbus.exceptions.DBusException("not supported",
                                                name="org.freedesktop.DBus.Error.UnknownMethod")
        print(self.props["Id"], "Activate", flush=True)

    @dbus.service.method(ITEM, in_signature="ii")
    def SecondaryActivate(self, x, y):
        print(self.props["Id"], "SecondaryActivate", flush=True)

    @dbus.service.method(ITEM, in_signature="ii")
    def ContextMenu(self, x, y):
        print(self.props["Id"], "ContextMenu", flush=True)

    @dbus.service.method(ITEM, in_signature="is")
    def Scroll(self, delta, orientation):
        print(self.props["Id"], "Scroll", delta, orientation, flush=True)

    @dbus.service.signal(ITEM, signature="s")
    def NewStatus(self, status):
        pass

    @dbus.service.signal(ITEM)
    def NewIcon(self):
        pass


class Menu(dbus.service.Object):
    def __init__(self, bus, path):
        super().__init__(bus, path)
        self.revision = 1
        self.muted = False
        self.status = 1  # radio: 1 online, 2 away

    def node(self, id, props, children=()):
        return dbus.Struct((dbus.Int32(id), dbus.Dictionary(props, signature="sv"),
                            dbus.Array([dbus.Struct(c, variant_level=1) for c in children], signature="v")),
                           signature="ia{sv}av")

    def layout(self):
        status = [
            self.node(21, {"label": "_Online", "toggle-type": "radio", "toggle-state": int(self.status == 1)}),
            self.node(22, {"label": "_Away", "toggle-type": "radio", "toggle-state": int(self.status == 2)}),
        ]
        return self.node(0, {"children-display": "submenu"}, [
            self.node(1, {"label": "_Open Fake Chat", "icon-name": "mail-message-new"}),
            self.node(2, {"label": "Mute notifications", "toggle-type": "checkmark",
                          "toggle-state": int(self.muted)}),
            self.node(3, {"type": "separator"}),
            self.node(20, {"label": "Status", "children-display": "submenu"}, status),
            self.node(4, {"label": "Can't touch this", "enabled": False}),
            self.node(5, {"label": "Hidden", "visible": False}),
            self.node(6, {"label": "_Quit"}),
        ])

    @dbus.service.method(MENU, in_signature="iias", out_signature="u(ia{sv}av)")
    def GetLayout(self, parent, depth, names):
        return dbus.UInt32(self.revision), self.layout()

    @dbus.service.method(MENU, in_signature="i", out_signature="b")
    def AboutToShow(self, id):
        print("menu AboutToShow", id, flush=True)
        return False

    @dbus.service.method(MENU, in_signature="isvu")
    def Event(self, id, event, data, timestamp):
        print("menu Event", id, event, flush=True)
        if event != "clicked":
            return
        if id == 2:
            self.muted = not self.muted
        elif id in (21, 22):
            self.status = id - 20
        else:
            return
        self.revision += 1
        self.LayoutUpdated(dbus.UInt32(self.revision), 0)

    @dbus.service.signal(MENU, signature="ui")
    def LayoutUpdated(self, revision, parent):
        pass


def main():
    seconds = int(sys.argv[1]) if len(sys.argv) > 1 else 60
    bus = dbus.SessionBus()
    chat_path = "/org/ayatana/NotificationItem/fake_chat"
    chat = Item(bus, chat_path, {
        "Category": "Communications", "Id": "fake-chat", "Title": "Fake Chat", "Status": "Active",
        "IconName": "", "IconThemePath": "",
        "IconPixmap": dbus.Array([pixmap(22, 0xff2fb5ff), pixmap(64, 0xff0a84ff)], signature="(iiay)"),
        "AttentionIconName": "", "AttentionIconPixmap": dbus.Array([pixmap(64, 0xffff453a)], signature="(iiay)"),
        "ToolTip": dbus.Struct(("", dbus.Array([], signature="(iiay)"), "Fake Chat", "<b>3</b> unread"),
                               signature="sa(iiay)ss"),
        "ItemIsMenu": False, "Menu": dbus.ObjectPath("/MenuBar"),
    })
    Menu(bus, "/MenuBar")
    name = dbus.service.BusName(f"org.kde.StatusNotifierItem-{os.getpid()}-2", bus)
    sync = Item(bus, "/StatusNotifierItem", {
        "Category": "ApplicationStatus", "Id": "fake-sync", "Title": "Fake Sync", "Status": "Active",
        "IconName": "folder-sync", "IconThemePath": "", "IconPixmap": dbus.Array([], signature="(iiay)"),
        "ToolTip": dbus.Struct(("", dbus.Array([], signature="(iiay)"), "", ""), signature="sa(iiay)ss"),
        "ItemIsMenu": False, "Menu": dbus.ObjectPath("/NO_DBUSMENU"),
    })
    watcher = bus.get_object("org.kde.StatusNotifierWatcher", "/StatusNotifierWatcher")
    register = watcher.get_dbus_method("RegisterStatusNotifierItem", "org.kde.StatusNotifierWatcher")
    register(chat_path)
    register(name.get_name())
    print("registered", flush=True)

    def attention():
        chat.props["Status"] = "NeedsAttention"
        chat.NewStatus("NeedsAttention")
        print("fake-chat NeedsAttention", flush=True)
        return False

    GLib.timeout_add_seconds(5, attention)
    GLib.timeout_add_seconds(seconds, loop.quit)
    loop.run()
    del sync


loop = GLib.MainLoop()
if __name__ == "__main__":
    main()
