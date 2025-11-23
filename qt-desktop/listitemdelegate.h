#ifndef LISTITEMDELEGATE_H
#define LISTITEMDELEGATE_H


#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include <QSize>
#include <QPainter>


class ListItemDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setWidth(option.rect.width());
        size.setHeight(size.height() + 6); // espacio para separador
        return size;
    }

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override {

        // Pintar el item normal
        QStyledItemDelegate::paint(painter, option, index);

        // Dibujar línea separadora al final del item
        painter->save();

        QPen pen(QColor(180, 180, 180)); // gris suave
        pen.setWidth(1);
        painter->setPen(pen);

        int y = option.rect.bottom();
        painter->drawLine(option.rect.left(), y,
                          option.rect.right(), y);

        painter->restore();
    }
};

#endif // LISTITEMDELEGATE_H
