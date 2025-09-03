import QtQuick
import QtQuick.Controls.Universal
import QtQuick.Layouts

ApplicationWindow {
    id: root
    visible: true
    width: 400
    height: 250
    title: "ZIP Extractor"
    Universal.theme: Universal.System
    Universal.accent: palette.highlight

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15

        Label {
            text: "Context Menu Integration"
            font.pointSize: 14
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: "Add 'Extract Here' to right-click menu for ZIP files.\nThis allows you to right-click any ZIP file and extract it."
            color: "gray"
            wrapMode: Text.WordWrap
            Layout.preferredWidth: 350
            Layout.alignment: Qt.AlignHCenter
            horizontalAlignment: Text.AlignHCenter
        }

        Label {
            text: RegistryHelper.isContextMenuRegistered() ?
                  "✓ Context menu is registered" :
                  "✗ Context menu not registered"
            color: RegistryHelper.isContextMenuRegistered() ? "green" : "red"
            Layout.alignment: Qt.AlignHCenter
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 10

            Button {
                text: "Add"
                enabled: !RegistryHelper.isContextMenuRegistered()
                onClicked: {
                    if (RegistryHelper.registerContextMenu(RegistryHelper.getCurrentAppPath())) {
                        // Force refresh of the status
                        root.forceActiveFocus()
                    }
                }
            }

            Button {
                text: "Remove"
                enabled: RegistryHelper.isContextMenuRegistered()
                onClicked: {
                    if (RegistryHelper.unregisterContextMenu()) {
                        // Force refresh of the status
                        root.forceActiveFocus()
                    }
                }
            }

            Button {
                text: "Cancel"
                onClicked: Qt.quit()
            }
        }

        Item { Layout.fillHeight: true }
    }
}
