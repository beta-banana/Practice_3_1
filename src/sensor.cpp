#include "sensor.h"
bool Sensor::is_ready() const { return ready_; }
void Sensor::set_ready(bool ready) { ready_ = ready; }
QString Gyroscope::name() const { return "Гироскоп"; }
double Gyroscope::read() const { return 1.2; }
QString Accelerometer::name() const { return "Акселерометр"; }
double Accelerometer::read() const { return 9.81; }
QString Barometer::name() const { return "Барометр"; }
double Barometer::read() const { return 1012.5; }
