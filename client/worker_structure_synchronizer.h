#ifndef DAS_WORKER_STRUCTURE_SYNCHRONIZER_H
#define DAS_WORKER_STRUCTURE_SYNCHRONIZER_H

#include <plus/das/structure_synchronizer_base.h>

namespace Das {

class Worker;

class Worker_Structure_Synchronizer : public Ver_2_4::Structure_Synchronizer_Base
{
public:
    Worker_Structure_Synchronizer(Worker* worker);
private:
    virtual void send_modify_response(uint8_t struct_type, const QByteArray &buffer, uint32_t user_id);

    Worker* worker_;
};

} // namespace Das

#endif // DAS_WORKER_STRUCTURE_SYNCHRONIZER_H
