#pragma once
#include <QMainWindow>
#include <memory>
#include <vector>
#include "autopilot.h"
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE
class QLabel; class QTimer; class QPushButton; class QDoubleSpinBox;
class QListWidget; class QTableWidget; class QLineSeries; class QValueAxis; class TrajectoryView;
class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent=nullptr);
    ~MainWindow() override;
private slots:
    void start();
    void pause();
    void reset();
    void on_telemetry_updated(double altitude,double error,double control);
    void update_chart(double altitude,double error,double control);
    void update_scene(double altitude,double error,double control);
    void on_fault_detected(const QString& reason);
    void on_navigation_changed(const QString& name);
    void on_target_changed(double target);
private:
    void build_dashboard();
    void wire_signals();
    void log_event(const QString& message);
    void set_status(const QString& message,const QString& tone);
    void set_navigation_failure(SimulatedNavigation* source,QPushButton* button,const QString& name);
    std::unique_ptr<Ui::MainWindow> ui_;
    Quadcopter craft_;
    Hexacopter spareHexa_;
    Octocopter spareOcto_;
    std::vector<UAV*> fleet_;
    Gyroscope gyro_;
    Accelerometer accelerometer_;
    Barometer barometer_;
    GPSNavigation gps_;
    InertialNavigation ins_;
    VisualNavigation visual_;
    PIDController controller_;
    NavigationManager* navigation_;
    Autopilot* autopilot_;
    QTimer* cycle_timer_;
    QLabel *altitudeLabel_,*errorLabel_,*outputLabel_,*statusLabel_,*navigationLabel_,*timeLabel_,*targetLabel_;
    QDoubleSpinBox* targetSpin_;
    QPushButton *startButton_,*pauseButton_,*sensorButton_,*gpsButton_,*restoreGpsButton_,*insButton_,*visualButton_;
    QTableWidget* table_;
    QListWidget* journal_;
    QLineSeries *altitudeSeries_,*errorSeries_;
    QValueAxis *timeAxis_,*heightAxis_;
    TrajectoryView* trajectory_;
    bool fault_latched_=false;
    bool first_fault_shown_=false;
};
