#include "qtmonkey_app_api.hpp"

#include <cassert>

#include "common.hpp"
#include "json11.hpp"

namespace qt_monkey_app
{

using json11::Json;

namespace
{
struct QStringJsonTrait final {
    explicit QStringJsonTrait(const QString &s) : str_(s) {}
    std::string to_json() const { return str_.toStdString(); }
private:
    const QString &str_;
};
}

QByteArray userAppEventToFromMonkeyAppPacket(const QString &scriptLines)
{
    auto json = Json::object{
        {"event", Json::object{{"script", QStringJsonTrait{scriptLines}}}}};
    // TODO: remove unnecessary allocation
    const std::string res = Json{json}.dump();
    return QByteArray{res.c_str()};
}

QByteArray userAppErrorsToFromMonkeyAppPacket(const QString &errMsg)
{
   auto json = Json::object{
        {"app errors", QStringJsonTrait{errMsg}}};
    // TODO: remove unnecessary allocation
    const std::string res = Json{json}.dump();
    return QByteArray{res.c_str()};
}

void parseOutputFromMonkeyApp(
    const QByteArray &data, size_t &stopPos,
    const std::function<void(QString)> &onNewUserAppEvent,
    const std::function<void(QString)> &onUserAppError,
    const std::function<void(QString)> &onParseError)
{
    assert(data.size() >= 0);
    stopPos = 0;
    std::string::size_type parserStopPos;
    std::string err;
    // TODO: remove unnecessary allocation
    auto jsonArr = Json::parse_multi(
        std::string{data.data(), static_cast<size_t>(data.size())},
        parserStopPos, err);
    stopPos = parserStopPos;
    for (const Json &elm : jsonArr) {
        if (elm.is_null())
            continue;
        if (elm.is_object() && elm.object_items().size() == 1u
            && elm.object_items().begin()->first == "event") {
            const Json &eventJson = elm.object_items().begin()->second;
            if (!eventJson.is_object() || eventJson.object_items().size() != 1u
                || eventJson.object_items().begin()->first != "script"
                || !eventJson.object_items().begin()->second.is_string()) {
                onParseError(QStringLiteral("event"));
                return;
            }
            onNewUserAppEvent(QString::fromUtf8(eventJson.object_items()
                                                    .begin()
                                                    ->second.string_value()
                                                    .c_str()));
        } else if (elm.is_object() && elm.object_items().size() == 1u && 
                   elm.object_items().begin()->first == "app errors") {
            auto it = elm.object_items().begin();
            if (!it->second.is_string()) {
                onParseError(QStringLiteral("app errors"));
                return;
            }
            onUserAppError(QString::fromUtf8(it->second.string_value().c_str()));
        }
    }
}
}