import QtQuick
import QtQuick.Controls.Universal
import QtQuick.Layouts

ApplicationWindow {
    id: root
    visible: true
    width: 500
    height: mainLyt.implicitHeight + 20
    title: "ZipExtract"
    Universal.theme: Universal.System
    Universal.accent: palette.highlight

    Component.onCompleted: {
        if (initialZipPath !== "") {
            ZipExtractor.startExtraction(initialZipPath)
        }
    }

    ColumnLayout {
        id: mainLyt
        anchors.fill: parent
        spacing: 10
        anchors.margins: 10

        Label {
            text: initialZipPath || ""
            Layout.fillWidth: true
            elide: Text.ElideMiddle
            font.bold: true
        }

        ProgressBar {
            Layout.fillWidth: true
            value: ZipExtractor.progress / 100.0
            visible: ZipExtractor.totalFiles > 0
        }

        GridLayout {
            columns: 2
            visible: ZipExtractor.isExtracting || ZipExtractor.totalFiles > 0

            Label { text: "Current file:" }
            Label {
                text: ZipExtractor.currentFileName
                Layout.fillWidth: true
                elide: Text.ElideMiddle
            }

            Label { text: "Progress:" }
            Label { text: ZipExtractor.currentFile + " / " + ZipExtractor.totalFiles }

            Label { text: "ETA:" }
            Label { text: ZipExtractor.eta }
        }
    }

    Connections {
        target: ZipExtractor
        function onExtractionFinished(success, message) {
            Qt.quit()
        }
    }
}
