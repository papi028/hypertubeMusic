import QtQuick
import HtMusic
import HtMusic.App

Column {
    id: root

    spacing: Theme.spaced(8)

    SettingRow {
        width: parent.width
        title: qsTr("Proxy mode")
        caption: ProxySettings.mode === ProxySettings.System
            ? qsTr("Use system proxy settings (%1)").arg(ProxySettings.systemProxySummary)
            : ProxySettings.mode === ProxySettings.Custom
            ? qsTr("Route all network requests and audio streaming through a custom proxy")
            : qsTr("Direct connection without any proxy")
        controlWidth: Theme.sized(280)

        SegmentedTabs {
            id: proxyModes

            width: Theme.sized(280)
            labels: [qsTr("System"), qsTr("Custom"), qsTr("Direct")]
            current: ProxySettings.mode
            onCurrentChanged: {
                ProxySettings.mode = proxyModes.current;
                proxyModes.current = Qt.binding(() => ProxySettings.mode);
            }
        }
    }

    SettingRow {
        width: parent.width
        visible: ProxySettings.mode === ProxySettings.System
        title: qsTr("System proxy")
        caption: ProxySettings.systemProxySummary
        controlWidth: refreshBtn.implicitWidth

        PillButton {
            id: refreshBtn

            icon: "refresh"
            text: qsTr("Detect again")
            onClicked: ProxySettings.refreshSystemProxy()
        }
    }

    SettingRow {
        width: parent.width
        visible: ProxySettings.mode === ProxySettings.Custom
        title: qsTr("Protocol")
        caption: qsTr("Proxy protocol used for API calls and media streaming")
        controlWidth: Theme.sized(180)

        SegmentedTabs {
            id: typeTabs

            width: Theme.sized(180)
            labels: [qsTr("HTTP"), qsTr("SOCKS5")]
            current: ProxySettings.type === ProxySettings.Socks5 ? 1 : 0
            onCurrentChanged: {
                ProxySettings.type = typeTabs.current === 1 ? ProxySettings.Socks5 : ProxySettings.Http;
                typeTabs.current = Qt.binding(() => ProxySettings.type === ProxySettings.Socks5 ? 1 : 0);
            }
        }
    }

    SettingRow {
        width: parent.width
        visible: ProxySettings.mode === ProxySettings.Custom
        title: qsTr("Proxy host")
        caption: qsTr("Host name or IP address (e.g. 192.168.49.1 for PDA Net)")
        controlWidth: Theme.sized(240)

        InputField {
            id: hostField

            width: Theme.sized(240)
            text: ProxySettings.host
            placeholder: "192.168.49.1"
            onAccepted: ProxySettings.host = hostField.text.trim()

            Connections {
                target: ProxySettings
                function onChanged() {
                    if (!hostField.focused)
                        hostField.text = ProxySettings.host;
                }
            }
        }
    }

    SettingRow {
        width: parent.width
        visible: ProxySettings.mode === ProxySettings.Custom
        title: qsTr("Proxy port")
        caption: qsTr("Port number of the proxy server (e.g. 8000)")
        controlWidth: Theme.sized(120)

        InputField {
            id: portField

            width: Theme.sized(120)
            text: ProxySettings.port > 0 ? ProxySettings.port.toString() : ""
            placeholder: "8000"
            maximumLength: 5
            onAccepted: {
                const val = parseInt(portField.text.trim());
                if (!isNaN(val) && val > 0 && val <= 65535)
                    ProxySettings.port = val;
            }

            Connections {
                target: ProxySettings
                function onChanged() {
                    if (!portField.focused)
                        portField.text = ProxySettings.port > 0 ? ProxySettings.port.toString() : "";
                }
            }
        }
    }

    SettingRow {
        width: parent.width
        visible: ProxySettings.mode === ProxySettings.Custom
        title: qsTr("Username")
        caption: qsTr("Optional username for proxy authentication")
        controlWidth: Theme.sized(240)

        InputField {
            id: userField

            width: Theme.sized(240)
            text: ProxySettings.user
            placeholder: qsTr("Optional")
            onAccepted: ProxySettings.user = userField.text.trim()

            Connections {
                target: ProxySettings
                function onChanged() {
                    if (!userField.focused)
                        userField.text = ProxySettings.user;
                }
            }
        }
    }

    SettingRow {
        width: parent.width
        visible: ProxySettings.mode === ProxySettings.Custom
        title: qsTr("Password")
        caption: qsTr("Optional password for proxy authentication")
        controlWidth: Theme.sized(240)

        InputField {
            id: passField

            width: Theme.sized(240)
            text: ProxySettings.password
            placeholder: qsTr("Optional")
            onAccepted: ProxySettings.password = passField.text

            Connections {
                target: ProxySettings
                function onChanged() {
                    if (!passField.focused)
                        passField.text = ProxySettings.password;
                }
            }
        }
    }

    SettingRow {
        width: parent.width
        visible: ProxySettings.mode === ProxySettings.Custom
        title: qsTr("Save and apply")
        caption: qsTr("Save custom proxy configuration and apply to player engine")
        controlWidth: applyBtn.implicitWidth

        PillButton {
            id: applyBtn

            text: qsTr("Apply")
            toggled: true
            onClicked: {
                ProxySettings.host = hostField.text.trim();
                const portVal = parseInt(portField.text.trim());
                if (!isNaN(portVal) && portVal > 0 && portVal <= 65535)
                    ProxySettings.port = portVal;
                ProxySettings.user = userField.text.trim();
                ProxySettings.password = passField.text;
                ProxySettings.apply();
            }
        }
    }

    SettingRow {
        width: parent.width
        title: qsTr("Connection status")
        caption: Connectivity.checking
            ? qsTr("Testing connection through %1...").arg(ProxySettings.effectiveProxy)
            : Connectivity.online
            ? qsTr("Connected online via %1").arg(ProxySettings.effectiveProxy)
            : qsTr("Offline or unreachable via %1").arg(ProxySettings.effectiveProxy)
        controlWidth: testBtn.implicitWidth

        PillButton {
            id: testBtn

            icon: "refresh"
            text: Connectivity.checking ? qsTr("Checking...") : qsTr("Test connection")
            interactive: !Connectivity.checking
            onClicked: Connectivity.check()
        }
    }
}
