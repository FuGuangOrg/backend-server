#pragma once

#include <QString>
#include <QVariant>
#include <QObject>
#include "event_bus_global.h"

class EVENT_BUS_EXPORT interface_event_bus
{
public:
    interface_event_bus() = default;
    virtual ~interface_event_bus() = default;

    virtual void publish(const QString& event, const QVariant& payload) = 0;
    virtual void subscribe(const QString& event, QObject* receiver, const char* slot) = 0;
    virtual void unsubscribe(const QString& event, QObject* receiver, const char* slot) = 0;
};