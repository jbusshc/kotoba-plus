#ifndef LISTITEMDELEGATE_H
#define LISTITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include <QSize>
#include <QPainter>
#include <QFont>

#if defined(Q_OS_WIN)
#define FONT "Yu Gothic UI"
#else
#define FONT "Noto Sans JP"
#endif

class ListItemDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    // ---- Cambiar fuente aquí ----
    void initStyleOption(QStyleOptionViewItem *option,
                         const QModelIndex &index) const override
    {
        QStyledItemDelegate::initStyleOption(option, index);

        QFont font;
        font.setFamily(FONT);   // excelente para japonés (puedes cambiarlo)
        font.setPointSize(12);             // más grande y legible
        font.setStyleStrategy(QFont::PreferAntialias); // mejor suavizado
        
        option->font = font;
    }

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override 
    {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setWidth(option.rect.width());
        size.setHeight(size.height() + 6); // espacio para separador
        return size;
    }

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override 
    {
        // Pintar contenido estándar con la fuente que pusimos
        QStyledItemDelegate::paint(painter, option, index);

        // Línea separadora
        painter->save();

        QPen pen(QColor(180, 180, 180));
        pen.setWidth(1);
        painter->setPen(pen);

        int y = option.rect.bottom();
        painter->drawLine(option.rect.left(), y,
                          option.rect.right(), y);

        painter->restore();
    }
};

#endif // LISTITEMDELEGATE_H
