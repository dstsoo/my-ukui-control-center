/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2019 Tianjin KYLIN Information Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#include "keyboardcontrol.h"
#include "ui_keyboardcontrol.h"

#include <QGSettings>

#include <QDebug>
#include <QMouseEvent>

#define KEYBOARD_SCHEMA "org.ukui.peripherals-keyboard"
#define REPEAT_KEY "repeat"
#define DELAY_KEY "delay"
#define RATE_KEY "rate"

#define KBD_LAYOUTS_SCHEMA "org.mate.peripherals-keyboard-xkb.kbd"
#define KBD_LAYOUTS_KEY "layouts"

#define CC_KEYBOARD_OSD_SCHEMA "org.ukui.control-center.osd"
#define CC_KEYBOARD_OSD_KEY "show-lock-tip"

KeyboardControl::KeyboardControl() : mFirstLoad(true)
{
    pluginName = tr("Keyboard");
    pluginType = DEVICES;
}

KeyboardControl::~KeyboardControl()
{
    if (!mFirstLoad) {
        delete ui;
        if (settingsCreate) {
            delete kbdsettings;
            delete settings;
        }
    }
}

QString KeyboardControl::get_plugin_name() {
    return pluginName;
}

int KeyboardControl::get_plugin_type() {
    return pluginType;
}

QWidget *KeyboardControl::get_plugin_ui() {
    if (mFirstLoad) {
        ui = new Ui::KeyboardControl;
        pluginWidget = new QWidget;
        pluginWidget->setAttribute(Qt::WA_DeleteOnClose);
        ui->setupUi(pluginWidget);

        mFirstLoad = false;
        settingsCreate = false;

        setupStylesheet();
        setupComponent();

        // ???????????????????????????GSettings
        const QByteArray id(KEYBOARD_SCHEMA);
        // ?????????????????????GSettings
        const QByteArray idd(KBD_LAYOUTS_SCHEMA);
        // ?????????????????????GSettings
        const QByteArray iid(CC_KEYBOARD_OSD_SCHEMA);
        // ??????????????????GSettings???????????????????????????
        osdSettings = new QGSettings(iid);

        if (QGSettings::isSchemaInstalled(id) && QGSettings::isSchemaInstalled(idd)){
            settingsCreate = true;

            kbdsettings = new QGSettings(idd);
            settings = new QGSettings(id);

            //???????????????????????????
            layoutmanagerObj = new KbdLayoutManager();

            setupConnect();
            initGeneralStatus();

            rebuildLayoutsComBox();
        }

    }
    return pluginWidget;
}

void KeyboardControl::plugin_delay_control() {

}

const QString KeyboardControl::name() const {

   return QStringLiteral("keyboard");
}

void KeyboardControl::setupStylesheet(){

    //~ contents_path /keyboard/Enable repeat key
    ui->enableLabel->setText(tr("Enable repeat key"));
    //~ contents_path /keyboard/Delay
    ui->delayLabel->setText(tr("Delay"));
    //~ contents_path /keyboard/Speed
    ui->speedLabel->setText(tr("Speed"));
    //~ contents_path /keyboard/Input characters to test the repetition effect:
    ui->repeatLabel->setText(tr("Input characters to test the repetition effect:"));
    //~ contents_path /keyboard/Tip of keyboard
    ui->tipLabel->setText(tr("Tip of keyboard"));
    //~ contents_path /keyboard/Keyboard layout
    ui->layoutLabel->setText(tr("Keyboard layout"));
    //~ contents_path /keyboard/reset default layout
//    ui->resetLabel->setText(tr("reset default layout"));

    ui->titleLabel->setStyleSheet("QLabel{font-size: 18px; color: palette(windowText);}");
    ui->title2Label->setStyleSheet("QLabel{font-size: 18px; color: palette(windowText);}");
}

void KeyboardControl::setupComponent(){

    addWgt = new HoverWidget("");
    addWgt->setObjectName("addwgt");
    addWgt->setMinimumSize(QSize(580, 50));
    addWgt->setMaximumSize(QSize(960, 50));
    addWgt->setStyleSheet("HoverWidget#addwgt{background: palette(button); border-radius: 4px;}HoverWidget:hover:!pressed#addwgt{background: #3D6BE5; border-radius: 4px;}");

    QHBoxLayout *addLyt = new QHBoxLayout;

    QLabel * iconLabel = new QLabel();
    QLabel * textLabel = new QLabel(tr("Install layouts"));
    QPixmap pixgray = ImageUtil::loadSvg(":/img/titlebar/add.svg", "black", 12);
    iconLabel->setPixmap(pixgray);
    addLyt->addWidget(iconLabel);
    addLyt->addWidget(textLabel);
    addLyt->addStretch();
    addWgt->setLayout(addLyt);

    // ????????????Widget??????
    connect(addWgt, &HoverWidget::enterWidget, this, [=](QString mname) {
        Q_UNUSED(mname);
        QPixmap pixgray = ImageUtil::loadSvg(":/img/titlebar/add.svg", "white", 12);
        iconLabel->setPixmap(pixgray);
        textLabel->setStyleSheet("color: palette(base);");
    });

    // ????????????
    connect(addWgt, &HoverWidget::leaveWidget, this, [=](QString mname) {
        Q_UNUSED(mname);
        QPixmap pixgray = ImageUtil::loadSvg(":/img/titlebar/add.svg", "black", 12);
        iconLabel->setPixmap(pixgray);
        textLabel->setStyleSheet("color: palette(windowText);");
    });

    ui->addLyt->addWidget(addWgt);

    // ?????????????????????
    ui->repeatFrame_5->hide();

    // ????????????????????????
    keySwitchBtn = new SwitchButton(pluginWidget);
    ui->enableHorLayout->addWidget(keySwitchBtn);

    // ????????????????????????
    tipKeyboardSwitchBtn = new SwitchButton(pluginWidget);
    ui->tipKeyboardHorLayout->addWidget(tipKeyboardSwitchBtn);

    // ?????????????????????
    numLockSwitchBtn = new SwitchButton(pluginWidget);
    ui->numLockHorLayout->addWidget(numLockSwitchBtn);
}

void KeyboardControl::setupConnect(){
    connect(keySwitchBtn, &SwitchButton::checkedChanged, this, [=](bool checked) {
        setKeyboardVisible(checked);
        settings->set(REPEAT_KEY, checked);
    });

    connect(ui->delayHorSlider, &QSlider::valueChanged, this, [=](int value) {
        settings->set(DELAY_KEY, value);
    });

    connect(ui->speedHorSlider, &QSlider::valueChanged, this, [=](int value) {
        settings->set(RATE_KEY, value);
    });

    connect(settings,&QGSettings::changed,this,[=](const QString &key) {
       if(key == "rate") {
           ui->speedHorSlider->setValue(settings->get(RATE_KEY).toInt());
       } else if(key == "repeat") {
           keySwitchBtn->setChecked(settings->get(REPEAT_KEY).toBool());
           setKeyboardVisible(keySwitchBtn->isChecked());
       } else if(key == "delay") {
           ui->delayHorSlider->setValue(settings->get(DELAY_KEY).toInt());
       }
    });

    connect(osdSettings,&QGSettings::changed,this,[=](const QString &key) {
       if(key == "showLockTip") {
           tipKeyboardSwitchBtn->blockSignals(true);
           tipKeyboardSwitchBtn->setChecked(osdSettings->get(CC_KEYBOARD_OSD_KEY).toBool());
           tipKeyboardSwitchBtn->blockSignals(false);
       }
    });

    connect(addWgt, &HoverWidget::widgetClicked, this, [=](QString mname) {
        Q_UNUSED(mname);
        KbdLayoutManager * templayoutManager = new KbdLayoutManager;
        templayoutManager->exec();
    });

    connect(ui->resetBtn, &QPushButton::clicked, this, [=] {
        kbdsettings->reset(KBD_LAYOUTS_KEY);
        if ("zh_CN" == QLocale::system().name()) {
            kbdsettings->set(KBD_LAYOUTS_KEY, "cn");
        } else {
            kbdsettings->set(KBD_LAYOUTS_KEY, "us");
        }
    });

    connect(kbdsettings, &QGSettings::changed, [=](QString key) {
        if (key == KBD_LAYOUTS_KEY)
            rebuildLayoutsComBox();
    });

    connect(tipKeyboardSwitchBtn, &SwitchButton::checkedChanged, this, [=](bool checked) {
        osdSettings->set(CC_KEYBOARD_OSD_KEY, checked);
    });

#if QT_VERSION <= QT_VERSION_CHECK(5, 12, 0)
    connect(ui->layoutsComBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [=](int index){
#else
    connect(ui->layoutsComBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
#endif
        QStringList layoutsList;
        layoutsList.append(ui->layoutsComBox->currentData(Qt::UserRole).toString());
        for (int i = 0; i < ui->layoutsComBox->count(); i++){
            QString id = ui->layoutsComBox->itemData(i, Qt::UserRole).toString();
            if (i != index) //????????????item
                layoutsList.append(id);
        }
        kbdsettings->set(KBD_LAYOUTS_KEY, layoutsList);
    });
}

void KeyboardControl::initGeneralStatus() {
    //????????????????????????
    keySwitchBtn->setChecked(settings->get(REPEAT_KEY).toBool());
    setKeyboardVisible(keySwitchBtn->isChecked());

    //???????????????????????????
    ui->delayHorSlider->setValue(settings->get(DELAY_KEY).toInt());

    //???????????????????????????
    ui->speedHorSlider->setValue(settings->get(RATE_KEY).toInt());

    tipKeyboardSwitchBtn->blockSignals(true);
    tipKeyboardSwitchBtn->setChecked(osdSettings->get(CC_KEYBOARD_OSD_KEY).toBool());
    tipKeyboardSwitchBtn->blockSignals(false);

}

void KeyboardControl::rebuildLayoutsComBox() {
    QStringList layouts = kbdsettings->get(KBD_LAYOUTS_KEY).toStringList();
    ui->layoutsComBox->blockSignals(true);
    //??????????????????????????????
    ui->layoutsComBox->clear();

    //??????????????????????????????
    for (QString layout : layouts) {
        ui->layoutsComBox->addItem(layoutmanagerObj->kbd_get_description_by_id(const_cast<const char *>(layout.toLatin1().data())), layout);
    }
    ui->layoutsComBox->blockSignals(false);
    if (0 == ui->layoutsComBox->count()) {
        ui->layoutsComBox->setVisible(false);
    } else {
        ui->layoutsComBox->setVisible(true);
    }
}

void KeyboardControl::setKeyboardVisible(bool checked) {
    ui->repeatFrame_1->setVisible(checked);
    ui->repeatFrame_2->setVisible(checked);
    ui->repeatFrame_3->setVisible(checked);
}
