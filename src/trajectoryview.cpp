#include "trajectoryview.h"
#include <QGraphicsScene>
#include <QGraphicsPathItem>
#include <QGraphicsPolygonItem>
#include <QPainterPath>
#include <QResizeEvent>
#include <QScrollBar>
TrajectoryView::TrajectoryView(QWidget* parent):QGraphicsView(parent),scene_(new QGraphicsScene(this)) {
    setObjectName("graphicsView"); setScene(scene_); setFrameShape(QFrame::NoFrame);
    setRenderHint(QPainter::Antialiasing); setBackgroundBrush(QColor("#F8FAFF"));
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scene_->setSceneRect(-8,-30,620,290);
    for(int h=0;h<=500;h+=100) {
        scene_->addLine(0,220-h*0.4,600,220-h*0.4,QPen(QColor("#E3E9F4"),1));
        auto* text=scene_->addText(QString::number(h)+" м",QFont("Arial",17)); text->setDefaultTextColor(QColor("#7186A7")); text->setPos(0,198-h*0.4);
    }
    target_=scene_->addLine(0,140,600,140,QPen(QColor("#F4B547"),2,Qt::DashLine));
    trail_=scene_->addPath({},QPen(QColor("#16B9B0"),4));
    QPolygonF drone; drone<<QPointF(0,-15)<<QPointF(13,12)<<QPointF(0,6)<<QPointF(-13,12);
    marker_=scene_->addPolygon(drone,QPen(Qt::white,2),QBrush(QColor("#1814F3")));
    marker_->setZValue(2); marker_->setPos(0,160);
    auto* caption=scene_->addText("Профиль высоты • последние 120 с",QFont("Arial",17));
    caption->setDefaultTextColor(QColor("#7186A7")); caption->setPos(50,235);
}
void TrajectoryView::append(double time,double altitude,double target) {
    points_.enqueue(QPointF(time,altitude)); while(points_.size()>1200) points_.dequeue();
    const double start=std::max(0.,time-120.);
    QPainterPath path;
    for(int i=0;i<points_.size();++i) {
        const QPointF p((points_[i].x()-start)*5,220-points_[i].y()*0.4);
        if(i==0) path.moveTo(p); else path.lineTo(p);
    }
    trail_->setPath(path); marker_->setPos((time-start)*5,220-altitude*0.4);
    target_->setLine(0,220-target*0.4,600,220-target*0.4);
}
void TrajectoryView::resizeEvent(QResizeEvent* event) { QGraphicsView::resizeEvent(event); fitInView(scene_->sceneRect(),Qt::KeepAspectRatio); }
