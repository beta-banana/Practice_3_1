#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "trajectoryview.h"
#include <QApplication>
#include <QButtonGroup>
#include <QChart>
#include <QChartView>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineSeries>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QStyle>
#include <QTableWidget>
#include <QTimer>
#include <QValueAxis>
#include <algorithm>
#include <cmath>

namespace {
QLabel* label(const QString& text,const QString& name,QWidget* parent=nullptr) {
    auto* w=new QLabel(text,parent); w->setObjectName(name); return w;
}
QPushButton* button(const QString& text,const QString& name,const QString& role,QWidget* parent=nullptr) {
    auto* w=new QPushButton(text,parent); w->setObjectName(name); w->setProperty("role",role);
    w->setCursor(Qt::PointingHandCursor); w->setMinimumHeight(34); return w;
}
QFrame* card(QVBoxLayout*& layout,const QString& title,QWidget* parent) {
    auto* frame=new QFrame(parent); frame->setProperty("card",true);
    layout=new QVBoxLayout(frame); layout->setContentsMargins(20,18,20,16); layout->setSpacing(10);
    if(!title.isEmpty()) layout->addWidget(label(title,"sectionTitle",frame)); return frame;
}
void tone(QLabel* w,const QString& value) {
    w->setProperty("tone",value); w->style()->unpolish(w); w->style()->polish(w);
}
QString number(double v,int digits=1) { return QString::number(std::abs(v)<0.5*std::pow(10.,-digits)?0.:v,'f',digits); }
}
MainWindow::MainWindow(QWidget* parent):QMainWindow(parent),ui_(std::make_unique<Ui::MainWindow>()),
    fleet_{&craft_,&spareHexa_,&spareOcto_},gps_(&craft_),ins_(&craft_),visual_(&craft_) {
    ui_->setupUi(this); setMinimumSize(1040,720);
    QFile style(":/assets/style.qss"); if(style.open(QFile::ReadOnly)) setStyleSheet(QString::fromUtf8(style.readAll()));
    navigation_=new NavigationManager({&gps_,&ins_,&visual_},this);
    autopilot_=new Autopilot(&craft_,{&gyro_,&accelerometer_,&barometer_},navigation_,&controller_,this);
    cycle_timer_=new QTimer(this); cycle_timer_->setObjectName("cycleTimer"); cycle_timer_->setInterval(100); cycle_timer_->setTimerType(Qt::PreciseTimer);
    build_dashboard(); wire_signals(); navigation_->check_priority(); set_status("Остановлено","neutral");
    on_telemetry_updated(autopilot_->altitude(),autopilot_->error(),autopilot_->control()); log_event("Станция готова. Целевая высота 200 м.");
}
MainWindow::~MainWindow() = default;
void MainWindow::build_dashboard() {
    auto* side=ui_->sidebarLayout; side->setContentsMargins(0,24,0,24); side->setSpacing(7);
    side->addSpacing(61);
    auto* navGroup=new QButtonGroup(this); navGroup->setExclusive(true);
    auto* overview=button("⌂   Обзор полёта","navOverview","nav",ui_->sidebar);
    auto* fleetNav=button("▦   Парк аппаратов","navFleet","nav",ui_->sidebar);
    auto* systemsNav=button("◇   Системы и отказы","navSystems","nav",ui_->sidebar);
    for(auto* b:{overview,fleetNav,systemsNav}) { b->setCheckable(true); navGroup->addButton(b); side->addWidget(b); }
    overview->setChecked(true); side->addStretch();
    auto* sideFooter=new QWidget(ui_->sidebar); auto* footer=new QVBoxLayout(sideFooter); footer->setContentsMargins(22,8,18,0);
    footer->addWidget(label("Учебный стенд","muted",sideFooter)); footer->addWidget(label("Практическое занятие 3\nВариант 2","muted",sideFooter)); footer->addSpacing(10); footer->addWidget(label("●  Цикл 10 Гц","muted",sideFooter)); side->addWidget(sideFooter);
    auto* header=ui_->headerLayout; header->setContentsMargins(26,10,26,10); header->addWidget(label("Наземная станция","headerTitle",ui_->header)); header->addStretch();
    timeLabel_=label("00:00.0","missionTime",ui_->header); header->addWidget(timeLabel_); header->addSpacing(18);
    statusLabel_=label("Остановлено","labelStatus",ui_->header); header->addWidget(statusLabel_);
    auto* main=ui_->dashboardLayout; main->setContentsMargins(24,20,24,22); main->setSpacing(18);
    auto* heading=new QWidget(ui_->dashboard); auto* titleColumn=new QVBoxLayout(heading); titleColumn->setContentsMargins(0,0,0,0); titleColumn->setSpacing(5);
    titleColumn->addWidget(label("Мониторинг миссии","pageTitle",heading)); titleColumn->addWidget(label("Квадрокоптер К-01  /  высота, регулирование и навигация","muted",heading)); main->addWidget(heading);
    auto* metrics=new QHBoxLayout; metrics->setSpacing(18); QVBoxLayout *altLayout,*errLayout,*outLayout;
    auto* altCard=card(altLayout,"",ui_->dashboard); altCard->setObjectName("altitudeCard"); altLayout->addWidget(label("Текущая высота","metricCaption",altCard));
    altitudeLabel_=label("150.0 м","labelAltitude",altCard); altLayout->addWidget(altitudeLabel_); targetLabel_=label("Цель 200 м","targetCaption",altCard); altLayout->addWidget(targetLabel_);
    auto* errCard=card(errLayout,"",ui_->dashboard); errLayout->addWidget(label("Ошибка регулирования","metricCaption",errCard)); errorLabel_=label("50.0 м","labelError",errCard); errLayout->addWidget(errorLabel_); errLayout->addWidget(label("Целевая − текущая высота","muted",errCard));
    auto* outCard=card(outLayout,"",ui_->dashboard); outLayout->addWidget(label("Управление ПИД","metricCaption",outCard)); outputLabel_=label("0.00 м/с","labelOutput",outCard); outLayout->addWidget(outputLabel_); outLayout->addWidget(label("Команда вертикальной скорости","muted",outCard));
    for(auto* frame:{altCard,errCard,outCard}) { frame->setMinimumHeight(123); metrics->addWidget(frame); } main->addLayout(metrics);
    auto* center=new QHBoxLayout; center->setSpacing(18); QVBoxLayout *chartLayout,*controlLayout;
    auto* chartCard=card(chartLayout,"Высота и ошибка регулирования",ui_->dashboard); chartCard->setMinimumHeight(350);
    altitudeSeries_=new QLineSeries(this); altitudeSeries_->setObjectName("altitudeSeries"); altitudeSeries_->setName("Высота"); altitudeSeries_->setPen(QPen(QColor("#1814F3"),2.8));
    errorSeries_=new QLineSeries(this); errorSeries_->setObjectName("errorSeries"); errorSeries_->setName("Ошибка"); errorSeries_->setPen(QPen(QColor("#16B9B0"),2.5));
    auto* chart=new QChart; chart->setBackgroundVisible(false); chart->setMargins(QMargins(0,0,0,0)); chart->setAnimationOptions(QChart::NoAnimation); chart->addSeries(altitudeSeries_); chart->addSeries(errorSeries_);
    timeAxis_=new QValueAxis(chart); timeAxis_->setRange(0,30); timeAxis_->setLabelFormat("%.0f"); timeAxis_->setTitleText("Время полёта, с"); timeAxis_->setTickCount(6);
    heightAxis_=new QValueAxis(chart); heightAxis_->setRange(-25,325); heightAxis_->setLabelFormat("%.0f"); heightAxis_->setTitleText("Высота / ошибка, м"); heightAxis_->setTickCount(6);
    for(auto* axis:{timeAxis_,heightAxis_}) { axis->setLabelsColor(QColor("#7186A7")); axis->setGridLineColor(QColor("#EFF2F8")); axis->setLineVisible(false); axis->setTitleBrush(QColor("#7186A7")); axis->setLabelsFont(QFont("Arial",10)); axis->setTitleFont(QFont("Arial",10)); }
    timeAxis_->setGridLineVisible(false); chart->addAxis(timeAxis_,Qt::AlignBottom); chart->addAxis(heightAxis_,Qt::AlignLeft);
    for(auto* series:{altitudeSeries_,errorSeries_}) { series->attachAxis(timeAxis_); series->attachAxis(heightAxis_); }
    chart->legend()->setVisible(true); chart->legend()->setAlignment(Qt::AlignTop); chart->legend()->setLabelColor(QColor("#7186A7")); chart->legend()->setFont(QFont("Arial",11));
    auto* chartView=new QChartView(chart,chartCard); chartView->setObjectName("chartView"); chartView->setRenderHint(QPainter::Antialiasing); chartView->setMinimumHeight(270); chartLayout->addWidget(chartView,1);
    auto* controlCard=card(controlLayout,"Управление полётом",ui_->dashboard); controlCard->setMinimumWidth(308);
    controlLayout->addWidget(label("Целевая высота · 50–500 м","muted",controlCard));
    targetSpin_=new QDoubleSpinBox(controlCard); targetSpin_->setObjectName("spinTarget"); targetSpin_->setRange(50,500); targetSpin_->setSingleStep(10); targetSpin_->setDecimals(0); targetSpin_->setSuffix(" м"); targetSpin_->setValue(200); targetSpin_->setKeyboardTracking(false); controlLayout->addWidget(targetSpin_);
    auto* commands=new QHBoxLayout; commands->setSpacing(8); startButton_=button("Старт","btnStart","primary",controlCard); pauseButton_=button("Пауза","btnPause","secondary",controlCard); pauseButton_->setEnabled(false);
    auto* resetButton=button("Сброс","btnReset","secondary",controlCard); commands->addWidget(startButton_); commands->addWidget(pauseButton_); commands->addWidget(resetButton); controlLayout->addLayout(commands);
    navigationLabel_=label("Навигация: СНС","labelNavigation",controlCard); controlLayout->addWidget(navigationLabel_);
    auto* failures=new QGridLayout; failures->setSpacing(7); sensorButton_=button("Отказ сенсора","btnFaultSensor","danger",controlCard); gpsButton_=button("Отказ СНС","btnFaultGPS","secondary",controlCard);
    restoreGpsButton_=button("Вернуть СНС","btnRestoreGPS","secondary",controlCard); restoreGpsButton_->setEnabled(false); insButton_=button("Отказ ИНС","btnFaultINS","secondary",controlCard); visualButton_=button("Отказ камеры","btnFaultVisual","secondary",controlCard);
    failures->addWidget(sensorButton_,0,0,1,2); failures->addWidget(gpsButton_,1,0); failures->addWidget(restoreGpsButton_,1,1); failures->addWidget(insButton_,2,0); failures->addWidget(visualButton_,2,1); controlLayout->addLayout(failures); controlLayout->addStretch();
    sensorButton_->setToolTip("Отключить гироскоп. На следующем цикле сработает аварийная защита."); resetButton->setToolTip("Восстановить сенсоры и навигацию, продолжить полёт.");
    center->addWidget(chartCard,7); center->addWidget(controlCard,4); main->addLayout(center);
    auto* bottom=new QHBoxLayout; bottom->setSpacing(18); QVBoxLayout *fleetLayout,*trajectoryLayout,*journalLayout;
    auto* fleetCard=card(fleetLayout,"Парк аппаратов",ui_->dashboard); fleetCard->setMinimumHeight(238);
    table_=new QTableWidget(3,3,fleetCard); table_->setObjectName("tableFleet"); table_->setHorizontalHeaderLabels({"Тип","Заряд","Статус"}); table_->verticalHeader()->hide(); table_->setShowGrid(false); table_->setEditTriggers(QAbstractItemView::NoEditTriggers); table_->setSelectionMode(QAbstractItemView::NoSelection);
    table_->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch); table_->horizontalHeader()->setSectionResizeMode(1,QHeaderView::ResizeToContents); table_->horizontalHeader()->setSectionResizeMode(2,QHeaderView::ResizeToContents); table_->setMinimumHeight(158);
    for(int i=0;i<int(fleet_.size());++i) { table_->setItem(i,0,new QTableWidgetItem(fleet_[i]->type())); table_->item(i,0)->setToolTip(QString("%1 · моторов: %2").arg(fleet_[i]->type()).arg(fleet_[i]->motor_count())); table_->setItem(i,1,new QTableWidgetItem(number(fleet_[i]->battery(),0)+" %")); table_->setItem(i,2,new QTableWidgetItem(i==0?"Подключён":"Резерв")); table_->setRowHeight(i,36); }
    fleetLayout->addWidget(table_); fleetLayout->addWidget(label("К-01 активен · 2 в резерве","muted",fleetCard));
    auto* trajectoryCard=card(trajectoryLayout,"Траектория полёта",ui_->dashboard); trajectory_=new TrajectoryView(trajectoryCard); trajectory_->setMinimumHeight(174); trajectoryLayout->addWidget(trajectory_,1);
    auto* journalCard=card(journalLayout,"Журнал событий",ui_->dashboard); journal_=new QListWidget(journalCard); journal_->setObjectName("eventJournal"); journal_->setWordWrap(true); journal_->setMinimumHeight(174); journalLayout->addWidget(journal_);
    bottom->addWidget(fleetCard,4); bottom->addWidget(trajectoryCard,4); bottom->addWidget(journalCard,4); main->addLayout(bottom); main->addWidget(label("Учебная модель вертикального полёта. Навигация СНС → ИНС → визуальная.","muted",ui_->dashboard)); main->addStretch();
    connect(resetButton,&QPushButton::clicked,this,&MainWindow::reset);
    connect(overview,&QPushButton::clicked,this,[this,heading] { ui_->dashboardScroll->ensureWidgetVisible(heading); });
    connect(fleetNav,&QPushButton::clicked,this,[this,fleetCard] { ui_->dashboardScroll->ensureWidgetVisible(fleetCard); });
    connect(systemsNav,&QPushButton::clicked,this,[this,controlCard] { ui_->dashboardScroll->ensureWidgetVisible(controlCard); });
}
void MainWindow::wire_signals() {
    connect(cycle_timer_,&QTimer::timeout,autopilot_,&Autopilot::run_cycle);
    // Один сигнал — три приёмника. Модель не знает о виджетах.
    connect(autopilot_,&Autopilot::telemetry_updated,this,&MainWindow::on_telemetry_updated);
    connect(autopilot_,&Autopilot::telemetry_updated,this,&MainWindow::update_chart);
    connect(autopilot_,&Autopilot::telemetry_updated,this,&MainWindow::update_scene);
    connect(autopilot_,&Autopilot::fault_detected,this,&MainWindow::on_fault_detected);
    connect(autopilot_,&Autopilot::target_changed,this,&MainWindow::on_target_changed);
    connect(navigation_,&NavigationManager::active_system_changed,this,&MainWindow::on_navigation_changed);
    connect(startButton_,&QPushButton::clicked,this,&MainWindow::start); connect(pauseButton_,&QPushButton::clicked,this,&MainWindow::pause);
    connect(targetSpin_,&QDoubleSpinBox::valueChanged,this,[this](double v) { autopilot_->set_target_altitude(v); });
    connect(sensorButton_,&QPushButton::clicked,this,[this] { gyro_.set_ready(false); sensorButton_->setEnabled(false); log_event("Гироскоп не готов; ожидается проверка цикла."); });
    connect(gpsButton_,&QPushButton::clicked,this,[this] { set_navigation_failure(&gps_,gpsButton_,"СНС"); restoreGpsButton_->setEnabled(true); });
    connect(insButton_,&QPushButton::clicked,this,[this] { set_navigation_failure(&ins_,insButton_,"ИНС"); });
    connect(visualButton_,&QPushButton::clicked,this,[this] { set_navigation_failure(&visual_,visualButton_,"Визуальная"); });
    connect(restoreGpsButton_,&QPushButton::clicked,this,[this] { gps_.set_valid(true); gpsButton_->setEnabled(true); restoreGpsButton_->setEnabled(false); log_event("Спутниковая навигация восстановлена; проверка на следующем цикле."); });
}
void MainWindow::start() { if(fault_latched_) return; cycle_timer_->start(); table_->item(0,2)->setText("В полёте"); table_->item(0,2)->setForeground(QColor("#138C74")); set_status("Полёт выполняется","success"); startButton_->setEnabled(false); pauseButton_->setEnabled(true); log_event("Старт: циклы автопилота 10 Гц."); }
void MainWindow::pause() { cycle_timer_->stop(); table_->item(0,2)->setText("Пауза"); table_->item(0,2)->setForeground(QColor("#7186A7")); set_status("Пауза","neutral"); startButton_->setEnabled(true); pauseButton_->setEnabled(false); log_event("Пауза. Состояние полёта сохранено."); }
void MainWindow::reset() {
    gyro_.set_ready(true); accelerometer_.set_ready(true); barometer_.set_ready(true); gps_.set_valid(true); ins_.set_valid(true); visual_.set_valid(true); fault_latched_=false; first_fault_shown_=false;
    sensorButton_->setEnabled(true); gpsButton_->setEnabled(true); insButton_->setEnabled(true); visualButton_->setEnabled(true); restoreGpsButton_->setEnabled(false);
    for(auto* box:findChildren<QMessageBox*>()) box->close(); log_event("Сброс: сенсоры и навигация восстановлены."); start();
}
void MainWindow::set_status(const QString& message,const QString& value) { statusLabel_->setText(message); tone(statusLabel_,value); statusLabel_->setToolTip(message); statusLabel_->setWordWrap(true); statusLabel_->setMaximumWidth(365); }
void MainWindow::on_telemetry_updated(double altitude,double error,double control) {
    altitudeLabel_->setText(number(altitude)+" м"); errorLabel_->setText(number(error)+" м"); outputLabel_->setText(number(control,2)+" м/с");
    const double t=autopilot_->elapsed(); const int minutes=int(t)/60; timeLabel_->setText(QString("%1:%2").arg(minutes,2,10,QChar('0')).arg(t-minutes*60,4,'f',1,QChar('0')));
    table_->item(0,1)->setText(number(craft_.battery(),0)+" %"); table_->item(0,2)->setText(cycle_timer_->isActive()?"В полёте":"Подключён"); table_->item(0,2)->setForeground(QColor(cycle_timer_->isActive()?"#138C74":"#7186A7"));
}
void MainWindow::update_chart(double altitude,double error,double) {
    const double t=autopilot_->elapsed(); altitudeSeries_->append(t,altitude); errorSeries_->append(t,error); for(auto* series:{altitudeSeries_,errorSeries_}) if(series->count()>1200) series->removePoints(0,series->count()-1200);
    timeAxis_->setRange(std::max(0.,t-120.),std::max(30.,t)); double low=-25,high=325;
    for(auto* series:{altitudeSeries_,errorSeries_}) for(const auto& p:series->points()) { low=std::min(low,p.y()-20); high=std::max(high,p.y()+20); } heightAxis_->setRange(low,high);
}
void MainWindow::update_scene(double altitude,double,double) { trajectory_->append(autopilot_->elapsed(),altitude,autopilot_->target_altitude()); }
void MainWindow::on_fault_detected(const QString& reason) {
    cycle_timer_->stop(); fault_latched_=true; startButton_->setEnabled(false); pauseButton_->setEnabled(false); set_status("Авария: "+reason,"error"); table_->item(0,2)->setText("Авария"); table_->item(0,2)->setForeground(QColor("#C43856")); log_event("Авария: "+reason);
    if(!first_fault_shown_) { first_fault_shown_=true; auto* box=new QMessageBox(QMessageBox::Warning,"Аварийная остановка",reason,QMessageBox::Ok,this); box->setObjectName("faultDialog"); box->button(QMessageBox::Ok)->setText("Понятно"); box->setInformativeText("Цикл остановлен. Нажмите «Сброс», чтобы восстановить системы и продолжить полёт."); box->setAttribute(Qt::WA_DeleteOnClose); box->setWindowModality(Qt::WindowModal); box->open(); }
}
void MainWindow::on_navigation_changed(const QString& name) { navigationLabel_->setText("Навигация: "+name); tone(navigationLabel_,name=="СНС"?"success":name=="Недоступна"?"error":"warning"); log_event("Активная навигация: "+name); }
void MainWindow::on_target_changed(double target) { targetLabel_->setText("Цель "+number(target,0)+" м"); errorLabel_->setText(number(autopilot_->error())+" м"); log_event("Новая целевая высота: "+number(target,0)+" м."); }
void MainWindow::set_navigation_failure(SimulatedNavigation* source,QPushButton* b,const QString& name) { source->set_valid(false); b->setEnabled(false); log_event(name+" недоступна; ожидается проверка приоритета."); }
void MainWindow::log_event(const QString& message) { auto* item=new QListWidgetItem(QString("%1 с  %2").arg(autopilot_->elapsed(),0,'f',1).arg(message)); item->setToolTip(item->text()); if(message.contains("Авария")) item->setForeground(QColor("#C43856")); journal_->insertItem(0,item); while(journal_->count()>200) delete journal_->takeItem(journal_->count()-1); }
