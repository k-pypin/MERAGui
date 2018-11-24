#include "dialog.h"
#include "ui_dialog.h"
#include <QMessageBox>
#include "Device/include/switchdevice.h"
#include <lib/qtmaterialtheme.h>
#include <QLabel>
#include <qtmaterialdialog.h>
Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    //setAttribute(Qt::WA_DeleteOnClose);
    this->setSizeGripEnabled(true);
    this->setWindowFlag(Qt::FramelessWindowHint);
    QLabel *label = new QLabel("Sphinx", this);
    label->setAttribute(Qt::WA_TranslucentBackground);
    label->setForegroundRole(QPalette::Foreground);
    label->setContentsMargins(6, 0, 0, 0);

    QPalette palette = label->palette();
    palette.setColor(label->foregroundRole(), Qt::white);
    label->setPalette(palette);

    label->setFont(QFont("Roboto", 18, QFont::Normal));

    m_recognizeBtn = new QtMaterialIconButton(QtMaterialTheme::icon("av", "mic_off"),this);
    m_recognizeBtn->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
    m_recognizeBtn->setIconSize(QSize(24, 24));
    m_recognizeBtn->setColor(Qt::white);
    m_recognizeBtn->setFixedWidth(42);
    connect(m_recognizeBtn, SIGNAL(clicked()), this, SLOT(startRecognizeClicked()));

    m_settingsBtn = new QtMaterialIconButton(QtMaterialTheme::icon("action", "settings"));
    m_settingsBtn->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
    m_settingsBtn->setIconSize(QSize(24, 24));
    m_settingsBtn->setColor(Qt::white);
    m_settingsBtn->setFixedWidth(42);
    connect(m_settingsBtn, SIGNAL(clicked()), this, SLOT(recognitionSettingsClicked()));

    m_appBar = new QtMaterialAppBar(this);
    m_appBar->setFixedHeight(65);
    m_appBar->appBarLayout()->addWidget(m_recognizeBtn);
    m_appBar->appBarLayout()->addWidget(label, 1);
    m_appBar->appBarLayout()->addWidget(m_settingsBtn);
    m_appBar->appBarLayout()->setAlignment(m_settingsBtn, Qt::AlignRight);
    m_appBar->setCursor(QCursor(Qt::CursorShape::OpenHandCursor));
    m_appBar->installEventFilter(this);
    ui->verticalLayout->insertWidget(0, m_appBar);

    QHBoxLayout* toolBtnsHLayout = new QHBoxLayout(this);
    QToolButton * closeBtn = new QToolButton(this);
    closeBtn->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
    closeBtn->setIcon(QIcon(":/images/assets/cross.png"));
    closeBtn->resize(16, 16);
    closeBtn->setIconSize(QSize(16,16));
    closeBtn->setStyleSheet("QToolButton {\
                                border: 1px solid #ABABAB;\
                                background-color: #EE7B42;\
                            }\
                            QToolButton:hover {\
                                background-color: #FFA642;\
                            }");
    QToolButton * minimizeBtn = new QToolButton(this);
    minimizeBtn->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
    minimizeBtn->setIcon(QIcon(":/images/assets/underline.png"));
    minimizeBtn->resize(16, 16);
    minimizeBtn->setIconSize(QSize(16,16));
    minimizeBtn->setStyleSheet("QToolButton {\
                                border: 1px solid #ABABAB;\
                                background-color: #00BCD4;\
                            }\
                            QToolButton:hover {\
                                background-color: #88CFDF;\
                            }");
    toolBtnsHLayout->addWidget(minimizeBtn, 0, Qt::AlignRight);
    toolBtnsHLayout->addWidget(closeBtn);
    connect(closeBtn, SIGNAL(clicked()), this, SLOT(closeBtnClicked()));
    connect(minimizeBtn, SIGNAL(clicked()), this, SLOT(minimizeBtnClicked()));
    ui->verticalLayout->insertLayout(0, toolBtnsHLayout);

    addButton = new QToolButton(this);
    addButton->setText("+");
    addButton->setToolTip("Добавить устройство в группу");
    addButton->setIcon(QIcon(":/images/assets/plus.png"));
    addButton->setStyleSheet("QToolButton {\
                             border: 1px solid #ABABAB;\
                             background-color: white;\
                             }\
                             QToolButton:hover {\
                                 background-color: #E1E1E1;\
                             }");
    addButton->resize(20,20);
    addButton->setIconSize(QSize(20,20));
    addButton->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));
    ui->tabDevices->setCornerWidget(addButton, Qt::Corner::BottomRightCorner);

    addDlg = new AddDeviceDialog(this, _sdevicesFilename);
    cdlg = new ConnectionDialog(this, _sfilename);
    cdlg->setModal(true);
    cdlg->show();
    connect(cdlg, &ConnectionDialog::accepted, this, &Dialog::connectionDialogAccepted);
    connect(cdlg, &ConnectionDialog::rejected, this, &Dialog::connectionDialogRejected);
    connect(addButton, &QToolButton::clicked, this, &Dialog::addButtonClicked);
}

Dialog::~Dialog()
{
    delete ui;
    delete _recognizer;
    delete cdlg;
    delete _mclient;


    if(_recognizeThread) {
        _recognizeThread->exit();
        _recognizeThread->deleteLater();
    }
//    for(size_t i = 0; i < vecSwitch.size(); ++i)
//        delete vecSwitch[i];
}

void Dialog::onRecognize(string command) {
    QString text = QString::fromStdString(command);
    qDebug() << text;
    vector<QString> vecVar{"name", "action", "obj", "pretext", "where"};
    std::map<QString,QString> Gram;
    QStringList splittedText = text.split(' ');
    for(size_t i = 0; i < splittedText.size(); ++i)
        Gram[vecVar[i]] = splittedText[i];
    executeCommand(Gram);
}

void Dialog::executeCommand(std::map<QString,QString>& gram) {
    QString path = "";
    QString state = "null";
    vector<QString> OffState{"выключи", "отключи", "погаси"};
    if(std::find(OffState.begin(),OffState.end(), gram.at("action")) != OffState.end())
        state = "OFF";
    else if(gram.at("action") == "включи")
        state = "ON";
    if(gram.at("where") == "кухне")
        path = _cData.Username + "/feeds/kitchen.lamp-on-the-kitchen-1";
    else if(gram.at("where") == "зале")
        path = _cData.Username + "/feeds/zal.lampa-v-zalie";
    if(path != "")
        _mclient->publish(path, state.toUtf8());
}

void Dialog::brokerDisconnected() {
    QMessageBox::warning(this, QLatin1String("Error"), QLatin1String("Client disconnect from broker."));
}

void Dialog::updateLogStateChange() {
    qDebug() << "State changed to " << _mclient->state();
    if(_mclient->state() == QMqttClient::Connected) {


        auto subscription = _mclient->subscribe(QString(_cData.Username + "/feeds/+"));
        if (!subscription) {
                QMessageBox::critical(this, QLatin1String("Error"), QLatin1String("Could not subscribe. Is there a valid connection?"));
                return;
            }
    }
}

void Dialog::replyFinished(QNetworkReply* reply) {
    QJsonDocument jDocument = QJsonDocument::fromJson(reply->readAll());
    QJsonArray jA = jDocument.array();
    for(int i = 0; i < jA.size(); ++i) {
        QJsonObject jO = jA.at(i).toObject();
        QString group = jO.value("group").toObject().value("name").toString();
        //devices.push_back(new SwitchDevice(jO.value("name").toString(), jO.value("last_value").toString(), group, jO.value("key").toString()));
        availableDevicesByGroup[group].emplace_back(jO.value("name").toString(), jO.value("key").toString());
    }

    for(auto& x : availableDevicesByGroup) {
        ui->tabDevices->addTab(new QWidget(), x.first);
    }
    reply->deleteLater();
    //devices.shrink_to_fit();
    generateControls();
}

void Dialog::connectionDialogAccepted() {
    this->_cData = cdlg->getData();
    getAllFeeds();
    initMQTTClient();
}

void Dialog::connectionDialogRejected() {
    if(QMessageBox::warning(this, "Внимание", "Закрытие данного диалогового окна приведет к прекращению работы программы!\n Вы уверены, что хотите выйти?",QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No) {
        cdlg->show();
    } else {
        this->close();
    }

}

void Dialog::initMQTTClient() {
    _mclient = new QMqttClient();
    _mclient->setHostname(_cData.Host);
    _mclient->setPort(_cData.Port.toInt());
    _mclient->setUsername(_cData.Username);
    _mclient->setPassword(_cData.Password); //"36d27d262cda42cf8e357bb722795f72"
    connect(_mclient, &QMqttClient::stateChanged, this, &Dialog::updateLogStateChange);
    connect(_mclient, &QMqttClient::disconnected, this, &Dialog::brokerDisconnected);
    connect(_mclient, &QMqttClient::messageReceived, this, [this](const QByteArray &message, const QMqttTopicName &topic) {
            const QString content = QDateTime::currentDateTime().toString()
                        + QLatin1String(" Received Topic: ")
                        + topic.name()
                        + QLatin1String(" Message: ")
                        + message
                        + QLatin1Char('\n');
//            Switch* tSwitch = nullptr;
//            tSwitch = find_switch(topic.name());
//            if(tSwitch){
//                tSwitch->blockSignals(true);
//                int checked = Qt::CheckState::Unchecked;
//                if(message == "ON")
//                    checked = Qt::CheckState::Checked;
//                else if(message == "OFF")
//                    checked = Qt::CheckState::Unchecked;
//                if(tSwitch->checkState() != checked)
//                    tSwitch->setChecked(checked);
//                tSwitch->blockSignals(false);
//            }
        });
    _mclient->connectToHost();
}

void Dialog::getAllFeeds() {
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(replyFinished(QNetworkReply*)));
    manager->get(QNetworkRequest(QUrl("http://" + _cData.Host  + "/api/v2/" + _cData.Username + "/feeds?X-AIO-Key=" + _cData.Password)));
}

void Dialog::generateControls() {
//    vecSwitch.resize(devices.size());
//    for(size_t i = 0; i < devices.size(); ++i) {
//        vecSwitch[i] = new Switch(devices[i]->getName());
//        vecSwitch[i]->setLayoutDirection(Qt::RightToLeft);
//        vecSwitch[i]->setAccessibleDescription(_cData.Username + "/feeds/" + devices[i]->getFeed());
//        ui->verticalLayout->addWidget(vecSwitch[i]);
//        connect(vecSwitch[i], &Switch::toggled, this, [i, this](bool checked) -> void {
//                    devices[i]->setValue(checked ? "ON" : "OFF");
//                    _mclient->publish(QString(_cData.Username + "/feeds/" + devices[i]->getFeed()), devices[i]->getValue().toUtf8());
//                });
//    }
}

//Switch * Dialog::find_switch(QString topic) {
//    auto sw = std::find_if(vecSwitch.begin(),vecSwitch.end(), [&topic](Switch* s)->bool {
//        return s->accessibleDescription() == topic;
//    });
//    if(sw != vecSwitch.end())
//        return *sw;
//    else
//        return nullptr;
//}

void Dialog::recognitionSettingsClicked()
{
    rsDlg = new RecognizerSettingsDialog(this, _sfilename);
    rsDlg->show();
}

void Dialog::startRecognizeClicked()
{
    if(this->m_recognizeBtnChecked) {
        this->m_recognizeBtnChecked = false;
        this->m_recognizeBtn->setIcon(QtMaterialTheme::icon("av","mic"));
        QSettings sett(_sfilename, QSettings::IniFormat);
        QString model = sett.value("SPHINX/MODEL").toString();
        QString mdef = sett.value("SPHINX/MDEF").toString();
        QString dict = sett.value("SPHINX/DICTIONARY").toString();
        QString gramm = sett.value("SPHINX/GRAMMAR").toString();
        QString aDev = sett.value("SPHINX/ADEVICE").toString();
        if(model.isEmpty() || mdef.isEmpty() || dict.isEmpty() || gramm.isEmpty() || aDev.isEmpty())
        {         
            this->m_recognizeBtn->setIcon(QtMaterialTheme::icon("av","mic_off"));
            QMessageBox::critical(this, "Ошибка", "Невозможно начать распознавание!\nЗаданы не все параметры распознавания.");
            if(rsDlg) {
                rsDlg->show();
                return;
            } else {
                rsDlg = new RecognizerSettingsDialog(this, _sfilename);
                rsDlg->show();
                return;
            }
        }
        if(_recognizeThread) {
            _recognizeThread->quit();
            _recognizeThread->wait();
            delete _recognizeThread;
        }

        if(_recognizer)
            _recognizer->deleteLater();

        this->_recognizer = new SphinxRecognizer(model.toStdString(), dict.toStdString(), mdef.toStdString(), gramm.toStdString());
        _recognizeThread = new QThread;
        this->_recognizer->moveToThread(_recognizeThread);
        connect(this, SIGNAL(startRecognition(string)), this->_recognizer, SLOT(startRecognition(string)));
        connect(this->_recognizer, SIGNAL(recognized(string)), this, SLOT(onRecognize(string)));
        _recognizeThread->start();
        emit this->startRecognition(aDev.toStdString());
    } else {
        this->m_recognizeBtnChecked = true;
        this->m_recognizeBtn->setIcon(QtMaterialTheme::icon("av","mic_off"));
        if(this->_recognizer)
            this->_recognizer->stopRecognition();
    }
}

void Dialog::addButtonClicked() {
    QString tabText = ui->tabDevices->tabText(ui->tabDevices->currentIndex());
    addDlg->selectDevice(this->availableDevicesByGroup[tabText]);
}

void Dialog::closeBtnClicked() {
    this->close();
}

void Dialog::minimizeBtnClicked() {
    this->setWindowState(Qt::WindowState::WindowMinimized);
}

bool Dialog::eventFilter(QObject* object, QEvent* event) {
    if(object == m_appBar) {        
        QtMaterialAppBar* bar = static_cast<QtMaterialAppBar*>(object);
        if(event->type() == QEvent::MouseButtonPress){
            QMouseEvent* m_event = static_cast<QMouseEvent*>(event);
            if(m_event->button() == Qt::LeftButton) {
                bar->setCursor(QCursor(Qt::CursorShape::ClosedHandCursor));
                mpos = m_event->pos();
                return true;
            }
        } else if(event->type() == QEvent::MouseButtonRelease) {
            bar->setCursor(QCursor(Qt::CursorShape::OpenHandCursor));
            mpos = QPoint(-1,-1);
            return true;
        } else if(event->type() == QEvent::MouseMove) {
            QMouseEvent* m_event = static_cast<QMouseEvent*>(event);
            if(mpos.x() >= 0 && m_event->buttons() && Qt::LeftButton) {
                QPoint diff = m_event->pos() - mpos;
                QPoint newpos = this->pos() + diff;
                this->move(newpos);
            }
            return true;
        } else {
            return false;
        }
    }
    return false;
}
