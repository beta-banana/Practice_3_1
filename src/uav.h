#pragma once
#include <QString>

// Общий контракт аппаратов сохранён из предыдущей работы.
class UAV {
public:
    virtual ~UAV() = default;
    virtual QString type() const = 0;
    virtual int motor_count() const = 0;
    virtual void takeoff();
    virtual void land();
    void apply_control(double commandedSpeed, double dt);
    double altitude() const;
    double vertical_speed() const;
    double battery() const;
private:
    double altitude_ = 150.0;
    double vertical_speed_ = 0.0;
    double battery_ = 86.0;
    bool airborne_ = true;
};
class Quadcopter final : public UAV {
public: QString type() const override; int motor_count() const override;
};
class Hexacopter final : public UAV {
public: QString type() const override; int motor_count() const override;
};
class Octocopter final : public UAV {
public: QString type() const override; int motor_count() const override;
};
