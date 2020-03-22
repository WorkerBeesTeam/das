#include <future>

#include <Helpz/db_base.h>
#include <Helpz/db_builder.h>

#include <Das/db/dig_status_type.h>
#include <Das/section.h>
#include <Das/commands.h>

#include <dbus/dbus_interface.h>

#include "elements.h"

namespace Das {
namespace Bot {

using namespace std;
using namespace Helpz::DB;

Elements::Elements(const Bot_Base &controller, uint32_t user_id, const Scheme_Item &scheme, const std::vector<std::string> &cmd, const std::string &msg_data) :
    Menu_Item(controller, user_id, scheme),
    msg_data_(msg_data),
    begin_(cmd.cbegin() + 3), cmd_(cmd)
{
}

void Elements::generate_answer()
{
    create_keyboard();
    back_data_ = msg_data_.substr(0, msg_data_.size() - cmd_.back().size() - 1);
    scheme_suffix_ = "WHERE scheme_id = " + QString::number(scheme_.parent_id_or_id());

    text_ = scheme_.title_.toStdString();

    if (cmd_.size() <= 3)
        fill_sections();
    else // if (cmd.size() > 3)
    {
        uint32_t sct_id = stoi(*begin_++);
        fill_section(sct_id);
    }

    keyboard_->inlineKeyboard.push_back(makeInlineButtonRow(back_data_, "Назад (<<)"));

    if (cmd_.size() <= 4)
        text_ += ':';
}

void Elements::process_user_data(const string &text)
{
    text_ = "Ещё не реализованно: " + msg_data_ + " = " + text;
}

void Elements::fill_sections()
{
    text_ += " - Секции";

    Base& db = Base::get_thread_local_instance();
    const QVector<Section> sct_vect = db_build_list<Section>(db, scheme_suffix_);
    for (const Section& sct: sct_vect)
    {
        const string data = msg_data_ + '.' + to_string(sct.id());
        keyboard_->inlineKeyboard.push_back(makeInlineButtonRow(data, sct.name().toStdString()));
    }
}

void Elements::fill_section(uint32_t sct_id)
{
    Base& db = Base::get_thread_local_instance();

    QString sct_sql = "SELECT name FROM das_section ";
    sct_sql += scheme_suffix_;
    sct_sql += " AND id = ";
    sct_sql += QString::number(sct_id);
    QSqlQuery q = db.exec(sct_sql);

    text_ += " - ";
    if (q.next())
        text_ += q.value(0).toString().toStdString();

    if (cmd_.size() == 4)
    {
        fill_groups(sct_id);
    }
    else
    {
        uint32_t dig_id = stoi(*begin_++);
        fill_group(sct_id, dig_id);
    }
}

void Elements::fill_groups(uint32_t sct_id)
{
    text_ += " - Группы";

    Scheme_Status scheme_status;

    std::future<void> scheme_status_task = std::async(std::launch::async, [this, &scheme_status]() -> void
    {
        QMetaObject::invokeMethod(dbus_iface_, "get_scheme_status", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(Scheme_Status, scheme_status),
            Q_ARG(uint32_t, scheme_.id()));
    });

    std::string btn_text;

    Base& db = Base::get_thread_local_instance();
    QSqlQuery q = db.exec(get_group_sql(sct_id));

    scheme_status_task.get();

    while (q.next())
    {
        const DB::Device_Item_Group group(q.value(0).toUInt(), q.value(1).toString());

        btn_text.clear();

        for (const DIG_Status& status: scheme_status.status_set_)
        {
            if (status.group_id() == group.id())
            {
                QSqlQuery status_q = db.exec("SELECT category_id FROM das_dig_status_type WHERE id = " + QString::number(status.status_id()));
                if (status_q.next())
                    btn_text = default_status_category_emoji(status_q.value(0).toUInt());
                break;
            }
        }

        if (!btn_text.empty())
            btn_text += ' ';
        btn_text += group.title().toStdString();

        const string data = msg_data_ + '.' + to_string(group.id());
        keyboard_->inlineKeyboard.push_back(makeInlineButtonRow(data, btn_text));
    }
}

void Elements::fill_group(uint32_t sct_id, uint32_t dig_id)
{
    if (cmd_.size() == 5)
        fill_group_info(sct_id, dig_id);
    else if (*begin_ == "mode")
        fill_group_mode(dig_id);
    else if (*begin_ == "param")
        fill_group_param(dig_id);
}

void Elements::fill_group_info(uint32_t sct_id, uint32_t dig_id)
{
    Scheme_Status scheme_status;

    std::future<void> scheme_status_task = std::async(std::launch::async, [this, &scheme_status]() -> void
    {
        QMetaObject::invokeMethod(dbus_iface_, "get_scheme_status", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(Scheme_Status, scheme_status),
            Q_ARG(uint32_t, scheme_.id()));
    });

    Base& db = Base::get_thread_local_instance();
    QSqlQuery q = db.exec(get_group_sql(sct_id) + " AND dig.id = " + QString::number(dig_id));
    text_ += " - ";
    if (q.next())
        text_ += q.value(1).toString().toStdString();
    text_ += ':';

    const QString dev_item_sql =
            "SELECT IF(di.name IS NULL OR di.name = '', dit.title, di.name) as title, dv.value, s.name FROM das_device_item di "
            "LEFT JOIN das_device_item_type dit ON dit.id = di.type_id "
            "LEFT JOIN das_device_item_value dv ON dv.item_id = di.id AND dv.scheme_id = "
            + QString::number(scheme_.id()) +
            " LEFT JOIN das_sign_type s ON s.id = dit.sign_id "
            "WHERE di.scheme_id = "
            + QString::number(scheme_.parent_id_or_id()) +
            " AND di.group_id = "
            + QString::number(dig_id) +
            " ORDER BY di.device_id ASC";
    q = db.exec(dev_item_sql);

    QString dev_item_text;
    while(q.next())
    {
        dev_item_text += "\n  - ";
        dev_item_text += q.value(0).toString();
        dev_item_text += ": ";
        if (q.value(1).isNull())
            dev_item_text += "Не подключен";
        else
        {
            dev_item_text += q.value(1).toString();
            if (!q.value(2).isNull())
                dev_item_text += q.value(2).toString();
        }
    }

    std::string mode_btn_text = "Режим";

    const QString mode_sql = "WHERE scheme_id = " + QString::number(scheme_.id()) + " AND group_id = " + QString::number(dig_id);
    const QVector<DB::DIG_Mode> dig_mode_vect = db_build_list<DB::DIG_Mode>(db, mode_sql);
    if (!dig_mode_vect.empty())
    {
        const DB::DIG_Mode& mode = dig_mode_vect.front();

        const QVector<DB::DIG_Mode_Type> dig_mode_type_vect = db_build_list<DB::DIG_Mode_Type>(db, scheme_suffix_);
        for (const DB::DIG_Mode_Type& mode_type: dig_mode_type_vect)
        {
            if (mode_type.id() == mode.mode_id())
            {
                mode_btn_text += ": ";
                mode_btn_text += mode_type.title().toStdString();
                break;
            }
        }
    }

    scheme_status_task.get();

    if (!scheme_status.status_set_.empty())
    {
        QString status_text;
        QString status_sql = "WHERE scheme_id = ";
        status_sql += QString::number(scheme_.parent_id_or_id());
        status_sql += " AND id IN (";

        for (const DIG_Status& status: scheme_status.status_set_)
            if (status.group_id() == dig_id)
                status_sql += QString::number(status.status_id()) + ',';

        status_sql.replace(status_sql.size() - 1, 1, QChar(')'));

        status_sql = db.select_query(db_table<DIG_Status_Type>(), status_sql, {
                                         DIG_Status_Type::COL_id,
                                         DIG_Status_Type::COL_text,
                                         DIG_Status_Type::COL_category_id
                                     });

        std::map<uint32_t, std::pair<QString, uint32_t>> status_map;
        q = db.exec(status_sql);
        while(q.next())
            status_map.emplace(q.value(0).toUInt(), std::make_pair(q.value(1).toString(), q.value(2).toUInt()));

        for (const DIG_Status& status: scheme_status.status_set_)
        {
            auto status_it = status_map.find(status.status_id());
            if (status_it == status_map.cend())
                continue;

            text_ += '\n';
            text_ += default_status_category_emoji(status_it->second.second);

            status_text = status_it->second.first;
            for (const QString& arg: status.args())
                status_text = status_text.arg(arg);

            text_ += status_text.toStdString();
        }
    }

    text_ += '\n';
    text_ += dev_item_text.toStdString();

    keyboard_->inlineKeyboard.push_back(makeInlineButtonRow(msg_data_ + ".mode", mode_btn_text));
    keyboard_->inlineKeyboard.push_back(makeInlineButtonRow(msg_data_ + ".param", "Уставки"));
}

void Elements::fill_group_mode(uint32_t dig_id)
{
    text_.clear();

    uint32_t mode_id;

    std::string mode_btn_base;

    if (cmd_.size() == 6)
    {
        mode_btn_base = msg_data_;

        const QString mode_sql = "WHERE scheme_id = " + QString::number(scheme_.id()) + " AND group_id = " + QString::number(dig_id);
        Base& db = Base::get_thread_local_instance();
        const QVector<DB::DIG_Mode> dig_mode_vect = db_build_list<DB::DIG_Mode>(db, mode_sql);
        if (!dig_mode_vect.empty())
        {
            const DB::DIG_Mode& mode = dig_mode_vect.front();
            mode_id = mode.mode_id();
        }
    }
    else
    {
        mode_id = stoi(*++begin_);
        if (mode_id && user_id_)
        {
            QByteArray data;
            QDataStream ds(&data, QIODevice::WriteOnly);
            ds << mode_id << dig_id;
            QMetaObject::invokeMethod(dbus_iface_, "send_message_to_scheme", Qt::QueuedConnection,
                                      Q_ARG(uint32_t, scheme_.id()), Q_ARG(uint8_t, WS_CHANGE_DIG_MODE), Q_ARG(uint32_t, user_id_), Q_ARG(QByteArray, data));
        }

        mode_btn_base = back_data_;
        back_data_ = back_data_.substr(0, back_data_.size() - cmd_.at(cmd_.size() - 2).size() - 1);
    }

    vector<TgBot::InlineKeyboardButton::Ptr> mode_btn_vect;

    std::string mode_text;
    Base& db = Base::get_thread_local_instance();
    const QVector<DB::DIG_Mode_Type> dig_mode_type_vect = db_build_list<DB::DIG_Mode_Type>(db, scheme_suffix_);
    for (const DB::DIG_Mode_Type& mode: dig_mode_type_vect)
    {
        mode_text.clear();
        if (mode.id() == mode_id)
            mode_text = "✅ ";
        mode_text += mode.title().toStdString();
        mode_btn_vect.push_back(makeInlineButton(mode_btn_base + '.' + to_string(mode.id()), mode_text));
    }

    keyboard_->inlineKeyboard.push_back(std::move(mode_btn_vect));
}

void Elements::fill_group_param(uint32_t dig_id)
{
    text_.clear();

    std::string param_btn_base;

    if (cmd_.size() == 6)
    {
        param_btn_base = msg_data_;
    }
    else
    {
        skip_edit_ = true;
        text_ = "Введите значение уставки:";
        return;
        param_btn_base = back_data_;
        back_data_ = back_data_.substr(0, back_data_.size() - cmd_.at(cmd_.size() - 2).size() - 1);
    }

    const QString sql =
            "SELECT dp.id, dptp.title, dpt.title, dpv.value FROM das_dig_param dp "
            "LEFT JOIN das_dig_param_type dpt ON dpt.id = dp.param_id "
            "LEFT JOIN das_dig_param_type dptp ON dptp.id = dpt.parent_id "
            "LEFT JOIN das_dig_param_value dpv ON dpv.group_param_id = dp.id AND dpv.scheme_id = "
            + QString::number(scheme_.id()) +
            " WHERE dpt.value_type != 7 AND dp.scheme_id = "
            + QString::number(scheme_.parent_id_or_id()) +
            " AND dp.group_id = "
            + QString::number(dig_id);

    Base& db = Base::get_thread_local_instance();
    QSqlQuery q = db.exec(sql);
    string text;
    while (q.next())
    {
        const string data = param_btn_base + '.' + q.value(0).toString().toStdString();
        text = q.value(1).toString().toStdString();
        if (!text.empty())
            text += " - ";
        text += q.value(2).toString().toStdString();
        text += ": ";
        text += q.value(3).toString().toStdString();
        keyboard_->inlineKeyboard.push_back(makeInlineButtonRow(data, text));
    }
}

QString Elements::get_group_sql(uint32_t sct_id) const
{
    return  "SELECT dig.id, IF(dig.title IS NULL OR dig.title = '', dt.title, dig.title) as title "
            "FROM das_device_item_group dig "
            "LEFT JOIN das_dig_type dt ON dt.id = dig.type_id "
            "WHERE dig.scheme_id = " + QString::number(scheme_.parent_id_or_id())
            + " AND dig.section_id = " + QString::number(sct_id);
}

} // namespace Bot
} // namespace Das
