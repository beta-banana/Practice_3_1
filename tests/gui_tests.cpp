#include <QtTest>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QListWidget>
#include <QTableWidget>
#include <QTimer>
#include <QMessageBox>
#include <QLineSeries>
#include <QChartView>
#include <QChart>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QDir>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include "mainwindow.h"
#include "autopilot.h"

class GuiTests : public QObject {
    Q_OBJECT
private:
    QString out_;
    void save(MainWindow& w,const QString& name) {
        QApplication::processEvents();
        QVERIFY(w.grab().save(out_+"/"+name+".png"));
    }
    void click(MainWindow& w,const char* id) { auto* b=w.findChild<QPushButton*>(id); QVERIFY(b); QTest::mouseClick(b,Qt::LeftButton); }
    void closeWarning(MainWindow& w) { for(auto* b:w.findChildren<QMessageBox*>()) if(b->isVisible()) b->accept(); QApplication::processEvents(); }
private slots:
    void initTestCase() { out_=qEnvironmentVariable("STATION_VERIFY_DIR",QDir::currentPath()+"/verification"); QDir().mkpath(out_); }
    void integratedRequiredScenarios() {
        MainWindow w; w.show(); QTest::qWait(100);
        QJsonObject results;
        auto* timer=w.findChild<QTimer*>("cycleTimer"); auto* pilot=w.findChild<Autopilot*>();
        auto* status=w.findChild<QLabel*>("labelStatus"); auto* target=w.findChild<QDoubleSpinBox*>("spinTarget");
        auto* nav=w.findChild<QLabel*>("labelNavigation"); auto* fleet=w.findChild<QTableWidget*>("tableFleet");
        auto* chart=w.findChild<QChartView*>("chartView")->chart();
        auto* altitude=qobject_cast<QLineSeries*>(chart->series().at(0)); auto* error=qobject_cast<QLineSeries*>(chart->series().at(1));
        auto* scene=w.findChild<QGraphicsView*>("graphicsView")->scene();
        QVERIFY(timer && pilot && status && target && nav && fleet && altitude && error);
        QCOMPARE(status->text(),QString("Остановлено")); QCOMPARE(fleet->rowCount(),3);
        QCOMPARE(target->minimum(),50.); QCOMPARE(target->maximum(),500.); QCOMPARE(target->singleStep(),10.);
        save(w,"00-stopped");
        QSignalSpy telemetry(pilot,&Autopilot::telemetry_updated);
        click(w,"btnStart"); QVERIFY(timer->isActive()); QTest::qWait(1050);
        QVERIFY(telemetry.count()>=9 && telemetry.count()<=12);
        results["ticks_in_1050ms"]=telemetry.count();
        for(int i=0;i<180;++i) pilot->run_cycle();
        QCOMPARE(altitude->count(),telemetry.count()); QCOMPARE(error->count(),telemetry.count());
        QVERIFY(std::abs(pilot->error())<2); QVERIFY(nav->text().contains("СНС"));
        const auto* marker=w.findChild<QGraphicsView*>("graphicsView");
        Q_UNUSED(marker);
        results["healthy_altitude"]=pilot->altitude(); results["healthy_error"]=pilot->error(); results["healthy_time"]=pilot->elapsed();
        save(w,"01-healthy");
        click(w,"btnFaultSensor"); QTest::qWait(160);
        QVERIFY(!timer->isActive()); QVERIFY(status->text().contains("Гироскоп"));
        QVERIFY(w.findChild<QListWidget*>("eventJournal")->item(0)->text().contains("Гироскоп"));
        QVERIFY(!w.findChildren<QMessageBox*>().isEmpty());
        const int frozen=telemetry.count(); QTest::qWait(220); QCOMPARE(telemetry.count(),frozen);
        QVERIFY(w.findChild<QMessageBox*>("faultDialog")->grab().save(out_+"/02-sensor-dialog.png"));
        results["fault_reason"]=status->text(); results["fault_timer_active"]=timer->isActive();
        closeWarning(w); save(w,"02-sensor-fault");
        click(w,"btnReset"); QVERIFY(timer->isActive());
        click(w,"btnFaultGPS"); QTest::qWait(140);
        QVERIFY(timer->isActive()); QVERIFY(nav->text().contains("ИНС")); QCOMPARE(nav->property("tone").toString(),QString("warning"));
        results["fallback_navigation"]=nav->text(); results["fallback_timer_active"]=timer->isActive();
        save(w,"03-navigation-fallback");
        click(w,"btnRestoreGPS"); QTest::qWait(140); QVERIFY(nav->text().contains("СНС"));
        const double oldError=pilot->error(); target->setValue(300);
        QVERIFY(std::abs(pilot->error()-oldError-100)<1e-7);
        results["target_error_jump"]=pilot->error()-oldError;
        for(int i=0;i<350;++i) pilot->run_cycle();
        QVERIFY(std::abs(pilot->error())<2); results["target_300_altitude"]=pilot->altitude(); results["target_300_error"]=pilot->error(); results["target_300_time"]=pilot->elapsed(); save(w,"04-target-300");
        click(w,"btnFaultSensor"); QTest::qWait(140); closeWarning(w);
        click(w,"btnReset"); click(w,"btnReset"); QTest::qWait(140);
        QVERIFY(timer->isActive()); QVERIFY(status->text().contains("выполняется"));
        QCOMPARE(status->property("tone").toString(),QString("success"));
        results["reset_status"]=status->text(); results["reset_timer_active"]=timer->isActive();
        save(w,"05-repeated-reset");
        click(w,"btnPause"); QCOMPARE(fleet->item(0,2)->text(),QString("Пауза")); const double t=pilot->elapsed(); QTest::qWait(220); QCOMPARE(pilot->elapsed(),t);
        click(w,"btnStart"); QTest::qWait(140); QVERIFY(pilot->elapsed()>t);
        click(w,"btnPause");
        const int itemCount=scene->items().size();
        for(int i=0;i<2000;++i) pilot->run_cycle();
        QCOMPARE(scene->items().size(),itemCount); QVERIFY(altitude->count()<=1200); QVERIFY(error->count()<=1200);
        w.resize(1050,740); QTest::qWait(100); save(w,"06-small-window");
        QFile resultFile(out_+"/scenario-results.json"); QVERIFY(resultFile.open(QIODevice::WriteOnly)); resultFile.write(QJsonDocument(results).toJson());
    }
    void secondLevelFallbackAndTotalFailure() {
        MainWindow w; w.show(); click(w,"btnStart");
        click(w,"btnFaultGPS"); click(w,"btnFaultINS"); QTest::qWait(140);
        QVERIFY(w.findChild<QLabel*>("labelNavigation")->text().contains("Визуальная"));
        QVERIFY(w.findChild<QTimer*>("cycleTimer")->isActive());
        click(w,"btnFaultVisual"); QTest::qWait(140);
        QVERIFY(!w.findChild<QTimer*>("cycleTimer")->isActive());
        QVERIFY(w.findChild<QLabel*>("labelStatus")->text().contains("навигации"));
        closeWarning(w);
        click(w,"btnReset"); QTest::qWait(140);
        QVERIFY(w.findChild<QLabel*>("labelNavigation")->text().contains("СНС"));
        QVERIFY(w.findChild<QTimer*>("cycleTimer")->isActive());
    }
};
QTEST_MAIN(GuiTests)
#include "gui_tests.moc"
