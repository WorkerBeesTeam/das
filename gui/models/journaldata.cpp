#include <cassert>
#include <cmath>

#include "journaldata.h"

#include "journalmodel.h"
#include "serverapicall.h"

#include <QDebug>


namespace Das {
namespace Gui {

JournalModelData::JournalModelData(JournalModel * model):
    model_{model},
    is_at_top_{true},
    has_no_more_top_{false},
    has_no_more_bottom_{false},
    prev_position_{0},
    view_port_{-1, -1},
    state_{StateStarted},
    max_count_{150}
{
    auto * server = ServerApiCall::instance();

    logEventReceivedConnection = QObject::connect(server, &ServerApiCall::logEventReceived,
            [this](const QVector<Log_Event_Item>& event_pack)
    {
        for (auto&& event: event_pack)
        {
            processNewEvent(event);
        }
    });

    journalEventsConnection =
            QObject::connect(server, &ServerApiCall::journalEventsAvailable,
            [this](const std::deque<JournalItem> & items, std::size_t count, JournalModelData::Direction direction) {
        processEvents(items, count, direction);
    });

    journalEventsInitialConnection =
            QObject::connect(server, &ServerApiCall::journalEventsInitialAvailable,
            [this](const std::deque<JournalItem> & items, std::size_t offset, std::size_t count) {
        processInitialEvents(items, offset, count);
    });

    requestDataInitial(0, 75);
    timer_.setInterval(1000);

    QObject::connect(&timer_, &QTimer::timeout, [this](){ checkBounds(); });
}


JournalModelData::~JournalModelData()
{
    timer_.stop();
    QObject::disconnect(logEventReceivedConnection);
    QObject::disconnect(journalEventsConnection);
    QObject::disconnect(journalEventsInitialConnection);
}


void JournalModelData::requestData(std::size_t pk_from, std::size_t pk_to, JournalModelData::Direction direction)
{
    if (state() != StateReady) {
        qDebug() << "another request is not answered yet, ignoring request";
        return;
    }

    setState(StateWaitingForResponse);

    qDebug() << "requesting data: pk_from = " << pk_from << " pk_to = " << pk_to << " direction = " << direction;
    auto * server = ServerApiCall::instance();

    auto limit = static_cast<std::size_t>(std::abs(static_cast<long>(pk_from - pk_to)) + 1);
    server->get_eventlog(pk_from, pk_to, limit, direction);
}


void JournalModelData::requestDataInitial(std::size_t offset, std::size_t count)
{
    setState(StateWaitingForResponse);

    auto * server = ServerApiCall::instance();

    server->get_eventlog_initial(offset, count);
}

void JournalModelData::requestOldItems()
{
    const std::size_t fetch_number = 25;
    auto size = items_.size();
    const auto & last_item = items_[size - 1];
    std::size_t last_id = 0; // last_item.id() - 1;
    auto first_id_signed = last_id - (fetch_number - 1);
    std::size_t first_id = first_id_signed > 0 ? first_id_signed : 0;

    requestData(first_id, last_id, DirectionDown);
}

void JournalModelData::requestNewItems()
{
    const std::size_t fetch_number = 25;

    const auto & first_item = items_[0];
    std::size_t first_id = 0; // first_item.id() + 1;
    std::size_t  last_id = first_id + (fetch_number - 1);

    requestData(first_id, last_id, DirectionUp);
}


void JournalModelData::checkBounds()
{
    auto items_left_bottom = static_cast<int>(size()) - view_port_.bottom;
    if (items_left_bottom < 35) {
        requestOldItems();
    } else if (!is_at_top_){
        auto items_left_top = view_port_.top;
        if (items_left_top < 35) {
            requestNewItems();
        }
    }
}


void JournalModelData::processInitialEvents(const std::deque<JournalItem> & items,
                                            std::size_t offset, std::size_t count)
{
    Q_UNUSED(offset)
    push_back(items);

    setState(StateReady);

    has_no_more_top_ = true;

    if (items.size() < count) {
        has_no_more_bottom_ = true;
    }

    setState(StateReady);

    timer_.start();
}


void JournalModelData::processEvents(const std::deque<JournalItem> & items,
                                     std::size_t count, JournalModelData::Direction direction)
{
    bool has_no_more = items.size() < count;
    qDebug() << "events received, size = " << items.size() << " direction = " << direction;

    switch (direction)
    {
    case DirectionUp:
    {
        add_front(items);
        break;
    }

    case DirectionDown:
    {
        push_back(items);
        break;
    }

    default:
        assert(false && "unknown direction");
        break;
    }

    if (has_no_more && direction == DirectionUp) {
        is_at_top_ = true;
    }

    if (has_no_more && direction == DirectionDown) {
        qDebug() << "no more items at bottom";
    }

    setState(StateReady);
}


void JournalModelData::processNewEvent(const JournalItem & item)
{
    if (is_at_top_ && state() != StateStarted)
    {
        auto it = item;
//        it.set_id(new_id_++);

        model_->beforeInsertRows(0, 0);
        items_.push_front(std::move(it));
        model_->afterInsertRows();
        qDebug() << "log message received: " << it.toString();

        invokeProcessCountChanged();
        if (!timer_.isActive()) {
            timer_.start();
        }
    } else {
        qDebug() << "log message received, ignoring message";
    }
}


namespace {
template <class T, class L, class U>
T limit_number(T number, L lower_limit, U upper_limit) {
    if (number < static_cast<T>(lower_limit))
        return static_cast<T>(lower_limit);
    if (number > static_cast<T>(upper_limit))
        return static_cast<T>(upper_limit);

    return number;
}

}

void JournalModelData::onPositionChanged(int position, int top, int bottom)
{
    long cur_position = limit_number(position, 0, 100);

    // if no significant changes
    if (prev_position_ == cur_position) {
        if (top != view_port_.top)
            view_port_.top = top;

        if (bottom != view_port_.bottom)
            view_port_.bottom = bottom;

        return;
    }

    Direction direction = (cur_position - prev_position_) > 0 ? DirectionDown : DirectionUp;

    auto size = items_.size();

    qDebug() << "position changed: " << cur_position
             << "size = " << size
             << " top=" << top << " bottom=" << bottom
             << "direction=" << direction
             << ((direction == DirectionDown) ? ("items_left=" + QString::number(static_cast<int>(size) - view_port_.bottom)) : "");

//    if (has_no_more_bottom_) {
//        return;
//    }


    if (size == 0) { // questionable
        return;
    }

    if (direction == DirectionDown) {
        auto items_left = static_cast<int>(size) - view_port_.bottom;
        if (items_left < 35) {
            requestOldItems();
        }
    } else if (!is_at_top_){
        auto items_left = view_port_.top;
        if (items_left < 35) {
            requestNewItems();
        }
    }

    prev_position_ = cur_position;
}


void JournalModelData::add_front(const std::deque<JournalItem> & items)
{ 
    if (items.size() == 0)
        return;

//    qDebug() << "adding items to front: count = " << items.size()
//             << " first_id = " << items[0].id()
//             << " last_id = " << items[items.size() - 1].id();

    int last_index = static_cast<int>(items.size() - 1);

    model_->beforeInsertRows(0, last_index);
    items_.insert(items_.begin(), items.begin(), items.end());
    model_->afterInsertRows();

    invokeProcessCountChanged();
}


void JournalModelData::push_back(const std::deque<JournalItem> & items)
{
    if (items.size() == 0)
        return;

//    qDebug() << "adding items to back: count = " << items.size()
//             << " first_id = " << items[0].id()
//             << " last_id = " << items[items.size() - 1].id();

    int first = static_cast<int>(items_.size());
    int last = static_cast<int>(items_.size() + items.size() - 1);

    model_->beforeInsertRows(first, last);
    items_.insert(items_.end(), items.begin(), items.end());
    model_->afterInsertRows();

    qDebug() << "items added back, size = " << items_.size();

    invokeProcessCountChanged();
}


void JournalModelData::remove_front(std::size_t count)
{
    int size = static_cast<int>(std::min(count, items_.size()));
    if (size == 0)
        return;

    is_at_top_ = false;

    int first = 0;
    int last = size - 1;

//    qDebug() << "removing items from the front: count = " << size
//             << " first_id = " << items_[first].id()
//             << " last_id = " << items_[last].id();


    model_->beforeRemoveRows(first, last);
    items_.erase(std::begin(items_), std::begin(items_) + size);
    model_->afterRemoveRows();

    qDebug() << "items removed front, size = " << items_.size();
}


void JournalModelData::remove_back(std::size_t count)
{
    int size = static_cast<int>(std::min(count, items_.size()));
    if (size == 0)
        return;

    int last = static_cast<int>(items_.size()) - 1;
    int first = last - size + 1;

//    qDebug() << "removing items from the back: count = " << size
//             << " first_id = " << items_[first].id()
//             << " last_id = " << items_[last].id();

    model_->beforeRemoveRows(first, last);
    items_.erase(std::end(items_) - size, std::end(items_));
    model_->afterRemoveRows();

    qDebug() << "items removed back, size = " << items_.size();
}


void JournalModelData::processCountChanged()
{
    if (items_.size() < max_count_)
        return;

    // magick numbers
    if (prev_position_ < 50) {
        remove_back(25);
    } else {
        remove_front(25);
    }
}


void JournalModelData::invokeProcessCountChanged()
{
    auto queue_call_func = [this] () { processCountChanged(); };
#if (QT_VERSION >= QT_VERSION_CHECK(5,10,0))
    QMetaObject::invokeMethod(model_, queue_call_func, Qt::QueuedConnection);
#else
    QTimer::singleShot(0, queue_call_func);
#endif
}

}
}

