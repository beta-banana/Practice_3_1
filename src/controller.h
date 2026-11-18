#pragma once
#include <QString>
class FlightController {
public:
    virtual ~FlightController() = default;
    virtual double calculate_control(double error,double dt) = 0;
    virtual QString controller_name() const = 0;
};
class SimpleController final : public FlightController {
public: double calculate_control(double error,double dt) override; QString controller_name() const override;
};
class PIDController final : public FlightController {
public:
    PIDController(double kp=0.8,double ki=0.015,double kd=0.12);
    double calculate_control(double error,double dt) override;
    QString controller_name() const override;
private:
    double kp_,ki_,kd_;
    double integral_=0;
    double previous_error_=0;
    bool has_previous_=false;
};
