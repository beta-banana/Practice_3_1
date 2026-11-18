#pragma once
#include "uav.h"
#include "sensor.h"
#include "navigation.h"
#include "controller.h"
#include <QObject>
#include <vector>

class Autopilot final : public QObject {
    Q_OBJECT
public:
    Autopilot(UAV* vehicle,const std::vector<Sensor*>& sensors,NavigationManager* navigation,
              FlightController* controller,QObject* parent=nullptr);
    bool set_target_altitude(double value);
    double target_altitude() const;
    double altitude() const;
    double error() const;
    double control() const;
    double elapsed() const;
public slots:
    void run_cycle();
signals:
    void telemetry_updated(double altitude,double error,double control);
    void fault_detected(const QString& reason);
    void target_changed(double target);
private:
    UAV* vehicle_;
    std::vector<Sensor*> sensors_;
    NavigationManager* navigation_;
    FlightController* controller_;
    double target_altitude_=200;
    double control_=0;
    double elapsed_=0;
};
