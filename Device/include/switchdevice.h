#ifndef SWITCHDEVICE_H
#define SWITCHDEVICE_H
#include "adevice.h"
#include <qtmaterialtoggle.h>
#include <QLabel>
#include <QtTextToSpeech>
class SwitchDevice : public ADevice {
private:
    QtMaterialToggle* toggleWidget;
    QLabel* label;
    QHBoxLayout* HLay = nullptr;
    QWidget* widget = nullptr;
    QFrame* hDivider = nullptr;
    bool needPublish = false;
public:
    SwitchDevice(QJsonObject obj);
    ~SwitchDevice() override {
        HLay->deleteLater();
        label->deleteLater();
        toggleWidget->deleteLater();
        widget->deleteLater();
        hDivider->deleteLater();
    }
    void setValue(QString new_value) override;
    void setValueFromBack(QString new_value) override;
    void checkTrigger(QString triggerPhrase) override;
    QString getType() override;
    void insertWidgetsIntoLayout(QLayout* layout) override;
};

#endif // SWITCHDEVICE_H
