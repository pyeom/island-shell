"""Fake NetworkManager, BlueZ and UPower for testing without the hardware.

Run on a private bus that island-shell uses as its *system* bus (see
fake-system.sh). Objects, roughly as the real daemons expose them:

  NetworkManager: one Wi-Fi device, 4 access points ("CasaWiFi" is saved
    and active, "Cafe" is open, "Vecino" and "Oficina" are secured),
    ActivateConnection / AddAndActivateConnection / Disconnect / RequestScan
  BlueZ: adapter hci0 (powered), "WH-1000XM4" headphones (paired,
    connected, 10% battery); discovery adds a "Xbox Controller"
  UPower: laptop battery at 12% discharging, the headphones at 10% and a
    gamepad at 80%

Every call is printed, so tests can check what the shell asked for.
"""
import dbus
import dbus.service
from dbus.mainloop.glib import DBusGMainLoop
from gi.repository import GLib

DBusGMainLoop(set_as_default=True)
bus = dbus.SessionBus()  # the private "system" bus
PROPS = "org.freedesktop.DBus.Properties"


def log(*args):
    print(*args, flush=True)


class Obj(dbus.service.Object):
    """An object with properties on any number of interfaces."""

    def __init__(self, path, props):
        super().__init__(bus, path)
        self.path = path
        self.props = props  # {interface: {name: value}}

    @dbus.service.method(PROPS, in_signature="ss", out_signature="v")
    def Get(self, iface, name):
        return self.props[iface][name]

    @dbus.service.method(PROPS, in_signature="s", out_signature="a{sv}")
    def GetAll(self, iface):
        return dbus.Dictionary(self.props.get(iface, {}), signature="sv")

    @dbus.service.method(PROPS, in_signature="ssv")
    def Set(self, iface, name, value):
        log("Set", self.path, iface, name, value)
        self.update(iface, **{name: value})

    @dbus.service.signal(PROPS, signature="sa{sv}as")
    def PropertiesChanged(self, iface, changed, invalidated):
        pass

    def update(self, iface, **changes):
        self.props[iface].update(changes)
        self.PropertiesChanged(iface, dbus.Dictionary(changes, signature="sv"), [])


O = dbus.ObjectPath
NM = "org.freedesktop.NetworkManager"
NMP = "/org/freedesktop/NetworkManager"

aps = {}
for i, (ssid, strength, secured) in enumerate(
        [("CasaWiFi", 82, True), ("Cafe", 55, False), ("Vecino", 40, True), ("Oficina", 20, True)], 1):
    aps[ssid] = Obj(f"{NMP}/AccessPoint/{i}", {NM + ".AccessPoint": {
        "Ssid": dbus.ByteArray(ssid.encode()), "Strength": dbus.Byte(strength),
        "Flags": dbus.UInt32(1 if secured else 0), "WpaFlags": dbus.UInt32(0),
        "RsnFlags": dbus.UInt32(0x188 if secured else 0)}})

active = Obj(f"{NMP}/ActiveConnection/1", {NM + ".Connection.Active": {
    "Id": "CasaWiFi", "Type": "802-11-wireless", "Devices": dbus.Array([O(f"{NMP}/Devices/1")], signature="o")}})


class WifiDevice(Obj):
    @dbus.service.method(NM + ".Device.Wireless", out_signature="ao")
    def GetAllAccessPoints(self):
        return dbus.Array([O(ap.path) for ap in aps.values()], signature="o")

    @dbus.service.method(NM + ".Device.Wireless", in_signature="a{sv}")
    def RequestScan(self, options):
        log("RequestScan")

    @dbus.service.method(NM + ".Device")
    def Disconnect(self):
        log("Disconnect")
        self.update(NM + ".Device.Wireless", ActiveAccessPoint=O("/"))
        root.update(NM, PrimaryConnection=O("/"), PrimaryConnectionType="", State=dbus.UInt32(20))


device = WifiDevice(f"{NMP}/Devices/1", {
    NM + ".Device": {"DeviceType": dbus.UInt32(2), "Interface": "wlan0"},
    NM + ".Device.Wireless": {"ActiveAccessPoint": O(aps["CasaWiFi"].path)}})

saved = {}


class Connection(Obj):
    def __init__(self, path, ssid):
        super().__init__(path, {})
        self.ssid = ssid

    @dbus.service.method(NM + ".Settings.Connection", out_signature="a{sa{sv}}")
    def GetSettings(self):
        return {"connection": {"id": self.ssid, "type": "802-11-wireless"},
                "802-11-wireless": {"ssid": dbus.ByteArray(self.ssid.encode())}}


saved["CasaWiFi"] = Connection(f"{NMP}/Settings/1", "CasaWiFi")


class Settings(Obj):
    @dbus.service.method(NM + ".Settings", out_signature="ao")
    def ListConnections(self):
        return dbus.Array([O(c.path) for c in saved.values()], signature="o")


Settings(f"{NMP}/Settings", {})


def activate(ssid):
    active.update(NM + ".Connection.Active", Id=ssid)
    device.update(NM + ".Device.Wireless", ActiveAccessPoint=O(aps[ssid].path))
    root.update(NM, PrimaryConnection=O(active.path), PrimaryConnectionType="802-11-wireless",
                State=dbus.UInt32(70))


class Root(Obj):
    @dbus.service.method(NM, in_signature="ooo", out_signature="o")
    def ActivateConnection(self, connection, dev, specific):
        ssid = next(s for s, c in saved.items() if c.path == connection)
        log("ActivateConnection", ssid)
        activate(ssid)
        return O(active.path)

    @dbus.service.method(NM, in_signature="a{sa{sv}}oo", out_signature="oo")
    def AddAndActivateConnection(self, settings, dev, ap):
        ssid = bytes(settings["802-11-wireless"]["ssid"]).decode()
        psk = settings.get("802-11-wireless-security", {}).get("psk", "")
        log("AddAndActivateConnection", ssid, "psk=" + ("set" if psk else "none"))
        if ssid != "Cafe" and psk != "secreto":
            raise dbus.DBusException("Secrets were required, but not provided",
                                     name="org.freedesktop.NetworkManager.AgentManager.NoSecrets")
        saved[ssid] = Connection(f"{NMP}/Settings/{len(saved) + 1}", ssid)
        activate(ssid)
        return O(saved[ssid].path), O(active.path)


root = Root(NMP, {NM: {
    "WirelessEnabled": True, "WirelessHardwareEnabled": True, "State": dbus.UInt32(70),
    "PrimaryConnection": O(active.path), "PrimaryConnectionType": "802-11-wireless",
    "AllDevices": dbus.Array([O(device.path)], signature="o")}})

# ---- BlueZ -----------------------------------------------------------------

BZ = "org.bluez"
OM = "org.freedesktop.DBus.ObjectManager"
bt_objects = {}


class BtDevice(Obj):
    @dbus.service.method(BZ + ".Device1")
    def Connect(self):
        log("Bluetooth Connect", self.props[BZ + ".Device1"]["Alias"])
        self.update(BZ + ".Device1", Connected=True)

    @dbus.service.method(BZ + ".Device1")
    def Disconnect(self):
        log("Bluetooth Disconnect", self.props[BZ + ".Device1"]["Alias"])
        self.update(BZ + ".Device1", Connected=False)

    @dbus.service.method(BZ + ".Device1")
    def Pair(self):
        log("Bluetooth Pair", self.props[BZ + ".Device1"]["Alias"])
        self.update(BZ + ".Device1", Paired=True)


class Adapter(Obj):
    @dbus.service.method(BZ + ".Adapter1")
    def StartDiscovery(self):
        log("StartDiscovery")
        self.update(BZ + ".Adapter1", Discovering=True)
        GLib.timeout_add(800, add_controller)

    @dbus.service.method(BZ + ".Adapter1")
    def StopDiscovery(self):
        log("StopDiscovery")
        self.update(BZ + ".Adapter1", Discovering=False)


class Manager(dbus.service.Object):
    def __init__(self):
        super().__init__(bus, "/")

    @dbus.service.method(OM, out_signature="a{oa{sa{sv}}}")
    def GetManagedObjects(self):
        return {O(p): o.props for p, o in bt_objects.items()}

    @dbus.service.signal(OM, signature="oa{sa{sv}}")
    def InterfacesAdded(self, path, interfaces):
        pass


manager = Manager()
bt_objects["/org/bluez/hci0"] = Adapter("/org/bluez/hci0", {BZ + ".Adapter1": {
    "Powered": True, "Discovering": False, "Alias": "PC"}})
bt_objects["/org/bluez/hci0/dev_AA"] = BtDevice("/org/bluez/hci0/dev_AA", {
    BZ + ".Device1": {"Alias": "WH-1000XM4", "Icon": "audio-headphones", "Paired": True, "Connected": True},
    BZ + ".Battery1": {"Percentage": dbus.Byte(10)}})


def add_controller():
    path = "/org/bluez/hci0/dev_BB"
    if path not in bt_objects:
        bt_objects[path] = BtDevice(path, {BZ + ".Device1": {
            "Alias": "Xbox Controller", "Icon": "input-gaming", "Paired": False, "Connected": False}})
        manager.InterfacesAdded(O(path), bt_objects[path].props)
        log("InterfacesAdded Xbox Controller")
    return False

# ---- UPower ----------------------------------------------------------------

UP = "org.freedesktop.UPower"
UPD = UP + ".Device"


def up_device(path, **props):
    return Obj(path, {UPD: props})


display = up_device(f"/org/freedesktop/UPower/devices/DisplayDevice", Type=dbus.UInt32(2), IsPresent=True,
                    Percentage=12.0, State=dbus.UInt32(2), PowerSupply=True, Model="")
devices = [
    up_device("/org/freedesktop/UPower/devices/battery_BAT0", Type=dbus.UInt32(2), IsPresent=True,
              Percentage=12.0, State=dbus.UInt32(2), PowerSupply=True, Model="BAT0"),
    up_device("/org/freedesktop/UPower/devices/headset_dev_AA", Type=dbus.UInt32(19), IsPresent=True,
              Percentage=10.0, State=dbus.UInt32(2), PowerSupply=False, Model="WH-1000XM4"),
    up_device("/org/freedesktop/UPower/devices/gaming_input_1", Type=dbus.UInt32(12), IsPresent=True,
              Percentage=80.0, State=dbus.UInt32(2), PowerSupply=False, Model="DualSense"),
]


class UPowerRoot(Obj):
    @dbus.service.method(UP, out_signature="ao")
    def EnumerateDevices(self):
        return dbus.Array([O(d.path) for d in devices], signature="o")


UPowerRoot("/org/freedesktop/UPower", {UP: {"OnBattery": True}})

names = [dbus.service.BusName(n, bus) for n in (NM, BZ, UP)]
log("fake system services ready")
GLib.MainLoop().run()
