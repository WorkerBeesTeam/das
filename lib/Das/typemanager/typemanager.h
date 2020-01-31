#ifndef DAI_TYPEMANAGER_H
#define DAI_TYPEMANAGER_H

#include <QVariant>
#include <map>

#include "plugintypemanager.h"
#include "paramtypemanager.h"

namespace Dai {

enum LogType : uint8_t {
    ValueLog = 1,
    EventLog,
};

static QString logTableName(uint8_t log_type) {
    switch (log_type) {
    case ValueLog: return "house_logs";
    case EventLog: return "house_eventlog";
    default: break;
    }
    return {};
}

// ---

struct DAI_LIBRARY_SHARED_EXPORT ItemType : public TitledType {

    enum RegisterType {
        rtInvalid,
        rtDiscreteInputs,
        rtCoils,
        rtInputRegisters,
        rtHoldingRegisters,
        rtFile,
        rtSimpleButton,
    };

    enum SaveAlgorithm {
        saInvalid,
        saDontSave,
        saSaveImmediately,
        saSaveByTimer,
    };

    ItemType(uint id = 0,
             const QString& name = QString(),
             const QString& title = QString(),
             uint groupType = 0,
             bool groupDisplay = false,
             bool raw = false,
             uint sign = 0,
             quint8 registerType = rtInvalid,
             quint8 saveAlgorithm = saInvalid);

    bool raw = false;
    bool groupDisplay = false;
    quint8 registerType = rtInvalid;
    quint8 saveAlgorithm = saInvalid;
    uint groupType = 0;
    uint sign = 0;
};

QDataStream& operator<<(QDataStream& ds, const ItemType& itemType);
QDataStream& operator>>(QDataStream& ds, ItemType& itemType);

struct DAI_LIBRARY_SHARED_EXPORT ItemTypeManager : public TitledTypeManager<ItemType>
{
    bool needNormalize(uint type_id) const;
    bool groupDisplay(uint type_id) const;
    uint groupType(uint type_id) const;
    uint sign(uint type_id) const;
    uchar registerType(uint type_id) const;
    uchar saveAlgorithm(uint type_id) const;
};

// ---

struct DAI_LIBRARY_SHARED_EXPORT ItemGroupType : public TitledType {
    ItemGroupType(uint id = 0, const QString& name = QString(), const QString& title = QString(),
                  uint code = 0, const QString& description = QString());
    uint code;
    QString description;
};

QDataStream& operator<<(QDataStream& ds, const ItemGroupType& groupType);
QDataStream& operator>>(QDataStream& ds, ItemGroupType& groupType);

struct DAI_LIBRARY_SHARED_EXPORT ItemGroupTypeManager : public TitledTypeManager<ItemGroupType>
{
    uint code(uint type_id) const;
};

// ---

struct DAI_LIBRARY_SHARED_EXPORT GroupStatus : public TitledType {
    GroupStatus(uint id = 0, const QString& name = QString(), const QString& text = QString(), uint type_id = 0,
                bool isMultiValue = true, uint value = 0, uint groupType_id = 0, bool inform = true);
    uint type_id;
    bool isMultiValue;
    uint value;
    uint groupType_id;
    bool inform;
};

QDataStream& operator<<(QDataStream& ds, const GroupStatus& status);
QDataStream& operator>>(QDataStream& ds, GroupStatus& status);

struct DAI_LIBRARY_SHARED_EXPORT StatusManager : public TitledTypeManager<GroupStatus>
{
};

// ---

struct DAI_LIBRARY_SHARED_EXPORT StatusType : public TitledType {
    StatusType(uint id = 0, const QString& name = QString(), const QString& title = QString(), const QString& color = QString());
    QString color;
};

QDataStream& operator<<(QDataStream& ds, const StatusType& statusType);
QDataStream& operator>>(QDataStream& ds, StatusType& statusType);

struct DAI_LIBRARY_SHARED_EXPORT StatusTypeManager : public TitledTypeManager<StatusType> {};

// ---

struct DAI_LIBRARY_SHARED_EXPORT CodeCommon : public BaseType {
    CodeCommon(uint id = 0, const QString& name = QString(), uint global_id = 0);
    CodeCommon(const CodeCommon& other);
    CodeCommon(CodeCommon&& other);
    CodeCommon& operator=(const CodeCommon& other);
    CodeCommon& operator=(CodeCommon&& other);
    uint global_id;
};
QDataStream& operator<<(QDataStream& ds, const CodeCommon& code);
QDataStream& operator>>(QDataStream& ds, CodeCommon& code);

struct DAI_LIBRARY_SHARED_EXPORT CodeItem : public CodeCommon {
    CodeItem(uint id = 0, const QString& name = QString(), uint global_id = 0, const QString& text = QString());
    CodeItem(const CodeItem& other);
    CodeItem(CodeItem&& other);
    CodeItem& operator=(const CodeItem& other);
    CodeItem& operator=(CodeItem&& other);
    bool operator==(const CodeItem& other) const;

    QString text;
};
QDataStream& operator<<(QDataStream& ds, const CodeItem& code);
QDataStream& operator>>(QDataStream& ds, CodeItem& code);

struct DAI_LIBRARY_SHARED_EXPORT CodeSumm : public CodeCommon {
    CodeSumm() = default;
    CodeSumm(const CodeItem& other);
    quint16 checksum;

    static quint16 get_checksum(const CodeItem& other);
};

QDataStream& operator<<(QDataStream& ds, const CodeSumm& code);
QDataStream& operator>>(QDataStream& ds, CodeSumm& code);

struct DAI_LIBRARY_SHARED_EXPORT CodeManager : public BaseTypeManager<CodeItem>{};

// ---

struct DAI_LIBRARY_SHARED_EXPORT ModeType : public TitledType {
    ModeType(uint id = 0, const QString& name = QString(), const QString& title = QString(), uint group_type_id = 0);
    uint group_type_id;
};

QDataStream& operator<<(QDataStream& ds, const ModeType& modeType);
QDataStream& operator>>(QDataStream& ds, ModeType& modeType);

struct DAI_LIBRARY_SHARED_EXPORT ModeTypeManager : public TitledTypeManager<ModeType> {};

// ---

typedef BaseTypeManager<BaseType> SimpleTypeManager;
typedef SimpleTypeManager SignManager;
//typedef TitledTypeManager<TitledType> CodeManager;

/*class SignManager
{
    QString m_empty;
    std::map<uint, QString> m_signs;
public:
    std::map<uint, QString>& signs();

    void add(uint id, const QString& signStr);
    const QString& sign(uint id) const;
};*/

struct DAI_LIBRARY_SHARED_EXPORT TypeManagers {
    ItemTypeManager ItemTypeMng;
    SignManager SignMng;
    ItemGroupTypeManager GroupTypeMng;
    ModeTypeManager ModeTypeMng;
    ParamTypeManager ParamMng;
    CodeManager CodeMng;
    StatusManager StatusMng;
    StatusTypeManager StatusTypeMng;
    std::shared_ptr<PluginTypeManager> PluginTypeMng;
};

QString DAI_LIBRARY_SHARED_EXPORT signByDevItem(const TypeManagers* mng, uint dev_item_type);
const QString& codeByGroupType(const TypeManagers* mng, uint group_type);

} // namespace Dai

#endif // DAI_TYPEMANAGER_H
