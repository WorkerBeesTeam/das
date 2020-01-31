import QtQuick 2.5

MouseArea {
    id: btn
    width: size
    height: size

    property int size: 100
    property int lineWidth: 5
    property color lineColor: "#CCCCCC"
    property color bgColor: "#212121"
    property bool isPlus: false
    property bool signVisible: true
    onSignVisibleChanged:
        canvas.requestPaint()

    Canvas {
        id: canvas
        anchors.fill: parent

        onPaint: {
            var ctx = getContext("2d");

            ctx.fillStyle = btn.bgColor;
            ctx.lineWidth = 1;

            roundRect(ctx, 0, 0, 5, true)

            if (btn.signVisible)
            {
                drawLine(ctx,
                         btn.lineWidth, height / 2,
                         width - btn.lineWidth, height / 2)

                if (isPlus)
                    drawLine(ctx,
                             width / 2, btn.lineWidth,
                             width / 2, height - btn.lineWidth)
            }
        }

        function drawLine(ctx, x1, y1, x2, y2) {
            ctx.beginPath();
            ctx.moveTo(x1, y1);
            ctx.lineTo(x2, y2);
            ctx.lineWidth = btn.lineWidth;
            ctx.strokeStyle = btn.lineColor;
            ctx.stroke();
        }

        function roundRect(ctx, x, y, radius, fill, stroke) {
            if (typeof stroke === 'undefined') stroke = false;
            if (typeof radius === 'undefined') radius = 5;
            if (typeof radius === 'number') {
                radius = {tl: radius, tr: radius, br: radius, bl: radius};
            } else {
                var defaultRadius = {tl: 0, tr: 0, br: 0, bl: 0};
                for (var side in defaultRadius) {
                    radius[side] = radius[side] || defaultRadius[side];
                }
            }
            ctx.beginPath();
            ctx.moveTo(x + radius.tl, y);
            ctx.lineTo(x + width - radius.tr, y);
            ctx.quadraticCurveTo(x + width, y, x + width, y + radius.tr);
            ctx.lineTo(x + width, y + height - radius.br);
            ctx.quadraticCurveTo(x + width, y + height, x + width - radius.br, y + height);
            ctx.lineTo(x + radius.bl, y + height);
            ctx.quadraticCurveTo(x, y + height, x, y + height - radius.bl);
            ctx.lineTo(x, y + radius.tl);
            ctx.quadraticCurveTo(x, y, x + radius.tl, y);
            ctx.closePath();
            if (fill)
                ctx.fill();
            if (stroke)
                ctx.stroke();
        }
    }
}
