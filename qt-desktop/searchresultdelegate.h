#ifndef SEARCHRESULTDELEGATE_H
#define SEARCHRESULTDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>

class SearchResultDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit SearchResultDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

void paint(QPainter *painter, const QStyleOptionViewItem &option,
           const QModelIndex &index) const override {
        painter->save();

        // Fondo seleccionado o normal
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, option.palette.highlight());
            painter->setPen(option.palette.highlightedText().color());
        } else {
            painter->setPen(option.palette.text().color());
        }

        // Obtener texto con saltos
        QString text = index.data(Qt::DisplayRole).toString();

        // Fuente y alineación
        QFont font = option.font;
        font.setPointSize(12);
        painter->setFont(font);

        QRect textRect = option.rect.adjusted(5, 5, -5, -5);

        // Dibujar texto con salto de línea
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, text);

        // Dibujar línea separadora al final del ítem
        QPen penLine(Qt::gray);
        penLine.setWidth(1);
        painter->setPen(penLine);

        int y = option.rect.bottom();
        painter->drawLine(option.rect.left(), y, option.rect.right(), y);

        painter->restore();
    }


    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QString text = index.data(Qt::DisplayRole).toString();

        QFont font = option.font;
        font.setPointSize(12);

        QFontMetrics fm(font);
        int width = option.rect.width() > 0 ? option.rect.width() : 300; // ancho aproximado

        QRect rect = fm.boundingRect(0, 0, width - 10, 1000, Qt::TextWordWrap, text);

        return QSize(rect.width() + 10, rect.height() + 10);
    }
};

#endif // SEARCHRESULTDELEGATE_H
