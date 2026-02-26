/**************************************************
 * 事件总线接口，实现插件之间通信
 **************************************************/

#pragma once
#include <QObject>
#include <QMap>
#include <QList>
#include "interface_event_bus.h"
#include "event_register.h"

struct Subscriber
{
    QObject* receiver{ nullptr };
    QByteArray slot{""};
    bool operator==(const Subscriber& other) const
    {
        return receiver == other.receiver && slot == other.slot;
    }
};

class EVENT_BUS_EXPORT event_bus : public QObject, public interface_event_bus
{
    Q_OBJECT
public:
    event_bus(QObject* parent = nullptr);
    virtual ~event_bus() override;
    void publish(const QString& event, const QVariant& payload) override;
    void subscribe(const QString& event, QObject* receiver, const char* slot) override;
    void unsubscribe(const QString& event, QObject* receiver, const char* slot) override;

private:
    QMap<QString, QList<Subscriber>> m_subscribers;
};