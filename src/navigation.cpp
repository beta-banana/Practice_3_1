#include "navigation.h"
#include "uav.h"
#include <limits>
SimulatedNavigation::SimulatedNavigation(const UAV* craft):craft_(craft) {}
Position SimulatedNavigation::get_position() const {
    return {55.75,37.61,craft_?craft_->altitude():std::numeric_limits<double>::quiet_NaN()};
}
bool SimulatedNavigation::is_valid() const { return valid_ && craft_; }
void SimulatedNavigation::set_valid(bool valid) { valid_=valid; }
QString GPSNavigation::system_name() const { return "СНС"; }
QString InertialNavigation::system_name() const { return "ИНС"; }
QString VisualNavigation::system_name() const { return "Визуальная"; }
NavigationManager::NavigationManager(std::vector<NavigationSystem*> sources,QObject* parent)
    :QObject(parent),sources_(std::move(sources)) {}
bool NavigationManager::check_priority() {
    NavigationSystem* next=nullptr;
    for(auto* source:sources_) if(source && source->is_valid()) { next=source; break; }
    if(next!=active_system_) { active_system_=next; emit active_system_changed(system_name()); }
    return active_system_!=nullptr;
}
bool NavigationManager::is_valid() const { return active_system_ && active_system_->is_valid(); }
QString NavigationManager::system_name() const { return active_system_?active_system_->system_name():"Недоступна"; }
Position NavigationManager::get_position() const {
    const double nan=std::numeric_limits<double>::quiet_NaN();
    return is_valid()?active_system_->get_position():Position{nan,nan,nan};
}
