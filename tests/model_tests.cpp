#include <QtTest>
#include "autopilot.h"
#include <limits>

// Все проверки обращаются к реальным моделям, без подмены зависимостей.
class ModelTests : public QObject {
    Q_OBJECT
private slots:
    void regulationConvergesAndTargetChanges() {
        Quadcopter craft; Gyroscope gyro; Accelerometer accel; Barometer baro;
        GPSNavigation gps(&craft); InertialNavigation ins(&craft); VisualNavigation visual(&craft);
        NavigationManager nav({&gps, &ins, &visual}); PIDController pid;
        Autopilot pilot(&craft, {&gyro,&accel,&baro}, &nav, &pid);
        QSignalSpy samples(&pilot, &Autopilot::telemetry_updated);
        pilot.run_cycle();
        QCOMPARE(samples.count(), 1);
        QVERIFY(craft.altitude() > 150.0);
        for(int i=0;i<800;++i) pilot.run_cycle();
        QVERIFY2(std::abs(pilot.error()) < 0.5, qPrintable(QString::number(pilot.error())));
        const double before=pilot.error();
        pilot.set_target_altitude(300);
        QVERIFY(std::abs(pilot.error()-before-100) < 1e-8);
        QCOMPARE(pilot.target_altitude(), 300.0);
        for(int i=0;i<1200;++i) pilot.run_cycle();
        QVERIFY(std::abs(pilot.error()) < 0.5);
    }
    void priorityFallbackAndReturnPreserveFlight() {
        Quadcopter craft; GPSNavigation gps(&craft); InertialNavigation ins(&craft); VisualNavigation visual(&craft);
        NavigationManager nav({&gps,&ins,&visual});
        QSignalSpy changes(&nav,&NavigationManager::active_system_changed);
        QVERIFY(nav.check_priority()); QCOMPARE(nav.system_name(),QString("СНС"));
        gps.set_valid(false); QVERIFY(nav.check_priority()); QCOMPARE(nav.system_name(),QString("ИНС"));
        QCOMPARE(nav.get_position().altitude,150.0);
        ins.set_valid(false); QVERIFY(nav.check_priority()); QCOMPARE(nav.system_name(),QString("Визуальная"));
        gps.set_valid(true); QVERIFY(nav.check_priority()); QCOMPARE(nav.system_name(),QString("СНС"));
        QCOMPARE(changes.count(),4);
        nav.check_priority(); QCOMPARE(changes.count(),4);
    }
    void sensorAndNavigationFaultsSuppressTelemetry() {
        Quadcopter craft; Gyroscope gyro; GPSNavigation gps(&craft); PIDController pid;
        NavigationManager nav({&gps}); Autopilot pilot(&craft,{&gyro},&nav,&pid);
        QSignalSpy samples(&pilot,&Autopilot::telemetry_updated), faults(&pilot,&Autopilot::fault_detected);
        gyro.set_ready(false); pilot.run_cycle(); QCOMPARE(samples.count(),0); QCOMPARE(faults.count(),1);
        QVERIFY(faults.last()[0].toString().contains("Гироскоп")); QCOMPARE(craft.altitude(),150.0);
        gyro.set_ready(true); gps.set_valid(false); pilot.run_cycle(); QCOMPARE(samples.count(),0); QCOMPARE(faults.count(),2);
        gps.set_valid(true); pilot.run_cycle(); QCOMPARE(samples.count(),1);
    }
    void invalidTargetsAreRejectedAndBoundsWork() {
        Quadcopter craft; Gyroscope gyro; GPSNavigation gps(&craft); PIDController pid;
        NavigationManager nav({&gps}); Autopilot pilot(&craft,{&gyro},&nav,&pid);
        for(double v : {49.,501.,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()}) {
            QVERIFY(!pilot.set_target_altitude(v)); QCOMPARE(pilot.target_altitude(),200.0);
        }
        QVERIFY(pilot.set_target_altitude(50));
        for(int i=0;i<1500;++i) pilot.run_cycle();
        QVERIFY(std::abs(pilot.error())<0.5);
        QVERIFY(pilot.set_target_altitude(500));
        for(int i=0;i<2500;++i) pilot.run_cycle();
        QVERIFY(std::abs(pilot.error())<0.5); QVERIFY(craft.altitude()>=0);
    }
    void missingDependenciesFailSafely() {
        PIDController pid; NavigationManager nav({});
        Autopilot pilot(nullptr,{},&nav,&pid);
        QSignalSpy faults(&pilot,&Autopilot::fault_detected);
        pilot.run_cycle(); QCOMPARE(faults.count(),1);
        Quadcopter craft; Gyroscope gyro; GPSNavigation gps(&craft); NavigationManager good({&gps});
        Autopilot noController(&craft,{&gyro},&good,nullptr);
        QSignalSpy missing(&noController,&Autopilot::fault_detected);
        noController.run_cycle(); QCOMPARE(missing.count(),1);
    }
};
QTEST_GUILESS_MAIN(ModelTests)
#include "model_tests.moc"
