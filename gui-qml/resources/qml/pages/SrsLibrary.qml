import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Page {
    title: "SRS Library"

    /* Cuando la página se hace visible (o se crea), forzar carga inicial.
       Esto cubre el caso en que srsLibraryVM esté listo pero el ComboBox
       ya haya disparado onCurrentTextChanged antes de que el deck estuviera
       poblado. */
    Component.onCompleted: {
        if (srsLibraryVM)
            srsLibraryVM.setFilter("All")
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 10
        anchors.margins: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            TextField {
                id: searchField
                Layout.fillWidth: true
                placeholderText: "Search word or meaning..."

                onTextChanged: {
                    if (srsLibraryVM)
                        srsLibraryVM.setSearch(text)
                }
            }

            ComboBox {
                id: filterBox
                model: ["All", "New", "Learning", "Review", "Suspended"]

                onActivated: {
                    /* Usar onActivated (acción explícita del usuario) en vez de
                       onCurrentTextChanged que dispara también en la inicialización,
                       antes de que el ViewModel esté listo. */
                    if (srsLibraryVM)
                        srsLibraryVM.setFilter(currentText)
                }
            }
        }

        Label {
            Layout.fillWidth: true
            opacity: 0.6
            text: cardList.count + " cards"
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#444"
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: cardList
                anchors.fill: parent

                model: srsLibraryVM
                clip: true
                spacing: 2

                /* reuseItems: true puede suprimir el refresco visual cuando
                   el modelo hace beginResetModel/endResetModel. Se desactiva
                   para garantizar que los delegates se recreen correctamente
                   tras cada búsqueda o filtro. */
                reuseItems: false
                cacheBuffer: 400

                delegate: SrsCardDelegate {
                    width: cardList.width

                    word:    model.word
                    meaning: model.meaning
                    state:   model.state
                    due:     model.due
                    entryId: model.entryId

                    onOpenDetails: function(id) {
                        stack.push("qrc:/qml/pages/SrsCardDetails.qml", { entryId: id })
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                visible: cardList.count === 0
                text: "No cards found"
                opacity: 0.5
            }
        }
    }
}
