#pragma once
#include <QString>
class Sensor {
public:
    virtual ~Sensor() = default;
    virtual QString name() const = 0;
    virtual double read() const = 0;
    bool is_ready() const;
    void set_ready(bool ready);
private:
    bool ready_ = true;
};
class Gyroscope final : public Sensor {
public: QString name() const override; double read() const override;
};
class Accelerometer final : public Sensor {
public: QString name() const override; double read() const override;
};
class Barometer final : public Sensor {
public: QString name() const override; double read() const override;
};
