#include "autopilot.h"
#include <cmath>
Autopilot::Autopilot(UAV* vehicle,const std::vector<Sensor*>& sensors,NavigationManager* navigation,
                     FlightController* controller,QObject* parent)
    :QObject(parent),vehicle_(vehicle),sensors_(sensors),navigation_(navigation),controller_(controller) {}
bool Autopilot::set_target_altitude(double value) {
    if(!std::isfinite(value)||value<50||value>500) return false;
    if(target_altitude_!=value) { target_altitude_=value; emit target_changed(value); }
    return true;
}
double Autopilot::target_altitude() const { return target_altitude_; }
double Autopilot::altitude() const { return vehicle_?vehicle_->altitude():0; }
double Autopilot::error() const { return target_altitude_-altitude(); }
double Autopilot::control() const { return control_; }
double Autopilot::elapsed() const { return elapsed_; }
void Autopilot::run_cycle() {
    if(!vehicle_) { emit fault_detected("Аппарат не задан"); return; }
    if(sensors_.empty()) { emit fault_detected("Сенсоры не заданы"); return; }
    for(auto* sensor:sensors_) {
        if(!sensor || !sensor->is_ready() || !std::isfinite(sensor->read())) {
            emit fault_detected("Отказ сенсора: "+(sensor?sensor->name():"неизвестен")); return;
        }
    }
    if(!navigation_||!navigation_->check_priority()||!navigation_->is_valid()) {
        emit fault_detected("Нет доступной навигации"); return;
    }
    const Position position=navigation_->get_position();
    if(!std::isfinite(position.altitude)||!std::isfinite(position.latitude)||!std::isfinite(position.longitude)
       ||std::abs(position.latitude)>90||std::abs(position.longitude)>180||position.altitude<0) {
        emit fault_detected("Некорректные координаты навигации"); return;
    }
    if(!controller_) { emit fault_detected("Контроллер не задан"); return; }
    constexpr double dt=0.1;
    control_=controller_->calculate_control(target_altitude_-position.altitude,dt);
    if(!std::isfinite(control_)) { emit fault_detected("Некорректный выход контроллера"); return; }
    vehicle_->apply_control(control_,dt);
    elapsed_+=dt;
    emit telemetry_updated(altitude(),error(),control_);
}
