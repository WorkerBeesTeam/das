#include <QString>
#include <QtTest>
#include <QCoreApplication>
#include <QSignalSpy>

#include "Das/proto_scheme.h"
#include <plus/das/database_delete_info.h>
#include <plus/das/structure_synchronizer_base.h>

namespace Das
{

class LibTest : public QObject
{
    Q_OBJECT

    std::shared_ptr<Proto_Scheme> mng;
public:
    LibTest()
    {
    }

private Q_SLOTS:

    void initTestCase() {
        mng = std::make_shared<Proto_Scheme>(nullptr);
        QVERIFY(!!mng);
    }
    void cleanupTestCase() {
        mng.reset();
        QVERIFY(!mng);
    }

    void has_scheme_id_test()
    {
        QCOMPARE(Database::has_scheme_id<Device>(), true);
        QCOMPARE(Database::has_scheme_id<User>(), false);
        QCOMPARE(Database::has_scheme_id<User_Groups>(), false);
        QCOMPARE(Database::has_scheme_id<Auth_Group>(), false);
        QCOMPARE(Database::has_scheme_id<Auth_Group_Permission>(), false);
    }

    // ---------- Proto_Scheme ----------
    void Proto_SchemeInit() {
/*
#pragma GCC warning "Get TypesCount"
        const uint TypesCount = 18;
        Prt::ServerInfo info;

        int item_id = 0, group_id = 0, param_id = 0;

        for (int i = 1; i <= 3; i++)
        {
            auto dev = info.add_device();
            dev->set_id( i );
            dev->set_address( i );

            for (int j = 0; j < 3; j++)
                for (uint k = 1; k < TypesCount; ++k)
                {
                    auto item = dev->add_item();
                    item->set_type( k );
                    item->set_id(++item_id);
                }

            auto sct = info.add_section();
            sct->set_id( i );

            sct->mutable_day()->set_start(3600);
            sct->mutable_day()->set_end(7200);

            for (int j = 1; j < 4; ++j)
            {
                auto group = sct->add_group();
                group->set_type( j );
                group->set_id(++group_id);
            }
        }

        mng->init(info);*/
    }
    // ---------- Proto_Scheme ----------

    // ---------- DayTime ----------
    void qTimeFromSecondsTest() {
        const int h = 13, m = 35, s = 42;
        QTime time(h, m, s);
        QVERIFY(time.secsTo(qTimeFromSecs((h * 3600) + (m * 60) + s)) == 0);
    }
    void toSecondsTest() {
        const int h = 13, m = 35, s = 42;
        QTime time(h, m, s);
        QVERIFY(toSecs(time) == (h * 3600) + (m * 60) + s);
    }
    void TimeRangeInitValues() {
        TimeRange dt(7200, 3600);
        QVERIFY( dt.start() == 3600 && dt.end() == 7200);
    }

    void DayTimeNotIsDay_data() {
        QTest::addColumn<int>("start");
        QTest::addColumn<int>("end");
        QTest::addColumn<bool>("result");

        QTest::newRow("Now is day") << -60 << 60 << true;
        QTest::newRow("Day after now") << 30 << 60 << false;
        QTest::newRow("Day before now") << -60 << -30 << false;
    }
    void DayTimeNotIsDay() {
        QFETCH(int,  start);
        QFETCH(int,  end);
        QFETCH(bool, result);

        auto c = QTime::currentTime();
        TimeRange dt(toSecs(c.addSecs(start)), toSecs(c.addSecs(end)));

        QVERIFY(dt.inRange() == result);
    }
    // ---------- DayTime ----------

    // ---------- Device_Item ----------
    void Device_ItemEmitIfNewValue() {/*
        Device_Item item(nullptr, nullptr);
        QSignalSpy spy(&item, SIGNAL(valueChanged()));
        Device_Item::ValueType val;
        val.set_v_int32(7);

        item.setData( 5 );
        item.setData( val );
        item.setData( 7 );
        item.setData( val );
        QCOMPARE(spy.count(), 2);*/
    }
    void Device_ItemIsConnected() {
        /*
        Device_Item item(nullptr, nullptr), item1(nullptr, nullptr), item2(nullptr, nullptr);
        item.setData(7);
        item2.setData(7);
        item2.clear_value();
        QVERIFY(item.isConnected() && !item1.isConnected() && !item2.isConnected());*/
    }
    // ---------- Device_Item ----------

    // ---------- Group ----------
    // ---------- Group ----------

    // ---------- Section ----------
    void SectionIsAuto() {

    }

    // ---------- Section ----------
    void testCase1()
    {
        QVERIFY2(true, "Failure");
    }
};
} // namespace Das

QTEST_MAIN(Das::LibTest)

#include "tst_libtest.moc"
