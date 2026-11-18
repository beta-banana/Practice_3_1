#pragma once
#include <QGraphicsView>
#include <QQueue>
class QGraphicsPathItem;
class QGraphicsPolygonItem;
class QGraphicsLineItem;
class TrajectoryView final : public QGraphicsView {
public:
    explicit TrajectoryView(QWidget* parent=nullptr);
    void append(double time,double altitude,double target);
protected:
    void resizeEvent(QResizeEvent* event) override;
private:
    QGraphicsScene* scene_;
    QGraphicsPathItem* trail_;
    QGraphicsPolygonItem* marker_;
    QGraphicsLineItem* target_;
    QQueue<QPointF> points_;
};
