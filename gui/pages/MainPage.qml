import QtQuick 2.6
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3

import Das 1.0
import "helpers"

SectionsView {
    id: sectionView
    model: SectionsModel { id: sctModel }

    delegate: Rectangle {
        width: parent.width
        height: item.height
        color: model.statusColor

        Text {
            visible: text.length
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            text: model.statusText
            font.pointSize: 11
            color: model.textColor
        }

        ItemDelegate {
            id: item
            anchors.fill: parent

            width: parent.width
            text: model.title

            property color testColor: model.textColor
            onTestColorChanged: contentItem.color = model.textColor

            font.pointSize: 13
            highlighted: ListView.isCurrentItem

            Row {
                anchors.right: parent.right
                anchors.rightMargin: 16
                anchors.verticalCenter: parent.verticalCenter

                spacing: 8

                Text {
                    text: model.value
                    color: model.textColor
                }

                Text {
                    text: model.sign
                    color: model.textColor
                }
            }

            onClicked:
            {
                var typeToString = function(t) {
                    switch(t) {
                    case DIG_Param_Type.IntType: return 'Int';
                    case DIG_Param_Type.BoolType: return 'Bool';
                    case DIG_Param_Type.FloatType: return 'Float';
                    case DIG_Param_Type.StringType: return 'String';
                    case DIG_Param_Type.BytesType: return 'Bytes';
                    case DIG_Param_Type.TimeType: return 'Time';
                    case DIG_Param_Type.RangeType: return 'Range';
                    case DIG_Param_Type.ComboType: return 'ComboBox';
                    default: return 'Unknown';
                    }
                }

                var param = model.param;
                var param_count = param.count();
                console.log(model.title + ' param count: ' + param_count);

                for (var i = 0; i < param_count; ++i) {
                    var p = param.get(i);
                    if (p === null) {
                        console.error('parameter is null');
                        return;
                    }

                    console.log(' - ' + p.type.title + ': ' + p.value + ' (' + typeToString(p.type.type) + ')');
                    if (p.type.type === DIG_Param_Type.ComboType)
                        console.log(JSON.stringify(sctModel.getComboParamValues(p)));
                }

                if (param_count > 0) {
                    stackView.push(Qt.resolvedUrl('ParamPage.qml'), {
                                       modelItem: model,
                                       title: model.name,
                                       param: model.param,
                                       sectModel: sctModel
                                   });
                }

                return;

                var typeStr = worker.layoutTypeToString(model.type);
                if (!typeStr)
                    return;
                var itemUrl = "helpers/ParamPage%1.qml".arg(typeStr)

                stackView.push(Qt.resolvedUrl(itemUrl), {
                                   modelItem: model,
                                   title: model.name,
                                   param: model.param
                               })

//                stackView.push({
//                                   item: Qt.resolvedUrl(itemUrl),
//                                   properties: {
//                                       modelItem: model,
//                                       title: model.name,
//                                       param: model.param
//                                   }
//                               })
            }
        }
    }
}
