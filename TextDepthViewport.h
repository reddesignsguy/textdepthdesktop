#ifndef TEXTDEPTHVIEWPORT_H
#define TEXTDEPTHVIEWPORT_H


#include <QQuickPaintedItem>
#include <iostream>

class TextDepthViewport : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
public:
    TextDepthViewport(QQuickItem *parent = nullptr);

    void mousePressEvent(QMouseEvent* event)
    {

        QPointF localPos = event->position();        // item-local coords
        QPointF scenePos = event->scenePosition();   // scene coords
        QPointF globalPos = event->globalPosition(); // screen coords

        qDebug() << "scene pos: "<< scenePos;

        qDebug() << "local pos (real!): " << localPos;
        qDebug() << "global pos: " << globalPos;
        qDebug() << "-----------" ;
    }

    void paint(QPainter *painter) override;

    QString text() const { std::cout<< "hi"<<std::endl; }
    void setText(const QString &text);

    // TODO: Refactor me! Im so ugly!
    // Q_INVOKABLE void writeToPhotoshop();
signals:
    void textChanged();
};

#endif // TEXTDEPTHVIEWPORT_H
