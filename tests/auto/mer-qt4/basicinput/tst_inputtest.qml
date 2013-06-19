import QtQuickTest 1.0
import QtQuick 1.0
import Sailfish.Silica 1.0
import QtMozilla 1.0
import "../../shared/componentCreation.js" as MyScript
import "../../shared/sharedTests.js" as SharedTests

ApplicationWindow {
    id: appWindow

    property bool mozViewInitialized : false
    property string inputContent : ""
    property int inputState : -1
    property bool changed : false
    property int focusChange : -1
    property int cause : -1
    property string inputType : ""

    function isState(state, focus, cause)
    {
        return appWindow.changed === true && appWindow.inputState === state && appWindow.focusChange === focus && appWindow.cause === cause;
    }

    QmlMozContext {
        id: mozContext
    }
    Connections {
        target: mozContext.instance
        onOnInitialized: {
            // Gecko does not switch to SW mode if gl context failed to init
            // and qmlmoztestrunner does not build in GL mode
            // Let's put it here for now in SW mode always
            mozContext.instance.setIsAccelerated(true);
            mozContext.instance.addComponentManifest(mozContext.getenv("QTTESTSROOT") + "/components/TestHelpers.manifest");
        }
    }

    QmlMozView {
        id: webViewport
        visible: true
        focus: true
        anchors.fill: parent
        Connections {
            target: webViewport.child
            onViewInitialized: {
                webViewport.child.loadFrameScript("chrome://tests/content/testHelper.js");
                appWindow.mozViewInitialized = true
                webViewport.child.addMessageListener("testembed:elementpropvalue");
            }
            onHandleSingleTap: {
                print("HandleSingleTap: [",point.x,",",point.y,"]");
            }
            onRecvAsyncMessage: {
                // print("onRecvAsyncMessage:" + message + ", data:" + data)
                switch (message) {
                case "testembed:elementpropvalue": {
                    // print("testembed:elementpropvalue value:" + data.value);
                    appWindow.inputContent = data.value;
                    break;
                }
                default:
                    break;
                }
            }
            onImeNotification: {
                print("onImeNotification: state:" + state + ", open:" + open + ", cause:" + cause + ", focChange:" + focusChange + ", type:" + type)
                appWindow.changed = true
                appWindow.inputState = state
                appWindow.cause = cause
                appWindow.focusChange = focusChange
                appWindow.inputType = type
            }
        }
    }

    resources: TestCase {
        id: testcaseid
        name: "mozContextPage"
        when: windowShown

        function cleanup() {
            mozContext.dumpTS("tst_inputtest cleanup")
        }

        function test_Test1LoadInputPage()
        {
            mozContext.dumpTS("test_Test1LoadInputPage start")
            testcaseid.verify(MyScript.waitMozContext())
            testcaseid.verify(MyScript.waitMozView())
            webViewport.child.url = "data:text/html,<input id=myelem value=''>";
            testcaseid.verify(MyScript.waitLoadFinished(webViewport))
            testcaseid.compare(webViewport.child.loadProgress, 100);
            while (!webViewport.child.painted) {
                testcaseid.wait();
            }
            mouseClick(webViewport, 10, 10)
            while (!appWindow.isState(1, 0, 3)) {
                testcaseid.wait();
            }
            appWindow.inputState = false;
            keyClick(Qt.Key_K);
            keyClick(Qt.Key_O);
            keyClick(Qt.Key_R);
            keyClick(Qt.Key_P);
            webViewport.child.sendAsyncMessage("embedtest:getelementprop", {
                                                name: "myelem",
                                                property: "value"
                                               })
            while (appWindow.inputContent == "") {
                testcaseid.wait();
            }
            testcaseid.compare(appWindow.inputContent, "korp");
            mozContext.dumpTS("test_Test1LoadInputPage end");
        }

        function test_Test1LoadInputURLPage()
        {
            mozContext.dumpTS("test_Test1LoadInputPage start")
            testcaseid.verify(MyScript.waitMozContext())
            testcaseid.verify(MyScript.waitMozView())
            appWindow.inputContent = ""
            appWindow.inputType = ""
            webViewport.child.url = "data:text/html,<input type=number id=myelem value=''>";
            testcaseid.verify(MyScript.waitLoadFinished(webViewport))
            testcaseid.compare(webViewport.child.loadProgress, 100);
            while (!webViewport.child.painted) {
                testcaseid.wait();
            }
            mouseClick(webViewport, 10, 10)
            while (!appWindow.isState(1, 0, 3)) {
                testcaseid.wait();
            }
            appWindow.inputState = false;
            keyClick(Qt.Key_1);
            keyClick(Qt.Key_2);
            keyClick(Qt.Key_3);
            keyClick(Qt.Key_4);
            webViewport.child.sendAsyncMessage("embedtest:getelementprop", {
                                                name: "myelem",
                                                property: "value"
                                               })
            while (appWindow.inputContent == "") {
                testcaseid.wait();
            }
            while (appWindow.inputType == "") {
                testcaseid.wait();
            }
            testcaseid.compare(appWindow.inputContent, "1234");
            mozContext.dumpTS("test_Test1LoadInputPage end");
        }
    }
}