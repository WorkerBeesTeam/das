#ifndef JOURNALMODELDATA_H
#define JOURNALMODELDATA_H

#include "Das/log/log_pack.h"

#include <QObject>
#include <QTimer>

#include <deque>
#include <unordered_set>

namespace Das {
namespace Gui {

using JournalItem = Log_Event_Item;

class JournalModel;

///\class JournalModelData
/// manages data for JournalModel
class JournalModelData
{
public:
    /// constructor
    explicit JournalModelData(JournalModel * model);

    /// destructor
    ~JournalModelData();

    struct ViewPort
    {
        int top;
        int bottom;
    };

    /// data request/response state
    enum State {
        StateStarted,           ///< initial state
        StateReady,             ///< data request answered
        StateWaitingForResponse ///< request was made, waiting for response
    };

    /// direction of requested items
    enum Direction {
        DirectionUp,
        DirectionDown
    };

    /// processes response for initial data request
    void processInitialEvents(const std::deque<JournalItem> & items, std::size_t offset, std::size_t count);

    /// processes response for ususal data request
    void processEvents(const std::deque<JournalItem> & items, std::size_t count, Direction direction);

    /// processes incoming data event
    void processNewEvent(const JournalItem & item);

    /// returns items count
    std::size_t size() const { return items_.size(); }

    /// returns const reference to an item
    const JournalItem & operator[](std::size_t i) const { return items_[i]; }

    /// handles scrollbar indicator position change
    void onPositionChanged(int position, int top, int bottom);

    /// returns state
    State state() const { return state_; }

    /// requests some number of old data items
    void requestOldItems();

    /// requests some number of new data items
    void requestNewItems();

    /// checks bounds :)
    void checkBounds();

private:
    /// requests data items which id are in range
    /// @param pk_from is start id value of the range
    /// @param pk_to is last id value of the range
    /// @param direction used by callback and means where to insert data
    void requestData(std::size_t pk_from, std::size_t pk_to, Direction direction);

    /// requests data items
    /// @param offset is the number of items to skip
    /// @param count is the number of items to return
    void requestDataInitial(std::size_t offset, std::size_t count);

    /// sets state
    void setState(State state) { state_ = state; }

    /// adds items to the front of the queue
    /// used for adding fresh events
    void add_front(const std::deque<JournalItem> & items);

    /// adds items to the back of the queue
    /// used for adding loaded old events
    void push_back(const std::deque<JournalItem> & items);

    /// removes items from front the of the queue
    void remove_front(std::size_t count);

    /// removes items from the back of the queue
    void remove_back(std::size_t count);

    /// when item count changes need to ensure,
    /// that the whole item count is less than max value
    void processCountChanged();

    /// enqueues invoking processCountChanded from the model context
    void invokeProcessCountChanged();

    JournalModel * model_;              ///< Target model
    bool is_at_top_;                    ///< Flag whether object is in reveiving events state
    bool has_no_more_top_;              ///< No more items at the top
    bool has_no_more_bottom_;           ///< No more items at the bottom
    long prev_position_;                ///< Last known position of scrollbar
    ViewPort view_port_;                ///< Top and bottom displayed items
    State state_;                       ///< Request state
    unsigned int max_count_;            ///< Max items count in list
//    uint32_t new_id_;                    ///< New id for event received from websocket

    std::deque<JournalItem> items_;     ///< Container with data
    std::unordered_set<uint32_t> ids_;   ///< Set of ids
    QTimer timer_;                      ///< Timer for checking bounds

    // connection keepers
    QMetaObject::Connection logEventReceivedConnection;
    QMetaObject::Connection journalEventsConnection;
    QMetaObject::Connection journalEventsInitialConnection;
};

}
}


#endif // JOURNALMODELDATA_H
