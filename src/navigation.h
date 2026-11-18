#pragma once
#include <QObject>
#include <QString>
#include <vector>
class UAV;
struct Position { double latitude; double longitude; double altitude; };
class NavigationSystem {
public:
    virtual ~NavigationSystem() = default;
    virtual Position get_position() const = 0;
    virtual QString system_name() const = 0;
    virtual bool is_valid() const = 0;
};
// Все источники наблюдают один аппарат: смена источника не переносит БПЛА.
class SimulatedNavigation : public NavigationSystem {
public:
    explicit SimulatedNavigation(const UAV* craft);
    Position get_position() const override;
    bool is_valid() const override;
    void set_valid(bool valid);
private:
    const UAV* craft_;
    bool valid_ = true;
};
class GPSNavigation final : public SimulatedNavigation {
public: using SimulatedNavigation::SimulatedNavigation; QString system_name() const override;
};
class InertialNavigation final : public SimulatedNavigation {
public: using SimulatedNavigation::SimulatedNavigation; QString system_name() const override;
};
class VisualNavigation final : public SimulatedNavigation {
public: using SimulatedNavigation::SimulatedNavigation; QString system_name() const override;
};
class NavigationManager final : public QObject, public NavigationSystem {
    Q_OBJECT
public:
    explicit NavigationManager(std::vector<NavigationSystem*> sources, QObject* parent=nullptr);
    bool check_priority();
    Position get_position() const override;
    bool is_valid() const override;
    QString system_name() const override;
signals:
    void active_system_changed(const QString& name);
private:
    std::vector<NavigationSystem*> sources_;
    NavigationSystem* active_system_ = nullptr;
};
