#include "shotgeometry.h"

#include <QTest>

using namespace island::shots;

class TestShotGeometry : public QObject {
    Q_OBJECT

private slots:
    void windowBoxesOnShownWorkspaces() {
        const QByteArray monitors = R"([
            {"name":"DP-2","activeWorkspace":{"id":1},"specialWorkspace":{"id":0}},
            {"name":"DP-3","activeWorkspace":{"id":2},"specialWorkspace":{"id":-98}}
        ])";
        const QByteArray clients = R"([
            {"mapped":true,"hidden":false,"at":[0,0],"size":[960,1080],"workspace":{"id":1},"floating":false,"focusHistoryID":2},
            {"mapped":true,"hidden":false,"at":[-1914,50],"size":[842,1024],"workspace":{"id":2},"floating":false,"focusHistoryID":0},
            {"mapped":true,"hidden":false,"at":[100,100],"size":[400,300],"workspace":{"id":1},"floating":true,"focusHistoryID":5},
            {"mapped":true,"hidden":false,"at":[5,5],"size":[10,10],"workspace":{"id":3},"floating":false,"focusHistoryID":1},
            {"mapped":true,"hidden":true,"at":[6,6],"size":[10,10],"workspace":{"id":1},"floating":false,"focusHistoryID":3},
            {"mapped":false,"hidden":false,"at":[7,7],"size":[10,10],"workspace":{"id":1},"floating":false,"focusHistoryID":4},
            {"mapped":true,"hidden":false,"at":[-1900,10],"size":[500,500],"workspace":{"id":-98},"floating":false,"focusHistoryID":6}
        ])";
        QCOMPARE(windowBoxes(clients, monitors),
                 QStringLiteral("100,100 400x300\n-1914,50 842x1024\n0,0 960x1080\n-1900,10 500x500\n"));
    }

    void windowBoxesEmpty() {
        QCOMPARE(windowBoxes("[]", "[]"), QString());
        QCOMPARE(windowBoxes("garbage", "garbage"), QString());
    }

    void activeWindow() {
        QCOMPARE(*activeWindowBox(R"({"address":"0x1","at":[-1914,50],"size":[842,1024]})"),
                 QStringLiteral("-1914,50 842x1024"));
        QVERIFY(!activeWindowBox("{}"));
        QVERIFY(!activeWindowBox(R"({"address":"0x1","at":[0,0],"size":[0,0]})"));
    }
};

QTEST_GUILESS_MAIN(TestShotGeometry)
#include "tst_shotgeometry.moc"
