#include "controller.h"
#include <algorithm>
#include <cmath>
double SimpleController::calculate_control(double error,double) { return 0.5*error; }
QString SimpleController::controller_name() const { return "Пропорциональный"; }
PIDController::PIDController(double kp,double ki,double kd):kp_(kp),ki_(ki),kd_(kd) {}
double PIDController::calculate_control(double error,double dt) {
    if(!std::isfinite(error)||!std::isfinite(dt)||dt<=0) return 0;
    const double derivative=has_previous_?(error-previous_error_)/dt:0;
    const double candidate=std::clamp(integral_+error*dt,-50.,50.);
    const double raw=kp_*error+ki_*candidate+kd_*derivative;
    // Условное интегрирование предотвращает накопление при насыщении.
    if(std::abs(raw)<=18. || raw*error<0) integral_=candidate;
    previous_error_=error; has_previous_=true;
    return std::clamp(kp_*error+ki_*integral_+kd_*derivative,-18.,18.);
}
QString PIDController::controller_name() const { return "ПИД"; }
