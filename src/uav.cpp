#include "uav.h"
#include <algorithm>
#include <cmath>
void UAV::takeoff() { airborne_ = true; }
void UAV::land() { airborne_ = false; vertical_speed_ = 0; }
void UAV::apply_control(double commandedSpeed, double dt) {
    if (!airborne_ || !std::isfinite(commandedSpeed) || !std::isfinite(dt) || dt<=0) return;
    // Учебная вертикальная динамика с инерцией привода 0.5 с.
    vertical_speed_ += (std::clamp(commandedSpeed,-18.,18.)-vertical_speed_)*std::min(dt/0.5,1.0);
    altitude_ = std::max(0.0,altitude_+vertical_speed_*dt);
    battery_ = std::max(0.0,battery_-0.002*dt);
}
double UAV::altitude() const { return altitude_; }
double UAV::vertical_speed() const { return vertical_speed_; }
double UAV::battery() const { return battery_; }
QString Quadcopter::type() const { return "Квадрокоптер"; }
int Quadcopter::motor_count() const { return 4; }
QString Hexacopter::type() const { return "Гексакоптер"; }
int Hexacopter::motor_count() const { return 6; }
QString Octocopter::type() const { return "Октокоптер"; }
int Octocopter::motor_count() const { return 8; }
